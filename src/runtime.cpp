#include "runtime.hpp"

#include "device.hpp"
#include "scheduler.hpp"

namespace clb {

Runtime::Runtime(vector<Device>&& devices, size_t size)
    :  // m_barrier(devices.size()),
       // m_devices(devices),
      m_devices(move(devices)),
      // m_device2(device2),
      m_size(size),
      m_sema_all_ready(m_devices.size())
// m_barrier(2)
{
  // m_barrier = semaphore(m_devices.size());
  // m_devices = move(devices);
  m_barrier = make_shared<semaphore>(m_devices.size());
  // m_sema_all_ready = make_unique<semaphore>(m_devices.size());
  m_sema_ready = make_unique<semaphore>(1);

  m_mutex_duration = new mutex();
  m_time_init = std::chrono::system_clock::now().time_since_epoch();
  m_time = std::chrono::system_clock::now().time_since_epoch();
  m_duration_actions.reserve(2);         // to improve
  m_duration_offset_actions.reserve(2);  // to improve
}

void Runtime::saveDuration(ActionType action) {
  lock_guard<mutex> lock(*m_mutex_duration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = chrono::duration_cast<chrono::milliseconds>(t2 - m_time).count();
  m_duration_actions.push_back(make_tuple(diff_ms, action));
  m_time = t2;
}
void Runtime::saveDurationOffset(ActionType action) {
  lock_guard<mutex> lock(*m_mutex_duration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = chrono::duration_cast<chrono::milliseconds>(t2 - m_time_init).count();
  m_duration_offset_actions.push_back(make_tuple(diff_ms, action));
}
// TODO cl_int instead void
// template <typename T>
// void setKernelArg(cl_uint index, const T &value)
// {
//   for (auto& device : m_devices){
//     device.setKernelArg(index, value);
//   }
//
// template <typename T>
// void Runtime::setInBuffer(shared_ptr<vector<T>> array) {
//   for (auto& device : m_devices) {
//     device.setInBuffer(array);
//   }
// }
// template <typename T>
// void Runtime::setOutBuffer(shared_ptr<vector<T>> array) {
//   for (auto& device : m_devices) {
//     device.setOutBuffer(array);
//   }
// }
// void Runtime::setInBuffer(Buffer& buffer){
//   for (auto& device : m_devices){
//     device.setInBuffer(buffer);
//   }
// }
// void Runtime::setOutBuffer(Buffer& buffer){
//   for (auto& device : m_devices){
//     device.setOutBuffer(buffer);
//   }
// }

void Runtime::printStats() {
  cout << "Runtime durations:\n";
  for (auto& t : m_duration_actions) {
    auto d = get<0>(t);
    auto action = get<1>(t);
    switch (action) {
      case ActionType::initDiscovery:
        cout << " initDiscovery: " << d << " ms.\n";
        break;
    }
  }
  for (auto& device : m_devices) {
    device.printStats();
  }
  m_scheduler->printStats();
}

void Runtime::setKernel(const string& source, const string& kernel) {
  for (auto& device : m_devices) {
    device.setKernel(source, kernel);
  }
}

void Runtime::discoverDevices() {
  cout << "discoverDevices\n";
  // vector<cl::Platform> platforms;
  // vector<vector<cl::Device>> platform_devices;
  cl::Platform::get(&m_platforms);
  cout << "platforms: " << m_platforms.size() << "\n";
  auto i = 0;
  for (auto& platform : m_platforms) {
    vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    cout << "platform: " << i++ << " devices: " << devices.size() << "\n";
    m_platform_devices.push_back(move(devices));
  }
}
cl::Platform Runtime::usePlatformDiscovery(uint sel_platform) {
  auto last_platform = m_platforms.size() - 1;
  if (sel_platform < 0 || sel_platform > last_platform) {
    throw runtime_error("invalid platform selected");
  }
  return m_platforms[sel_platform];
}
cl::Device Runtime::useDeviceDiscovery(uint sel_platform, uint sel_device) {
  auto last_platform = m_platforms.size() - 1;
  if (sel_platform < 0 || sel_platform > last_platform) {
    throw runtime_error("invalid platform selected");
  }
  auto last_device = m_platform_devices[sel_platform].size() - 1;
  if (sel_device < 0 || sel_device > last_device) {
    throw runtime_error("invalid device selected");
  }
  return m_platform_devices[sel_platform][sel_device];
}

void Runtime::run() {
  m_scheduler->start();

  discoverDevices();
  saveDuration(ActionType::initDiscovery);
  saveDurationOffset(ActionType::initDiscovery);

  for (auto& device : m_devices) {
    // device.usePlatformDevice(m_platforms, m_platform_devices);
    device.setBarrier(m_barrier);
    device.start();
    // waitReady();
    cout << "Runtime is waitReady\n";
    // this_thread::sleep_for(100ms); // NOTE only in OCL1.2
  }

  cout << "Runtime is waiting\n";
  // waitAllReady();
  cout << "Runtime is ready\n";

  for (auto& device : m_devices) {
    device.notifyRun();
  }
  // m_device2.start();
  // cout << "Runtime::wait\n";
  // m_barrier.wait(2);
  m_barrier.get()->wait(m_devices.size());
  // cout << "Runtime::waited\n";
}

void Runtime::notifyAllReady() { m_sema_all_ready.notify(1); }
void Runtime::waitAllReady() { m_sema_all_ready.wait(m_devices.size()); }
void Runtime::notifyReady() { m_sema_ready.get()->notify(1); }
void Runtime::waitReady() { m_sema_ready.get()->wait(1); }

void Runtime::setScheduler(Scheduler* scheduler) {
  m_scheduler = scheduler;
  m_scheduler->setTotalSize(m_size);
  configDevices();
}
// Runtime
void Runtime::configDevices() {
  auto id = 0;
  vector<Device*> devices;
  int len = m_devices.size();
  for (auto i = 0; i < len; ++i) {
    // for (auto& device : m_devices){
    Device& device = m_devices[i];
    device.setID(id++);
    device.setScheduler(m_scheduler);
    device.setRuntime(this);
    devices.push_back(&device);
  }
  m_scheduler->setDevices(move(devices));
}

}  // namespace clb
