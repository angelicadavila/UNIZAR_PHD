#ifndef CLBALANCER_SCHEDULER_HGUIDED_HPP
#define CLBALANCER_SCHEDULER_HGUIDED_HPP 1

#include <queue>
#include <thread>
#include <vector>
// #include "device.hpp"
#include <iostream>
#include <mutex>

// #include "rang.hpp"
#include "semaphore.hpp"
#include "work.hpp"

#include "scheduler.hpp"

using namespace std;

namespace clb {
class Device;
class Scheduler;
class HGuidedScheduler;
// class Scheduler;
// enum WorkSplit;
void scheduler_thread_func(HGuidedScheduler& sched);

class HGuidedScheduler : public Scheduler {
 public:
  enum WorkSplit {
    Raw = 0,
    By_Devices = 1,  // work / ndevices
                     //                 Decr2 = 2, // first 50%, second 50% * 50%, etc
                     //                 Incr2 = 3, // last 50%, before last 50% * 50%, etc
  };

  // void run();

  // HGuidedScheduler() = delete;
  // HGuidedScheduler(WorkSplit wsplit);
  HGuidedScheduler(WorkSplit wsplit = WorkSplit::By_Devices);
  ~HGuidedScheduler();

  // make it Noncopyable
  HGuidedScheduler(HGuidedScheduler const&) = delete;
  HGuidedScheduler& operator=(HGuidedScheduler const&) = delete;

  // make it movable
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

  // void setRawProportions(const vector<float>& props);
  // void setRawProportions(const vector<float>& props){
  // cout << "HGuidedScheduler::setRawProportions\n";
  // // vector<tuple<size_t, size_t>> proportions;
  // auto last = m_ndevices - 1;
  // if (props.size() < last){
  //   throw runtime_error("proportions < number of devices - 1");
  // }

  // for (auto prop : props){
  //   if (prop <= 0.0f || prop >= 1.0f){
  //     throw runtime_error("proportion should be between (0.0f, 1.0f)");
  //   }
  // }
  // m_raw_proportions = move(props);
  // m_wsplit = WorkSplit::Raw;
  // }
  // void setDevices(int ndevices){ m_ndevices = ndevices; }
  void setDevices(vector<Device*>&& devices);

  void calcProportions() override;
  void setRawProportions(const vector<float>& props);
  void normalizeRawProportions();

  int getWorkIndex(Device* device) override;
  Work getWork(uint queue_index) override;

  // Thread API
  void init();

  void printStats() override;

  // void callbackDevice(Device* device) override;
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
  mutex m_mutex_work;
  vector<Work> m_queue_work;
  vector<vector<uint>> m_queue_id_work;
  uint m_queue_work_size;  // should be atomic
  vector<uint> m_chunk_todo;
  vector<uint> m_chunk_given;
  vector<uint> m_chunk_done;
  uint m_devices_working;
  vector<float> m_raw_proportions;
  // vector<vector<uint>> m_chunk_id_todo;
  // vector<vector<uint>> m_chunk_id_done;

  // new
  WorkSplit m_wsplit;
  bool m_has_work;
  semaphore m_sema_requests;
  semaphore m_sema_callbacks;
  queue<int> m_requests;

  size_t m_size_rem;
  size_t m_size_rem_given;
  size_t m_size_rem_completed;
  size_t m_size_given;  // used for the offset
  size_t m_worksize;
  size_t m_work_last;  // when there is a rest
};

}  // namespace clb

#endif /* CLBALANCER_SCHEDULER_HGUIDED_HPP */
