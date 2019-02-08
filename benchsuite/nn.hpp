#ifndef ENGINECL_BENCHS_NEAREST_HPP
#define ENGINECL_BENCHS_NEAREST_HPP

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

using std::ostream;

void
do_nearest(int tscheduler,
            int tdevices,
            bool check,
            uint N_data,
            int chunksize,
            float prop,
            float prop2
            );

// here it comes the threshold when operating with floats and doing roundings
class Nearest
{
public:
  Nearest(int N_data)
    : _total_size(N_data)
    ,_input_lat(N_data*2)
    ,_distance(_total_size,0)
    ,_distance_aux(_total_size,0)
  {
    init_data(N_data*2);
  }
  void init_data(uint);
  string get_kernel_str();

  // private:
  int _total_size;

//#pragma GCC diagnostic ignored "-Wignored-attributes"
  vector<float,vecAllocator<float>> _input_lat;  
  vector<float,vecAllocator<float>> _distance;  
  vector<float,vecAllocator<float>> _distance_aux;  
//#pragma GCC diagnostic pop
};

#endif /* ENGINECL_BENCHS_NEAREST_HPP */
