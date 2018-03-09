#ifndef ENGINECL_BENCHS_NBODY_HPP
#define ENGINECL_BENCHS_NBODY_HPP

#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include <cmath>
#include <cstdlib>
#include <omp.h>

#include "EngineCL.hpp"
#include "utils/io.hpp"
#include "utils/rand.hpp"

#include "main.hpp"

using std::ostream;

// #define THRESHOLD 0.51

#define KERNEL_FLOPS 20

#define DEL_T 0.005f
#define ESP_SQR 500.0f

void
do_nbody_base(int tscheduler,
              int tdevices,
              uint check,
              uint particles,
              int chunksize,
              vector<float>& props);
void
do_nbody(int tscheduler,
         int tdevices,
         uint check,
         uint particles,
         int chunksize,
         vector<float>& props);

// here it comes the threshold when operating with floats and doing roundings
// template<int T>
// float
// round_to_decimal(float f)
// {
//   auto inc = pow(10, T);
//   return round((f * inc + 0.5) / inc);
// }

#endif /* ENGINECL_BENCHS_NBODY_HPP */
