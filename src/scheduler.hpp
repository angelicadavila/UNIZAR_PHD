#ifndef CLBALANCER_SCHEDULER_HPP
#define CLBALANCER_SCHEDULER_HPP 1

#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "semaphore.hpp"
#include "work.hpp"

using namespace std;

namespace clb {
class Device;

class Scheduler {
 public:
  virtual void start() = 0;
  virtual void calcProportions() = 0;
  virtual void waitCallbacks() = 0;
  virtual void setTotalSize(size_t size) = 0;
  virtual tuple<size_t, size_t> splitWork(size_t size, float prop, size_t bound) = 0;
  virtual void setDevices(vector<Device*>&& devices) = 0;
  virtual int getWorkIndex(Device* device) = 0;
  virtual Work getWork(uint queue_index) = 0;

  virtual void printStats() = 0;

  virtual void callback(int queue_index) = 0;
  virtual void req_work(Device* device) = 0;
  virtual void enq_work(Device* device) = 0;
  virtual void preenq_work() = 0;
};

}  // namespace clb

#endif /* CLBALANCER_SCHEDULER_HPP */
