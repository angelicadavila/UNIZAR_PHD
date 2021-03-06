/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#include "schedulers/dynamic.hpp"

#include <tuple>
#include <cassert>
#include "device.hpp"
#include "scheduler.hpp"

#define ATOMIC 1
// #define ATOMIC 0
//#define  FRAMES 1
//#define  FRAMES 1
//#define  FRAMES 20 //for AES
namespace ecl {

void
scheduler_thread_func(DynamicScheduler& sched)
{
//  auto time1 = std::chrono::system_clock::now().time_since_epoch();
//  sched.saveDuration(ActionType::schedulerStart);
//  sched.saveDurationOffset(ActionType::schedulerStart);
//  sched.preenq_work();
  while (sched.hasWork()) {
    auto moreReqs = true;
    do {
      auto device = sched.getNextRequest();
      if (device != nullptr) { // != nullptr
        sched.enq_work(device);
        device->notifyWork();
      } else {
        moreReqs = false;
      }
    } while (moreReqs);
    sched.waitCallbacks();
  }
    sched.notifyDevices();
}

DynamicScheduler::DynamicScheduler(WorkSplit wsplit)
  : m_wsplit(wsplit)
  , m_has_work(false)
  , m_sema_requests(1)
  , m_sema_callbacks(1)
  , m_chunks_done(0)
  , m_requests_max(0)
  , m_requests_idx(0)
  , m_requests_idx_done(0)
  , m_requests_list(0, 0)
{
  m_mutex_duration = new mutex();
  m_time_init = std::chrono::system_clock::now().time_since_epoch();
  m_time = std::chrono::system_clock::now().time_since_epoch();
  m_duration_actions.reserve(8);        // NOTE to improve
  m_duration_offset_actions.reserve(8); // NOTE to improve
  //m_frames=FRAMES;
}

DynamicScheduler::~DynamicScheduler()
{
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void
DynamicScheduler::endScheduler()
{
    saveDuration(ActionType::schedulerEnd);
        saveDurationOffset(ActionType::schedulerEnd);
}


void
DynamicScheduler::printStats()
{
  auto sum = 0;
  auto len = m_devices.size();
  for (uint i = 0; i < len; ++i) {
    sum += m_chunk_todo[i];
  }
  cout << "DynamicScheduler:\n";
  cout << "chunks: " << sum << "\n";
  cout << "chunks (ATOMIC): " << m_chunks_done << "\n";
  sum= m_chunks_done;
  for (uint i = 0; i < len; ++i) {
    cout<<"chunkPerDevice_"<< i <<": "<<((double)m_chunk_todo[i]/(double)sum)*100<<" %.\n";
  }
  sum=0;
  for (uint i = 0; i < len; ++i) {
    sum+=m_devices[i]->getWorkSize();
  }
  for (uint i = 0; i < len; ++i) {
    cout<<"WorkPerDevice_"<< i <<": "<<((double)m_devices[i]->getWorkSize()/(double)sum)*100<<" %.\n";
  }
  cout << "duration offsets from init:\n";
  for (auto& t : m_duration_offset_actions) {
    Inspector::printActionTypeDuration(std::get<1>(t), std::get<0>(t));
  }
  auto first_item=m_duration_offset_actions.size()-m_duration_offset_actions.size()/2; 
  auto last_time=(std::get<0>(m_duration_offset_actions[m_duration_offset_actions.size()-1]));
  auto time_imbalance=last_time-(std::get<0>(m_duration_offset_actions[first_item]));


  auto last_item=m_duration_offset_actions.size()-1; 
  auto init_time=(std::get<0>(m_duration_offset_actions[0]));
  auto time_run_sched=(std::get<0>(m_duration_offset_actions[last_item]))-init_time;
  cout<< "imbalanceKernel: "<<((double)time_imbalance/(double)time_run_sched)*100<<" %.\n"; 
  cout<< "executionKernel: "<<time_run_sched<<" us.\n"; 
  cout<< "executionKernel: "<<time_run_sched/1000<<" ms.\n"; 
}

void
DynamicScheduler::notifyDevices()
{
  for (auto dev : m_devices) {
    dev->notifyWork();
  }
  for (auto dev : m_devices) {
    //dev->notifyEvent();
  }
}

void
DynamicScheduler::saveDuration(ActionType action)
{
  lock_guard<mutex> lock(*m_mutex_duration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - m_time).count();
  m_duration_actions.push_back(make_tuple(diff_ms, action));
  m_time = t2;
}
void
DynamicScheduler::saveDurationOffset(ActionType action)
{
  lock_guard<mutex> lock(*m_mutex_duration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - m_time_init).count();
  m_duration_offset_actions.push_back(make_tuple(diff_ms, action));
}
void
DynamicScheduler::setWorkSize(size_t size,size_t frame)
{
  setWorkSize(size);
  m_frames=frame;
}

void
DynamicScheduler::setWorkSize(size_t size)
{
  auto bound = 64;//128;
  size_t total = m_size;
  size_t times = size / bound;
  size_t rest = size % bound;
  size_t given = bound;
  if (!size) {
    throw runtime_error("requirement: worksize > 0");
  }
  if (rest > 0) {
    given = (times + 1) * bound;
  } else {
    given = size;
  }
  if (total < given) {
    given = total;
    m_work_last = total;
  } else {
    times = total / given;
    rest = total - (times * given);
    total = times * given;
    m_work_last = rest;
  }
  m_worksize = given;
  
  m_frames=1;//FRAMES;
  IF_LOGGING(cout << "m_worksize (chunk size): " << m_worksize << "\n");
}

void
DynamicScheduler::setGWS(NDRange gws)
{
  m_gws = gws;

}

void
DynamicScheduler::setLWS(size_t lws)
{
  m_lws = lws;
}

void
DynamicScheduler::setWSBound(float ws_bound)
{
  m_ws_bound = ws_bound;
}

bool
DynamicScheduler::hasWork()
{
  return m_size_rem_completed != 0;
}

void
DynamicScheduler::waitCallbacks()
{
  m_sema_callbacks.wait(1);
}

void
DynamicScheduler::waitRequests()
{
  m_sema_requests.wait(1);
}

void
DynamicScheduler::notifyCallbacks()
{
  m_sema_callbacks.notify(1);
}

void
DynamicScheduler::notifyRequests()
{
  m_sema_requests.notify(1);
}

void
DynamicScheduler::setTotalSize(size_t size)
{
  // cout << "setTotalSize: " << size << "\n";
  m_size = size;
  m_has_work = true;
  m_size_rem = size;           // NOTE(dyn) statement
  m_size_given = 0;            // NOTE(dyn) used for the offset
  m_size_rem_given = size;     // NOTE(dyn)
  m_size_rem_completed = size; // NOTE(dyn)
}

tuple<size_t, size_t>
DynamicScheduler::splitWork(size_t size, float prop, size_t bound)
{
  return std::make_tuple<size_t, size_t>( 0, 0); // NOTE(dyn)
  size_t given = bound * (static_cast<size_t>(prop * size) / bound);
  size_t rem = size - given;
  return std::make_tuple( given, rem );
}

void
DynamicScheduler::setDevices(vector<Device*>&& devices)
{
  m_devices = move(devices);
  m_ndevices = m_devices.size();
  m_chunk_todo = vector<uint>(m_ndevices, 0);
  m_chunk_given = vector<uint>(m_ndevices, 0);
  m_chunk_done = vector<uint>(m_ndevices, 0);
  m_device_enable = vector<uint>(m_ndevices, 1);

  m_requests_max = m_ndevices * 2;
  m_requests_list = move(vector<uint>(m_requests_max, 0));
  // TODO expected callbacks
  // m_queue_work.reserve(256);
  m_queue_work.reserve(65536);

  m_queue_id_work.reserve(m_ndevices);
  m_queue_id_work = move(vector<vector<uint>>(m_ndevices, vector<uint>()));
  for (auto& q_id_work : m_queue_id_work) {
    // q_id_work.reserve(256);
    q_id_work.reserve(65536);
  }
  m_devices_working = 0;
}

/**
 * expects worksize set
 */
void
DynamicScheduler::calcProportions()
{
  return; // NOTE(dyn)
  vector<tuple<size_t, size_t>> proportions;
  int len = m_ndevices;
  uint last = len - 1;
  size_t wsize_given_acc = 0;
  size_t wsize_given = 0;
  size_t wsize_rem = m_size;
  switch (m_wsplit) {
    case WorkSplit::Raw:
      for (uint i = 0; i < last; ++i) {
        auto prop = m_raw_proportions[i];
        tie(wsize_given, wsize_rem) = splitWork(wsize_rem, prop, 128);
        size_t wsize_offset = wsize_given_acc;
        proportions.push_back(make_tuple(wsize_given, wsize_offset));
        wsize_given_acc += wsize_given;
      }
      proportions.push_back(make_tuple(wsize_rem, wsize_given_acc)); // the last
      break;
    case WorkSplit::By_Devices:
      for (uint i = 0; i < last; ++i) {
        // proportions.push_back(  );
        tie(wsize_given, wsize_rem) = splitWork(wsize_rem, 1.0f / len, 128);
        // IF_LOGGING(cout << "given: " << wsize_given << " rem: " << wsize_rem << "\n");
        size_t wsize_offset = wsize_given_acc;
        proportions.push_back(make_tuple(wsize_given, wsize_offset));
        wsize_given_acc += wsize_given;
      }
      proportions.push_back(make_tuple(wsize_rem, wsize_given_acc)); // the last
      break;
  }
  m_proportions = move(proportions);
  for (auto prop : m_proportions) {
    IF_LOGGING(cout << "proportion: size: " << std::get<0>(prop) << " offset:" << std::get<1>(prop)
                    << "\n");
  }
}

void
DynamicScheduler::start()
{
  m_thread = thread(scheduler_thread_func, std::ref(*this));
}

/**
 * should be called from only 1 thread (scheduler)
 */
void
DynamicScheduler::enq_work(Device* device)
{
  int id = device->getID();
  /*
   uint now=(m_chunk_done[id]/m_size);
   uint prop=m_proportion_static[id];
   if (prop>=now){
   m_size_rem=0; //finish execution
    }

  */

  if (m_size_rem > 0) {

    size_t size = m_worksize;
    size_t index = -1;
    size_t tmp_cond=0; 
    {
      lock_guard<mutex> guard(m_mutex_work);
      size_t offset = m_size_given;
     //if the new chunk is large that total size, use remaining size 
   /*  if(offset+size>=m_size)
      {
         size=m_size_rem;
       cout<<"\n------- 1\n";
      }
    */
     if (offset==(m_size/m_frames)){  
          if (m_frames>0){
                m_frames--;
                offset=0;
                size = m_worksize;//test if it is necessary
                m_size_given =0;
            }
       cout<<"\n------- 3\n";
     }
 
     tmp_cond=offset+size;
     if(tmp_cond>=(m_size/m_frames)){
         //size=(m_size/m_frames)-offset;
         //size=ceil(size/128)*128;  
         offset=0;
	 if (m_size/m_frames<offset){
             size=m_size_rem;
             cout<<"\n------- 21\n";
          }
       cout<<"\n------- 21\n";
      }
     if(offset+size>=m_size)
      {
         size=m_size_rem;
       cout<<"\n------- 1\n";
      }

     if (size>m_size_rem)
         size=m_size_rem;
      cout<<"m_size_rem "<<m_size_rem<<"\n m_size "<<m_size <<"\n";
      cout<<"offset"<<offset<<"\n";
      m_size_rem -= size;
      m_size_given += size;
      index = m_queue_work.size();
      m_queue_work.push_back(Work(id, offset, size, m_ws_bound));
      m_queue_id_work[id].push_back(index);
      m_chunk_todo[id]++;
    }
  } else 
 {
   m_device_enable[id]=0; 
  }
}

void
DynamicScheduler::preenq_work()
{
}

void
DynamicScheduler::req_work(Device* device)
{

  //auto time1 = std::chrono::system_clock::now().time_since_epoch();
  saveDuration(ActionType::schedulerStart);
  saveDurationOffset(ActionType::schedulerStart);

#if ATOMIC == 1
  if (m_size_rem_completed > 0) {
    auto idx = m_requests_idx++ % m_requests_max;
    m_requests_list[idx] = device->getID() + 1;
  }
#else
  {
    lock_guard<mutex> guard(m_mutex_work);
    if (m_size_rem_completed) {
      m_requests.push(device->getID());
    }
  }
#endif
  notifyCallbacks();
}

void
DynamicScheduler::callback(int queue_index)
{
#if ATOMIC == 1
  Work work = m_queue_work[queue_index];
  int id = work.device_id;
  m_chunks_done++;
  m_size_rem_completed -= work.size;
  if (m_size_rem_completed > 0) {
    auto idx = m_requests_idx++ % m_requests_max;
    m_requests_list[idx] = id + 1;
  }
  else{
    m_device_enable[id]=0;
  }
#else
  {
    lock_guard<mutex> guard(m_mutex_work);
    Work work = m_queue_work[queue_index];
    int id = work.device_id;
    m_chunk_done[id]++;
    m_size_rem_completed -= work.size;
    if (m_size_rem_completed) {
      m_requests.push(id);
    }
  }
#endif
  notifyCallbacks();
}

/**
 * call this function only if you are awaken by scheduler
 */
int
DynamicScheduler::getWorkIndex(Device* device)
{
  int id = device->getID();
  //lock_guard<mutex> guard(m_mutex_work);
  // if (m_size_rem_given > 0 && m_device_enable[id])  {
   if ( m_device_enable[id])  {
    uint next = 0;
    int index = -1;
    next = m_chunk_given[id]++;
   // m_size_rem_given -= m_worksize;
    index = m_queue_id_work[id][next];
    return index;
  } else {
    return -1;
  }
}

Work
DynamicScheduler::getWork(uint queue_index)
{
  //lock_guard<mutex> guard(m_mutex_work);
  return m_queue_work[queue_index];
}

Device*
DynamicScheduler::getNextRequest()
{
  Device* dev = nullptr;
#if ATOMIC == 1
  uint idx_done = m_requests_idx_done % m_requests_max;
  uint id = m_requests_list[idx_done];
  if (id > 0) {
    dev = m_devices[id - 1];
    m_requests_list[idx_done] = 0;
    m_requests_idx_done++;
  }
#else
  lock_guard<mutex> guard(m_mutex_work);
  if (!m_requests.empty()) {
    int id = m_requests.front();
    m_requests.pop();
    dev = m_devices[id];
  }
#endif
  return dev;
}
} // namespace ecl
