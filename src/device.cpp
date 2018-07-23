/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of eclalancer which is released under MIT License.
 * See file LICENSE for full license details.
 */
#include "./device.hpp"

#include "buffer.hpp"
#include "inspector.hpp"
#include "runtime.hpp"
#include "scheduler.hpp"

#include <chrono>
#include <thread>
#include <iostream>
#include <functional>
struct CBData
{
  int queue_index;
  ecl::Device* device;
  CBData(int queue_index_, ecl::Device* device_)
    : queue_index(queue_index_)
    , device(device_)
  {
  }
};

void CL_CALLBACK
callbackRead(cl_event /*event*/, cl_int /*status*/, void* data)
{
  IF_LOGGING(cout << "CB\n");
  CBData* cbdata = reinterpret_cast<CBData*>(data);
  ecl::Device* device = cbdata->device;
  IF_LOGGING(cout << device->getID() << " callback " << cbdata->queue_index << "\n");
  ecl::Scheduler* sched = device->getScheduler();
  device->saveDuration(ecl::ActionType::completeWork);
  sched->callback(cbdata->queue_index);
  delete cbdata;
}

void oclContextCallback (const char *errinfo, const void *, size_t, void *) {
     printf ("Context callback: %s\n", errinfo);
 }


namespace ecl {

void
device_thread_func(Device& device)
{

  device.init();
//  if (device.getID()==1)
//	  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  device.barrier_init();

  auto time1 = std::chrono::system_clock::now().time_since_epoch();
  cout<<"----------init device:"<<device.getID()<<"\n";
  Scheduler* sched = device.getScheduler();
  device.saveDuration(ActionType::deviceStart);
  device.saveDurationOffset(ActionType::deviceStart);

  Runtime* runtime = device.getRuntime();
  device.saveDuration(ActionType::deviceReady);
  device.saveDurationOffset(ActionType::deviceReady);
  runtime->notifyReady();
  runtime->notifyAllReady();

  device.waitRun();
  device.saveDuration(ActionType::deviceRun);
  device.saveDurationOffset(ActionType::deviceRun);

  sched->req_work(&device);

  auto cont = true;
  while (cont) {
    device.waitWork();
    auto queue_index = sched->getWorkIndex(&device);

    if (queue_index >= 0) { //
      Work work = sched->getWork(queue_index);

      device.do_work(work.offset, work.size, work.bound, queue_index);
    } else {
      IF_LOGGING(cout << "device id " << device.getID() << " finished\n");
     
      //Read the last chunk
      //NOTE set m_prev_events in last chunk from sched to avoid overlapping
      device.readBuffers();
      device.wait_queue();
      sched->endScheduler();
      cout << "device id " << device.getID() << " finished\n";
			device.notifyBarrier();
      cont = false;
    }
  }

  device.saveDuration(ActionType::deviceEnd);
  device.saveDurationOffset(ActionType::deviceEnd);
}

Device::Device(uint sel_platform, uint sel_device)
  : m_sel_platform(sel_platform)
  , m_sel_device(sel_device)
  , m_nargs(0)
  , m_program_type(ProgramType::Source)
{
#pragma GCC diagnostic ignored "-Wtype-limits"
  if (!(sel_platform >= 0 && sel_device >= 0)) {
    throw runtime_error("platform and device are zero-indexed");
  }
#pragma GCC diagnostic pop
  m_mutex_duration = new mutex();
  m_time_init = std::chrono::system_clock::now().time_since_epoch();
  m_time = std::chrono::system_clock::now().time_since_epoch();
  m_works = 0;
  m_works_size = 0;
  m_sema_work = new semaphore(1);
  m_sema_run.reset(new semaphore(1)); //  = make_unique<semaphore>(1);
  m_duration_actions.reserve(1024);     // NOTE to improve
  m_duration_offset_actions.reserve(8); // NOTE to improve
  m_gws= m_gws=vector <size_t>(3,1);
  m_lws=vector <size_t>(3,1);
  //default values
  m_gws[0]=0;//to set the chunk size
  m_lws[0]=128;

  //define the buffer of aoutput, between global and auxiliar
  switch_out=0;
//  m_lws[0]=64;
}

Device::~Device()
{
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void
Device::printStats()
{
  cout << "Device id: " << getID() << "\n";
  showInfo();
  cout << "program type: ";
  switch (m_program_type) {
    case ProgramType::CustomBinary:
      cout << "custom binary\n";
      break;
    case ProgramType::CustomSource:
      cout << "custom source\n";
      break;
    default:
      cout << "source\n";
  }
  cout << "works: " << m_works << " works_size: " << m_works_size << "\n";
  size_t acc = 0;
  size_t total = 0;
  cout << "duration increments:\n";
  for (auto& t : m_duration_actions) {
    auto d = std::get<0>(t);
    auto action = std::get<1>(t);
    if (action == ActionType::completeWork) {
      acc += d;
    } else {
      Inspector::printActionTypeDuration(action, d);
    }
    total += d;
  }
  cout << " completeWork: " << acc << " us.\n";
  cout << " total: " << total << " us.\n";
  cout << "duration offsets from init:\n";
  for (auto& t : m_duration_offset_actions) {
    Inspector::printActionTypeDuration(std::get<1>(t), std::get<0>(t));
  }
}

void
Device::saveDuration(ActionType action)
{
  lock_guard<mutex> lock(*m_mutex_duration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - m_time).count();
  m_duration_actions.push_back(make_tuple(diff_ms, action));
  m_time = t2;
}
void
Device::saveDurationOffset(ActionType action)
{
  lock_guard<mutex> lock(*m_mutex_duration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - m_time_init).count();
  m_duration_offset_actions.push_back(make_tuple(diff_ms, action));
}

void
Device::setKernelArg(cl_uint index, const uint bytes, ArgType type)
{
  if (type != ArgType::LocalAlloc) {
    throw runtime_error("setLocalArg(uint, uint, ArgType) only admits ArgType::LocalAlloc");
  }
  IF_LOGGING(cout << "setKernelArg bytes ArgType index: " << index << "\n");
  m_arg_index.push_back(index);

  IF_LOGGING(cout << "bytes: " << bytes << "\n");

  m_arg_type.push_back(type);
  m_arg_bytes.push_back(bytes);
  m_arg_ptr.push_back(NULL);
  m_nargs++;
}

void
Device::setKernelArgLocalAlloc(cl_uint index, const uint bytes)
{
  IF_LOGGING(cout << "setKernelArgLocalAlloc index: " << index << "\n");
  m_arg_index.push_back(index);

  IF_LOGGING(cout << "bytes: " << bytes << "\n");

  m_arg_type.push_back(ArgType::LocalAlloc);
  // m_arg_size.push_back(-1);
  m_arg_bytes.push_back(bytes);
  m_arg_ptr.push_back(NULL);
  m_nargs++;
}

void
Device::setScheduler(Scheduler* sched)
{
  m_sched = sched;
}
Scheduler*
Device::getScheduler()
{
  return m_sched;
}

void
Device::setID(int id)
{
  m_id = id;
}
int
Device::getID()
{
  return m_id;
}

void
Device::waitRun()
{
  m_sema_run.get()->wait(1);
}

void
Device::notifyRun()
{
  m_sema_run.get()->notify(1);
}

void
Device::waitWork()
{
  m_sema_work->wait(1);
}

void
Device::notifyWork()
{
  m_sema_work->notify(1);
}

string&
Device::getBuffer()
{
  return m_info_buffer;
}

void
Device::start()
{
  m_thread = thread(device_thread_func, std::ref(*this));
}
void
Device::useRuntimeDiscovery()
{
  Runtime* runtime = m_runtime;
  IF_LOGGING(cout << m_sel_platform << " - " << m_sel_device << "\n");
  m_platform = runtime->usePlatformDiscovery(m_sel_platform);
  m_device = runtime->useDeviceDiscovery(m_sel_platform, m_sel_device);
}

//use by runtime
void
Device::setKernel(const string& source, const string& kernel)
{
  if (m_program_type == ProgramType::Source) {
    m_program_source = source;
  } else {
    IF_LOGGING(cout << "Using custom Kernel\n");
  }
  m_kernel_str = kernel;
}

void
Device::setKernel(const vector<char>& binary)
{
  IF_LOGGING(cout << "Provided binary Kernel\n");
  m_program_type = ProgramType::CustomBinary;
  m_program_binary = binary;
}

void
Device::setKernel(const string& source)
{
  IF_LOGGING(cout << "Provided source Kernel\n");
  m_program_type = ProgramType::CustomSource;
  m_program_source = source;
}

void
Device::waitEvent()
{
  cl::Event::waitForEvents({ m_end });
}
void
Device::notifyEvent()
{
  m_end.setStatus(CL_COMPLETE);
}

void
Device::setBarrier(shared_ptr<semaphore> barrier)
{
  m_barrier = barrier;
}

void
Device::setBarrier_init(shared_ptr<semaphore> barrier)
{
  m_barrier_init = barrier;
}

void
Device::do_work(size_t offset, size_t size, float bound, int queue_index)
{
  if (!size) {
    return callbackRead(nullptr, CL_COMPLETE, this);
  }
/*  if (m_prev_events.size() && m_works) {
    m_prev_events.clear();
  }
*/
//  cout<<"offset:"<<offset<<" device:"<<getID()<<"\n";
 
 cl::Event m_event_kernel; 
 auto offset_for_bytes=offset;//use to read offset_bytes
  offset*=m_internal_chunk;
  cl_int status;
  if(m_gws[0]!=1) 
    m_gws[0] = size;//pass to global
#if ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED == 1
  status=m_queue.enqueueNDRangeKernel(m_kernel,
                               cl::NDRange(offset),
                               cl::NDRange(gws),
                               cl::NDRange(CL_LWS),
                               nullptr,
                               nullptr);
#else
 int len=m_out_arg_index.size();
 for (int j=0;j<len;j++){
    if (switch_out==0){
      m_kernel.setArg(m_out_arg_index[j],m_out_buffers[m_out_arg_pos[j]]);
    }else{
      m_kernel.setArg(m_out_arg_index[j],m_out_aux_buffers[m_out_arg_pos[j]]);
    }
}
 m_kernel.setArg(m_nargs,(uint) size);
 //m_kernel.setArg(m_nargs+1,(uint) offset);
 uint static_offset=0;
 m_kernel.setArg(m_nargs+1,(uint) static_offset);
 cout<<"offset: "<<offset<<" size:"<< size<<"\n gws:"<<m_gws[0]<<"-lws: "<<m_lws[0]<<"\n";
 //if(getID()==0)
 
 
   writeBuffers(size,offset);
{
  status=m_queue.enqueueNDRangeKernel(
                          m_kernel, cl::NullRange, 
                          cl::NDRange(m_gws[0],m_gws[1],m_gws[2]), 
                          cl::NDRange(m_lws[0],m_lws[1],m_lws[2]),
                          nullptr ,&(m_event_kernel));
                          
  CL_CHECK_ERROR(status,"NDRange problem");
  }
#endif
  
  //m_queue.finish();
  //Conditional Read to overlapping in a Kernel_i with read_i-1
  //*NOTE: to avoid overlapping init in sched the vector m_Prev_readParams

  m_prev_readParams[0]=size; 
  m_prev_readParams[1]=offset_for_bytes;
 
  readBuffers();
#if ECL_OPERATION_BLOCKING_READ == 1
  
  //wait finish kernel
 // m_queue.finish();
  
  #if ECL_PROFILING ==1
  ulong time_qkrn, time_skrn,time_stkrn, time_ekrn;
  
  time_skrn=m_event_kernel.getProfilingInfo<CL_PROFILING_COMMAND_SUBMIT>();
  time_qkrn= m_event_kernel.getProfilingInfo<CL_PROFILING_COMMAND_QUEUED>();
  time_stkrn= m_event_kernel.getProfilingInfo<CL_PROFILING_COMMAND_START>();
  time_ekrn= m_event_kernel.getProfilingInfo<CL_PROFILING_COMMAND_END>();
  cout<<"**time_krn "<<getID()<<": "<<time_qkrn<<","<<time_skrn<<","<<time_stkrn<<","<<time_ekrn<<" \n";
  #endif
  ecl::Scheduler* sched = getScheduler();
  sched->callback(queue_index);
  saveDuration(ecl::ActionType::completeWork);

#else
  auto cbdata = new CBData(queue_index, this);
 // exit(0);
 //evread.setCallback(CL_COMPLETE, callbackRead, cbdata);
  m_queue.flush();
#endif  
  m_works++;
  m_works_size += size;
}

void
Device::init()
{
  m_time = std::chrono::system_clock::now().time_since_epoch();
  m_info_buffer.reserve(128);
  saveDuration(ActionType::init);
  saveDurationOffset(ActionType::init);

  useRuntimeDiscovery();
  saveDuration(ActionType::useDiscovery);
  saveDurationOffset(ActionType::useDiscovery);

  initContext();
  saveDuration(ActionType::initContext);
  initQueue();
  saveDuration(ActionType::initQueue);
  initBuffers();
  saveDuration(ActionType::initBuffers);
  saveDurationOffset(ActionType::initBuffers);
  initKernel();
  saveDuration(ActionType::initKernel);
  saveDurationOffset(ActionType::initKernel);
  initEvents();
  //writeBuffers();
  saveDuration(ActionType::writeBuffers);
  saveDurationOffset(ActionType::writeBuffers);
  // work();
  m_prev_readParams=  vector<size_t>(2,0);   
   //std::string namefile="index_dev_";//+getID();
}

void
Device::notifyBarrier()
{
  m_barrier.get()->notify(1);
}
//wait all device initialization
void
Device::barrier_init()
{
  m_barrier_init.get()->notify(1);
  m_barrier_init.get()->wait(0);
  m_barrier_init.get()->notify(1);
}
/**
 * \brief Does not check bounds
 */
void
Device::initByIndex(uint sel_platform, uint sel_device)
{
  IF_LOGGING(cout << "initByIndex\n");
  IF_LOGGING(cout << sel_platform << " - " << sel_device << "\n");
  vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);
  m_platform = platforms.at(sel_platform);
  IF_LOGGING(cout << sel_platform << " - " << sel_device << "\n");
  vector<cl::Device> devices;
  m_platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
  m_device = devices.at(sel_device);
  IF_LOGGING(cout << sel_platform << " - " << sel_device << "\n");
}

void
Device::initContext()
{
  cl_int cl_err;
  cout << "initContext\n";
  vector<cl::Device>tmp_device(1,m_device);
  
  //cl::Context context(m_device, nullptr,&oclContextCallback,nullptr,&cl_err );
  cl::Context context(tmp_device, nullptr,&oclContextCallback,nullptr,&cl_err );
  CL_CHECK_ERROR(cl_err, "Context");
  m_context = move(context);
}

void
Device::initQueue()
{
  IF_LOGGING(cout << "initQueue\n");
  cl_int cl_err;

  cl::Context& context = m_context;
  cl::Device& device = m_device;
  
  cl::CommandQueue queue(context, device, 
                          #if CLB_PROFILING==1
                          CL_QUEUE_PROFILING_ENABLE,
                          #else
                          0,
                          #endif
                          &cl_err);
  CL_CHECK_ERROR(cl_err, "CommandQueue queue");
  m_queue = move(queue);
  
  cl::CommandQueue queueRead(context, device,
                          #if CLB_PROFILING==1
                          CL_QUEUE_PROFILING_ENABLE,
                          #else
                          0,
                          #endif
                          &cl_err);
  CL_CHECK_ERROR(cl_err, "CommandQueue queueRead");
  m_queueRead = move(queueRead);

}

void
Device::initBuffers()
{
  IF_LOGGING(cout << "initBuffers\n");
  cl_int cl_err = CL_SUCCESS;

  cl_int buffer_in_flags = CL_MEM_READ_WRITE;
  //cl_int buffer_in_flags[] = {CL_MEM_READ_WRITE| CL_MEM_BANK_1_ALTERA,CL_MEM_READ_WRITE| CL_MEM_BANK_2_ALTERA};
  //cl_int buffer_out_flags = CL_MEM_READ_WRITE| CL_MEM_BANK_1_ALTERA;
  cl_int buffer_out_flags = CL_MEM_READ_WRITE;

  m_in_buffers.reserve(m_in_ecl_buffers.size());
  m_out_buffers.reserve(m_out_ecl_buffers.size());
  m_out_aux_buffers.reserve(m_out_ecl_buffers.size());
  
  auto len = m_in_ecl_buffers.size();
  for (uint i = 0; i < len; ++i) {
    ecl::Buffer& b = m_in_ecl_buffers[i];
    auto data = b.data();
    IF_LOGGING(cout << "in [data] " << data << "\n");
    IF_LOGGING(cout << "in [address] " << b.get() << "\n");
    IF_LOGGING(cout << "in [size] " << b.size() << "\n");
    uint lim_size=getLimMemory();
    IF_LOGGING(cout << "in [bytes] " <<  lim_size << "\n");
    cl::Buffer tmp_buffer(m_context, buffer_in_flags, lim_size, NULL);
    CL_CHECK_ERROR(cl_err, "in buffer " + i);
    m_in_buffers.push_back(move(tmp_buffer));
    IF_LOGGING(cout << "in buffer: " << &m_in_buffers[i] << "\n");
  }

  len = m_out_ecl_buffers.size();
  for (uint i = 0; i < len; ++i) {
    ecl::Buffer& b = m_out_ecl_buffers[i];
    auto data = b.data();
    IF_LOGGING(cout << "out [data] " << data << "\n");
    IF_LOGGING(cout << "out [address] " << b.get() << "\n");
    IF_LOGGING(cout << "out [size] " << b.size() << "\n");
    uint lim_size=getLimMemory();
    IF_LOGGING(cout << "out [bytes] " << lim_size << "\n");
    cl::Buffer tmp_buffer(m_context, buffer_out_flags, lim_size, NULL);
    CL_CHECK_ERROR(cl_err, "out buffer " + i);
    m_out_buffers.push_back(move(tmp_buffer));
    IF_LOGGING(cout << "out buffer: " << &m_out_buffers[i] << "\n");
  }
  len = m_out_aux_ecl_buffers.size();
  for (uint i = 0; i < len; ++i) {
    ecl::Buffer& b = m_out_aux_ecl_buffers[i];
    auto data = b.data();
    IF_LOGGING(cout << "out [data] " << data << "\n");
    IF_LOGGING(cout << "out [address] " << b.get() << "\n");
    IF_LOGGING(cout << "out [size] " << b.size() << "\n");
    uint lim_size=getLimMemory();
    IF_LOGGING(cout << "out [bytes] " <<  lim_size<< "\n");
    cl::Buffer tmp_buffer(m_context, buffer_out_flags, lim_size, NULL);
    CL_CHECK_ERROR(cl_err, "out buffer " + i);
    m_out_aux_buffers.push_back(move(tmp_buffer));
    IF_LOGGING(cout << "out buffer: " << &m_out_aux_buffers[i] << "\n");
  }
}
void
Device::writeBuffers(size_t size, size_t offset)
{

  auto len = m_in_ecl_buffers.size();
  for (uint i = 0; i < len; ++i) {
    Buffer& b = m_in_ecl_buffers[i];
    auto data= b.dataWithOffset(offset);//*m_internal_chunk);
    size_t size_bytes = b.byBytes(size)*m_internal_chunk;
//    IF_LOGGING(cout << "writeBuffers [array] " << b.get() << " data: " << data << " buffer: "
//                   << &m_in_buffers[i] << " size: " << size_bytes << " bytes: " << b.bytes() << "\n");

  CL_CHECK_ERROR(m_queue.enqueueWriteBuffer(
      m_in_buffers[i], CL_TRUE, 0, size_bytes, data, NULL,NULL ));//
  }
}
//void
//Device::writeBuffers(bool /* dummy */)
//{
//  IF_LOGGING(cout << "writeBuffers\n");
//
//  auto len = m_in_ecl_buffers.size();
//  m_prev_events.reserve(len);
//  m_prev_events.resize(len);
//  for (uint i = 0; i < len; ++i) {
//    Buffer& b = m_in_ecl_buffers[i];
//    auto data = b.data();
//    auto size = b.size();
//    IF_LOGGING(cout << "writeBuffers [array] " << b.get() << " data: " << data << " buffer: "
//                    << &m_in_buffers[i] << " size: " << size << " bytes: " << b.bytes() << "\n");
//    CL_CHECK_ERROR(m_queue.enqueueWriteBuffer(
//      m_in_buffers[i], CL_TRUE, 0, b.bytes(), data, NULL,NULL ));//&(m_prev_events.data()[i])));
//  }
//  m_queue.finish();
//}

void
Device::readBuffers()
{
  #if CLB_PROFILING == 1
  cl::Event m_event_read;
  #endif
  size_t sizeR=m_prev_readParams[0]; 
  size_t offsetR=m_prev_readParams[1];
  auto len = m_out_ecl_buffers.size();
  cl_int status;  
  if(sizeR!=0)
  {
    for (uint i = 0; i < len; ++i) {
        Buffer& b = m_out_ecl_buffers[i];
        size_t size_bytes = b.byBytes(sizeR)*m_internal_chunk;
        auto offset_bytes = b.byBytes(offsetR)*m_internal_chunk;
      auto address= offset_bytes;
      if(address & 0x3){
       cout<<"unaligned \n";
       }
      cout<< "Read sizebyte: "<<size_bytes<<" offsetby: "<<offset_bytes<<"\n";
      //status= m_queueRead.enqueueReadBuffer(m_out_buffers[i],
      if (switch_out==0){
          status= m_queue.enqueueReadBuffer(m_out_buffers[i],
                                  CL_TRUE,0,
                                //  offset_bytes,
                                  size_bytes,
                                  b.dataWithOffset(offsetR*m_internal_chunk),
                                  nullptr,
                            #if CLB_PROFILING == 1
                                  &m_event_read);
                            #else 
                                  nullptr);
                            #endif
        switch_out=1;
      }else{
    
         status= m_queue.enqueueReadBuffer(m_out_aux_buffers[i],
                                  CL_TRUE,0,
                                //  offset_bytes,
                                  size_bytes,
                                  b.dataWithOffset(offsetR*m_internal_chunk),
                                  nullptr,
                            #if CLB_PROFILING == 1
                                  &m_event_read);
                            #else 
                                  nullptr);
                            #endif
        switch_out=0;
      }
        //CL_CHECK_ERROR(status,"Reading memory problem");
      }
  }
  #if CLB_PROFILING == 1
  ulong time_qkrn, time_skrn,time_stkrn, time_ekrn;
  time_skrn=  m_event_read.getProfilingInfo<CL_PROFILING_COMMAND_SUBMIT>();
  time_qkrn=  m_event_read.getProfilingInfo<CL_PROFILING_COMMAND_QUEUED>();
  time_stkrn= m_event_read.getProfilingInfo<CL_PROFILING_COMMAND_START>();
  time_ekrn=  m_event_read.getProfilingInfo<CL_PROFILING_COMMAND_END>();
  cout<<"**time_read "<<getID()<<": "<<time_qkrn<<","<<time_skrn<<","<<time_stkrn<<","<<time_ekrn<<" \n";
  #endif
}

