#ifndef CLBALANCER_BENCHS_SOBEL_HPP
#define CLBALANCER_BENCHS_SOBEL_HPP

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

#define THRESHOLD 0.51

void
do_sobel(int tscheduler,
            int tdevices,
            bool check,
            uint N_rand,
            int chunksize,
            float prop
            );


// here it comes the threshold when operating with floats and doing roundings

class Sobel
{
public:
  Sobel(int img_size)
    : _total_size(img_size)
    ,_input_img(_total_size)
    ,_out(_total_size)
    ,_out_aux(_total_size)
  {
    init_image();
  }
  void init_image();
  void outFrame(unsigned *frameData);
  string get_kernel_str();

  // private:
  int _total_size;
#pragma GCC diagnostic ignored "-Wignored-attributes"
  vector<int,vecAllocator<int>> _input_img;  // image
  vector<int,vecAllocator<int>> _out;  // blurred
  vector<int,vecAllocator<int>> _out_aux;  // blurred
#pragma GCC diagnostic pop
};

#endif /* CLBALANCER_BENCHS_SOBEL_HPP */
