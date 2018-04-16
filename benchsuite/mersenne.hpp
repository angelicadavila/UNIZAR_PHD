#ifndef CLBALANCER_BENCHS_MERSENNE_HPP
#define CLBALANCER_BENCHS_MERSENNE_HPP

#include "vecAllocator.hpp"
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include <omp.h>
#include <cmath>
#include <cstdlib>


#include "EngineCL.hpp"
#include "utils/io.hpp"

// #include "device.hpp"
// #include "runtime.hpp"
// #include "sched.hpp"

using std::ostream;

#define THRESHOLD 0.51

void
do_mersenne(int tscheduler,
            int tdevices,
            bool check,
            uint N_rand,
            int chunksize,
            float prop
            );


// here it comes the threshold when operating with floats and doing roundings

class Mersenne
{
public:
  Mersenne(int N_rand)
    : _total_size(N_rand)
    ,_input_seed(_total_size)
    ,_out(_total_size)
  {
    init_seed();
  }
  void init_seed();
  string get_kernel_str();

  // private:
  int _total_size;
#pragma GCC diagnostic ignored "-Wignored-attributes"
  vector<int,vecAllocator<int>> _input_seed;  // image
  vector<float,vecAllocator<float>> _out;  // blurred
#pragma GCC diagnostic pop
};

#endif /* CLBALANCER_BENCHS_MERSENNE_HPP */
