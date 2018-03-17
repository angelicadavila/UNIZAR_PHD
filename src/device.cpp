/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of clbalancer which is released under MIT License.
 * See file LICENSE for full license details.
 */
#include "./device.hpp"

#include "buffer.hpp"
#include "inspector.hpp"
#include "runtime.hpp"
#include "scheduler.hpp"

#include <chrono>
#include <thread>

struct CBData
{
  int queue_index;
  clb::Device* device;
  CBData(int queue_index_, clb::Device* device_)
    : queue_index(queue_index_)
    , device(device_)
  {}
};

void CL_CALLBACK
callbackRead(cl_event /*event*/, cl_int /*status*/, void* data)
{
  CBData* cbdata = reinterpret_cast<CBData*>(data);
  clb::Device* device = cbdata->device;
  clb::Scheduler* sched = device->getScheduler();
  device->saveDuration(clb::ActionType::completeWork);
  sched->callback(cbdata->queue_index);
  delete cbdata;
}

namespace clb {

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
      device.do_work(work.offset, work.size, queue_index);
    } else {
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
  m_sema_run = make_unique<semaphore>(1);
  m_duration_actions.reserve(1024);     // NOTE to improve
  m_duration_offset_actions.reserve(8); // NOTE to improve
  m_gws= m_gws=vector <size_t>(3,1);
  m_lws=vector <size_t>(3,1);
  //default values
  m_gws[0]=0;//to set the chunk size
  m_lws[0]=128;
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
Device::setKernelArg(cl_uint /* index */, ::size_t /* size */, const void* /* ptr */)
{
  throw runtime_error("Device::setKernelArg size ptr* not implemented");
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
  cout << m_sel_platform << " - " << m_sel_device << "\n";
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
    cout << "Using custom Kernel\n";
  }
  m_kernel_str = kernel;
}

void
Device::setKernel(const vector<char>& binary)
{
  cout << "Provided binary Kernel\n";
  m_program_type = ProgramType::CustomBinary;
  m_program_binary = binary;
}

void
Device::setKernel(const string& source)
{
  cout << "Provided source Kernel\n";
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
Device::do_work(size_t offset, size_t size, int queue_index)
{
  if (!size) {
    return callbackRead(nullptr, CL_COMPLETE, this);
  }
  if (m_prev_events.size() && m_works) {
    m_prev_events.clear();
  }

//  cout<<"offset:"<<offset<<" device:"<<getID()<<"\n";
  auto offset_for_bytes=offset;//use to read offset_bytes
  offset*=m_internal_chunk;
  cl_int status;
  if(m_gws[0]!=1) 
    m_gws[0] = size;//pass to global

#if CLB_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED == 1
  status=m_queue.enqueueNDRangeKernel(m_kernel,
                               cl::NDRange(offset),
                               cl::NDRange(gws),
                               cl::NDRange(CL_LWS),
                               nullptr,
                               nullptr);
#else
 m_kernel.setArg(m_nargs,(uint) size );
 m_kernel.setArg(m_nargs+1, (uint)offset);
  status=m_queue.enqueueNDRangeKernel(
                          m_kernel, cl::NullRange, 
                          cl::NDRange(m_gws[0],m_gws[1],m_gws[2]), 
                          cl::NDRange(m_lws[0],m_lws[1],m_lws[2]),
                          nullptr,&m_event_kernel);
#endif

  m_prev_events=vector<cl::Event>({ m_event_kernel });
  CL_CHECK_ERROR(status,"NDRange problem");

  //kernel n + Read n-1
  status= m_queueRead.finish();
  CL_CHECK_ERROR(status,"finish queue"); 

  auto len = m_out_clb_buffers.size();
  for (uint i = 0; i < len; ++i) {
    Buffer& b = m_out_clb_buffers[i];
    size_t size_bytes = b.byBytes(size)*m_internal_chunk;
    auto offset_bytes = b.byBytes(offset_for_bytes)*m_internal_chunk;
  //cout<<"sizebyte: "<<size_bytes<<" offsetby: "<<offset_bytes<<"\n";
  status= m_queueRead.enqueueReadBuffer(m_out_buffers[i],
                              CL_FALSE,
                              offset_bytes,
                              size_bytes,
                              b.dataWithOffset(offset),
                              &m_prev_events,
                              nullptr);
    CL_CHECK_ERROR(status,"Reading memory problem");

  }
#if CLB_OPERATION_BLOCKING_READ == 1
  clb::Scheduler* sched = getScheduler();
  //wait finish kernel
  m_queue.finish();
  saveDuration(clb::ActionType::completeWork);
  sched->callback(queue_index);
#else
  auto cbdata = new CBData(queue_index, this);
  evread.setCallback(CL_COMPLETE, callbackRead, cbdata);
  m_queue.flush();
#endif  
  m_works++;
  m_works_size += size;
}

void
Device::waitFinish(){
	m_queue.finish();
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
  writeBuffers();
  saveDuration(ActionType::writeBuffers);
  saveDurationOffset(ActionType::writeBuffers);
  // work();

  //file debug
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
  cout << "initByIndex\n";
  cout << sel_platform << " - " << sel_device << "\n";
  vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);
  m_platform = platforms.at(sel_platform);
  cout << sel_platform << " - " << sel_device << "\n";
  vector<cl::Device> devices;
  m_platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
  m_device = devices.at(sel_device);
  cout << sel_platform << " - " << sel_device << "\n";
}

