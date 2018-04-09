#include "schedulers/proportional.hpp"

#include <tuple>
#include <cassert>
#include <cmath>
#include "device.hpp"
#include "proportional.hpp"

#define ATOMIC 1
// #define ATOMIC 0

namespace clb {

void
scheduler_thread_func(ProportionalScheduler& sched)
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

ProportionalScheduler::ProportionalScheduler(WorkSplit wsplit)
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
}

ProportionalScheduler::~ProportionalScheduler()
{
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void
ProportionalScheduler::endScheduler()
{
    saveDuration(ActionType::schedulerEnd);
    saveDurationOffset(ActionType::schedulerEnd);
}

void
ProportionalScheduler::printStats()
{
  auto sum = 0;
  auto len = m_devices.size();
  for (uint i = 0; i < len; ++i) {
    sum += m_chunk_done[i];
  }
  cout << "ProportionalScheduler:\n";
  cout << "chunks: " << sum << "\n";
  cout << "chunks (ATOMIC): " << m_chunks_done << "\n";
  cout << "duration offsets from init:\n";
  for (auto& t : m_duration_offset_actions) {
    Inspector::printActionTypeDuration(std::get<1>(t), std::get<0>(t));
  }
  auto last_item=m_duration_offset_actions.size()-1; 
  auto init_time=(std::get<0>(m_duration_offset_actions[0]));
  auto time_run_sched=(std::get<0>(m_duration_offset_actions[last_item]))-init_time;
  cout<< "executionKernel: "<<time_run_sched<<" us.\n"; 
}

void
ProportionalScheduler::notifyDevices()
{
  for (auto dev : m_devices) {
    dev->notifyWork();
  }
  for (auto dev : m_devices) {
    dev->notifyEvent();
  }
}

void
ProportionalScheduler::saveDuration(ActionType action)
{
  lock_guard<mutex> lock(*m_mutex_duration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - m_time).count();
  m_duration_actions.push_back(make_tuple(diff_ms, action));
  m_time = t2;
}
void
ProportionalScheduler::saveDurationOffset(ActionType action)
{
  lock_guard<mutex> lock(*m_mutex_duration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - m_time_init).count();
  m_duration_offset_actions.push_back(make_tuple(diff_ms, action));
}

void
ProportionalScheduler::setWorkSize(size_t size)
{
  auto bound = 128;
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
    cout<<"--------give"<<given<<"\n";
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
}

bool
ProportionalScheduler::hasWork()
{
  lock_guard<mutex> guard(m_mutex_work);
  return m_size_rem_completed != 0;
}

void
ProportionalScheduler::waitCallbacks()
{
  m_sema_callbacks.wait(1);
}

void
ProportionalScheduler::waitRequests()
{
  m_sema_requests.wait(1);
}

void
ProportionalScheduler::notifyCallbacks()
{
  m_sema_callbacks.notify(1);
}

void
ProportionalScheduler::notifyRequests()
{
  m_sema_requests.notify(1);
}

void
ProportionalScheduler::setTotalSize(size_t size)
{
  m_size = size;
  m_has_work = true;
  m_size_rem = size;           // NOTE(dyn) statement
  m_size_given = 0;            // NOTE(dyn) used for the offset
  m_size_rem_given = size;     // NOTE(dyn)
  m_size_rem_completed = size; // NOTE(dyn)
}

tuple<size_t, size_t>
ProportionalScheduler::splitWork(size_t size, float prop, size_t bound)
{
  return { 0, 0 }; // NOTE(dyn)
  size_t given = bound * (static_cast<size_t>(prop * size) / bound);
  // cout << "given: " << given << "\n";
  size_t rem = size - given;
  return { given, rem };
}

void
ProportionalScheduler::setDevices(vector<Device*>&& devices)
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
ProportionalScheduler::calcProportions()
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
        // cout << "given: " << wsize_given << " rem: " << wsize_rem << "\n";
        size_t wsize_offset = wsize_given_acc;
        proportions.push_back(make_tuple(wsize_given, wsize_offset));
        wsize_given_acc += wsize_given;
      }
      proportions.push_back(make_tuple(wsize_rem, wsize_given_acc)); // the last
      break;
  }
  m_proportions = move(proportions);
  for (auto prop : m_proportions) {
    cout << "proportion: size: " << std::get<0>(prop) << " offset:" << std::get<1>(prop) << "\n";
  }
}

void
ProportionalScheduler::start()
{
  m_thread = thread(scheduler_thread_func, std::ref(*this));
}

/**
 * should be called from only 1 thread (scheduler)
 */
void
ProportionalScheduler::enq_work(Device* device)
{
  int id = device->getID();
  //lock_guard<mutex> guard(m_mutex_work);
  if (m_size_rem > 0 ) {
    size_t size = m_worksize;
    size_t index = -1;
    {
      lock_guard<mutex> guard(m_mutex_work);
        //calulate chunk size proportional to work done until now
        auto prop=0.0; 
      for (auto i=0; i<(int)m_devices.size();i++){
        prop+=m_chunk_todo[i];
        }
      
      //promedio ejecutado 
      prop=prop==0?1:prop;
      prop=m_chunk_todo[id]/prop;

      prop=prop==0?1:prop;
      cout<<"m_chunk_todo"<<m_chunk_todo[id]<<"Proporcion"<<prop<<"\n";
      size+=std::floor(prop*4)*size;

      size_t offset = m_size_given;
      if(offset+size>m_size)
      {
         size=m_size_rem;
      }

      m_size_rem -= size;
      m_size_given += size;
      index = m_queue_work.size();
      m_queue_work.push_back(Work(id, offset, size));
      m_queue_id_work[id].push_back(index);
      m_chunk_todo[id]++;
    }
  } else 
 {
   m_device_enable[id]=0; 
  }
}

void
ProportionalScheduler::preenq_work()
{}

void
ProportionalScheduler::req_work(Device* device)
{

  auto time1 = std::chrono::system_clock::now().time_since_epoch();
  saveDuration(ActionType::schedulerStart);
  saveDurationOffset(ActionType::schedulerStart);

  if (m_size_rem_completed > 0) {
    auto idx = m_requests_idx++ % m_requests_max;
    m_requests_list[idx] = device->getID() + 1;
  }
  notifyCallbacks();
}

void
ProportionalScheduler::callback(int queue_index)
{
  Work work = m_queue_work[queue_index];
  int id = work.device_id;
  m_chunks_done++;
  m_size_rem_completed -= work.size;
  //cout<<work.size<<" work_size \n";
 if (m_size_rem_completed > 0) {
    auto idx = m_requests_idx++ % m_requests_max;
    m_requests_list[idx] = id + 1;
 }
 else
   m_device_enable[id]=0; 
  notifyCallbacks();
}

/**
 * call this function only if you are awaken by scheduler
 */
int
ProportionalScheduler::getWorkIndex(Device* device)
{
  int id = device->getID();
  //try if m_size_rem_given
  // lock_guard<mutex> guard(m_mutex_work);
   if (m_device_enable[id])  {
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
ProportionalScheduler::getWork(uint queue_index)
{
  lock_guard<mutex> guard(m_mutex_work);
  return m_queue_work[queue_index];
}

Device*
ProportionalScheduler::getNextRequest()
{
  Device* dev = nullptr;
  uint idx_done = m_requests_idx_done % m_requests_max;
  uint id = m_requests_list[idx_done];
  if (id > 0) {
    dev = m_devices[id - 1];
    m_requests_list[idx_done] = 0;
    m_requests_idx_done++;
  }
  return dev;
}
} // namespace clb
