#ifndef ENGINECL_BENCHS_BINOMIAL_HPP
#define ENGINECL_BENCHS_BINOMIAL_HPP

#include <memory>
#include <string>
#include <vector>

#include "EngineCL.hpp"

#include "main.hpp"
#include "utils/io.hpp"
#include "utils/memory.hpp"

void
do_binomial_base(int tscheduler,
                 int tdevices,
                 uint check,
                 int samples,
                 int chunksize,
                 vector<float>& props);

void
do_binomial(int tscheduler,
            int tdevices,
            uint check,
            int samples,
            int chunksize,
            vector<float>& props);

#endif /* ENGINECL_BENCHS_BINOMIAL_HPP */
