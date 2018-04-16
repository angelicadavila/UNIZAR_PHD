#ifndef CLBALANCER_BENCHS_GSM_HPP
#define CLBALANCER_BENCHS_GSM_HPP

#include "EngineCL.hpp"
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

#define N 8
#define M 160
void
do_gsm(int tscheduler,
            int tdevices,
            bool check,
            uint in_size,
            int chunksize,
            float prop
            );


// here it comes the threshold when operating with floats and doing roundings

class Gsm
{
public:
  Gsm(int img_size)
    : _total_size(img_size*M)
    ,_input(_total_size+1)
    ,_out1(_total_size)
    ,_out2(_total_size)
  {
    init_voice();
  }
  void init_voice();
  void outVoice(unsigned *frameData);
  string get_kernel_str();

  // private:
  int _total_size;
#pragma GCC diagnostic ignored "-Wignored-attributes"
  vector<short,vecAllocator<short>> _input;  // voice
  vector<short,vecAllocator<short>> _out1;  // Code
  vector<short,vecAllocator<short>> _out2;  // LARc
#pragma GCC diagnostic pop
};

#endif /* CLBALANCER_BENCHS_GSM_HPP */
