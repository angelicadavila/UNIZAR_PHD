#include "semaphore.hpp"
// #include "device.hpp"
// #include "scheduler.hpp"

// #include "runtime.hpp"

#include <chrono>
#include <iostream>
#include <string>

#include "assign.hpp"
#include "gaussian.hpp"
#include "vecadd.hpp"

// #include <omp.h>

// #include "fmt/format.h"
// #include "rang/rang.hpp"
// #include "spdlog/spdlog.h"

#include <thread>

// using namespace std::chrono_literals;
// using namespace std;

using std::stof;
using std::stoi;

void
usage()
{
  std::cout << "needs (scheduler = 0,1,2) (devices = 0,1,2) (bench = 0,1,2) (check "
               "0,1) (size * 128|size * "
               "128|image_width) (chunksize * "
               "128|min chunksize) (prop .f) (filter_width)\n"
               "  - scheduler: 0 (static), 1 (dynamic), 2 (hguided)\n"
               "  - devices: 0 (first, maybe CPU), 1 (second, maybe GPU), 2 (2 "
               "devices, first and second)\n"
               "  - bench: 0 (assign), 1 (vecadd), 2 (gaussian)\n"
               "  - check: 0 (no), 1 (yes)"
               "  - problem size (use a multiple of 128 for gaussian)\n"
               "  - chunk size (use a multiple of 128 for gaussian)\n"
               "  - proportion (proportion for the first device between 0.0 and 1.0)\n"
               "  - filter width (for gaussian)\n";
}

int
main(int argc, char* argv[])
{
  if (argc <= 7) {
    usage();
    throw runtime_error("wrong number of arguments");
  }

  auto tscheduler = stoi(argv[1]);

  auto tdevices = stoi(argv[2]);

  auto tbench = stoi(argv[3]);

  auto check = stoi(argv[4]) == 1;

  auto size = stoi(argv[5]);

  auto chunksize = stoi(argv[6]);

  auto prop = stof(argv[7]);

  if (tbench == 0) {
    do_assign(tscheduler, tdevices, check, size, chunksize, prop);
  } else if (tbench == 1) {
    do_vecadd(tscheduler, tdevices, check, size, chunksize, prop);
    // thread t1(do_vecadd, tscheduler, tdevices, check, size, chunksize, prop);
    // thread t2(do_vecadd, tscheduler, tdevices, check, size, chunksize, prop);
    // t1.join();
    // t2.join();

  } else if (tbench == 2) {
    auto image_width = size;
    auto filter_width = atoi(argv[8]);

    do_gaussian(tscheduler, tdevices, check, image_width, chunksize, prop, filter_width);
  }
  return 0;
}
