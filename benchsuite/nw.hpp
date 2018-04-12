#ifndef CLBALANCER_BENCHS_NW_HPP
#define CLBALANCER_BENCHS_NW_HPP

#include "clbalancer.hpp"
#include "vecAllocator.hpp"
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include <omp.h>
#include <cmath>
#include <cstdlib>


#include "clbalancer.hpp"
#include "utils/io.hpp"

// #include "device.hpp"
// #include "runtime.hpp"
// #include "sched.hpp"

using std::ostream;


void
do_needleman(int tscheduler,
            int tdevices,
            bool check,
            uint sq_length,
            int chunksize,
            float prop
            );


// here it comes the threshold when operating with floats and doing roundings

class Needleman
{
public:
  Needleman(int vec_size)
    : _total_size(vec_size*vec_size)
    ,_max_cols(vec_size)
    ,_max_rows(vec_size)
    ,_input_itemsets(_total_size)
    ,_reference(_total_size)
    ,_output_itemsets(_total_size)
  {
    init_vectors();
  }
  void init_vectors();
  void out_vector();
  string get_kernel_str();

  // private:
  int _total_size, _max_cols, _max_rows;
#pragma GCC diagnostic ignored "-Wignored-attributes"
  vector<int,vecAllocator<int>> _input_itemsets;  
  vector<int,vecAllocator<int>> _reference;  
  vector<int,vecAllocator<int>> _output_itemsets;  
#pragma GCC diagnostic pop
};

#endif /* CLBALANCER_BENCHS_NW_HPP */
