#ifndef CLBALANCER_DEVICE_HPP
#define CLBALANCER_DEVICE_HPP 1

#include <CL/cl.hpp>
#include <algorithm>
#include <iostream>
#include <string>
#include <thread>

// #include <qt/QtCore/QSemaphore>
// #include <QtCore/QSemaphore>
// #include "rang.hpp"
// #include "buffer.hpp"
#include "clutils.hpp"
#include "semaphore.hpp"
// #include "sched.hpp"

// #include "scheduler.hpp"
// #include "runtime.hpp"
#include "buffer.hpp"

using namespace std;

namespace clb {
class Scheduler;
class Runtime;
class Device;
class Buffer;

enum class ActionType {
  initQueue = 0,
  initBuffers = 1,
  writeBuffersDummy = 2,
  initKernel = 3,
  writeBuffers = 4,
  deviceStart = 5,
  deviceReady = 6,
  deviceRun = 7,
  completeWork = 8,
  deviceEnd = 9,

  initDiscovery = 10,
  initContext = 11,
  useDiscovery = 12,
  init = 13,
};
void printActionTypeDuration(ActionType action, size_t duration);

void device_thread_func(Device& device);

class Device {
 public:
  // Device() = default;

  // make it Noncopyable
  Device(Device const&) = delete;
  Device& operator=(Device const&) = delete;

  // make it movable
  Device(Device&&) = default;
  Device& operator=(Device&&) = default;

  // Public API
  // void usePlatformDevice(vector<vector<cl::Device>>& platform_devices);
  // void usePlatformDevice(vector<cl::Platform>& platforms, vector<vector<cl::Device>>& platform_devices);
  void start();
  // template <typename SchedulerPolicy>
  void setScheduler(Scheduler* sched);
  // void setScheduler(Scheduler* sched);
  // void setScheduler(Scheduler<SchedulerPolicy>* sched){ m_sched = sched; }
  // template <typename SchedulerPolicy>
  // Scheduler<SchedulerPolicy>* getScheduler(){ return m_sched; }
  Scheduler* getScheduler();
  void setBarrier(shared_ptr<semaphore> barrier);

  // void setInBuffer(shared_ptr<vector<int>> array);
  // void setInBuffer(shared_ptr<vector<cl_uchar4>> array);
  // void setOutBuffer(shared_ptr<vector<int>> array);
  // void setOutBuffer(shared_ptr<vector<cl_uchar4>> array);

  template <typename T>
  void setInBuffer(shared_ptr<vector<T>> array) {
    Buffer b(Direction::In);
    b.set(array);
    m_in_clb_buffers.push_back(move(b));

    // m_in_arrays.push_back(array);
    auto address = array.get();
    m_in_buffers_ptr.push_back(address);

    cout << "clb::Buffer in get: " << b.get() << "\n";
    cout << "clb::Buffer in data: " << b.data() << "\n";
    cout << "array: " << array.get() << "\n";
    cout << "address: " << address << "\n";
  }
  template <typename T>
  void setOutBuffer(shared_ptr<vector<T>> array) {
    Buffer b(Direction::Out);
    b.set(array);
    m_out_clb_buffers.push_back(move(b));

    // m_out_arrays.push_back(array);
    auto address = array.get();
    m_out_buffers_ptr.push_back(address);

    cout << "clb::Buffer out get: " << b.get() << "\n";
    cout << "clb::Buffer out data: " << b.data() << "\n";
    cout << "array: " << array.get() << "\n";
    cout << "address: " << address << "\n";
  }

  // void setInBuffer(Buffer& buffer);
  // void setOutBuffer(Buffer& buffer);
  void setKernel(const string& source, const string& kernel);
  void setID(int id);
  int getID();
  void waitWork();
  void notifyWork();

  void printStats();

  void saveDuration(ActionType action);
  void saveDurationOffset(ActionType action);

  template <typename T>
  void setKernelArg(cl_uint index, const T& value) {
    cout << "setKernelArg T\n";
    m_arg_index.push_back(index);
    m_arg_size.push_back(OpenCL::KernelArgumentHandler<T>::size(value));
    m_arg_bytes.push_back(OpenCL::KernelArgumentHandler<T>::size(value));
    cout << OpenCL::KernelArgumentHandler<T>::size(value) << "\n";
    // m_arg_bytes.push_back(sizeof(OpenCL::KernelArgumentHandler<T>::size(value)));
    // NOTE legacy OpenCL 1.2 (added (void*))
    // m_arg_ptr.push_back(OpenCL::KernelArgumentHandler<T>::ptr(value));
    m_arg_ptr.push_back((void*)OpenCL::KernelArgumentHandler<T>::ptr(value));
  }

