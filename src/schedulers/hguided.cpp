#include "schedulers/hguided.hpp"

#include <tuple>
#include "device.hpp"
#include "scheduler.hpp"

// #include "clbalancer.hpp"
// #include "rang.hpp"
// void CL_CALLBACK
// callbackRead(cl_event /*event*/, cl_int /*status*/, void* data)
// {
//   clb::Device* device = reinterpret_cast<clb::Device*>(data);
//   clb::HGuidedScheduler* sched = device.getHGuidedScheduler();
//   cout << "callbackRead\n";
//   sched.callbackDone(device.getID());
//   // device->notifyEvent();
// }

using namespace std;

namespace clb {

void scheduler_thread_func(HGuidedScheduler& sched) {
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
    // cout << "sched awaken\n";
  }
  sched.notifyDevices();
  // device.wait();
  // this_thread::sleep_for(1s);
  // device.init();
  // cout << "sched thread about to req_work to HGuidedScheduler\n";
  // device.getHGuidedScheduler()->req_work(&device);
  // cout << "sched thread about to waitEvent\n";
  // cout << "thread about to sleep: " << device.getBuffer() << "\n";
  // device.waitEvent();
  // device.show();
  // this_thread::sleep_for(2s);
  // cout << "sched thread about to notifyBarrier: " << device.getBuffer() << "\n";
  // device.notifyBarrier();
}

// HGuidedScheduler::HGuidedScheduler(WorkSplit wsplit = WorkSplit::By_Devices)
HGuidedScheduler::HGuidedScheduler(WorkSplit wsplit)
    : m_wsplit(wsplit),
      m_has_work(false),
      m_sema_requests(1),
      m_sema_callbacks(1)
