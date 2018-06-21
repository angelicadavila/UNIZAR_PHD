/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of eclalancer which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef CLENGINE_SCHEDULER_PART_HPP
#define CLENGINE_SCHEDULER_PART_HPP 1

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "inspector.hpp"
#include "scheduler.hpp"
#include "semaphore.hpp"
#include "work.hpp"

using std::atomic;
using std::lock_guard;
using std::make_tuple;
using std::mutex;
using std::queue;
using std::thread;
using std::tie;

namespace ecl {
enum class ActionType;
class Device;
class Scheduler;
class SwarmScheduler;

void
scheduler_thread_func(SwarmScheduler& sched);

class SwarmScheduler : public Scheduler
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

  SwarmScheduler(WorkSplit wsplit = WorkSplit::By_Devices);
  ~SwarmScheduler();

  SwarmScheduler(SwarmScheduler const&) = delete;
  SwarmScheduler& operator=(SwarmScheduler const&) = delete;

  SwarmScheduler(SwarmScheduler&&) = default;
  SwarmScheduler& operator=(SwarmScheduler&&) = default;
  // particle barrier
  uint particleBarrier(Device* device);

  //bounds

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

  void setGWS(NDRange gws) override;
  void setLWS(size_t lws) override;
  void setWSBound(float ws_bound) override;
  

  void calcProportions(); 
  tuple<size_t, size_t> splitWork(size_t size, float prop, size_t bound);

   //Finish time
  void updateTime(uint id);

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
  vector<tuple<size_t, size_t>> m_proportions;
  vector<float> m_raw_proportions;

  // new
  WorkSplit m_wsplit;
  bool m_has_work;
  semaphore m_sema_requests;
  semaphore m_sema_callbacks;
  queue<int> m_requests;
  atomic<size_t> m_chunks_done;
  int m_requests_max;
  atomic<uint> m_requests_idx;
  atomic<uint> m_requests_idx_done;
  vector<uint> m_requests_list;

  size_t m_size_rem;
  size_t m_size_given; 
  size_t m_worksize;
  size_t m_work_last;

  NDRange m_gws;  
  size_t m_lws;  
  float m_ws_bound;
  
  atomic<int>  m_size_rem_completed;

  mutex* m_mutex_duration;
  std::chrono::duration<double> m_time_init;
  std::chrono::duration<double> m_time;
  vector<tuple<size_t, ActionType>> m_duration_actions;
  vector<tuple<size_t, ActionType>> m_duration_offset_actions;

  //Particle SwarmScheduler
  uint m_num_particles;
  uint g_barrier;
  vector<uint> p_barrier;
  float c1; //cognitive
  float c2; //social
  float w; //intertia
  float Vmin;//velocity limit
  float Vmax;//velocity limit

  vector<vector<uint>>  Xi;//solution vector paticles*devices
  vector<vector<float>> Pi;//local best
  vector<vector<float>> Vi;//Velocity
  vector<float> Pi_thg;//local best
  vector<float> Pgbest;//global best
  vector<vector<float>> Thg_Xi;//througput best
  vector<vector<double>> Time_Xi;//time in each device
  vector<vector<double>> duration_Xi;
  vector<uint>  m_id_particle; //index of particle evaluate

  //extra barriers
  std::chrono::duration<double> base_time;
};

} // namespace ecl

#endif /* CLENGINE_SCHEDULER_PROP_HPP */
