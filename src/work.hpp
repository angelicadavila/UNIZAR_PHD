/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of clbalancer which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef CLBALANCER_WORK_HPP
#define CLBALANCER_WORK_HPP 1

class Work
{
public:
  Work() = default;
  Work(int _device_id, size_t _offset, size_t _size)
    : device_id(_device_id)
    , offset(_offset)
    , size(_size)
  {}

  int device_id;
  size_t offset;
  size_t size;
};

inline size_t
splitWorkLikeHGuided(size_t total, size_t min_worksize, size_t lws, float comp_power)
{
  const auto K = 2;
  auto ret = total * comp_power / K;
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
  return ret;
}




#endif // CLBALANCER_WORK_HPP