// m_device(device),
// m_barrier(1)
{}
HGuidedScheduler::~HGuidedScheduler() {
  // cout << "destroy\n";
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void HGuidedScheduler::printStats() {
  auto sum = 0;
  auto len = m_devices.size();
  for (uint i = 0; i < len; ++i) {
    sum += m_chunk_done[i];
  }
  cout << "HGuidedScheduler:\n";
  cout << "chunks: " << sum << "\n";
}

void HGuidedScheduler::notifyDevices() {
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

// Min Chunk size
void HGuidedScheduler::setWorkSize(size_t size) {
  // size_t given, rem;
  auto bound = 128;
  size_t total = m_size;
  size_t times = size / bound;
  size_t rest = size % bound;
  size_t given = bound;
  if (!size) {
    throw runtime_error("requirement: worksize > 0");
  }
  if (rest) {
    times++;
    // given = (times + 1) * bound;
    // } else {
    // given = size;
  }
  given = times * bound;  // min chunk multiple of LWS
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

bool HGuidedScheduler::hasWork() {
  // return m_has_work;
  return m_size_rem_completed != 0;
}

void HGuidedScheduler::waitCallbacks() {
  // cout << "waitCallbacks tid: " << this_thread::get_id() << "\n";
  m_sema_callbacks.wait(1);
}
void HGuidedScheduler::waitRequests() { m_sema_requests.wait(1); }
void HGuidedScheduler::notifyCallbacks() {
  // cout << "notifyCallbacks tid: " << this_thread::get_id() << "\n";
  m_sema_callbacks.notify(1);
}
void HGuidedScheduler::notifyRequests() { m_sema_requests.notify(1); }

void HGuidedScheduler::setTotalSize(size_t size) {
  m_size = size;
  m_has_work = true;
  m_size_rem = size;            // NOTE(dyn) statement
  m_size_given = 0;             // NOTE(dyn) used for the offset
  m_size_rem_given = size;      // NOTE(dyn)
  m_size_rem_completed = size;  // NOTE(dyn)
  // calcProportions();
}

tuple<size_t, size_t> HGuidedScheduler::splitWork(size_t size, float prop, size_t bound) {
  return {0, 0};  // NOTE(dyn)
  // prop = 0.3323419f;
  size_t given = bound * (static_cast<size_t>(prop * size) / bound);
  // cout << "given: " << given << "\n";
  size_t rem = size - given;
  return {given, rem};
}

void HGuidedScheduler::setDevices(vector<Device*>&& devices) {
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
  m_raw_proportions.reserve(m_ndevices);
  // m_queue_id_work = vector< vector<uint> >(m_ndevices, vector<uint>());
  // for (uint i=0; i<m_ndevices; ++i){
  // vector< vector<uint> > v(m_ndevices, vector<uint>());
  //   m_queue_id_work
  // }
}

void HGuidedScheduler::start() {
  m_thread = thread(scheduler_thread_func, ref(*this));
  // cout << "thread start\n";
}

void HGuidedScheduler::setRawProportions(const vector<float>& props) { m_raw_proportions = move(props); }

void HGuidedScheduler::calcProportions() {}
void HGuidedScheduler::normalizeRawProportions() {
  // cout << "HGuidedScheduler::setRawProportions\n";
  // vector<tuple<size_t, size_t>> proportions;
  auto last = m_ndevices - 1;
  auto& props = m_raw_proportions;
  if (props.size() < last) {
    throw runtime_error("proportions < number of devices - 1");
  }

  if (last == 0) {
    m_raw_proportions = {1.0f};
  } else {
    auto sum = 0.0f;
    auto propLast = 0.0f;
    auto nprops = 0;
    for (auto prop : props) {
      cout << prop << "\n";
      if (prop <= 0.0f || prop >= 1.0f) {
        throw runtime_error("proportion should be between (0.0f, 1.0f)");
      }
      sum += prop;
      propLast = prop;
      nprops++;
    }
    auto wrong = true;
    // auto diff = 1.0f - sum;
    // auto diffAbs = abs(diff);
    // cout << "diff: " << diff << "\n";
    // auto ok = false;
    // if (diffAbs < 0.001f || diff < 0.0f){
    //   if (nprops == m_ndevices){ // we correct the last prop
    //     props[last] = 1.0f - (sum - propLast);
    //     diff = 1.0f - sum;
    //     ok = abs(1.0f - )
    //   }
    // }else
    if (nprops == last) {  // we set the last prop
      props[last] = 1.0f - sum;
      wrong = false;
    }
    if (wrong) {
      throw runtime_error("proportion exceeds 1.0f and cannot be corrected");
    }
    // m_raw_proportions = move(props);
  }
}
// should be called from only 1 thread (scheduler)
void HGuidedScheduler::enq_work(Device* device) {
  int id = device->getID();
  // cout << "HGuidedScheduler::enq_work for " << id << " m_size_rem: " << m_size_rem << "\n";
  if (m_size_rem > 0) {
    // for (auto prop : m_proportions){
    //   cout << "proportion: size: " << get<0>(prop) << " offset:" << get<1>(prop) << "\n";
    // }

    size_t offset = m_size_given;
    size_t size = m_worksize;

    auto prop = m_raw_proportions[id];
    size_t lws = 128;
    size_t min_worksize = m_worksize;  // assume m_worksize is multiple of 128
    auto new_size = splitWorkLikeHGuided(m_size_rem, min_worksize, lws, prop);

    size = new_size;

    cout << "enq_work offset: " << offset << " m_size_rem: " << m_size_rem << " prop: " << prop << " lws: " << lws
         << " new_size: " << new_size << " m_worksize: " << m_worksize << "\n";

    m_size_rem -= new_size;
    m_size_given += new_size;
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
      // cout << "CCC id: " << id << " index: "<< index << " size: " << size << " m_queue_id_work: " <<
      // m_queue_id_work.size() << "\n";
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
    cout << "HGuidedScheduler::enq_work  not enqueuing\n";
  }
}

void HGuidedScheduler::preenq_work() {
  // cout << "HGuidedScheduler::preenq_work\n";
  normalizeRawProportions();
  // m_devices_working = m_ndevices;
  // calcProportions();
  // for (auto device : m_devices){
  //   enq_work(device);
  // }
}

void HGuidedScheduler::req_work(Device* device) {
  // int id = device->getID();
  // cout << rang::fg::red <<  "YYY HGuidedScheduler::req_work id: " << id << rang::style::reset << "\n";
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

// accepts multiple threads
void HGuidedScheduler::callback(int queue_index) {
  // int id = device->getID();
  // cout << "HGuidedScheduler callback id " << id << "\n";
  {
    lock_guard<mutex> guard(m_mutex_work);
    Work work = m_queue_work[queue_index];
    int id = work.device_id;
    m_chunk_done[id]++;
    m_size_rem_completed -= work.size;  // m_worksize;
    // cout << "CCC callback index: " << queue_index << " id: " << id << " work.size: " << work.size << " chunk done: "
    // << m_chunk_done[id] << " work " << m_size_rem_completed << "\n";
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

// tuple<uint, uint> HGuidedScheduler::getAvailableWorks(){
//   return make_tuple();
// }
// uint HGuidedScheduler::getAvailableWorks(Device* device){
//   int id = device->getID();
//   return m_chunk_todo[id] - m_chunk_done[id];
// }
// call this function only if you are awaken by scheduler
int HGuidedScheduler::getWorkIndex(Device* device) {
  int id = device->getID();
  // cout << "Scheduler::getWorkIndex id " << id << " m_size_rem: " << m_size_rem << " m_size_rem_given: " <<
  // m_size_rem_given << "\n";
  if (m_size_rem_given > 0) {
    uint next = 0;
    int index = -1;
    {
      lock_guard<mutex> guard(m_mutex_work);
      next = m_chunk_given[id]++;
      index = m_queue_id_work[id][next];
      Work work = m_queue_work[index];
      // cout << "XXX id: " << id << " next: " << next << " index: " << index << " work.size: " << work.size << "\n";
      // m_size_rem_given -= m_worksize;
      m_size_rem_given -= work.size;
    }
    // return m_queue_id_work[id][next];
    return index;
  } else {
    return -1;
  }
}

Work HGuidedScheduler::getWork(uint queue_index) {
  // cout << "XXX getWork id: " << m_queue_work[queue_index].device_id << "\n";
  // {
  lock_guard<mutex> guard(m_mutex_work);
  return m_queue_work[queue_index];
  // }
  // return m_queue_work[queue_index];
}

// NOTE(dyn)
Device* HGuidedScheduler::getNextRequest() {
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
    // auto id = dev->getID();
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
