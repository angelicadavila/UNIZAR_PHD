#ifndef ENGINECL_BENCHS_MANDELBROT_HPP
#define ENGINECL_BENCHS_MANDELBROT_HPP

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
do_mandelbrot(int tscheduler,
            int tdevices,
            bool check,
            uint m_height,
            int chunksize,
            float prop,
            uint m_width
            );


// here it comes the threshold when operating with floats and doing roundings

class Mandelbrot
{
public:
  Mandelbrot(int _size)
    : _total_size(_size)
    ,_out(_total_size)
    ,_out_aux(_total_size)
  {
    init_matrix();
  }
  void init_matrix();
  void verify_out(unsigned *frameData);
  string get_kernel_str();

  // private:
  int _total_size;
#pragma GCC diagnostic ignored "-Wignored-attributes"
  vector<uint,vecAllocator<uint>> _out;  //output 
  vector<uint,vecAllocator<uint>> _out_aux;  //output 
#pragma GCC diagnostic pop
};

#endif /* ENGINECL_BENCHS_MANDELBROT_HPP */
