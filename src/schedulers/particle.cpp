#include "schedulers/particle.hpp"

#include <tuple>
#include <cassert>
#include <cmath>
#include "device.hpp"

#define ATOMIC 1
// #define ATOMIC 0
namespace ecl {

void
scheduler_thread_func(SwarmScheduler& sched)
{
  uint id_dev=0;
  sched.preenq_work();
  while (sched.hasWork()) {
    auto moreReqs = true;
    do {
      auto device = sched.getNextRequest();
      if (device != nullptr) { // != nullptr
        sched.enq_work(device);
        id_dev=device->getID();
        if (sched.particleBarrier(device)==0){
          device->notifyWork();
        }
      } else {
        moreReqs = false;
      }
    } while (moreReqs);
    sched.waitCallbacks();
  }
  sched.notifyDevices();
  sched.saveDuration(ActionType::schedulerEnd);
  sched.saveDurationOffset(ActionType::schedulerEnd);
}

uint
SwarmScheduler::particleBarrier(Device* device){
  int id = device->getID();
  return p_barrier[id];
}

SwarmScheduler::SwarmScheduler(WorkSplit wsplit)
  : m_wsplit(wsplit)
  , m_num_particles(4)
  , m_has_work(false)
  , m_sema_requests(1)
  , m_sema_callbacks(1)
  , m_chunks_done(0)
  , m_requests_max(0)
  , m_requests_idx(0)
  , m_requests_idx_done(0)
  , m_requests_list(0, 0)
{
  //constant values from
  //J. Kennedy, R.C. Eberhart, et al., “Particle swarm optimization”
  w=0.7;
  c1=2;
  c2=2;
  Vmin=128;
  Vmax=m_size/4;
  m_mutex_duration = new mutex();
  m_time_init = std::chrono::system_clock::now().time_since_epoch();
  m_time = std::chrono::system_clock::now().time_since_epoch();
  m_duration_actions.reserve(8);        // NOTE to improve
  m_duration_offset_actions.reserve(8); // NOTE to improve
}

SwarmScheduler::~SwarmScheduler()
{
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void
SwarmScheduler::endScheduler()
{
  saveDuration (ActionType::schedulerEnd);
  saveDurationOffset(ActionType::schedulerEnd);
}


void
SwarmScheduler::printStats()
{
  auto sum = 0;
  auto len = m_devices.size();
  for (uint i = 0; i < len; ++i) {
    sum += m_chunk_done[i];
  }
  cout << "SwarmScheduler:\n";
  cout << "chunks: " << sum << "\n";
  cout << "chunks (ATOMIC): " << m_chunks_done << "\n";
  cout << "duration offsets from init:\n";
  for (auto& t : m_duration_offset_actions) {
    Inspector::printActionTypeDuration(std::get<1>(t), std::get<0>(t));
  }
}

void
SwarmScheduler::notifyDevices()
{
  for (auto dev : m_devices) {
    dev->notifyWork();
  }
  for (auto dev : m_devices) {
    dev->notifyEvent();
  }
}

void
SwarmScheduler::saveDuration(ActionType action)
{
  lock_guard<mutex> lock(*m_mutex_duration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - m_time).count();
  m_duration_actions.push_back(make_tuple(diff_ms, action));
  m_time = t2;
}
void
SwarmScheduler::saveDurationOffset(ActionType action)
{
  lock_guard<mutex> lock(*m_mutex_duration);
  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - m_time_init).count();
  m_duration_offset_actions.push_back(make_tuple(diff_ms, action));
}

void
SwarmScheduler::setWorkSize(size_t size)
{
  if (!size) {
    throw runtime_error("requirement: worksize > 0");
  }
}

bool
SwarmScheduler::hasWork()
{
  return m_size_rem_completed != 0;
}

void
SwarmScheduler::waitCallbacks()
{
  m_sema_callbacks.wait(1);
}

void
SwarmScheduler::waitRequests()
{
  m_sema_requests.wait(1);
}

void
SwarmScheduler::notifyCallbacks()
{
  m_sema_callbacks.notify(1);
}

void
SwarmScheduler::notifyRequests()
{
  m_sema_requests.notify(1);
}

void SwarmScheduler::updateTime(uint id)
{
  auto id_particle=m_id_particle[id];
  auto t2 = std::chrono::system_clock::now().time_since_epoch(); 
  Time_Xi[id][id_particle]=std::chrono::duration_cast<std::chrono::microseconds>(t2 -base_time).count();
  cout<<"\n time_prev:"<<Time_Xi[id][id_particle]<<"\n";
  if (id_particle>0)
    Time_Xi[id][id_particle]-=Time_Xi[id][id_particle-1]; //base measured time

  //next particle
  m_id_particle[id]++;
  cout<<"\n time:"<<Time_Xi[id][id_particle]<< "\n id particle"<<id_particle;
}

void
SwarmScheduler::setTotalSize(size_t size)
{
  m_size = size;
  m_has_work = true;
  m_size_rem = size;
  m_size_rem_completed = size; // NOTE(dyn)
}

tuple<size_t, size_t>
SwarmScheduler::splitWork(size_t size, float prop, size_t bound)
{
  return std::make_tuple<size_t, size_t>( 0, 0 ); // NOTE(dyn)
  size_t given = bound * (static_cast<size_t>(prop * size) / bound);
  // cout << "given: " << given << "\n";
  size_t rem = size - given;
  return std::make_tuple( given, rem );
}

void
SwarmScheduler::setDevices(vector<Device*>&& devices)
{
  m_devices = move(devices);
  m_ndevices = m_devices.size();
  g_barrier=0;
  m_chunk_done = vector<uint>(m_ndevices, 0);
  m_chunk_given= vector<uint>(m_ndevices,0);
  m_chunk_todo = vector<uint>(m_ndevices,0);
  m_requests_max = m_ndevices * 2;
  m_requests_list = move(vector<uint>(m_requests_max, 0));
  // TODO expected callbacks
  // m_queue_work.reserve(256);
  m_queue_work.reserve(65536);

  m_queue_id_work.reserve(m_ndevices);
  m_queue_id_work = move(vector<vector<uint>>(m_ndevices, vector<uint>()));
  for (auto& q_id_work : m_queue_id_work) {
    // q_id_work.reserve(256);
    q_id_work.reserve(65536);
  }

  //Particle swarm optimization vectors
  Xi.resize(m_ndevices, std::vector<uint>(m_num_particles, 0));
  //limit  initialization problem
  uint init_lim=(m_size<10240)?2:4;
  for (uint i=0; i< m_num_particles;i++){
    for (uint j=0; j<m_ndevices;j++){
     //maximun 1/2 of problem can be assigned in initial state
      //Xi[i][j]=floor(rand()%(m_size/(m_ndevices * init_lim)));
      Xi[j][i]=floor(rand()%(80000)*128);
      cout<<" initial vector -- "<<Xi[j][i];
    }cout<<"\n";
  }

  cout<<"number of particles: "<<m_num_particles<<"\n";
  Pi.resize(m_ndevices, std::vector<float>(m_num_particles, 0));//personal local best
  Vi.resize(m_ndevices, std::vector<float>(m_num_particles, 0));//Velocity
  Pi_thg =vector<float>(m_num_particles, 0xffff); //local best througput
  Pgbest=vector<float>(m_ndevices, 0);//global best
  Thg_Xi.resize (m_ndevices, std::vector<float>(m_num_particles, 0));//througput best
  Time_Xi.resize( m_ndevices, std::vector<double>(m_num_particles, 0));//time in each device
  duration_Xi.resize(m_num_particles, std::vector<double>(m_ndevices, 0));//time in each device
  m_id_particle = vector<uint> (m_ndevices, 0);//counter of actual particle
  p_barrier = vector<uint>(m_ndevices,0); 
}

/**
 * expects worksize set
 */
void
SwarmScheduler::calcProportions()
{
  return; // NOTE
}
void
SwarmScheduler::setGWS(NDRange gws)
{
  m_gws = gws;

}

void
SwarmScheduler::setLWS(size_t lws)
{
  m_lws = lws;
}

void
SwarmScheduler::setWSBound(float ws_bound)
{
  m_ws_bound = ws_bound;
}



void
SwarmScheduler::start()
{
  m_thread = thread(scheduler_thread_func, std::ref(*this));
}

/**
 * should be called from only 1 thread (scheduler)
 */
void
SwarmScheduler::enq_work(Device* device)
{
  int id = device->getID();

  if (m_size_rem > 0) {
    //Send chunk of the particles
    auto id_particle=m_id_particle[id];
    size_t size =Xi[id][id_particle] ;
    cout<<"size: "<< size<< " id_Particle: "<<id_particle<<"\n";
    cout<<"size_remaining: "<< m_size_rem<< "\n";
    cout<<"barrier: "<< p_barrier[id] << "\n";
    cout<<"gbarrier: "<< g_barrier << "\n";
    size_t index = -1;
   uint wait_Xi=1; 
    //next particle
  if (id_particle==m_num_particles)
    {
      g_barrier++; //if all particles finished calculate the next position
     p_barrier[id]++; 
     m_id_particle[id]=0;
      wait_Xi=0; 
    }
//if all particles finished calculate the next position
  //we emulate the iteration in the point when all particles finish calculation.
   if(g_barrier==m_num_particles){
      cout<<"synchronize";
      g_barrier=0;
      //calculate the local and global best
      //througput of each particle
      double best_thg ; //save the best throuput
      double global_thg=0xffff ; //save the best throuput
      double best_i ; //save the best particle
      double worst_time ; //save the best throuput
      for (uint i=0; i<m_num_particles; i++)
      {
        double aux=Xi[0][i];
        worst_time=Time_Xi[0][i] ; //save the best throuput
        for (uint j=1; j<m_ndevices;j++){
            aux+=(Xi[j][i]);
            worst_time= (worst_time<Time_Xi[j][i])?Time_Xi[j][i]:worst_time;
        }
        best_thg=aux/worst_time;
        //local personal best particle 
         if (best_thg<Pi_thg[i]){
          Pi_thg[i]=best_thg;
            for (uint j=0; j<m_ndevices;j++)
              Pi[j][i]=Xi[j][i];
        }
       //global best particle
        if (best_thg<global_thg){
          global_thg=best_thg;
          for (uint j=0; j<m_ndevices;j++)
             Pgbest[j]=Xi[j][i];
        }
      }

      //calculate the new solution for all partcles 
      for (uint j=0; j<m_ndevices;j++)
      {
        p_barrier[j]=0;
          for (uint i=0; i<m_num_particles; i++){
            //update velocity
            Vi[j][i]=w*Vi[j][i]+ c1 * rand()*(Pi[j][i]-Xi[j][i])+ c2 * rand()*(Pgbest[j]-Xi[j][i]);
            Xi[j][i]=Xi[j][i]+Vi[j][i];
            cout<<"Velocity ---- "<<Vi[j][i];
          }
        exit(0); 
        //notify all stoped devices
        
        m_id_particle[j]=id_particle+1;
        if ( j !=id && m_size_rem>0){ 
          size =Xi[j][0] ;
          device = m_devices[j];
           size_t offset = m_size_given;
            m_size_rem -= size;
            m_size_given += size;
            index = m_queue_work.size();
            m_queue_work.push_back(Work(j, offset, size,m_ws_bound));
            m_queue_id_work[j].push_back(index);
            m_chunk_todo[j]++;
          device->notifyWork();
        }
      }
     base_time = std::chrono::system_clock::now().time_since_epoch();
   }
    
    if(wait_Xi && m_size_rem>0)
    {
      lock_guard<mutex> guard(m_mutex_work);
      size_t offset = m_size_given;
      m_size_rem -= size;
      m_size_given += size;
      index = m_queue_work.size();
      m_queue_work.push_back(Work(id, offset, size,m_ws_bound));
      m_queue_id_work[id].push_back(index);
      //m_chunk_todo[id]++;
    }
  } else {
    cout << "SwarmScheduler::enq_work  not enqueuing\n";
  }
}

void
SwarmScheduler::preenq_work()
{

}

void
SwarmScheduler::req_work(Device* device)
{

  //initial time to measure througput in devices
  saveDuration(ActionType::schedulerStart);
  saveDurationOffset(ActionType::schedulerStart);

  if (m_size_rem_completed > 0) {
    auto idx = m_requests_idx++ % m_requests_max;
    m_requests_list[idx] = device->getID() + 1;
  }

  notifyCallbacks();
  base_time = std::chrono::system_clock::now().time_since_epoch();
}

void
SwarmScheduler::callback(int queue_index)
{
  Work work = m_queue_work[queue_index];
  int id = work.device_id;
  m_chunks_done++;
  if (m_size_rem > 0) {
    auto idx = m_requests_idx++ % m_requests_max;
    m_requests_list[idx] = id + 1;
  }

  //save the time
  updateTime(id);
  //wakeup scheduler_thread_func
  notifyCallbacks();
}

/**
 * call this function only if you are awaken by scheduler
 */
int
SwarmScheduler::getWorkIndex(Device* device)
{
  int id = device->getID();

  lock_guard<mutex> guard(m_mutex_work);
  if (m_size_rem > 0) {
    uint next = 0;
    int index = -1;
    {
      next = m_chunk_given[id]++;
      index = m_queue_id_work[id][next];
    }
    return index;
  } else {
    return -1;
  }
}

Work
SwarmScheduler::getWork(uint queue_index)
{
  lock_guard<mutex> guard(m_mutex_work);
  return m_queue_work[queue_index];
}


//only thread Scheduler
Device*
SwarmScheduler::getNextRequest()
{
  Device* dev = nullptr;
  uint idx_done = m_requests_idx_done % m_requests_max;
  uint id = m_requests_list[idx_done];
  if (id > 0) {
    dev = m_devices[id - 1];
    m_requests_list[idx_done] = 0;
    m_requests_idx_done++;
  }

  return dev;
}
} // namespace ecl
