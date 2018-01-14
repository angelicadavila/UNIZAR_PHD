#ifndef CLBALANCER_SCHEDULER_STATIC_HPP
#define CLBALANCER_SCHEDULER_STATIC_HPP 1

#include <thread>
#include <vector>
// #include "device.hpp"
#include <iostream>
#include <mutex>

#include "semaphore.hpp"
#include "work.hpp"

#include "scheduler.hpp"

using namespace std;

namespace clb {
// class Device;
class StaticScheduler;
// class Scheduler;
// enum WorkSplit;

void scheduler_thread_func(StaticScheduler& sched);

class StaticScheduler : public Scheduler {
 public:
  enum WorkSplit {
    Raw = 0,
    By_Devices = 1,  // work / ndevices
                     // Decr2 = 2, // first 50%, second 50% * 50%, etc
                     // Incr2 = 3, // last 50%, before last 50% * 50%, etc
  };

  // make it Noncopyable
  StaticScheduler(StaticScheduler const&) = delete;
  StaticScheduler& operator=(StaticScheduler const&) = delete;

  // make it movable
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
  // void setDevices(int ndevices){ m_ndevices = ndevices; }
  void setDevices(vector<Device*>&& devices);

  int getWorkIndex(Device* device) override;
  Work getWork(uint queue_index) override;

  // Thread API
  void init();

  void printStats() override;

  void callback(int queue_index) override;
  void req_work(Device* device) override;
  void enq_work(Device* device) override;
  void preenq_work() override;
  // private:
  // Device& m_device;
  // semaphore m_barrier;
 private:
  thread m_thread;
  size_t m_size;
  vector<Device*> m_devices;  // maybe Device*?
  uint m_ndevices;
  semaphore m_sema;
  mutex m_mutex_work;
  vector<Work> m_queue_work;
  vector<vector<uint>> m_queue_id_work;
  uint m_queue_work_size;  // should be atomic
  vector<uint> m_chunk_todo;
  vector<uint> m_chunk_given;
  vector<uint> m_chunk_done;
  bool m_has_work;
  uint m_devices_working;
  vector<tuple<size_t, size_t>> m_proportions;
  vector<float> m_raw_proportions;
  WorkSplit m_wsplit;
};

}  // namespace clb

#endif /* CLBALANCER_SCHEDULER_STATIC_HPP */