  template <typename T>
  void setKernelArg(cl_uint index, const shared_ptr<T>& value) {
    cout << "setKernelArg shared_ptr T\n";
    m_arg_index.push_back(index);
    auto address = value.get();
    m_arg_size.push_back(0);
    m_arg_bytes.push_back(0);
    m_arg_ptr.push_back(address);
  }

  void setKernelArg(cl_uint index, ::size_t size, const void* ptr);

  void notifyEvent();
  void waitEvent();

  void notifyRun();
  void waitRun();

  // Thread API
  void init();
  void show();
  // void wait();
  // void notify();
  void notifyBarrier();
  void setPlatform(int sel_platform);
  void setDevice(int set_device);
  string& getBuffer();
  void showInfo();

  Device(int sel_platform, int sel_device);
  ~Device();

  void do_work(size_t offset, size_t size, int queue_index);

  Runtime* getRuntime();
  void setRuntime(Runtime* runtime);

 private:
  void useRuntimeDiscovery();
  void initByIndex(int sel_platform, int sel_device);
  void initContext();
  void initQueue();
  void initBuffers();
  void writeBuffers(bool dummy = false);
  void initKernel();
  void initEvents();

  int m_sel_platform;
  int m_sel_device;

  // template <typename SchedulerPolicy>
  // Scheduler<SchedulerPolicy>* m_sched;
  Scheduler* m_sched;
  Runtime* m_runtime;

  // semaphore m_sem_init;
  shared_ptr<semaphore> m_barrier;

  thread m_thread;
  string m_info_buffer;

  vector<cl_uint> m_arg_index;  // cl_uint -> uint (to avoid warning of ignored attributes)
  vector<size_t> m_arg_size;
  vector<size_t> m_arg_bytes;
  // vector<const void*> m_arg_ptr;
  vector<void*> m_arg_ptr;  // NOTE legacy OpenCL 1.2

  // vector<shared_ptr<vector<int>>> m_in_arrays;
  // vector<shared_ptr<vector<int>>> m_out_arrays;
  // vector<Buffer> m_in_arrays;
  // vector<Buffer> m_out_arrays;

  vector<void*> m_in_buffers_ptr;
  // vector<Buffer*> m_in_buffers_ptr;
  vector<cl::Buffer> m_in_buffers;
  vector<void*> m_out_buffers_ptr;
  // vector<Buffer*> m_out_buffers_ptr;
  vector<cl::Buffer> m_out_buffers;

  vector<clb::Buffer> m_in_clb_buffers;  // alt to m_in_arrays
  vector<clb::Buffer> m_out_clb_buffers;

  cl::Platform m_platform;
  cl::Device m_device;
  cl::Context m_context;
  cl::CommandQueue m_queue;
  shared_ptr<string> m_source_str;
  shared_ptr<string> m_kernel_str;
  cl::Kernel m_kernel;
  cl::UserEvent m_end;

  vector<cl::Event> m_prev_events;  // writeBuffers

  int m_id;  // given by the runtime
  // unique_ptr<semaphore> m_sema_work;
  // semaphore m_sema_work = semaphore(1);
  semaphore* m_sema_work;
  unique_ptr<semaphore> m_sema_run;
  // QSemaphore* qsem;

  size_t m_works;
  size_t m_works_size;
  mutex* m_mutex_duration;
  chrono::duration<double> m_time_init;
  chrono::duration<double> m_time;
  vector<tuple<size_t, ActionType>> m_duration_actions;
  vector<tuple<size_t, ActionType>> m_duration_offset_actions;
  // chrono::duration<double> m_time(0);
  // chrono::duration<double> m_time = std::chrono::duration<double>::zero();
  // chrono::duration<double> m_time(std::chrono::duration<double>::zero());
  // std::chrono::seconds sec(1);
  // std::chrono::seconds sec;
  // chrono::duration
};

}  // namespace clb

#endif /* CLBALANCER_DEVICE_HPP */
