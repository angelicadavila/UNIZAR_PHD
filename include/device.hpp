/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef ENGINECL_DEVICE_HPP
#define ENGINECL_DEVICE_HPP 1

#include <CL/cl.hpp>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <functional>
#include <tuple>
#include <utility>

#include <cmath>

#include "buffer.hpp"
#include "clutils.hpp"
#include "config.hpp"
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

namespace ecl {
enum class ActionType;
class Scheduler;
class Runtime;
class Device;
class Buffer;
class NDRange;

void
device_thread_func(Device& device);

enum class ProgramType
{
  Source = 0,
  CustomSource = 1,
  CustomBinary = 2,
};

enum class ArgType
{
  T = 0,
  Vector = 1,
  LocalAlloc = 2,
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
  void setBarrier_init(shared_ptr<semaphore> barrier_init);

  template<typename T>
  void setInBuffer(shared_ptr<T> array)
  {
    Buffer b(Direction::In);
    b.set(array);
    //b.set(array,getLimMemory() );
    m_in_ecl_buffers.push_back(move(b));

    auto address = array.get();
    m_in_buffers_ptr.push_back(address);

    cout << "ecl::Buffer in get: " << b.get() << "\n";
    cout << "ecl::Buffer in data: " << b.data() << "\n";
    cout << "array: " << array.get() << "\n";
    cout << "address: " << address << "\n";
  }
  
  template<typename T>
  void setInBuffer(shared_ptr<T> array,uint constant)
  {
    Buffer b(Direction::In);
    b.set(array, constant);
    //b.set(array,getLimMemory() );
    m_in_ecl_buffers.push_back(move(b));

    auto address = array.get();
    m_in_buffers_ptr.push_back(address);

    cout << "ecl::Buffer in get: " << b.get() << "\n";
    cout << "ecl::Buffer in data: " << b.data() << "\n";
    cout << "array: " << array.get() << "\n";
    cout << "address: " << address << "\n";
  }


  template<typename T>
  void setOutBuffer(shared_ptr<T> array)
  {
    Buffer b(Direction::Out);
    b.set(array);
   // b.set(array, getLimMemory());
    m_out_ecl_buffers.push_back(move(b));

    auto address = array.get();
    m_out_buffers_ptr.push_back(address);

    cout << "ecl::Buffer out get: " << b.get() << "\n";
    cout << "ecl::Buffer out data: " << b.data() << "\n";
    cout << "array: " << array.get() << "\n";
    cout << "address: " << address << "\n";
  }
  template<typename T>
  void setOutAuxBuffer(shared_ptr<T> array)
  {
    Buffer b(Direction::Out);
    //b.set(array);
    b.set(array, getLimMemory());
    m_out_aux_ecl_buffers.push_back(move(b));

    auto address = array.get();
    //m_out_aux_buffers_ptr.push_back(address);

    cout << "ecl::Buffer out get: " << b.get() << "\n";
    cout << "ecl::Buffer out data: " << b.data() << "\n";
    cout << "array: " << array.get() << "\n";
    cout << "address: " << address << "\n";
  }


/*  template<typename T>
  void setKernel(const T& file,
                 vector <size_t> global_work, 
                 vector <size_t> local_work)
  {
     if constexpr (std::is_same_v<T, string>)
    {
      m_program_type = ProgramType::CustomSource;
      m_program_source= file;
    }
    else{
      m_program_type = ProgramType::CustomBinary;
      m_program_binary = file;
    }
  //initializing the parameters of kernel execution
    m_gws=vector <size_t>(3,1);
    m_lws=vector <size_t>(3,1);
    for( int i=0; i<(int)global_work.size();i++)
      m_gws[i]=global_work[i];
   
    for( int i=0; i<(int)local_work.size();i++)
      m_lws[i]=local_work[i];
  }
  */

  void setKernel(const string& file,
                 vector <size_t> global_work, 
                 vector <size_t> local_work)
  {
      m_program_type = ProgramType::CustomSource;
      m_program_source= file;
  //initializing the parameters of kernel execution
    m_gws=vector <size_t>(3,1);
    m_lws=vector <size_t>(3,1);
    for( int i=0; i<(int)global_work.size();i++)
      m_gws[i]=global_work[i];
   
    for( int i=0; i<(int)local_work.size();i++)
      m_lws[i]=local_work[i];
  }


  void setKernel(const vector<char>& file,
                 vector <size_t> global_work, 
                 vector <size_t> local_work)
  {
      m_program_type = ProgramType::CustomBinary;
      m_program_binary = file;
  //initializing the parameters of kernel execution
    m_gws=vector <size_t>(3,1);
    m_lws=vector <size_t>(3,1);
    for( int i=0; i<(int)global_work.size();i++)
      m_gws[i]=global_work[i];
   
    for( int i=0; i<(int)local_work.size();i++)
      m_lws[i]=local_work[i];
  }

  void setKernel(const string& source);
  void setKernel(const vector<char>& source);
  void setKernel(const string& source, const string& kernel); // used by Runtime/Scheduler
  void setID(int id);
  int getID();
  void waitWork();
  void notifyWork();
  //funtion for the second queue
  void wait_queue();
  
