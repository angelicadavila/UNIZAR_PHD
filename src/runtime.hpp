#ifndef CLBALANCER_RUNTIME_HPP
#define CLBALANCER_RUNTIME_HPP 1

#include "clutils.hpp"
#include "semaphore.hpp"

// #include "scheduler.hpp"
#include "device.hpp"
// #include "buffer.hpp"
#include <memory>

namespace clb {
// class Device;
class Scheduler;

class Runtime {
 public:
  void run();

  // template <typename T>
  // void setInBuffer(shared_ptr<vector<int>> array);
  // void setInBuffer(shared_ptr<vector<int>> array);
  // void setInBuffer(shared_ptr<vector<float>> array);
  // void setInBuffer(shared_ptr<vector<cl_uchar4>> array);
  // // template <typename T>
  // void setOutBuffer(shared_ptr<vector<int>> array);
  // void setOutBuffer(shared_ptr<vector<float>> array);
  // void setOutBuffer(shared_ptr<vector<cl_uchar4>> array);

  template <typename T>
  void setInBuffer(shared_ptr<vector<T>> array) {
    for (auto& device : m_devices) {
      device.setInBuffer(array);
    }
  }
  template <typename T>
  void setOutBuffer(shared_ptr<vector<T>> array) {
    for (auto& device : m_devices) {
      device.setOutBuffer(array);
    }
  }
  // void setInBuffer(Buffer& buffer);
  // void setOutBuffer(Buffer& buffer);

  void setKernel(const string& source, const string& kernel);

  // TODO cl_int instead void
  template <typename T>
  void setKernelArg(cl_uint index, const T& value) {
    for (auto& device : m_devices) {
      device.setKernelArg(index, value);
    }
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

  Runtime(vector<Device>&& devices, size_t size);

 private:
  void configDevices();

  vector<cl::Platform> m_platforms;
  vector<vector<cl::Device>> m_platform_devices;

  shared_ptr<semaphore> m_barrier;
  vector<Device> m_devices;
  // Device& m_device2;
  Scheduler* m_scheduler;

  size_t m_size;
  shared_ptr<semaphore> m_sema_ready;
  semaphore m_sema_all_ready;

  mutex* m_mutex_duration;
  chrono::duration<double> m_time_init;
  chrono::duration<double> m_time;
  vector<tuple<size_t, ActionType>> m_duration_actions;
  vector<tuple<size_t, ActionType>> m_duration_offset_actions;
};

}  // namespace clb

#endif /* CLBALANCER_RUNTIME_HPP */
