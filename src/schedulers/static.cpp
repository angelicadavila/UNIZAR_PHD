/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of clbalancer which is released under MIT License.
 * See file LICENSE for full license details.
 */
#include "schedulers/static.hpp"

#include <tuple>

#include "device.hpp"

namespace clb {

void
scheduler_thread_func(StaticScheduler& sched)
{
//  auto time1 = std::chrono::system_clock::now().time_since_epoch();
//  sched.saveDuration(ActionType::schedulerStart);
//  sched.saveDurationOffset(ActionType::schedulerStart);
  sched.preenq_work();
  cout << "sched thread: wait callbacks\n";
  sched.waitCallbacks();
  cout << "sched thread: notified\n";
  sched.saveDuration(ActionType::schedulerEnd);
  sched.saveDurationOffset(ActionType::schedulerEnd);
}

StaticScheduler::StaticScheduler(WorkSplit wsplit)
  : m_sema(1)
  , m_has_work(false)
  , m_wsplit(wsplit)
{
  m_mutex_duration = new mutex();
  m_time_init = std::chrono::high_resolution_clock::now().time_since_epoch();
  m_time = std::chrono::high_resolution_clock::now().time_since_epoch();
  m_duration_actions.reserve(8);        // NOTE to improve
  m_duration_offset_actions.reserve(8); // NOTE to improve
}

StaticScheduler::~StaticScheduler()
{
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void
StaticScheduler::printStats()
{
  auto sum = 0;
  auto len = m_devices.size();
  for (uint i = 0; i < len; ++i) {
    sum += m_chunk_done[i];
  }
  cout << "StaticScheduler:\n";
  cout << "chunks: " << sum << "\n";
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
StaticScheduler::waitCallbacks()
{
  m_sema.wait(1);
}

void
StaticScheduler::notifyCallbacks()
{
  m_sema.notify(1);
}

void
StaticScheduler::setTotalSize(size_t size)
{
  m_size = size;
  m_has_work = true;
}

void
StaticScheduler::setDevices(vector<Device*>&& devices)
{
  m_devices = move(devices);
  m_ndevices = m_devices.size();
  m_chunk_todo = vector<uint>(m_ndevices, 0);
  m_chunk_given = vector<uint>(m_ndevices, 0);
  m_chunk_done = vector<uint>(m_ndevices, 0);
  m_queue_id_work.reserve(m_ndevices);
  m_queue_id_work = move(vector<vector<uint>>(m_ndevices, vector<uint>()));
  m_devices_working = 0;
}

void
StaticScheduler::setRawProportions(const vector<float>& props)
{
  cout << "StaticScheduler::setRawProportions\n";
  auto last = m_ndevices - 1;
  if (props.size() < last) {
    throw runtime_error("proportions < number of devices - 1");
  }

  if (last == 0) {
    m_raw_proportions = { 1.0f };
  } else {
    for (auto prop : props) {
      if (prop <= 0.0f || prop >= 1.0f) {
        throw runtime_error("proportion should be between (0.0f, 1.0f)");
      }
    }
    m_raw_proportions = move(props);
  }
  m_wsplit = WorkSplit::Raw;
}

tuple<size_t, size_t>
StaticScheduler::splitWork(size_t size, float prop, size_t bound)
{
  size_t given = bound * (static_cast<size_t>(prop * size) / bound);
  size_t rem = size - given;
  return { given, rem };
}

void
StaticScheduler::saveDuration(ActionType action)
{
  lock_guard<mutex> lock(*m_mutex_duration);
  auto t2 = std::chrono::high_resolution_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - m_time).count();
  m_duration_actions.push_back(make_tuple(diff_ms, action));
  m_time = t2;
}
void
StaticScheduler::saveDurationOffset(ActionType action)
{
  lock_guard<mutex> lock(*m_mutex_duration);
  auto t2 = std::chrono::high_resolution_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - m_time_init).count();
  m_duration_offset_actions.push_back(make_tuple(diff_ms, action));
}

/**
 * expects worksize set
 */
void
StaticScheduler::calcProportions()
{
  vector<tuple<size_t, size_t>> proportions;
  int len = m_ndevices;
  uint last = len - 1;
  size_t wsize_given_acc = 0;
  size_t wsize_given = 0;
  size_t wsize_rem = m_size;
  // cout << "calcProportions: " << m_wsplit << " wsize_rem: " << wsize_rem << "\n";
  switch (m_wsplit) {
    case WorkSplit::Raw:
      for (uint i = 0; i < last; ++i) {
        auto prop = m_raw_proportions[i];
        // for (auto prop : props){
        tie(wsize_given, wsize_rem) = splitWork(wsize_rem, prop, 128);
        size_t wsize_offset = wsize_given_acc;
        proportions.push_back(make_tuple(wsize_given, wsize_offset));
        wsize_given_acc += wsize_given;
      }
      proportions.push_back(make_tuple(wsize_rem, wsize_given_acc));
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
      proportions.push_back(make_tuple(wsize_rem, wsize_given_acc));
      break;
   }
  m_proportions = move(proportions);
  for (auto prop : m_proportions) {
    cout << "proportion: size: " << std::get<0>(prop) << " offset:" << std::get<1>(prop) << "\n";
  }
}

void
StaticScheduler::start()
{
  m_thread = thread(scheduler_thread_func, std::ref(*this));
  cout << "thread start\n";
}

/**
 * should be called from only 1 thread (scheduler)
 */
void
StaticScheduler::enq_work(Device* device)
{
  int id = device->getID();
  cout << "StaticScheduler::enq_work for " << id << "\n";
  if (m_chunk_todo[id] == 0) { // Static
    auto prop = m_proportions[id];
    size_t size, offset;
    tie(size, offset) = prop;
    // cout << "proportion for me id: " << id << " size: " << size << " offset: " << offset << "\n";

   lock_guard<mutex> guard(m_mutex_work);
    auto index = m_queue_work.size();
    m_queue_work.push_back(Work(id, offset, size));
    m_queue_id_work[id].push_back(index);

    m_chunk_todo[id]++;
  } else {
    cout << "StaticScheduler::enq_work  not enqueuing\n";
  }
}

void
StaticScheduler::preenq_work()
{
  cout << "StaticScheduler::preenq_work\n";
  m_devices_working = m_ndevices;
  calcProportions();
}

void
StaticScheduler::req_work(Device* device)
{
  auto time1 = std::chrono::high_resolution_clock::now().time_since_epoch(); 
//  auto time1 = std::chrono::system_clock::now().time_since_epoch();
  saveDuration(ActionType::schedulerStart);
  saveDurationOffset(ActionType::schedulerStart);
 cout << "StaticScheduler::req_work\n";
  //can be called for more than one threa
  enq_work(device);
  device->notifyWork();
}

void
StaticScheduler::callback(int queue_index)
{
  {
    lock_guard<mutex> guard(m_mutex_work);
    Work work = m_queue_work[queue_index];
    int id = work.device_id;
    m_chunk_done[id]++;
    if (m_chunk_done[id] == 1) { // Static
      Device* device = m_devices[id];
      m_devices_working--;
      device->notifyWork();
      device->notifyEvent();
      if (m_devices_working == 0) {
        notifyCallbacks();
      }
    }
  }
}

/**
 * call this function only if you are awaken by scheduler
 */
int
StaticScheduler::getWorkIndex(Device* device)
{
  int id = device->getID();
  if (m_has_work) {
    if (m_chunk_given[id] == 0) {
      uint next = 0;
      {
        lock_guard<mutex> guard(m_mutex_work);
        next = m_chunk_given[id]++;
      }
      return m_queue_id_work[id][next];
    } else { // no more for this device (static)
      return -1;
    }
  } else {
    return -1;
  }
}

Work
StaticScheduler::getWork(uint queue_index)
{
  return m_queue_work[queue_index];
}

} // namespace clb
