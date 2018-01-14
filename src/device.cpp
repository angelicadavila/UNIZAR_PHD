#include "./device.hpp"

#include "buffer.hpp"
#include "runtime.hpp"
#include "scheduler.hpp"

// #include "fmt/format.h"
// #include "rang/rang.hpp"

#define CL_LWS 128

struct CBData {
  clb::Device* device;
  int queue_index;
  CBData(int queue_index_, clb::Device* device_) : queue_index(queue_index_), device(device_) {}
};

void CL_CALLBACK callbackRead(cl_event /*event*/, cl_int /*status*/, void* data) {
  CBData* cbdata = reinterpret_cast<CBData*>(data);
  clb::Device* device = cbdata->device;
  // clb::Device* device = reinterpret_cast<clb::Device*>(data);
  clb::Scheduler* sched = device->getScheduler();
  device->saveDuration(clb::ActionType::completeWork);
  // cout << "ZZZ callbackRead: id " << device->getID() << " queue_index: " << cbdata->queue_index << "\n";
  sched->callback(cbdata->queue_index);
  delete cbdata;
  // cout << "callbackRead\n";
  // device->notifyEvent();
}

namespace clb {

void device_thread_func(Device& device) {
  // cout << "thread: init" << "\n";
  // this_thread::sleep_for(1s);
  // cout << "thread: about to wait\n";
  // device.wait();
  // cout << "thread: notified\n";
  auto time1 = std::chrono::system_clock::now().time_since_epoch();
  // this_thread::sleep_for(1s);
  device.init();
  // cout << "thread about to req_work to Scheduler\n";
  Scheduler* sched = device.getScheduler();
  // int id = device.getID();
  // auto time2 = std::chrono::system_clock::now().time_since_epoch();
  // auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time2 - time1).count();
  // time1 = time2;
  // cout << rang::fg::magenta << "device started id " << id << rang::style::reset << " (" << diff_ms << "ms.)\n";
  device.saveDuration(ActionType::deviceStart);
  device.saveDurationOffset(ActionType::deviceStart);
  // if (id == 0){
  //   this_thread::sleep_for(0.05s);
  // }
  // cout << rang::fg::magenta << "device continues id " << id << rang::style::reset << "\n";
  Runtime* runtime = device.getRuntime();
  device.saveDuration(ActionType::deviceReady);
  device.saveDurationOffset(ActionType::deviceReady);
  runtime->notifyReady();
  runtime->notifyAllReady();
  // cout << rang::fg::magenta << "device is ready id " << id << rang::style::reset << "\n";
  device.waitRun();
  device.saveDuration(ActionType::deviceRun);
  device.saveDurationOffset(ActionType::deviceRun);
  // time2 = std::chrono::system_clock::now().time_since_epoch();
  // diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time2 - time1).count();
  // time1 = time2;
  // cout << rang::fg::magenta << "device runs id " << id << rang::style::reset << " (" << diff_ms << "ms.)\n";

  sched->req_work(&device);

  auto cont = true;
  while (cont) {
    device.waitWork();
    auto queue_index = sched->getWorkIndex(&device);
    // auto color = (id == 0 ? rang::fg::cyan : rang::fg::green);
    // cout << color << "UUU device id " << id << " getWorkIndex: " << queue_index << rang::style::reset << "\n";
    // cout << "device thread queue_index: " << queue_index << "\n";
    if (queue_index >= 0) {  //
      Work work = sched->getWork(queue_index);
      // cout << "DDD device_id: " << work.device_id << " index: " << queue_index << " work.offset: " << work.offset <<
      // " work.size: " << work.size << "\n";
      device.do_work(work.offset, work.size, queue_index);
      // do_work
      // loop waiting
    } else {  // end
      // cout << "thread about to waitEvent id: " << id << "\n";
      // device.waitEvent();
      // device.show();
      // cout << "thread about to notifyBarrier: " << device.getBuffer() << "\n";
      cout << "device id " << device.getID() << " finished\n";
      device.notifyBarrier();
      cont = false;
    }
  }

  // time2 = std::chrono::system_clock::now().time_since_epoch();
  // diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time2 - time1).count();
  // time1 = time2;
  // cout << rang::fg::magenta << "device finish id " << id << rang::style::reset << " (" << diff_ms << "ms.)\n";
  device.saveDuration(ActionType::deviceEnd);
  device.saveDurationOffset(ActionType::deviceEnd);
  // cout << "thread about to waitEvent\n";
  // cout << "thread about to sleep: " << device.getBuffer() << "\n";
  // this_thread::sleep_for(2s);
}

Device::Device(int sel_platform, int sel_device)
    : m_sel_platform(sel_platform),
      m_sel_device(sel_device)
// m_works(0),
// m_works_size(0)
// m_sema_work(1)
// m_sem_init(1)
// m_thread(thread_func, ref(*this))
{
  // init();
  // cout << "Device()\n";
  // m_sema_work = move(semaphore(1));
  // m_sema_work = make_unique<semaphore>(1);
  // m_mutex_duration = mutex
  m_mutex_duration = new mutex();
  m_time_init = std::chrono::system_clock::now().time_since_epoch();
  m_time = std::chrono::system_clock::now().time_since_epoch();
  m_works = 0;
  m_works_size = 0;
  m_sema_work = new semaphore(1);
  // qsem = new QSemaphore();
  m_sema_run = make_unique<semaphore>(1);
  m_duration_actions.reserve(1024);      // to improve
  m_duration_offset_actions.reserve(8);  // to improve
  // m_info_buffer += "hola\n";
}

Device::~Device() {
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void Device::printStats() {
  cout << "Device id: " << getID() << "\n";
  showInfo();
  cout << "works: " << m_works << " works_size: " << m_works_size << "\n";
  size_t acc = 0;
  size_t total = 0;
  cout << "duration increments:\n";
  for (auto& t : m_duration_actions) {
    auto d = get<0>(t);
    auto action = get<1>(t);
    switch (action) {
      case ActionType::completeWork:
        acc += d;
        break;
      default:
        printActionTypeDuration(action, d);
    }
    total += d;
  }
  cout << " completeWork: " << acc << " ms.\n";
  cout << " total: " << total << " ms.\n";
  cout << "duration offsets from init:\n";
  for (auto& t : m_duration_offset_actions) {
    printActionTypeDuration(get<1>(t), get<0>(t));
  }
}

void Device::saveDuration(ActionType action) {
  lock_guard<mutex> lock(*m_mutex_duration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = chrono::duration_cast<chrono::milliseconds>(t2 - m_time).count();
  m_duration_actions.push_back(make_tuple(diff_ms, action));
  m_time = t2;
}
void Device::saveDurationOffset(ActionType action) {
  lock_guard<mutex> lock(*m_mutex_duration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = chrono::duration_cast<chrono::milliseconds>(t2 - m_time_init).count();
  m_duration_offset_actions.push_back(make_tuple(diff_ms, action));
}

void Device::setKernelArg(cl_uint index, ::size_t size, const void* ptr) {
  throw runtime_error("Device::setKernelArg size ptr* not implemented");
  // cout << "setKernelArg size ptr*\n";
  // m_arg_index.push_back(index);
  // m_arg_size.push_back(size);
  // m_arg_bytes.push_back(size);
  // m_arg_ptr.push_back(ptr);
}

void Device::setScheduler(Scheduler* sched) { m_sched = sched; }
Scheduler* Device::getScheduler() { return m_sched; }

void Device::setID(int id) { m_id = id; }
int Device::getID() { return m_id; }

void Device::waitRun() {
  // cout << "waitRun tid: " << this_thread::get_id() << "\n";
  m_sema_run.get()->wait(1);
}

void Device::notifyRun() {
  // cout << "notifyRun tid: " << this_thread::get_id() << "\n";
  m_sema_run.get()->notify(1);
}

void Device::waitWork() {
  // cout << "waitWork tid: " << this_thread::get_id() << "\n";
  // qsem->acquire(1);
  m_sema_work->wait(1);
  // m_sema_work.get()->wait(1);
}

void Device::notifyWork() {
  // cout << "notifyWork tid: " << this_thread::get_id() << "\n";
  // qsem->release(1);
  m_sema_work->notify(1);
  // m_sema_work.get()->notify(1);
}

string& Device::getBuffer() { return m_info_buffer; }

void Device::setPlatform(int platform) { m_sel_platform = platform; }
void Device::setDevice(int device) { m_sel_device = device; }

void Device::start() {
  m_thread = thread(device_thread_func, ref(*this));
  // cout << "start\n";
}
void Device::useRuntimeDiscovery() {
  Runtime* runtime = m_runtime;
  cout << m_sel_platform << " - " << m_sel_device << "\n";
  // copy
  m_platform = runtime->usePlatformDiscovery(m_sel_platform);
  m_device = runtime->useDeviceDiscovery(m_sel_platform, m_sel_device);
}
// void Device::usePlatformDevice(vector<cl::Platform>& platforms, vector<vector<cl::Device>>& platform_devices) {
//   cout << "usePlatformDevice\n";
//   auto sel_platform = m_sel_platform;
//   auto sel_device = m_sel_device;
//   cout << sel_platform << " - " << sel_device << "\n";
//   m_platform = move(platforms[sel_platform]);
//   auto last_device = platform_devices[sel_platform].size() - 1;
//   if (sel_device < 0 || sel_device > last_device) {
//     throw runtime_error("invalid device selected");
//   }
//   m_device = move(platform_devices[sel_platform][sel_device]);
//   saveDuration(ActionType::useDiscovery);
//   saveDurationOffset(ActionType::useDiscovery);
// }

// void Device::setInBuffer(shared_ptr<vector<int>> array) {
//   m_in_arrays.push_back(array);
//   auto address = array.get();
//   m_in_buffers_ptr.push_back(address);
// }
// void Device::setOutBuffer(shared_ptr<vector<int>> array) {
//   m_out_arrays.push_back(array);
//   auto address = array.get();
//   m_out_buffers_ptr.push_back(address);
// }

// void Device::setInBuffer(shared_ptr<vector<int>> array) {
//   Buffer b(Direction::In);
//   b.set(array);
//   m_in_clb_buffers.push_back(move(b));

//   m_in_arrays.push_back(array);
//   auto address = array.get();
//   m_in_buffers_ptr.push_back(address);

//   cout << "clb::Buffer in get: " << b.get() << "\n";
//   cout << "clb::Buffer in data: " << b.data() << "\n";
//   cout << "array: " << array.get() << "\n";
//   cout << "address: " << address << "\n";
// }
// void Device::setOutBuffer(shared_ptr<vector<int>> array) {
//   Buffer b(Direction::Out);
//   b.set(array);
//   m_out_clb_buffers.push_back(move(b));

//   m_out_arrays.push_back(array);
//   auto address = array.get();
//   m_out_buffers_ptr.push_back(address);

//   cout << "clb::Buffer out get: " << b.get() << "\n";
//   cout << "clb::Buffer out data: " << b.data() << "\n";
//   cout << "array: " << array.get() << "\n";
//   cout << "address: " << address << "\n";
// }

// void Device::setInBuffer(Buffer& buffer){
//   m_in_arrays.push_back(buffer);
//   // m_in_array = array;
//   auto address = &buffer;
//   m_in_buffers_ptr.push_back(address);
// }
// void Device::setOutBuffer(Buffer& buffer){
//   m_out_arrays.push_back(buffer);
//   // m_out_array = array;
//   auto address = &buffer;
//   m_out_buffers_ptr.push_back(address);
// }

void Device::setKernel(const string& source, const string& kernel) {
  // cout << "m_source_str:\n";
  cout << source;
  m_source_str = make_shared<string>(source);
  m_kernel_str = make_shared<string>(kernel);
}

void Device::waitEvent() {
  cl::Event::waitForEvents({m_end});  // vector<cl::Event>(end));
}
void Device::notifyEvent() {
  // cout << "end\n";
  m_end.setStatus(CL_COMPLETE);
}

void Device::setBarrier(shared_ptr<semaphore> barrier) { m_barrier = barrier; }

void Device::do_work(size_t offset, size_t size, int queue_index) {
  // cout << "Device do_work\n";
  if (!size) {
    return callbackRead(nullptr, CL_COMPLETE, this);
  }
  if (m_prev_events.size() && m_works) {  // > 0
    m_prev_events.clear();
  }
  // cout << "oye:\n";
  // int* in_array = m_in_array.get()->data();
  // int* out_array = m_out_array.get()->data();
  // cout << "in[0] " << in_array[0] << "\n";
  // cout << "out[0] " << out_array[0] << "\n";
  cl::Event evkernel;

  auto gws = size;
  // size_t gws = m_in_array.get()->size();

  // cout << "kernel\n";
  m_queue.enqueueNDRangeKernel(m_kernel, cl::NDRange(offset), cl::NDRange(gws), cl::NDRange(CL_LWS), &m_prev_events,
                               &evkernel);
  // m_queue.enqueueNDRangeKernel(m_kernel, cl::NDRange(0), cl::NDRange(gws), cl::NDRange(CL_LWS), NULL, NULL);
  // size_t size = sizeof(int) * m_in_array.get()->size();

  // cout << "size_bytes: " << sizeof(cl_uchar4) * size << "\n";
  // cout << "size_bytes: " << sizeof(cl_float) * size << "\n";
  // cout << "size_bytes: " << size_bytes << "\n";
  // cout << "offset_bytes: " << offset_bytes << "\n";
  // auto events = make_unique<vector<cl::Event>>();
  // events.get()->push_back(evkernel); // {evkernel});
  // auto events = unique_ptr<vector<cl::Event>>(new vector<cl::Event>({evkernel}));
  // cout << "in[0] " << in_array[0] << "\n";
  // cout << "out[0] " << out_array[0] << "\n";

  // cout << "read\n";
  // m_queue.enqueueReadBuffer(m_out_buffer, CL_TRUE, offset, size, m_out_array.get()->data() + offset, NULL, NULL);
  // m_queue.enqueueReadBuffer(m_out_buffer, CL_TRUE, offset, size, m_out_array.get()->data() + offset, events.get(),
  // &evread);
  cl::Event evread;
  vector<cl::Event> events({evkernel});

  auto len = m_out_clb_buffers.size();
  for (uint i = 0; i < len; ++i) {
    vector<cl::Event> levents = events;
    cl::Event levread;

    Buffer& b = m_out_clb_buffers[i];
    size_t size_bytes = b.byBytes(size);    // sizeof(int) * size
    auto offset_bytes = b.byBytes(offset);  // sizeof(int) * offset;
    // auto array = m_out_arrays[i].get();
    // auto size = sizeof(int) * array->size();
    // cl::Buffer tmp_buffer(m_context, buffer_flags, size, array->data());
    // CL_CHECK_ERROR(cl_err, "out buffer " + i);
    // m_queue.enqueueReadBuffer(m_out_buffers[i], CL_FALSE, offset_bytes, size_bytes,
    //                           m_out_arrays[i].get()->data() + offset, &levents, &levread);
    m_queue.enqueueReadBuffer(m_out_buffers[i], CL_FALSE, offset_bytes, size_bytes, b.dataWithOffset(offset), &levents,
                              &levread);
    // auto ptr = reinterpret_cast<cl_uchar4*>(b.data());
    // cout << ptr[0].s[0] << "\n";
    // cout << ptr[121].s[0] << "\n";
    events.push_back(levread);
    evread = levread;
  }
  // auto cbdata = make_unique<CBData>(this, queue_index);
  auto cbdata = new CBData(queue_index, this);

  // evread.setCallback(CL_COMPLETE, callbackRead, this);
  // evread.setCallback(CL_COMPLETE, callbackRead, &queue_index);
  evread.setCallback(CL_COMPLETE, callbackRead, cbdata);
  // cout << "id " << getID() << " flush work\n";
  m_queue.flush();
  // cout << "in[0] " << in_array[0] << "\n";
  // cout << "out[0] " << out_array[0] << "\n";
  m_works++;
  m_works_size += size;
}

void Device::init() {
  // cout << "init\n";
  m_time = std::chrono::system_clock::now().time_since_epoch();
  m_info_buffer.reserve(128);
  // initByIndex(m_sel_platform, m_sel_device);
  // saveDuration(ActionType::initDiscovery);
  // saveDurationOffset(ActionType::initDiscovery);
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
  // writeBuffers(true);
  // saveDuration(ActionType::writeBuffersDummy);
  initKernel();
  saveDuration(ActionType::initKernel);
  saveDurationOffset(ActionType::initKernel);
  initEvents();
  writeBuffers();
  saveDuration(ActionType::writeBuffers);
  saveDurationOffset(ActionType::writeBuffers);
  // work();
}

// void Device::wait(){
//   m_sem_init.wait(1);
// }
// void Device::notify(){
//   m_sem_init.notify(1);
// }
void Device::notifyBarrier() { m_barrier.get()->notify(1); }

/**
 * \brief Does not check bounds
 */
void Device::initByIndex(int sel_platform, int sel_device) {
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

void Device::initContext() {
  cout << "initContext\n";
  cl::Context context(m_device);
  m_context = move(context);
}

void Device::initQueue() {
  cout << "initQueue\n";
  cl_int cl_err;

  cl::Context& context = m_context;
  cl::Device& device = m_device;

  cl::CommandQueue queue(context, device, 0, &cl_err);
  CL_CHECK_ERROR(cl_err, "CommandQueue queue");
  m_queue = move(queue);
}

void Device::initBuffers() {
  cout << "initBuffers\n";
  cl_int cl_err = CL_SUCCESS;

  // cl_int buffer_flags = CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR;
  cl_int buffer_flags = CL_MEM_READ_WRITE;  // CL_INVALID_MEM_OBJECT
  cl_int buffer_in_flags = CL_MEM_READ_WRITE;
  // cl_int buffer_in_flags = CL_MEM_READ_ONLY;
  // cl_int buffer_in_flags = CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR;    // CL_INVALID_MEM_OBJECT
  // cl_int buffer_out_flags = CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR;  // CL_INVALID_MEM_OBJECT
  // cl_int buffer_inout_flags = CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR; // CL_INVALID_MEM_OBJECT
  cl_int buffer_out_flags = CL_MEM_READ_WRITE;
  // cl_int buffer_out_flags = CL_MEM_WRITE_ONLY;

  m_in_buffers.reserve(m_in_clb_buffers.size());    // avoid destroying refs
  m_out_buffers.reserve(m_out_clb_buffers.size());  // avoid destroying refs

  // auto len = m_in_arrays.size();
  auto len = m_in_clb_buffers.size();
  for (uint i = 0; i < len; ++i) {
    clb::Buffer& b = m_in_clb_buffers[i];
    auto data = b.data();  // array -> data
    // auto array = m_in_arrays[i].get();
    // auto size = sizeof(int) * array->size();
    // cout << "in [address] " << array << "\n";
    cout << "in [data] " << data << "\n";
    cout << "in [address] " << b.get() << "\n";
    cout << "in [size] " << b.size() << "\n";
    cout << "in [bytes] " << b.bytes() << "\n";
    // cl::Buffer tmp_buffer(m_context, buffer_in_flags, size, array->data());
    // cl::Buffer tmp_buffer(m_context, buffer_in_flags, b.bytes(), data);
    cl::Buffer tmp_buffer(m_context, buffer_in_flags, b.bytes(), NULL);
    CL_CHECK_ERROR(cl_err, "in buffer " + i);
    m_in_buffers.push_back(move(tmp_buffer));
    cout << "in buffer: " << &m_in_buffers[i] << "\n";
  }

  // len = m_out_arrays.size();
  len = m_out_clb_buffers.size();
  for (uint i = 0; i < len; ++i) {
    clb::Buffer& b = m_out_clb_buffers[i];
    auto data = b.data();  // array -> data
    // auto array = m_out_arrays[i].get();
    // auto size = sizeof(int) * array->size();
    // cout << "out [address] " << array << "\n";
    cout << "out [data] " << data << "\n";
    cout << "out [address] " << b.get() << "\n";
    cout << "out [size] " << b.size() << "\n";
    cout << "out [bytes] " << b.bytes() << "\n";
    // cl::Buffer tmp_buffer(m_context, buffer_out_flags, size, array->data());
    // cl::Buffer tmp_buffer(m_context, buffer_out_flags, b.bytes(), data);
    cl::Buffer tmp_buffer(m_context, buffer_out_flags, b.bytes(), NULL);
    CL_CHECK_ERROR(cl_err, "out buffer " + i);
    m_out_buffers.push_back(move(tmp_buffer));
    cout << "out buffer: " << &m_out_buffers[i] << "\n";
  }

  // size_t size = sizeof(int) * m_in_array.get()->size();
  // cl::Buffer in_buffer(m_context, buffer_flags, size, m_in_array.get()->data());
  // CL_CHECK_ERROR(cl_err, "in_buffer");

  // size = sizeof(int) * m_out_array.get()->size();
  // cl::Buffer out_buffer(m_context, buffer_flags, size, m_out_array.get()->data());
  // CL_CHECK_ERROR(cl_err, "out_buffer");

  // m_in_buffer = move(in_buffer);
  // m_out_buffer = move(out_buffer);

  // the order should depend upon m_buffers_ptr
  // m_buffers.push_back(&m_in_buffer);
  // m_buffers.push_back(&m_out_buffer);
}

void Device::writeBuffers(bool dummy) {
  cout << "writeBuffers\n";
  // if (dummy) {
  //   thread t1([&] {
  //     auto len = m_in_arrays.size();
  //     for (uint i = 0; i < len; ++i) {
  //       auto array = m_in_arrays[i].get();
  //       auto size = sizeof(int) * array->size();
  //       cout << "writeBuffers [array] " << array << " data: " << array->data() << " buffer: " << &m_in_buffers[i] <<
  //       " size: " << size << "\n"; CL_CHECK_ERROR(m_queue.enqueueWriteBuffer(m_in_buffers[i], CL_FALSE, 0, dummy ?
  //       128 : size,
  //                                                 array->data()));  // CL_TRUE to CL_FALSE
  //     }
  //   });
  //   t1.detach();
  //   return;
  // }

  // auto len = m_in_arrays.size();
  auto len = m_in_clb_buffers.size();
  m_prev_events.reserve(len);
  m_prev_events.resize(len);  // important with .data()
  for (uint i = 0; i < len; ++i) {
    Buffer& b = m_in_clb_buffers[i];
    auto data = b.data();
    // auto array = m_in_arrays[i].get();
    // auto size = sizeof(int) * array->size();
    auto size = b.size();
    cout << "writeBuffers [array] " << b.get() << " data: " << data << " buffer: " << &m_in_buffers[i]
         << " size: " << size << " bytes: " << b.bytes() << "\n";
    // cl::Buffer tmp_buffer(m_context, buffer_flags, size, array->data());
    // CL_CHECK_ERROR(m_queue.enqueueWriteBuffer(m_in_buffers[i], CL_FALSE, 0, dummy ? 128 : size, array->data())); //
    // CL_TRUE to CL_FALSE
    CL_CHECK_ERROR(m_queue.enqueueWriteBuffer(m_in_buffers[i], CL_FALSE, 0, b.bytes(), data, NULL,
                                              &(m_prev_events.data()[i])));  // CL_TRUE to CL_FALSE

    // CL_CHECK_ERROR(m_queue.enqueueWriteBuffer(m_in_buffers[i], CL_FALSE, 0, dummy ? 128 : size, array->data(), NULL,
    // &ev_arr[i])); // CL_TRUE to CL_FALSE m_prev_events.push_back(move(ev_arr[i]));
  }
  // size_t size = sizeof(int) * m_in_array.get()->size();
  // cout << "writeBuffers: " << size << "\n";
  // CL_CHECK_ERROR(m_queue.enqueueWriteBuffer(m_in_buffer, CL_TRUE, 0, size, m_in_array.get()->data()));
  // m_prev_events = move(events);
  // m_prev_events = move(ev);
}

void Device::initKernel() {
  cout << "initKernel\n";

  cl_int cl_err;

  cl::Program::Sources sources;
  const string* source_str = m_source_str.get();
  const string* kernel_str = m_kernel_str.get();
  sources.push_back({source_str->c_str(), source_str->length()});

  // cout << "m_source_str:\n";
  // cout << m_source_str->c_str();

  cl::Program program(m_context, sources);
  cl_err = program.build({m_device});
  if (cl_err != CL_SUCCESS) {
    cout << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_device) << "\n";
    CL_CHECK_ERROR(cl_err);
  }

  // for (auto buff : m_buffers_ptr){
  //   cout << "buff_ptr: " << buff << "\n";
  // }
  // for (auto buff : m_buffers){
  //   cout << "buff: " << buff << "\n";
  // }

  cl::Kernel kernel(program, kernel_str->c_str(), &cl_err);
  CL_CHECK_ERROR(cl_err, "kernel");

  auto len = m_arg_index.size();
  auto unassigned = m_in_buffers_ptr.size();  // only for in buffers
  for (uint i = 0; i < len; ++i) {
    auto index = m_arg_index[i];
    cout << "[i] " << index << "\n";
    auto size = m_arg_size[i];
    if (size == 0) {  // shared_ptr
      int pos = -1;
      auto address = m_arg_ptr[i];
      cout << "[value] " << m_arg_ptr[i] << "\n";
      cout << "[address] " << address << "\n";
      if (unassigned > 0) {  // usually more in buffers than out
        auto it = find(begin(m_in_buffers_ptr), end(m_in_buffers_ptr), address);
        cout << "it: " << *it << "\n";
        if (it != end(m_in_buffers_ptr)) {
          pos = distance(m_in_buffers_ptr.begin(), it);
          cout << "address: " << address << " position: " << pos << " buffer: " << &m_in_buffers[pos] << "\n";
          cl_err = kernel.setArg(index, m_in_buffers[pos]);  // m_in_buffer);
          CL_CHECK_ERROR(cl_err, "kernel arg in buffer " + to_string(i));
          unassigned--;
        }
      }

      if (pos == -1) {
        auto it = find(begin(m_out_buffers_ptr), end(m_out_buffers_ptr), address);
        if (it != end(m_out_buffers_ptr)) {
          auto pos = distance(m_out_buffers_ptr.begin(), it);
          cout << "address: " << address << " position: " << pos << " buffer: " << &m_out_buffers[pos] << "\n";
          cl_err = kernel.setArg(index, m_out_buffers[pos]);  // m_in_buffer);
          CL_CHECK_ERROR(cl_err, "kernel arg out buffer " + to_string(i));
        }
      }
    } else {  // other
      cout << "[size] " << m_arg_size[i] << "\n";
      cout << "[value] " << m_arg_ptr[i] << "\n";
      size_t size = m_arg_size[i];
      void* ptr = m_arg_ptr[i];
      // std::remove_const<const void*>::type ptr = m_arg_ptr[i];
      // void* ptr = &m_arg_ptr[i];
      // void* ptr = std::remove_const<const void*>(m_arg_ptr[i]); // NOTE legacy OpenCL 1.2
      // cl_err = kernel.setArg(i, m_arg_size[i], m_arg_ptr[i]);  // m_in_buffer);
      cl_err = kernel.setArg((cl_uint)i, m_arg_size[i], m_arg_ptr[i]);  // m_in_buffer);
      // cl_err = kernel.setArg(i, m_arg_size[i], (void*)m_arg_ptr[i]);  // m_in_buffer);
      // cl_err = kernel.setArg(static_cast<cl_uint>(i), size, (void*)ptr);  // m_in_buffer);
      // cl_err = kernel.setArg(static_cast<cl_uint>(i), size, (void*)ptr);  // m_in_buffer);
      CL_CHECK_ERROR(cl_err, "kernel arg " + to_string(i));
    }
  }

  m_kernel = move(kernel);
}

void Device::initEvents() {
  // cout << "initEvents\n";
  cl_int cl_err;
  cl::UserEvent end(m_context, &cl_err);
  CL_CHECK_ERROR(cl_err, "user event end");

  m_end = move(end);
}

void Device::showInfo() {
  CL_CHECK_ERROR(m_platform.getInfo(CL_PLATFORM_NAME, &m_info_buffer));
  if (m_info_buffer.size() > 2) m_info_buffer.erase(m_info_buffer.size() - 1, 1);
  // if (Show::showIfLessOrMore(show_info)) {
  cout << "Selected platform: " << m_info_buffer << "\n";
  // }
  CL_CHECK_ERROR(m_device.getInfo(CL_DEVICE_NAME, &m_info_buffer));
  if (m_info_buffer.size() > 2) m_info_buffer.erase(m_info_buffer.size() - 1, 1);
  // if (Show::showIfLessOrMore(show_info)) {
  cout << "Selected device: " << m_info_buffer << "\n";
  // }
}

void Device::show() {
  cout << "show\n";

  // int* in_array = m_in_array.get()->data();
  // int* out_array = m_out_array.get()->data();
  // cout << "in[0] " << in_array[0] << "\n";
  // cout << "out[0] " << out_array[0] << "\n";
}

Runtime* Device::getRuntime() { return m_runtime; }
void Device::setRuntime(Runtime* runtime) { m_runtime = runtime; }

void printActionTypeDuration(ActionType action, size_t d) {
  switch (action) {
    case ActionType::init:
      cout << " init: " << d << " ms.\n";
      break;
    case ActionType::useDiscovery:
      cout << " useDiscovery: " << d << " ms.\n";
      break;
    case ActionType::initDiscovery:
      cout << " initDiscovery: " << d << " ms.\n";
      break;
    case ActionType::initContext:
      cout << " initContext: " << d << " ms.\n";
      break;
    case ActionType::initQueue:
      cout << " initQueue: " << d << " ms.\n";
      break;
    case ActionType::initBuffers:
      cout << " initBuffers: " << d << " ms.\n";
      break;
    case ActionType::initKernel:
      cout << " initKernel: " << d << " ms.\n";
      break;
    case ActionType::writeBuffersDummy:
      cout << " writeBuffersDummy: " << d << " ms.\n";
      break;
    case ActionType::writeBuffers:
      cout << " writeBuffers: " << d << " ms.\n";
      break;
    case ActionType::deviceStart:
      cout << " deviceStart: " << d << " ms.\n";
      break;
    case ActionType::deviceReady:
      cout << " deviceReady: " << d << " ms.\n";
      break;
    case ActionType::deviceRun:
      cout << " deviceRun: " << d << " ms.\n";
      break;
    case ActionType::completeWork:
      cout << " completeWork: " << d << " ms.\n";
      break;
    case ActionType::deviceEnd:
      cout << " deviceEnd: " << d << " ms.\n";
      break;
  }
}
}  // namespace clb