void
Device::initContext()
{
  cout << "initContext\n";
  cl::Context context(m_device);
  m_context = move(context);
}

void
Device::initQueue()
{
  cout << "initQueue\n";
  cl_int cl_err;

  cl::Context& context = m_context;
  cl::Device& device = m_device;

  cl::CommandQueue queue(context, device, 0, &cl_err);
  CL_CHECK_ERROR(cl_err, "CommandQueue queue");
  m_queue = move(queue);
  
  cl::CommandQueue queueRead(context, device, 0, &cl_err);
  CL_CHECK_ERROR(cl_err, "CommandQueue queueRead");
  m_queueRead = move(queueRead);

}

void
Device::initBuffers()
{
  cout << "initBuffers\n";
  cl_int cl_err = CL_SUCCESS;

  cl_int buffer_in_flags = CL_MEM_READ_WRITE;
  cl_int buffer_out_flags = CL_MEM_READ_WRITE;

  m_in_buffers.reserve(m_in_clb_buffers.size());
  m_out_buffers.reserve(m_out_clb_buffers.size());

  auto len = m_in_clb_buffers.size();
  for (uint i = 0; i < len; ++i) {
    clb::Buffer& b = m_in_clb_buffers[i];
    auto data = b.data();
//    cout << "in [data] " << data << "\n";
//    cout << "in [address] " << b.get() << "\n";
//    cout << "in [size] " << b.size() << "\n";
//    cout << "in [bytes] " << b.bytes() << "\n";
    cl::Buffer tmp_buffer(m_context, buffer_in_flags, b.bytes(), NULL,&cl_err);
    CL_CHECK_ERROR(cl_err, "in buffer " + i);
    m_in_buffers.push_back(move(tmp_buffer));
//    cout << "in buffer: " << &m_in_buffers[i] << "\n";
  }

  len = m_out_clb_buffers.size();
  for (uint i = 0; i < len; ++i) {
    clb::Buffer& b = m_out_clb_buffers[i];
    auto data = b.data();
//   cout << "out [data] " << data << "\n";
//   cout << "out [address] " << b.get() << "\n";
//   cout << "out [size] " << b.size() << "\n";
//    cout << "out [bytes] " << b.bytes() << "\n";
    cl::Buffer tmp_buffer(m_context, buffer_out_flags, b.bytes(), NULL,&cl_err);
    CL_CHECK_ERROR(cl_err, "out buffer " + i);
    m_out_buffers.push_back(move(tmp_buffer));
//    cout << "out buffer: " << &m_out_buffers[i] << "\n";
  }
}

