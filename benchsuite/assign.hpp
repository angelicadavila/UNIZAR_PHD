#ifndef CLBALANCER_BENCHS_ASSIGN_HPP
#define CLBALANCER_BENCHS_ASSIGN_HPP

#include <memory>
#include <string>
#include <vector>
// #include "runtime.hpp"
// #include "sched.hpp"
// #include "device.hpp"
// #include "buffer.hpp"

#include "clbalancer.hpp"

using namespace std;

// namespace clb {
//   class Runtime;
//   class Scheduler;
//   class Device;
// }

void do_assign(int tscheduler, int tdevices, bool check, int wsize, int chunksize, float prop);

#endif /* CLBALANCER_BENCHS_ASSIGN_HPP */
