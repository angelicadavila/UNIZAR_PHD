/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef ENGINECL_SCHEDULER_STATIC_HPP
#define ENGINECL_SCHEDULER_STATIC_HPP 1

#include <iostream>
#include <mutex>
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
using std::thread;
using std::tie;

namespace ecl {
enum class ActionType;
class StaticScheduler;

void
scheduler_thread_func(StaticScheduler& sched);

class StaticScheduler : public Scheduler
{
public:
  enum WorkSplit
  {
    Raw = 0,
    By_Devices = 1, // work / ndevices
                    // Decr2 = 2, // first 50%, second 50% * 50%, etc
                    // Incr2 = 3, // last 50%, before last 50% * 50%, etc
  };

  StaticScheduler(StaticScheduler const&) = delete;
  StaticScheduler& operator=(StaticScheduler const&) = delete;

  StaticScheduler(StaticScheduler&&) = default;
  StaticScheduler& operator=(StaticScheduler&&) = default;

  // Public API
  void start() override;

  StaticScheduler(WorkSplit wsplit = WorkSplit::By_Devices);

  ~StaticScheduler();

  void notifyCallbacks();
  void waitCallbacks();

  void setTotalSize(size_t size);

  tuple<size_t, size_t> splitWork(size_t size, float prop, size_t bound);

  void calcProportions();

  void setRawProportions(const vector<float>& props);
  void setDevices(vector<Device*>&& devices);

  int getWorkIndex(Device* device) override;
  Work getWork(uint queue_index) override;

  // Thread API
  void init();

  void endScheduler() override;
  void printStats() override;

  void saveDuration(ActionType action);
  void saveDurationOffset(ActionType action);

  void callback(int queue_index) override;
  void req_work(Device* device) override;
  void enq_work(Device* device) override;
  void preenq_work() override;

  void setGWS(ecl::NDRange gws) override;
  void setLWS(size_t lws) override;
  void setWSBound(float ws_bound) override;

private:
  thread m_thread;
  size_t m_size;
  vector<Device*> m_devices;
  uint m_ndevices;
  semaphore m_sema;
  mutex m_mutex_work;
  vector<Work> m_queue_work;
  vector<vector<uint>> m_queue_id_work;
  uint m_queue_work_size;
  vector<uint> m_chunk_todo;
  vector<uint> m_chunk_given;
  vector<uint> m_chunk_done;
  bool m_has_work;
  uint m_devices_working;
  vector<tuple<size_t, size_t>> m_proportions;
  vector<float> m_raw_proportions;
  WorkSplit m_wsplit;

  ecl::NDRange m_gws;
  size_t m_lws;
  float m_ws_bound;

  mutex* m_mutex_duration;
  std::chrono::duration<double> m_time_init;
  std::chrono::duration<double> m_time;
  vector<tuple<size_t, ActionType>> m_duration_actions;
  vector<tuple<size_t, ActionType>> m_duration_offset_actions;

  //flag of call onex req_work()
  std::once_flag flag_init;
};

} // namespace ecl

#endif /* ENGINECL_SCHEDULER_STATIC_HPP */