  void printStats();

  void saveDuration(ActionType action);
  void saveDurationOffset(ActionType action);

  template<typename T>
  void setKernelArg(cl_uint index, const T& value)
  {
    IF_LOGGING(cout << "setKernelArg T index: " << index << "\n");
    m_arg_index.push_back(index);
    // m_arg_size.push_back(OpenCL::KernelArgumentHandler<T>::size(value));
    m_arg_type.push_back(ArgType::T);
    m_arg_bytes.push_back(OpenCL::KernelArgumentHandler<T>::size(value));
    IF_LOGGING(cout << OpenCL::KernelArgumentHandler<T>::size(value) << "\n");
    // m_arg_bytes.push_back(sizeof(OpenCL::KernelArgumentHandler<T>::size(value)));
    // NOTE legacy OpenCL 1.2 (added (void*))
    // m_arg_ptr.push_back(OpenCL::KernelArgumentHandler<T>::ptr(value));
    m_arg_ptr.push_back((void*)OpenCL::KernelArgumentHandler<T>::ptr(value));
    m_nargs++;
  }
 
  template<typename T, typename Ta>
  void setKernelArg(cl_uint index, const shared_ptr<vector<T,Ta>>& value)
  {
    IF_LOGGING(cout << "setKernelArg shared_ptr T index: " << index << "\n");
    m_arg_index.push_back(index);
    auto address = value.get();

    auto bytes = sizeof(T) * address->size();
    IF_LOGGING(cout << "bytes: " << bytes << "\n");

    m_arg_type.push_back(ArgType::Vector);
    m_arg_bytes.push_back(bytes);
    m_arg_ptr.push_back(address);
    m_nargs++;
  }

  void setKernelArg(cl_uint index, const uint bytes, ArgType type);

  void setKernelArgLocalAlloc(cl_uint index, const uint bytes);
  
  void notifyEvent();
  void waitEvent();

  void notifyRun();
  void waitRun();

   void setLWS(size_t lws);

  // Thread API
  void init();
  void show();
  void notifyBarrier();
  void barrier_init();
  string& getBuffer();
  void showInfo();

  void do_work(size_t offset, size_t size, float bound, int queue_index);
//  void do_work(size_t offset, size_t size, int queue_index);

  void waitFinish();
  Runtime* getRuntime();
  void setRuntime(Runtime* runtime);
  //number of data process by work item 
  void setInternalChunk(int internal_chunk);
  int getInternalChunk();
  //limit of memory per device
  void setLimMemory(int limit_memory);
  int getLimMemory();
  
  void set_globalWorkSize( size_t gws0);
  void set_globalWorkSize( size_t gws0, size_t gws1);
  void set_globalWorkSize( size_t gws0, size_t gws1,size_t gws2);

  void set_localWorkSize( size_t lws0);
  void set_localWorkSize( size_t lws0, size_t lws1);
  void set_localWorkSize( size_t lws0, size_t lws1,size_t lws2);
  
  void readBuffers();

private:
  void useRuntimeDiscovery();
  void initByIndex(uint sel_platform, uint sel_device);
  void initContext();
  void initQueue();
  void initBuffers();
  void writeBuffers(bool dummy = false);
  void writeBuffers(size_t size, size_t offset);
  void initKernel();
  void initEvents();

  uint m_sel_platform;
  uint m_sel_device;

  Scheduler* m_sched;
  Runtime* m_runtime;

  shared_ptr<semaphore> m_barrier;
  shared_ptr<semaphore> m_barrier_init;

  thread m_thread;
  string m_info_buffer;
#pragma GCC diagnostic ignored "-Wignored-attributes"
  vector<cl_uint> m_arg_index;
#pragma GCC diagnostic pop
  vector<size_t> m_gws;
  vector<size_t> m_lws;
  vector<ArgType> m_arg_type;
  vector<size_t> m_arg_size;
  uint m_nargs;
  vector<size_t> m_arg_bytes;
  vector<void*> m_arg_ptr; // NOTE legacy OpenCL 1.2

  vector<void*> m_in_buffers_ptr;
  vector<cl::Buffer> m_in_buffers;
  vector<void*> m_out_buffers_ptr;
  vector<void*> m_out_aux_buffers_ptr;
  vector<cl::Buffer> m_out_buffers;
  vector<cl::Buffer> m_out_aux_buffers;

  vector<ecl::Buffer> m_in_ecl_buffers;
  vector<ecl::Buffer> m_out_ecl_buffers;
  vector<ecl::Buffer> m_out_aux_ecl_buffers;
  vector<cl_uint> m_out_arg_index;
  vector<cl_uint> m_out_arg_pos;
  uint switch_out;

  cl::Platform m_platform;
  cl::Device m_device;
  cl::Context m_context;
  cl::CommandQueue m_queue;
  cl::CommandQueue m_queueRead;
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

   //internal chunk size execute in the kernel
  int m_internal_chunk;

  //Memory limit for device
  int m_lim_memory;

  vector<size_t> m_prev_readParams;
//   size_t m_lws;
};

} // namespace ecl

#endif /* CLBALANCER_DEVICE_HPP */