void
Device::initKernel()
{
  IF_LOGGING(cout << "initKernel\n");

  cl_int cl_err;

  cl::Program::Sources sources;
  cl::Program::Binaries binaries;
  cl::Program program;

  if (m_program_type == ProgramType::CustomBinary) {
    IF_LOGGING(cout << "size: " << m_program_binary.size() << "\n");
    binaries.push_back({ m_program_binary.data(), m_program_binary.size() });
#pragma GCC diagnostic ignored "-Wignored-attributes"
    vector<cl_int> status = { -1 };
#pragma GCC diagnostic pop
    program = move(cl::Program(m_context, { m_device }, binaries, &status, &cl_err));
    CL_CHECK_ERROR(cl_err, "building program from binary failed for device " + to_string(m_id));
  } else {
    sources.push_back({ m_program_source.c_str(), m_program_source.length() });
    program = move(cl::Program(m_context, sources));
  }
  string options;
  options.reserve(32);
  options += "-DECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED=" +
             to_string(ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED);

#if ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED == 0
  IF_LOGGING(cout << "Kernel should receive the last argument as 'uint offset' "
                     "(ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED == 0)\n");
#endif

  cl_err = program.build({ m_device }, options.c_str());
  if (cl_err != CL_SUCCESS) {
    cout << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_device) << "\n";
    CL_CHECK_ERROR(cl_err);
  } else {
    IF_LOGGING(cout << " Building info: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_device)
                    << "\n");
  }

  cl::Kernel kernel(program, m_kernel_str.c_str(), &cl_err);
  CL_CHECK_ERROR(cl_err, "kernel");

  auto len = m_arg_index.size();
  auto unassigned = m_in_buffers_ptr.size();
  for (uint i = 0; i < len; ++i) {
    auto index = m_arg_index[i];
    IF_LOGGING(cout << "[i] " << index << "\n");
    auto type = m_arg_type[i];
    if (type == ArgType::Vector) {
      IF_LOGGING(cout << "[ArgType::Vector]\n");
      int pos = -1;
      auto address = m_arg_ptr[i];
      auto assigned = false;
      IF_LOGGING(cout << "[value] " << m_arg_ptr[i] << "\n");
      IF_LOGGING(cout << "[address] " << address << "\n");
      if (unassigned > 0) { // usually more in buffers than out
        auto it = find(begin(m_in_buffers_ptr), end(m_in_buffers_ptr), address);
        IF_LOGGING(cout << "it: " << *it << "\n");
        if (it != end(m_in_buffers_ptr)) {
          pos = distance(m_in_buffers_ptr.begin(), it);
          IF_LOGGING(cout << "address: " << address << " position: " << pos
                          << " buffer: " << &m_in_buffers[pos] << "\n");
          cl_err = kernel.setArg(index, m_in_buffers[pos]);
          CL_CHECK_ERROR(cl_err, "kernel arg in buffer " + to_string(i));
          unassigned--;
          assigned = true;
        }
      }

      if (pos == -1) {
        auto it = find(begin(m_out_buffers_ptr), end(m_out_buffers_ptr), address);
        if (it != end(m_out_buffers_ptr)) {
          auto pos = distance(m_out_buffers_ptr.begin(), it);
          IF_LOGGING(cout << "address: " << address << " position: " << pos
                          << " buffer: " << &m_out_buffers[pos] << "\n");
          cl_err = kernel.setArg(index, m_out_buffers[pos]);
          m_out_arg_index.push_back(index);
          m_out_arg_pos.push_back(pos);
          CL_CHECK_ERROR(cl_err, "kernel arg out buffer " + to_string(i));
          assigned = true;
        }
      }

      if (!assigned) { // maybe empty space (__local)
        if (m_arg_bytes[i] > 0) {
          IF_LOGGING(cout << "ptr not buffer\n");
          IF_LOGGING(cout << "[bytes] " << m_arg_bytes[i] << "\n");
          size_t bytes = m_arg_bytes[i];
          vector<void*>* vptr = reinterpret_cast<vector<void*>*>(m_arg_ptr[i]);
          void* ptr = vptr->data();
          cl_err = kernel.setArg((cl_uint)i, bytes, ptr);
          CL_CHECK_ERROR(cl_err, "kernel arg " + to_string(i));
        } else {
          throw runtime_error("unknown kernel arg address " + to_string(i));
        }
      }
    } else if (type == ArgType::T) {
      IF_LOGGING(cout << "[ArgType::T]\n");
      // IF_LOGGING(cout << "[size] " << m_arg_size[i] << "\n");
      IF_LOGGING(cout << "[value] " << m_arg_ptr[i] << "\n");
      // ArgType size = m_arg_size[i];
      size_t bytes = m_arg_bytes[i];
      void* ptr = m_arg_ptr[i];
      IF_LOGGING(cout << "[bytes] " << bytes << "\n");
      IF_LOGGING(cout << "[mem] " << sizeof(cl_mem) << "\n");
      cl_err = kernel.setArg((cl_uint)i, bytes, ptr);
      
      CL_CHECK_ERROR(cl_err, "kernel arg " + to_string(i));
    } else { // ArgType::LocalAlloc
      IF_LOGGING(cout << "[ArgType::LocalAlloc]\n");
      IF_LOGGING(cout << "empy space __local\n");
      IF_LOGGING(cout << "[bytes] " << m_arg_bytes[i] << "\n");
      size_t bytes = m_arg_bytes[i];
      void* ptr = m_arg_ptr[i];
      // ptr should be NULL
      cl_err = kernel.setArg((cl_uint)i, bytes, ptr);
      CL_CHECK_ERROR(cl_err, "kernel arg " + to_string(i));
    }
  }

  m_kernel = move(kernel);
}

