/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#include "schedulers/hguided.hpp"

#include "device.hpp"
#include "scheduler.hpp"
#include <tuple>
#include <iomanip>
#define  FRAMES 1
//#define  FRAMES 20

namespace ecl {

void
scheduler_thread_func(HGuidedScheduler& sched)
{
//  auto time1 = std::chrono::system_clock::now().time_since_epoch();
//  sched.saveDuration(ActionType::schedulerStart);
//  sched.saveDurationOffset(ActionType::schedulerStart);
  sched.preenq_work();
  while (sched.hasWork()) {
    auto moreReqs = true;
    do {
      auto device = sched.getNextRequest();
      if (device != nullptr) {
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

HGuidedScheduler::HGuidedScheduler(WorkSplit wsplit)
  : m_wsplit(wsplit)
  , m_has_work(false)
  , m_sema_requests(1)
  , m_sema_callbacks(1)
{
  m_mutex_duration = new mutex();
  m_time_init = std::chrono::system_clock::now().time_since_epoch();
  m_time = std::chrono::system_clock::now().time_since_epoch();
  m_duration_actions.reserve(8);        // NOTE to improve
  m_duration_offset_actions.reserve(8); // NOTE to improve
}

HGuidedScheduler::~HGuidedScheduler()
{
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void
HGuidedScheduler::endScheduler()
{
  saveDuration(ActionType::schedulerEnd);
  saveDurationOffset(ActionType::schedulerEnd);
}

void
HGuidedScheduler::printStats()
{
  auto sum = 0;
  auto len = m_devices.size();
  for (uint i = 0; i < len; ++i) {
    sum += m_chunk_done[i];
  }
  std::cout << std::setprecision(2) << std::fixed;
  IF_LOGGING(cout << "HGuidedScheduler:\n");
  IF_LOGGING(cout << "chunks: " << sum << "\n");
  for (uint i = 0; i < len; ++i) {
    cout<<"chunkPerDevice_"<< i <<": "<<((double)m_chunk_done[i]/(double)sum)*100<<" %.\n";
  }
  sum=0;
  for (uint i = 0; i < len; ++i) {
    sum+=m_devices[i]->getWorkSize();
  }
  for (uint i = 0; i < len; ++i) {
    cout<<"WorkPerDevice_"<< i <<": "<<((double)m_devices[i]->getWorkSize()/(double)sum)*100<<" %.\n";
  }

  IF_LOGGING(cout << "duration offsets from init:\n");
  for (auto& t : m_duration_offset_actions) {
    IF_LOGGING(Inspector::printActionTypeDuration(std::get<1>(t), std::get<0>(t)));
  }
  
  auto first_item=m_duration_offset_actions.size()-m_duration_offset_actions.size()/2; 
  auto last_time=(std::get<0>(m_duration_offset_actions[m_duration_offset_actions.size()-1]));
  auto time_imbalance=last_time-(std::get<0>(m_duration_offset_actions[first_item]));

  auto last_item=m_duration_offset_actions.size()-1; 
  auto init_time=(std::get<0>(m_duration_offset_actions[0]));
  auto time_run_sched=(std::get<0>(m_duration_offset_actions[last_item]))-init_time;
//  cout<< "imbalanceKernel: "<<time_imbalance<<" us.\n"; 
  cout<< "imbalanceKernel: "<<((double)time_imbalance/(double)time_run_sched)*100<<" %.\n"; 
  cout<< "executionKernel: "<<time_run_sched<<" us.\n"; 
}

void
HGuidedScheduler::notifyDevices()
{
  for (auto dev : m_devices) {
    dev->notifyWork();
  }
  for (auto dev : m_devices) {
    dev->notifyEvent();
  }
}

void
HGuidedScheduler::saveDuration(ActionType action)
{
  lock_guard<mutex> lock(*m_mutex_duration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - m_time).count();
  m_duration_actions.push_back(make_tuple(diff_ms, action));
  m_time = t2;
}
void
HGuidedScheduler::saveDurationOffset(ActionType action)
{
  lock_guard<mutex> lock(*m_mutex_duration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - m_time_init).count();
  m_duration_offset_actions.push_back(make_tuple(diff_ms, action));
}

/**
 * Min Chunk size
 */
void
HGuidedScheduler::setWorkSize(size_t size)
{
  auto bound = CL_LWS;
  size_t total = m_size;
  size_t times = size / bound;
  size_t rest = size % bound;
  size_t given = bound;
  if (!size) {
    throw runtime_error("requirement: worksize > 0");
  }
  if (rest) {
    times++;
  }
  given = times * bound;
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
  if (m_worksize % bound != 0) {
    throw runtime_error("worksize if not multiple of LWS");
  }
}

void
HGuidedScheduler::setGWS(NDRange gws)
{
  m_gws = gws;
  m_size = gws[0]; // TODO review gws
  m_has_work = true;
  m_size_rem = m_size;           // NOTE(dyn) statement
  m_size_given = 0;              // NOTE(dyn) used for the offset
  m_size_rem_given = m_size;     // NOTE(dyn)
  m_size_rem_completed = m_size; // NOTE(dyn)
}

void
HGuidedScheduler::setLWS(size_t lws)
{
  m_lws = lws;
}

void
HGuidedScheduler::setWSBound(float ws_bound)
{
  m_ws_bound = ws_bound;
}

bool
HGuidedScheduler::hasWork()
{
  lock_guard<mutex> guard(m_mutex_work);
  return m_size_rem_completed != 0;
}

void
HGuidedScheduler::waitCallbacks()
{
  m_sema_callbacks.wait(1);
}

void
HGuidedScheduler::waitRequests()
{
  m_sema_requests.wait(1);
}

void
HGuidedScheduler::notifyCallbacks()
{
  m_sema_callbacks.notify(1);
}

void
HGuidedScheduler::notifyRequests()
{
  m_sema_requests.notify(1);
}

void
HGuidedScheduler::setTotalSize(size_t size)
{
  m_size = size;
  m_has_work = true;
  m_size_rem = size;           // NOTE(dyn) statement
  m_size_given = 0;            // NOTE(dyn) used for the offset
  m_size_rem_given = size;     // NOTE(dyn)
  m_size_rem_completed = size; // NOTE(dyn)
}

tuple<size_t, size_t>
HGuidedScheduler::splitWork(size_t /* size */, float /* prop */, size_t /* bound */)
{
  return std::make_tuple<size_t, size_t>( 0, 0);
}

void
HGuidedScheduler::setDevices(vector<Device*>&& devices)
{
  m_devices = move(devices);
  m_ndevices = m_devices.size();
  m_chunk_todo = vector<uint>(m_ndevices, 0);
  m_chunk_given = vector<uint>(m_ndevices, 0);
  m_chunk_done = vector<uint>(m_ndevices, 0);

  m_queue_work.reserve(256);

  m_queue_id_work.reserve(m_ndevices);
  m_queue_id_work = move(vector<vector<uint>>(m_ndevices, vector<uint>()));
  for (auto& q_id_work : m_queue_id_work) {
    q_id_work.reserve(256);
  }
  m_frames=FRAMES;
  
  m_devices_working = 0;
  m_raw_proportions.reserve(m_ndevices);
}

void
HGuidedScheduler::start()
{
  m_thread = thread(scheduler_thread_func, std::ref(*this));
}

void
HGuidedScheduler::setRawProportions(const vector<float>& props)
{
  m_raw_proportions = move(props);
}

void
HGuidedScheduler::calcProportions()
{
}

/**
 * Should be called only if m_ndevices > 0
 */
void
HGuidedScheduler::normalizeRawProportions()
{
  uint last = m_ndevices - 1;
  auto& props = m_raw_proportions;
  if (props.size() < last) {
    throw runtime_error("proportions < number of devices - 1");
  }

  if (last == 0) {
    m_raw_proportions = { 1.0f };
  } 
   else {
    auto sum = 0.0f;
    uint nprops = 0;
    for (auto prop : props) {
      IF_LOGGING(cout << prop << "\n");
      if (prop <= 0.0f || prop >= 1.0f) {
        throw runtime_error("proportion should be between (0.0f, 1.0f)");
      }
      sum += prop;
      nprops++;
    }

	for (int i=0; i<=last; i++){
		m_raw_proportions[i]/=sum;
		cout<<m_raw_proportions[i]<<" \n";
	}

//    auto wrong = true;
//    if (nprops == last) {
//      props[last] = 1.0f - sum;
//      wrong = false;
//    }
//    if (wrong) {
//      throw runtime_error("proportion exceeds 1.0f and cannot be corrected");
//    }
  }
}

/**
 * should be called from only 1 thread (scheduler)
 */
void
HGuidedScheduler::enq_work(Device* device)
{
  int id = device->getID();
  lock_guard<mutex> guard(m_mutex_work);
  if (m_size_rem > 0) {
    size_t offset = m_size_given;
    size_t size = m_worksize;

    auto prop = m_raw_proportions[id];
    // size_t lws = 128;
    // size_t lws = 256; // mandelbrot
    size_t lws = m_lws;
    size_t min_worksize = m_worksize;
    auto new_size = splitWorkLikeHGuided(m_size_rem, min_worksize, lws, prop);
    
    uint mem_lim=(device->getLimMemory());
    uint int_chunk=(device->getInternalChunk());
    m_lim_size=mem_lim/(int_chunk*4);
    uint mult= floor(m_lim_size/m_lws);
    m_lim_size=mult * m_lws;
    size=new_size;
    //limit memory size
    if (new_size > m_lim_size){
	    size = m_lim_size;
       cout<<"warning: lim_size memory, no schedule\n";
    }

    if (size+offset>m_size){
        size=m_size_rem;
    }
    if(offset==(m_size/FRAMES)){
      if (m_frames>0){
        m_frames--;
        m_size_given=0;
        offset=0;
      }
    }

    if (offset+size>(m_size/FRAMES)){
        new_size=(m_size/FRAMES)-offset;
        size=new_size; 
        if(size>m_lim_size) {
		size=m_lim_size;
 		cout<<"warning: lim_size memory, no schedule\n";	
	}
        if (size<0){
          size=m_size_rem;
        }
    }
    size_t index = -1;
    {
      m_size_rem -= size;
      m_size_given += size;
      index = m_queue_work.size();
      m_queue_work.push_back(Work(id, offset, size, m_ws_bound));
      m_queue_id_work[id].push_back(index);
      m_chunk_todo[id]++;
    }
  } else {
    IF_LOGGING(cout << "HGuidedScheduler::enq_work  not enqueuing\n");
  }
}

void
HGuidedScheduler::preenq_work()
{
  if (m_ndevices) {
    normalizeRawProportions();
  }
}

void
HGuidedScheduler::req_work(Device* device)
{
  
  
  auto time1 = std::chrono::system_clock::now().time_since_epoch();
  saveDuration(ActionType::schedulerStart);
  saveDurationOffset(ActionType::schedulerStart);
  {
    lock_guard<mutex> guard(m_mutex_work);
    if (m_size_rem_completed) {
      m_requests.push(device->getID());
    }
  }
  notifyCallbacks();
}

void
HGuidedScheduler::callback(int queue_index)
{
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
  notifyCallbacks();
}

/**
 * call this function only if you are awaken by scheduler
 */
int
HGuidedScheduler::getWorkIndex(Device* device)
{
  int id = device->getID();
  lock_guard<mutex> guard(m_mutex_work);
  if (m_size_rem_given > 0) {
    uint next = 0;
    int index = -1;
    {
      next = m_chunk_given[id]++;
      index = m_queue_id_work[id][next];
      Work work = m_queue_work[index];
      m_size_rem_given -= work.size;
    }
    return index;
  } else {
    return -1;
  }
}

Work
HGuidedScheduler::getWork(uint queue_index)
{
  lock_guard<mutex> guard(m_mutex_work);
  return m_queue_work[queue_index];
}

Device*
HGuidedScheduler::getNextRequest()
{
  lock_guard<mutex> guard(m_mutex_work);
  Device* dev = nullptr;
  if (!m_requests.empty()) {
    int id = m_requests.front();
    m_requests.pop();
    dev = m_devices[id];
  }
  return dev;
}

} // namespace ecl
