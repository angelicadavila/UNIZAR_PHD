/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#include "runtime.hpp"

#include "device.hpp"
#include "inspector.hpp"
#include "scheduler.hpp"

namespace ecl {

Runtime::Runtime(vector<Device>&& devices, NDRange gws, size_t lws, float ws_bound)
  : m_devices(move(devices))
  , m_gws(gws)
  , m_lws(lws)
  , m_ws_bound(ws_bound)
  , m_sema_all_ready(m_devices.size())
{
  m_barrier = make_shared<semaphore>(m_devices.size());
  m_barrier_init = make_shared<semaphore>(m_devices.size());
  m_sema_ready = make_unique<semaphore>(1);
  
  //If the internal chunk size its not set asumme 1 data per kernel execution
  m_internal_chunk=1;

  m_mutex_duration = new mutex();
  m_time_init = std::chrono::system_clock::now().time_since_epoch();
  m_time = std::chrono::system_clock::now().time_since_epoch();
  m_duration_actions.reserve(2);        // NOTE to improve
  m_duration_offset_actions.reserve(2); // NOTE to improve
}

void
Runtime::saveDuration(ActionType action)
{
  lock_guard<mutex> lock(*m_mutex_duration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - m_time).count();
  m_duration_actions.push_back(make_tuple(diff_ms, action));
  m_time = t2;
}

void
Runtime::saveDurationOffset(ActionType action)
{
  lock_guard<mutex> lock(*m_mutex_duration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::microseconds>(t2 - m_time_init).count();
  m_duration_offset_actions.push_back(make_tuple(diff_ms, action));
}

void
Runtime::printStats()
{
  cout << "Runtime durations:\n";
  for (auto& t : m_duration_actions) {
    auto d = std::get<0>(t);
    auto action = std::get<1>(t);
    if (action == ActionType::initDiscovery) {
      cout << " initDiscovery: " << d << " us.\n";
    }
  }
  for (auto& device : m_devices) {
    device.printStats();
  }
  m_scheduler->printStats();
}

void
Runtime::setKernel(const string& source, const string& kernel)
{
  for (auto& device : m_devices) {
    device.setKernel(source, kernel);
  }
}

void
Runtime::discoverDevices()
{
  IF_LOGGING(cout << "discoverDevices\n");
  cl::Platform::get(&m_platforms);
  IF_LOGGING(cout << "platforms: " << m_platforms.size() << "\n");
  auto i = 0;
  for (auto& platform : m_platforms) {
    vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    IF_LOGGING(cout << "platform: " << i++ << " devices: " << devices.size() << "\n");
    m_platform_devices.push_back(move(devices));
  }
}

cl::Platform
Runtime::usePlatformDiscovery(uint sel_platform)
{
  auto last_platform = m_platforms.size() - 1;
  if (sel_platform > last_platform) {
    throw runtime_error("invalid platform selected");
  }
  return m_platforms[sel_platform];
}

cl::Device
Runtime::useDeviceDiscovery(uint sel_platform, uint sel_device)
{
  auto last_platform = m_platforms.size() - 1;
  if (sel_platform > last_platform) {
    throw runtime_error("invalid platform selected");
  }
  auto last_device = m_platform_devices[sel_platform].size() - 1;
  if (sel_device > last_device) {
    throw runtime_error("invalid device selected");
  }
  return m_platform_devices[sel_platform][sel_device];
}

void
Runtime::run()
{
  m_scheduler->start();

  discoverDevices();
  saveDuration(ActionType::initDiscovery);
  saveDurationOffset(ActionType::initDiscovery);

  for (auto& device : m_devices) {
    device.setBarrier(m_barrier);
    device.setBarrier_init(m_barrier_init);
    device.start();
    device.setInternalChunk(m_internal_chunk);
    cout << "Runtime is waitReady\n";
    // NOTE OCL1.2 fails when multi-threaded discovery
  }

  cout << "Runtime is waiting\n";
  // waitAllReady();
  cout << "Runtime is ready\n";

  for (auto& device : m_devices) {
    device.notifyRun();
  }

  m_barrier.get()->wait(m_devices.size());
}

void
Runtime::notifyAllReady()
{
  m_sema_all_ready.notify(1);
}

void
Runtime::waitAllReady()
{
  m_sema_all_ready.wait(m_devices.size());
}

void
Runtime::notifyReady()
{
  m_sema_ready.get()->notify(1);
}

void
Runtime::waitReady()
{
  m_sema_ready.get()->wait(1);
}

void
Runtime::setScheduler(Scheduler* scheduler)
{
  m_scheduler = scheduler;
  m_scheduler->setTotalSize(m_gws[0]); // TODO gws[0] ?
  m_scheduler->setGWS(m_gws);
  m_scheduler->setLWS(m_lws);
  m_scheduler->setWSBound(m_ws_bound);
  configDevices();
}

void
Runtime::configDevices()
{
  auto id = 0;
  vector<Device*> devices;
  int len = m_devices.size();
  for (auto i = 0; i < len; ++i) {
    Device& device = m_devices[i];
    device.setID(id++);
    device.setScheduler(m_scheduler);
    device.setLWS(m_lws);
    device.setRuntime(this);
    devices.push_back(&device);
  }
  m_scheduler->setDevices(move(devices));
}

} // namespace ecl
