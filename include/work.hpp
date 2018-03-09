/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of EngineCL which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef ENGINECL_WORK_HPP
#define ENGINECL_WORK_HPP 1

// #include <ostream>
#include <string>

using std::runtime_error;
using std::to_string;

class Work
{
public:
  Work() = default;
  Work(int _device_id, size_t _offset, size_t _size, float _bound)
    : device_id(_device_id)
    , offset(_offset)
    , size(_size)
    , bound(_bound)
  {
  }

  int device_id;
  size_t offset;
  size_t size;
  float bound;
};

inline size_t
splitWorkLikeHGuided(size_t total, size_t min_worksize, size_t lws, float comp_power)
{
  const auto K = 2;
  uint ret = total * comp_power / K;
  uint mult = ret / lws;
  uint rem = static_cast<size_t>(ret) % lws;
  if (rem) {
    mult++;
  }
  ret = mult * lws;
  if (ret < min_worksize) {
    ret = min_worksize;
  }
  if (total < ret) {
    ret = total;
  }
  if ((ret % lws) != 0) {
    throw runtime_error("ret % lws: " + to_string(ret) + " % " + to_string(lws));
  }
  return ret;
}

#endif // ENGINECL_WORK_HPP
