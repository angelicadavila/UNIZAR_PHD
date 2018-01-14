#include "schedulers/dynamic.hpp"

#include <tuple>
#include "device.hpp"
#include "scheduler.hpp"

// #include "clbalancer.hpp"
// #include "rang.hpp"
// void CL_CALLBACK
// callbackRead(cl_event /*event*/, cl_int /*status*/, void* data)
// {
//   clb::Device* device = reinterpret_cast<clb::Device*>(data);
//   clb::DynamicScheduler* sched = device.getDynamicScheduler();
//   cout << "callbackRead\n";
//   sched.callbackDone(device.getID());
//   // device->notifyEvent();
// }

using namespace std;

namespace clb {

void scheduler_thread_func(DynamicScheduler& sched) {
  // cout << "sched thread: init" << "\n";
  // this_thread::sleep_for(1s);
  // cout << "thread: about to wait\n";
  sched.preenq_work();
  // cout << "sched thread: wait callbacks\n";
  while (sched.hasWork()) {
    // sched.waitRequests(); // someone requested more work?
    // cout << "sched thread: notified\n";
    auto moreReqs = true;
    do {
      auto device = sched.getNextRequest();
      // cout << "-sched device: " <<  device << "\n";
      if (device != nullptr) {  // != nullptr
        // cout << "->sched device: " <<  device << " id: " << device->getID() << "\n";
        // cout << "sched thread: getNextRequest() device not null\n";
        auto id = device->getID();
        if (id != 0 && id != 1) {
          cout << "HHH\n";
        } else {
          sched.enq_work(device);
          device->notifyWork();  // give new work to someone that requested
        }
      } else {
        moreReqs = false;
      }
    } while (moreReqs);
    sched.waitCallbacks();  // someone finished
  }
  sched.notifyDevices();
  // device.wait();
  // this_thread::sleep_for(1s);
  // device.init();
  // cout << "sched thread about to req_work to DynamicScheduler\n";
  // device.getDynamicScheduler()->req_work(&device);
  // cout << "sched thread about to waitEvent\n";
  // cout << "thread about to sleep: " << device.getBuffer() << "\n";
  // device.waitEvent();
  // device.show();
  // this_thread::sleep_for(2s);
  // cout << "sched thread about to notifyBarrier: " << device.getBuffer() << "\n";
  // device.notifyBarrier();
}

// DynamicScheduler::DynamicScheduler(WorkSplit wsplit = WorkSplit::By_Devices)
DynamicScheduler::DynamicScheduler(WorkSplit wsplit)
    : m_wsplit(wsplit),
      m_has_work(false),
      m_sema_requests(1),
      m_sema_callbacks(1)
// m_device(device),
// m_barrier(1)
{}
DynamicScheduler::~DynamicScheduler() {
  // cout << "destroy\n";
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void DynamicScheduler::printStats() {
  auto sum = 0;
  auto len = m_devices.size();
  for (uint i = 0; i < len; ++i) {
    sum += m_chunk_done[i];
  }
  cout << "DynamicScheduler:\n";
  cout << "chunks: " << sum << "\n";
}

void DynamicScheduler::notifyDevices() {
  // cout << "notifyDevices tid: " << this_thread::get_id() << "\n";
  for (auto dev : m_devices) {
    // cout << "notifyWork id: " << dev->getID() << "\n";
    dev->notifyWork();
  }
  for (auto dev : m_devices) {
    // cout << "notifyEvent id: " << dev->getID() << "\n";
    dev->notifyEvent();
  }
}

void DynamicScheduler::setWorkSize(size_t size) {
  // size_t given, rem;
  auto bound = 128;
  size_t total = m_size;
  size_t times = size / bound;
  size_t rest = size % bound;
  size_t given = bound;
  if (!size) {
    throw runtime_error("requirement: worksize > 0");
  }
  if (rest > 0) {
    given = (times + 1) * bound;
  } else {
    given = size;
  }
  if (total < given) {
    given = total;
    m_work_last = total;
  } else {
    times = total / given;
    rest = total - (times * given);
    total = times * given;
    m_work_last = rest;
  }
  // tie(given, rem) = splitWork(size, 1.0f, 128);
  // cout << "setWorkSize given: " << given << " from size: " << size << " with m_work_last: " << m_work_last << " and
  // total (m_size): " << m_size << "\n";
  m_worksize = given;
  // m_size = total; TODO change
}

bool DynamicScheduler::hasWork() {
  // return m_has_work;
  return m_size_rem_completed != 0;
}

void DynamicScheduler::waitCallbacks() {
  // cout << "waitCallbacks tid: " << this_thread::get_id() << "\n";
  m_sema_callbacks.wait(1);
}
void DynamicScheduler::waitRequests() { m_sema_requests.wait(1); }
void DynamicScheduler::notifyCallbacks() {
  // cout << "notifyCallbacks tid: " << this_thread::get_id() << "\n";
  m_sema_callbacks.notify(1);
}
void DynamicScheduler::notifyRequests() { m_sema_requests.notify(1); }

void DynamicScheduler::setTotalSize(size_t size) {
  m_size = size;
  m_has_work = true;
  m_size_rem = size;            // NOTE(dyn) statement
  m_size_given = 0;             // NOTE(dyn) used for the offset
  m_size_rem_given = size;      // NOTE(dyn)
  m_size_rem_completed = size;  // NOTE(dyn)
  // calcProportions();
}

tuple<size_t, size_t> DynamicScheduler::splitWork(size_t size, float prop, size_t bound) {
  return {0, 0};  // NOTE(dyn)
  // prop = 0.3323419f;
  size_t given = bound * (static_cast<size_t>(prop * size) / bound);
  // cout << "given: " << given << "\n";
  size_t rem = size - given;
  return {given, rem};
}

void DynamicScheduler::setDevices(vector<Device*>&& devices) {
  m_devices = move(devices);
  m_ndevices = m_devices.size();
  m_chunk_todo = vector<uint>(m_ndevices, 0);
  m_chunk_given = vector<uint>(m_ndevices, 0);
  m_chunk_done = vector<uint>(m_ndevices, 0);

  m_queue_work.reserve(256);

  m_queue_id_work.reserve(m_ndevices);
  m_queue_id_work = move(vector<vector<uint>>(m_ndevices, vector<uint>()));
  for (auto& q_id_work : m_queue_id_work) {
    q_id_work.reserve(256);
  }
  // m_requests_id.reserve(m_ndevices);
  m_devices_working = 0;
  // m_queue_id_work = vector< vector<uint> >(m_ndevices, vector<uint>());
  // for (uint i=0; i<m_ndevices; ++i){
  // vector< vector<uint> > v(m_ndevices, vector<uint>());
  //   m_queue_id_work
  // }
}
// expects worksize set
void DynamicScheduler::calcProportions() {
  return;  // NOTE(dyn)
  vector<tuple<size_t, size_t>> proportions;
  int len = m_ndevices;
  uint last = len - 1;
  size_t wsize_given_acc = 0;
  size_t wsize_given = 0;
  size_t wsize_rem = m_size;
  // cout << "calcProportions: " << m_wsplit << "\n";
  switch (m_wsplit) {
    case WorkSplit::Raw:
      for (uint i = 0; i < last; ++i) {
        auto prop = m_raw_proportions[i];
        // for (auto prop : props){
        tie(wsize_given, wsize_rem) = splitWork(wsize_rem, prop, 128);
        size_t wsize_offset = wsize_given_acc;
        proportions.push_back(make_tuple(wsize_given, wsize_offset));
        wsize_given_acc += wsize_given;
      }
      proportions.push_back(make_tuple(wsize_rem, wsize_given_acc));  // the last
      // m_proportions = move(proportions);
      // for (auto prop : m_proportions){
      //   cout << "proportion: size: " << get<0>(prop) << " offset:" << get<1>(prop) << "\n";
      // }
      break;
    case WorkSplit::By_Devices:
      for (uint i = 0; i < last; ++i) {
        // proportions.push_back(  );
        tie(wsize_given, wsize_rem) = splitWork(wsize_rem, 1.0f / len, 128);
        // cout << "given: " << wsize_given << " rem: " << wsize_rem << "\n";
        size_t wsize_offset = wsize_given_acc;
        proportions.push_back(make_tuple(wsize_given, wsize_offset));
        wsize_given_acc += wsize_given;
      }
      proportions.push_back(make_tuple(wsize_rem, wsize_given_acc));  // the last
      break;
      // case WorkSplit::Decr2:
      //   for (uint i=1; i<len; ++i){
      //     // proportions.push_back(  );
      //     tie(wsize_given, wsize_rem) = splitWork(wsize_rem, (1.0f/len)/(2*i), 128);
      //     cout << "given: " << wsize_given << " rem: " << wsize_rem << "\n";
      //     size_t wsize_offset = wsize_given_acc;
      //     proportions.push_back(make_tuple(wsize_given, wsize_offset));
      //     wsize_given_acc += wsize_given;
      //   }
      //   proportions.push_back(make_tuple(wsize_rem, wsize_given_acc)); // the last
      //   break;
      // case WorkSplit::Incr2:
      //   for (uint i=1; i<len; ++i){
      //     // proportions.push_back(  );
      //     tie(wsize_given, wsize_rem) = splitWork(wsize_rem, (1.0f/len)/(2*i), 128);
      //     cout << "given: " << wsize_given << " rem: " << wsize_rem << "\n";
      //     size_t wsize_offset = wsize_given_acc;
      //     proportions.push_back(make_tuple(wsize_given, wsize_offset));
      //     wsize_given_acc += wsize_given;
      //   }
      //   proportions.push_back(make_tuple(wsize_rem, wsize_given_acc)); // the last
      //   break;
  }
  m_proportions = move(proportions);
  for (auto prop : m_proportions) {
    cout << "proportion: size: " << get<0>(prop) << " offset:" << get<1>(prop) << "\n";
  }
}

void DynamicScheduler::start() {
  m_thread = thread(scheduler_thread_func, ref(*this));
  // cout << "thread start\n";
}

// should be called from only 1 thread (scheduler)
void DynamicScheduler::enq_work(Device* device) {
  int id = device->getID();
  // cout << "DynamicScheduler::enq_work for " << id << " m_size_rem: " << m_size_rem << "\n";
  if (m_size_rem > 0) {
    // for (auto prop : m_proportions){
    //   cout << "proportion: size: " << get<0>(prop) << " offset:" << get<1>(prop) << "\n";
    // }

    size_t offset = m_size_given;
    size_t size = m_worksize;

    m_size_rem -= size;
    m_size_given += size;
    // m_size_rem_given -= size;

    // if (m_work_first){
    //   size
    // }
    // if (m_work_rem < m_work_size){
    //   size
    // }
    // auto prop = m_proportions[id];
    // size_t size, offset;
    // tie(size, offset) = prop;
    // auto size = get<0>(prop);
    // auto offset = get<1>(prop);
    // cout << "proportion for me id: " << id << " size: " << get<0>(m_proportions[id]) << " offset:" <<
    // get<1>(m_proportions[id]) << "\n"; cout << "proportion for me id: " << id << " size: " << size << " offset: " <<
    // offset << "\n";

    // size_t offset = 0;
    // size_t size = 0;
    // size = m_size / m_ndevices;
    // offset = size * id;
    // if (id == 0){

    // }else if (id == 1){

    // }else{
    //   throw runtime_error("unsupported req_work");
    // }
    // {
    // lock_guard<mutex> guard(m_mutex_work);
    // m_queue_
    // }
    // m_queue_work[id].push_back(Work(id, offset, size));

    // m_chunk_id_todo[id]++;
    size_t index = -1;
    // cout << "before device_id: " << id << "\n";
    // cout << " work.offset: " << m_queue_work[index].offset << " work.size: " << m_queue_work[index].size << "\n";
    {
      // cout << " chunk_todo: " << m_chunk_todo[id] << "\n";
      lock_guard<mutex> guard(m_mutex_work);
      index = m_queue_work.size();
      m_queue_work.push_back(Work(id, offset, size));
      m_queue_id_work[id].push_back(index);
      // index -= 1;
      m_chunk_todo[id]++;
    }
    // cout << " index: " << index << "\n";
    // cout << " work.offset: " << m_queue_work[index].offset << " work.size: " << m_queue_work[index].size << "\n";
    // cout << "device_id: " << id << " chunk_todo: " << m_chunk_todo[id] << " index: " << index << " work.offset: " <<
    // m_queue_work[index].offset << " work.size: " << m_queue_work[index].size << "\n";
    // }
  } else {
    cout << "DynamicScheduler::enq_work  not enqueuing\n";
  }
}

void DynamicScheduler::preenq_work() {
  // cout << "DynamicScheduler::preenq_work\n";
  // m_devices_working = m_ndevices;
  // calcProportions();
  // for (auto device : m_devices){
  //   enq_work(device);
  // }
}

void DynamicScheduler::req_work(Device* device) {
  // int id = device->getID();
  // cout << rang::fg::red <<  "YYY DynamicScheduler::req_work id: " << id << rang::style::reset << "\n";
  // {
  //   lock_guard<mutex> guard(m_mutex_work);
  //   m_requests.push(device);
  // }
  // notifyRequests(); // to enqueue at the beginning everything
  {
    lock_guard<mutex> guard(m_mutex_work);
    if (m_size_rem_completed) {
      // cout << "req_work before push next m_requests[] " << m_requests.front() << "\n";
      m_requests.push(device->getID());
    }
  }
  notifyCallbacks();
}

void DynamicScheduler::callback(int queue_index) {
  {
    lock_guard<mutex> guard(m_mutex_work);
    Work work = m_queue_work[queue_index];
    int id = work.device_id;
    m_chunk_done[id]++;
    m_size_rem_completed -= work.size;
    // cout << "callback id " << id << " chunk done: " << m_chunk_done[id] << " work " << m_size_rem_completed << "\n";
    if (m_size_rem_completed) {
      // cout << "before push next m_requests[] " << m_requests.front() << "\n";
      m_requests.push(id);
      if (m_requests.size() > 2) {
        cout << "m_requests is bigger than 2\n";
      }
    }
  }
  notifyCallbacks();
}

// call this function only if you are awaken by scheduler
int DynamicScheduler::getWorkIndex(Device* device) {
  int id = device->getID();
  // cout << "Scheduler::getWorkIndex id " << id << " m_size_rem: " << m_size_rem << " m_size_rem_given: " <<
  // m_size_rem_given << "\n";
  if (m_size_rem_given > 0) {
    uint next = 0;
    int index = -1;
    {
      lock_guard<mutex> guard(m_mutex_work);
      next = m_chunk_given[id]++;
      m_size_rem_given -= m_worksize;
      index = m_queue_id_work[id][next];
    }
    // return m_queue_id_work[id][next];
    return index;
  } else {
    return -1;
  }
}

Work DynamicScheduler::getWork(uint queue_index) {
  // cout << "XXX getWork id: " << m_queue_work[queue_index].device_id << "\n";
  // {
  lock_guard<mutex> guard(m_mutex_work);
  return m_queue_work[queue_index];
  // }
  // return m_queue_work[queue_index];
}

// NOTE(dyn)
Device* DynamicScheduler::getNextRequest() {
  lock_guard<mutex> guard(m_mutex_work);
  Device* dev = nullptr;
  // m_requests.front();
  // int dev = m_requests.front();
  // cout << "getNR dev " << dev << "\n";
  if (!m_requests.empty()) {
    int id = m_requests.front();
    // if (dev != nullptr){
    // check that this device has finished its work given
    if (m_requests.size() > 2) {
      cout << "m_requests is bigger than 2\n";
    }

    // cout << "getNextRequest: size: " << m_requests.size() << " dev_id: " << dev->getID() << "\n";
    if (id != 0 && id != 1) {
      cout << "something is wrong!\n";
    }
    m_requests.pop();
    dev = m_devices[id];
    // cout << "next m_requests[] " << m_requests.front() << "\n";
    // for (auto dev : m_requests){
    // cout << "m_requests[] = " << dev << " ";
    // }
    // cout << "\n";
  } else {
    // cout << "getNextRequest empty\n";
  }
  return dev;
}

}  // namespace clb
