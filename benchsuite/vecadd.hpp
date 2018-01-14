#ifndef CLBALANCER_BENCHS_VECADD_HPP
#define CLBALANCER_BENCHS_VECADD_HPP

#include <memory>
#include <string>
#include <vector>

using namespace std;

#include "clbalancer.hpp"

// void do_vecadd(int tscheduler, int tdevices, bool check);
void do_vecadd(int tscheduler, int tdevices, bool check, int wsize, int chunksize, float prop);

#endif /* CLBALANCER_BENCHS_VECADD_HPP */
