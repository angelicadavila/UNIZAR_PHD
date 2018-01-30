#ifndef CLBALANCER_BENCHS_VECADD_HPP
#define CLBALANCER_BENCHS_VECADD_HPP

#include "vecAllocator.hpp"
#include <memory>
#include <string>
#include <vector>

#include "utils/io.hpp"

#include "clbalancer.hpp"

// void do_vecadd(int tscheduler, int tdevices, bool check);

void
do_vecadd_base(bool check, int wsize);

void
do_vecadd(int tscheduler, int tdevices, bool check, int wsize, int chunksize, float prop);

#endif /* CLBALANCER_BENCHS_VECADD_HPP */