void
Device::setLWS(size_t lws)
{
  //m_lws = lws;
}

void
Device::initEvents()
{
  cl_int cl_err;
  cl::UserEvent end(m_context, &cl_err);
  CL_CHECK_ERROR(cl_err, "user event end");
  m_end = move(end);
}

void
Device::wait_queue()
{
  m_queue.finish();
  m_queueRead.finish();
}

void
Device::showInfo()
{
  CL_CHECK_ERROR(m_platform.getInfo(CL_PLATFORM_NAME, &m_info_buffer));
  if (m_info_buffer.size() > 2)
    m_info_buffer.erase(m_info_buffer.size() - 1, 1);
  cout << "Selected platform: " << m_info_buffer << "\n";
  CL_CHECK_ERROR(m_device.getInfo(CL_DEVICE_NAME, &m_info_buffer));
  if (m_info_buffer.size() > 2)
    m_info_buffer.erase(m_info_buffer.size() - 1, 1);
  cout << "Selected device: " << m_info_buffer << "\n";
}

void
Device::show()
{
  IF_LOGGING(cout << "show\n");
}

Runtime*
Device::getRuntime()
{
  return m_runtime;
}
void
Device::setRuntime(Runtime* runtime)
{
  m_runtime = runtime;
}

void
Device::setInternalChunk(int internal_chunk){
    m_internal_chunk=internal_chunk;
}

int
Device::getInternalChunk(){
    return m_internal_chunk;
}

void
Device::setLimMemory(int limit_memory){
    m_lim_memory=limit_memory;
}

int
Device::getLimMemory(){
    return m_lim_memory;
}
void
Device::set_globalWorkSize( size_t gws0)
{
    m_gws[0]=gws0;
}
void
Device::set_globalWorkSize( size_t gws0, size_t gws1)
{
    m_gws[0]=gws0;
    m_gws[1]=gws1;
}
void
Device::set_globalWorkSize( size_t gws0, size_t gws1,size_t gws2)
{ 
    m_gws[0]=gws0;
    m_gws[1]=gws1;
    m_gws[2]=gws2;
}

void
Device::set_localWorkSize( size_t lws0)
{
    m_lws[0]=lws0;
}
void
Device::set_localWorkSize( size_t lws0, size_t lws1)
{
    m_lws[0]=lws0;
    m_lws[1]=lws1;
}
void
Device::set_localWorkSize( size_t lws0, size_t lws1,size_t lws2)
{ 
    m_lws[0]=lws0;
    m_lws[1]=lws1;
    m_lws[2]=lws2;
}

} // namespace ecl