void
Device::writeBuffers(bool /* dummy */)
{
//  cout << "writeBuffers\n";

  auto len = m_in_clb_buffers.size();
  m_prev_events.reserve(len);
  m_prev_events.resize(len);
  for (uint i = 0; i < len; ++i) {
    Buffer& b = m_in_clb_buffers[i];
    auto data = b.data();
    auto size = b.size();
//   cout << "writeBuffers [array] " << b.get() << " data: " << data
//         << " buffer: " << &m_in_buffers[i] << " size: " << size << " bytes: " << b.bytes() << "\n";
    CL_CHECK_ERROR(m_queue.enqueueWriteBuffer(
      m_in_buffers[i], CL_FALSE, 0, b.bytes(), data, NULL, &(m_prev_events.data()[i])));
  }
 }

void
Device::initKernel()
{
//  cout << "initKernel\n";

  cl_int cl_err;

  cl::Program::Sources sources;
  cl::Program::Binaries binaries;
  cl::Program program;

  if (m_program_type == ProgramType::CustomBinary) {
    cout << "size: " << m_program_binary.size() << "\n";
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
  options += "-DCLB_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED=" +
             to_string(CLB_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED);

#if CLB_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED == 0
  cout << "Kernel should receive the last argument as 'uint offset' "
          "(CLB_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED == 0)\n";
#endif

  cl_err = program.build({ m_device }, options.c_str());
  if (cl_err != CL_SUCCESS) {
    cout << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_device) << "\n";
    CL_CHECK_ERROR(cl_err);
  } else {
    cout << " Building info: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_device) << "\n";
  }

  cl::Kernel kernel(program, m_kernel_str.c_str(), &cl_err);
  CL_CHECK_ERROR(cl_err, "kernel");

  auto len = m_arg_index.size();
  auto unassigned = m_in_buffers_ptr.size();
  for (uint i = 0; i < len; ++i) {
    auto index = m_arg_index[i];
    cout << "[i] " << index << "\n";
    auto size = m_arg_size[i];
    if (size == 0) {
      int pos = -1;
      auto address = m_arg_ptr[i];
      auto assigned = false;
      cout << "[value] " << m_arg_ptr[i] << "\n";
      cout << "[address] " << address << "\n";
      if (unassigned > 0) { // usually more in buffers than out
        auto it = find(begin(m_in_buffers_ptr), end(m_in_buffers_ptr), address);
        cout << "it: " << *it << "\n";
        if (it != end(m_in_buffers_ptr)) {
          pos = distance(m_in_buffers_ptr.begin(), it);
          cout << "address: " << address << " position: " << pos
               << " buffer: " << &m_in_buffers[pos] << "\n";
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
          cout << "address: " << address << " position: " << pos
               << " buffer: " << &m_out_buffers[pos] << "\n";
          cl_err = kernel.setArg(index, m_out_buffers[pos]);
          CL_CHECK_ERROR(cl_err, "kernel arg out buffer " + to_string(i));
          assigned = true;
        }
      }

      if (!assigned) { // maybe empty space (__local)
        if (m_arg_bytes[i] > 0) {
          cout << "[bytes] " << m_arg_bytes[i] << "\n";
          size_t bytes = m_arg_bytes[i];
          // void* ptr = m_arg_ptr[i];
          cl_err = kernel.setArg((cl_uint)i, bytes, NULL);
          CL_CHECK_ERROR(cl_err, "kernel arg " + to_string(i));
        } else {
          throw runtime_error("unknown kernel arg address " + to_string(i));
        }
      }
    } else {
      cout << "[size] " << m_arg_size[i] << "\n";
      cout << "[value] " << m_arg_ptr[i] << "\n";
      size_t size = m_arg_size[i];
      void* ptr = m_arg_ptr[i];
      cl_err = kernel.setArg((cl_uint)i, size, ptr);
      CL_CHECK_ERROR(cl_err, "kernel arg " + to_string(i));
    }
  }

  m_kernel = move(kernel);
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
  cout << "show\n";
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

} // namespace clb
