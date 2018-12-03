#ifndef CLBALANCER_BENCHS_MATRIXMULT_HPP
#define CLBALANCER_BENCHS_MATRIXMULT_HPP

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
do_matrixMult(int tscheduler,
            int tdevices,
            bool check,
            uint m_height,
            int chunksize,
            float prop,
            uint m_width
            );


// here it comes the threshold when operating with floats and doing roundings

class MatrixMult
{
public:
  MatrixMult(int _size)
    : _total_size(_size)
    ,_input_A(_total_size)
    ,_input_B(_total_size)
    ,_out(_total_size)
  {
    init_matrix();
  }
  void init_matrix();
  void verify_out(unsigned *frameData);
  string get_kernel_str();

  // private:
  int _total_size;
//#pragma GCC diagnostic ignored "-Wignored-attributes"
  vector<float,vecAllocator<float>> _input_A;  // input
  vector<float,vecAllocator<float>> _input_B;  // input
  vector<float,vecAllocator<float>> _out;  //output 
//#pragma GCC diagnostic pop
};

#endif /* CLBALANCER_BENCHS_MATRIXMULT_HPP */
