#include "schedulers/static.hpp"

#include <tuple>
#include "device.hpp"
// void CL_CALLBACK
// callbackRead(cl_event /*event*/, cl_int /*status*/, void* data)
// {
//   clb::Device* device = reinterpret_cast<clb::Device*>(data);
//   clb::StaticScheduler* sched = device.getStaticScheduler();
//   cout << "callbackRead\n";
//   sched.callbackDone(device.getID());
//   // device->notifyEvent();
// }

using namespace std;

namespace clb {

void scheduler_thread_func(StaticScheduler& sched) {
  cout << "sched thread: init"
       << "\n";
  // this_thread::sleep_for(1s);
  // cout << "thread: about to wait\n";
  sched.preenq_work();
  cout << "sched thread: wait callbacks\n";
  sched.waitCallbacks();
  // device.wait();
  cout << "sched thread: notified\n";
  // this_thread::sleep_for(1s);
  // device.init();
  cout << "sched thread about to req_work to StaticScheduler\n";
  // device.getStaticScheduler()->req_work(&device);
  cout << "sched thread about to waitEvent\n";
  // cout << "thread about to sleep: " << device.getBuffer() << "\n";
  // device.waitEvent();
  // device.show();
  // this_thread::sleep_for(2s);
  // cout << "sched thread about to notifyBarrier: " << device.getBuffer() << "\n";
  // device.notifyBarrier();
}

StaticScheduler::StaticScheduler(WorkSplit wsplit)
    : m_sema(1),
      m_has_work(false),
      m_wsplit(wsplit)
// m_device(device),
// m_barrier(1)
{}
StaticScheduler::~StaticScheduler() {
  // cout << "destroy\n";
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void StaticScheduler::printStats() {
  auto sum = 0;
  auto len = m_devices.size();
  for (uint i = 0; i < len; ++i) {
    sum += m_chunk_done[i];
  }
  cout << "StaticScheduler:\n";
  cout << "chunks: " << sum << "\n";
}

void StaticScheduler::waitCallbacks() { m_sema.wait(1); }
void StaticScheduler::notifyCallbacks() { m_sema.notify(1); }

void StaticScheduler::setTotalSize(size_t size) {
  m_size = size;
  m_has_work = true;
  // calcProportions();
}

void StaticScheduler::setDevices(vector<Device*>&& devices) {
  m_devices = move(devices);
  m_ndevices = m_devices.size();
  m_chunk_todo = vector<uint>(m_ndevices, 0);
  m_chunk_given = vector<uint>(m_ndevices, 0);
  m_chunk_done = vector<uint>(m_ndevices, 0);
  m_queue_id_work.reserve(m_ndevices);
  m_queue_id_work = move(vector<vector<uint>>(m_ndevices, vector<uint>()));
  m_devices_working = 0;
  // m_queue_id_work = vector< vector<uint> >(m_ndevices, vector<uint>());
  // for (uint i=0; i<m_ndevices; ++i){
  // vector< vector<uint> > v(m_ndevices, vector<uint>());
  //   m_queue_id_work
  // }
}
void StaticScheduler::setRawProportions(const vector<float>& props) {
  cout << "StaticScheduler::setRawProportions\n";
  // vector<tuple<size_t, size_t>> proportions;
  auto last = m_ndevices - 1;
  if (props.size() < last) {
    throw runtime_error("proportions < number of devices - 1");
  }

  if (last == 0) {
    m_raw_proportions = {1.0f};
  } else {
    for (auto prop : props) {
      if (prop <= 0.0f || prop >= 1.0f) {
        throw runtime_error("proportion should be between (0.0f, 1.0f)");
      }
    }
    m_raw_proportions = move(props);
  }
  m_wsplit = WorkSplit::Raw;
}
tuple<size_t, size_t> StaticScheduler::splitWork(size_t size, float prop, size_t bound) {
  // prop = 0.3323419f;
  size_t given = bound * (static_cast<size_t>(prop * size) / bound);
  // cout << "given: " << given << "\n";
  size_t rem = size - given;
  return {given, rem};
}
// expects worksize set
void StaticScheduler::calcProportions() {
  vector<tuple<size_t, size_t>> proportions;
  int len = m_ndevices;
  uint last = len - 1;
  size_t wsize_given_acc = 0;
  size_t wsize_given = 0;
  size_t wsize_rem = m_size;
  cout << "calcProportions: " << m_wsplit << " wsize_rem: " << wsize_rem << "\n";
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
        cout << "given: " << wsize_given << " rem: " << wsize_rem << "\n";
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

void StaticScheduler::start() {
  m_thread = thread(scheduler_thread_func, ref(*this));
  cout << "thread start\n";
}

// should be called from only 1 thread (scheduler)
void StaticScheduler::enq_work(Device* device) {
  int id = device->getID();
  cout << "StaticScheduler::enq_work for " << id << "\n";
  if (m_chunk_todo[id] == 0) {  // Static
    // for (auto prop : m_proportions){
    //   cout << "proportion: size: " << get<0>(prop) << " offset:" << get<1>(prop) << "\n";
    // }
    auto prop = m_proportions[id];
    size_t size, offset;
    tie(size, offset) = prop;
    // auto size = get<0>(prop);
    // auto offset = get<1>(prop);
    // cout << "proportion for me id: " << id << " size: " << get<0>(m_proportions[id]) << " offset:" <<
    // get<1>(m_proportions[id]) << "\n";
    cout << "proportion for me id: " << id << " size: " << size << " offset: " << offset << "\n";

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
    auto index = m_queue_work.size();
    m_queue_work.push_back(Work(id, offset, size));
    m_queue_id_work[id].push_back(index);

    // m_chunk_id_todo[id]++;
    // {
    // lock_guard<mutex> guard(m_mutex_work);
    m_chunk_todo[id]++;
    cout << "device_id: " << id << " chunk_todo: " << m_chunk_todo[id] << " index: " << index
         << " work.offset: " << m_queue_work[index].offset << " work.size: " << m_queue_work[index].size << "\n";
    // }
  } else {
    cout << "StaticScheduler::enq_work  not enqueuing\n";
  }
}

void StaticScheduler::preenq_work() {
  cout << "StaticScheduler::preenq_work\n";
  m_devices_working = m_ndevices;
  calcProportions();
  // for (auto device : m_devices){
  //   enq_work(device);
  // }
}

void StaticScheduler::req_work(Device* device) {
  cout << "StaticScheduler::req_work\n";
  // int id = device->getID();
  enq_work(device);
  device->notifyWork();
  // device->do_work(offset, size);
}

void StaticScheduler::callback(int queue_index) {
  // cout << "StaticScheduler callback\n";
  {
    lock_guard<mutex> guard(m_mutex_work);
    Work work = m_queue_work[queue_index];
    int id = work.device_id;
    m_chunk_done[id]++;
    if (m_chunk_done[id] == 1) {  // Static
      Device* device = m_devices[id];
      m_devices_working--;
      device->notifyWork();
      device->notifyEvent();
      if (m_devices_working == 0) {
        notifyCallbacks();
      }
    }
  }
  // device->notifyEvent();
}

// call this function only if you are awaken by scheduler
int StaticScheduler::getWorkIndex(Device* device) {
  int id = device->getID();
  if (m_has_work) {
    if (m_chunk_given[id] == 0) {
      uint next = 0;
      {
        lock_guard<mutex> guard(m_mutex_work);
        next = m_chunk_given[id]++;
      }
      return m_queue_id_work[id][next];
    } else {  // no more for this device (static)
      return -1;
    }
  } else {
    return -1;
  }
}

Work StaticScheduler::getWork(uint queue_index) { return m_queue_work[queue_index]; }

}  // namespace clb
