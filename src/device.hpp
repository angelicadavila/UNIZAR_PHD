/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of clbalancer which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef CLBALANCER_DEVICE_HPP
#define CLBALANCER_DEVICE_HPP 1

#include <CL/cl.hpp>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <tuple>
#include <utility>

#include "buffer.hpp"
#include "clutils.hpp"
#include "semaphore.hpp"

using std::cout;
using std::move;
using std::mutex;
using std::shared_ptr;
using std::string;
using std::thread;
using std::to_string;
using std::tuple;
using std::unique_ptr;
using std::vector;

namespace clb {
enum class ActionType;
class Scheduler;
class Runtime;
class Device;
class Buffer;

void
device_thread_func(Device& device);

enum class ProgramType
{
  Source = 0,
  CustomSource = 1,
  CustomBinary = 2,
};

class Device
{
public:
  // Device() = default;
  Device(uint sel_platform, uint sel_device);
  ~Device();

  Device(Device const&) = delete;
  Device& operator=(Device const&) = delete;

  Device(Device&&) = default;
  Device& operator=(Device&&) = default;

  // Public API
  void start();
  void setScheduler(Scheduler* sched);
  Scheduler* getScheduler();
  void setBarrier(shared_ptr<semaphore> barrier);

  template<typename T>
  void setInBuffer(shared_ptr<vector<T>> array)
  {
    Buffer b(Direction::In);
    b.set(array);
    m_in_clb_buffers.push_back(move(b));

    auto address = array.get();
    m_in_buffers_ptr.push_back(address);

    cout << "clb::Buffer in get: " << b.get() << "\n";
    cout << "clb::Buffer in data: " << b.data() << "\n";
    cout << "array: " << array.get() << "\n";
    cout << "address: " << address << "\n";
  }
  template<typename T>
  void setOutBuffer(shared_ptr<vector<T>> array)
  {
    Buffer b(Direction::Out);
    b.set(array);
    m_out_clb_buffers.push_back(move(b));

    auto address = array.get();
    m_out_buffers_ptr.push_back(address);

    cout << "clb::Buffer out get: " << b.get() << "\n";
    cout << "clb::Buffer out data: " << b.data() << "\n";
    cout << "array: " << array.get() << "\n";
    cout << "address: " << address << "\n";
  }

  void setKernel(const string& source);
  void setKernel(const vector<char>& source);
  void setKernel(const string& source, const string& kernel); // used by Runtime/Scheduler
  void setID(int id);
  int getID();
  void waitWork();
  void notifyWork();

  void printStats();

  void saveDuration(ActionType action);
  void saveDurationOffset(ActionType action);

  template<typename T>
  void setKernelArg(cl_uint index, const T& value)
  {
    cout << "setKernelArg T index: " << index << "\n";
    m_arg_index.push_back(index);
    m_arg_size.push_back(OpenCL::KernelArgumentHandler<T>::size(value));
    m_arg_bytes.push_back(OpenCL::KernelArgumentHandler<T>::size(value));
    cout << OpenCL::KernelArgumentHandler<T>::size(value) << "\n";
    // m_arg_bytes.push_back(sizeof(OpenCL::KernelArgumentHandler<T>::size(value)));
    // NOTE legacy OpenCL 1.2 (added (void*))
    // m_arg_ptr.push_back(OpenCL::KernelArgumentHandler<T>::ptr(value));
    m_arg_ptr.push_back((void*)OpenCL::KernelArgumentHandler<T>::ptr(value));
    m_nargs++;
  }

  template<typename T>
  void setKernelArg(cl_uint index, const shared_ptr<T>& value)
  {
    cout << "setKernelArg shared_ptr T index: " << index << "\n";
    m_arg_index.push_back(index);
    auto address = value.get();
    m_arg_size.push_back(0);
    m_arg_bytes.push_back(0);
    m_arg_ptr.push_back(address);
    m_nargs++;
  }

  void setKernelArg(cl_uint index, ::size_t size, const void* ptr);

  void notifyEvent();
  void waitEvent();

  void notifyRun();
  void waitRun();

  // Thread API
  void init();
  void show();
  void notifyBarrier();
  string& getBuffer();
  void showInfo();

  void do_work(size_t offset, size_t size, int queue_index);

  Runtime* getRuntime();
  void setRuntime(Runtime* runtime);

private:
  void useRuntimeDiscovery();
  void initByIndex(uint sel_platform, uint sel_device);
  void initContext();
  void initQueue();
  void initBuffers();
  void writeBuffers(bool dummy = false);
  void initKernel();
  void initEvents();

  uint m_sel_platform;
  uint m_sel_device;

  Scheduler* m_sched;
  Runtime* m_runtime;

  shared_ptr<semaphore> m_barrier;

  thread m_thread;
  string m_info_buffer;
#pragma GCC diagnostic ignored "-Wignored-attributes"
  vector<cl_uint> m_arg_index;
#pragma GCC diagnostic pop
  vector<size_t> m_arg_size;
  uint m_nargs;
  vector<size_t> m_arg_bytes;
  vector<void*> m_arg_ptr; // NOTE legacy OpenCL 1.2

  vector<void*> m_in_buffers_ptr;
  vector<cl::Buffer> m_in_buffers;
  vector<void*> m_out_buffers_ptr;
  vector<cl::Buffer> m_out_buffers;

  vector<clb::Buffer> m_in_clb_buffers;
  vector<clb::Buffer> m_out_clb_buffers;

  cl::Platform m_platform;
  cl::Device m_device;
  cl::Context m_context;
  cl::CommandQueue m_queue;
  cl::Kernel m_kernel;
  cl::UserEvent m_end;
  string m_kernel_str;

  vector<cl::Event> m_prev_events;

  int m_id;
  semaphore* m_sema_work;
  unique_ptr<semaphore> m_sema_run;

  size_t m_works;
  size_t m_works_size;
  mutex* m_mutex_duration;
  std::chrono::duration<double> m_time_init;
  std::chrono::duration<double> m_time;
  vector<tuple<size_t, ActionType>> m_duration_actions;
  vector<tuple<size_t, ActionType>> m_duration_offset_actions;

  string m_program_source;
  vector<char> m_program_binary;
  ProgramType m_program_type;
};

} // namespace clb

#endif /* CLBALANCER_DEVICE_HPP */
