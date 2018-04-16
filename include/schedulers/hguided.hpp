/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef ENGINECL_SCHEDULER_HGUIDED_HPP
#define ENGINECL_SCHEDULER_HGUIDED_HPP 1

#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>
#include <vector>

#include "inspector.hpp"
#include "scheduler.hpp"
#include "semaphore.hpp"
#include "work.hpp"

using std::lock_guard;
using std::make_tuple;
using std::mutex;
using std::queue;
using std::thread;

namespace ecl {
enum class ActionType;
class Device;
class Scheduler;
class HGuidedScheduler;

void
scheduler_thread_func(HGuidedScheduler& sched);

class HGuidedScheduler : public Scheduler
{
public:
  enum WorkSplit
  {
    Raw = 0,
    // work / ndevices
    By_Devices = 1,
    // Decr2 = 2, // first 50%, second 50% * 50%, etc
    // Incr2 = 3, // last 50%, before last 50% * 50%, etc
  };

  HGuidedScheduler(WorkSplit wsplit = WorkSplit::By_Devices);
  ~HGuidedScheduler();

  HGuidedScheduler(HGuidedScheduler const&) = delete;
  HGuidedScheduler& operator=(HGuidedScheduler const&) = delete;

  HGuidedScheduler(HGuidedScheduler&&) = default;
  HGuidedScheduler& operator=(HGuidedScheduler&&) = default;

  // Public API
  void start() override;

  // NOTE(dyn) new func
  void setWorkSize(size_t size);

  // NOTE(dyn) new func
  bool hasWork();

  // NOTE(dyn)
  Device* getNextRequest();

  void notifyDevices();

  void waitRequests();
  void notifyRequests();
  void waitCallbacks();
  void notifyCallbacks();
  void setTotalSize(size_t size);

  tuple<size_t, size_t> splitWork(size_t size, float prop, size_t bound);

  void setDevices(vector<Device*>&& devices);

  void calcProportions() override;
  void setRawProportions(const vector<float>& props);
  void normalizeRawProportions();

  int getWorkIndex(Device* device) override;
  Work getWork(uint queue_index) override;

  // Thread API
  void init();

  void printStats() override;

  void endScheduler() override;
  void saveDuration(ActionType action);
  void saveDurationOffset(ActionType action);

  void callback(int queue_index) override;
  void req_work(Device* device) override;
  void enq_work(Device* device) override;
  void preenq_work() override;

  void setGWS(NDRange gws) override;
  void setLWS(size_t lws) override;
  void setWSBound(float ws_bound) override;

private:
  thread m_thread;
  size_t m_size;
  vector<Device*> m_devices;
  uint m_ndevices;
  mutex m_mutex_work;
  vector<Work> m_queue_work;
  vector<vector<uint>> m_queue_id_work;
  uint m_queue_work_size;
  vector<uint> m_chunk_todo;
  vector<uint> m_chunk_given;
  vector<uint> m_chunk_done;
  uint m_devices_working;
  vector<float> m_raw_proportions;

  // new
  WorkSplit m_wsplit;
  bool m_has_work;
  semaphore m_sema_requests;
  semaphore m_sema_callbacks;
  queue<int> m_requests;

  size_t m_size_rem;
  size_t m_size_rem_given;
  size_t m_size_rem_completed;
  size_t m_size_given;
  size_t m_worksize;
  size_t m_work_last;

  NDRange m_gws;
  size_t m_lws;
  size_t m_ws_bound;

  mutex* m_mutex_duration;
  std::chrono::duration<double> m_time_init;
  std::chrono::duration<double> m_time;
  vector<tuple<size_t, ActionType>> m_duration_actions;
  vector<tuple<size_t, ActionType>> m_duration_offset_actions;
};

} // namespace ecl

#endif /* ENGINECL_SCHEDULER_HGUIDED_HPP */
