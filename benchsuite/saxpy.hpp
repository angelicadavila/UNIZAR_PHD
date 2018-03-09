#ifndef ENGINECL_BENCHS_SAXPY_HPP
#define ENGINECL_BENCHS_SAXPY_HPP

#include <memory>
#include <string>
#include <vector>

#include "utils/io.hpp"

#include "EngineCL.hpp"

#include "main.hpp"

// void do_saxpy(int tscheduler, int tdevices, bool check);

void
do_saxpy_base(int tdevices, uint check, int wsize, float a);

void
do_saxpy(int tscheduler,
         int tdevices,
         uint check,
         int wsize,
         int chunksize,
         vector<float>& props,
         float a);

#endif /* ENGINECL_BENCHS_SAXPY_HPP */
