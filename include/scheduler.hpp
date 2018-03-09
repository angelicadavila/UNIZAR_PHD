/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef ENGINECL_SCHEDULER_HPP
#define ENGINECL_SCHEDULER_HPP 1

#include <iostream>
#include <mutex>
#include <thread>
#include <tuple>
#include <vector>

#include "clutils.hpp"
#include "ndrange.hpp"
#include "semaphore.hpp"
#include "work.hpp"

#define CL_LWS 128

using std::tuple;
using std::vector;

namespace ecl {
class Device;

class Scheduler
{
public:
  virtual void start() = 0;
  virtual void calcProportions() = 0;
  virtual void waitCallbacks() = 0;
  virtual void setTotalSize(size_t size) = 0;
  virtual tuple<size_t, size_t> splitWork(size_t size, float prop, size_t bound) = 0;
  virtual void setDevices(vector<Device*>&& devices) = 0;
  ///return a nullptr to stop device working
  virtual int getWorkIndex(Device* device) = 0;
  ///if exist work, take the index of getWorkIndex and get chunk_size
  virtual Work getWork(uint queue_index) = 0;

  virtual void printStats() = 0;
  ///funtion to wake up scheduler thread at final of kernel execution
  virtual void callback(int queue_index) = 0;
  ///request work from device to scheduler
  virtual void req_work(Device* device) = 0;
  ///thread scheduler to device enqueue work, before this notify device
  virtual void enq_work(Device* device) = 0;
  ///previos or initialize work in queue of device
  virtual void preenq_work() = 0;
  ///wait finish of kernel-
  virtual void endScheduler()=0;

  virtual void setGWS(NDRange gws) = 0;
  virtual void setLWS(size_t lws) = 0;
  virtual void setWSBound(float ws_bound) = 0;
};

} // namespace ecl

#endif /* ENGINECL_SCHEDULER_HPP */
