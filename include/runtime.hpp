/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef ENGINECL_RUNTIME_HPP
#define ENGINECL_RUNTIME_HPP 1

#include <chrono>
#include <memory>
#include <mutex>

#include "clutils.hpp"
#include "device.hpp"
#include "ndrange.hpp"
#include "semaphore.hpp"

using std::lock_guard;
using std::make_shared;
using std::make_tuple;
using std::shared_ptr;
using std::string;
using std::vector;

namespace ecl {
class Scheduler;

class Runtime
{
public:
  void run();

  template<typename T>
  void setInBuffer(shared_ptr<T> array)
  {
    for (auto& device : m_devices) {
      device.setInBuffer(array);
    }
  }
  template<typename T>
  void setInBuffer(shared_ptr<T> array,uint constant)
  {
    for (auto& device : m_devices) {
      device.setInBuffer(array,constant);
    }
  }

 template<typename T>
  void setOutBuffer(shared_ptr<T> array)
  {
    for (auto& device : m_devices) {
      device.setOutBuffer(array);
    }
  }
  template<typename T>
  void setOutAuxBuffer(shared_ptr<T> array)
  {
    for (auto& device : m_devices) {
      device.setOutAuxBuffer(array);
    }
  }
  void setKernel(const string& source, const string& kernel);

  template<typename T>
  void setKernelArg(cl_uint index, const T& value)
  {
    for (auto& device : m_devices) {
      device.setKernelArg(index, value);
    }
  }
  template<typename T>
  void setKernelArg(cl_uint index, const uint bytes, ArgType arg)
  {
    for (auto& device : m_devices) {
      device.setKernelArg(index, bytes, arg);
    }
  }
  void setKernelArgLocalAlloc(cl_uint index, const uint bytes);

  void setInternalChunk(int internal_chunk)
  {
     m_internal_chunk=internal_chunk;
   }

  void discoverDevices();

  void saveDuration(ActionType action);
  void saveDurationOffset(ActionType action);

  cl::Platform usePlatformDiscovery(uint sel_platform);
  cl::Device useDeviceDiscovery(uint sel_platform, uint sel_device);

  void printStats();

  void notifyAllReady();
  void waitAllReady();
  void notifyReady();
  void waitReady();

  void setScheduler(Scheduler* scheduler);

  Runtime(vector<Device>&& devices, NDRange gws, size_t lws = CL_LWS, float bound = 1.0f);

private:
  void configDevices();

  vector<cl::Platform> m_platforms;
  vector<vector<cl::Device>> m_platform_devices;

  shared_ptr<semaphore> m_barrier;
  shared_ptr<semaphore> m_barrier_init;
  vector<Device> m_devices;
  Scheduler* m_scheduler;

  NDRange m_gws;
  size_t m_lws;
  float m_ws_bound;

  shared_ptr<semaphore> m_sema_ready;
  semaphore m_sema_all_ready;

  mutex* m_mutex_duration;
  std::chrono::duration<double> m_time_init;
  std::chrono::duration<double> m_time;
  vector<tuple<size_t, ActionType>> m_duration_actions;
  vector<tuple<size_t, ActionType>> m_duration_offset_actions;

  int m_internal_chunk;
};

} // namespace ecl

#endif /* ENGINECL_RUNTIME_HPP */
