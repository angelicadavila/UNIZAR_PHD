#ifndef CLBALANCER_WORK_HPP
#define CLBALANCER_WORK_HPP 1

class Work {
 public:
  Work() = default;
  Work(int _device_id, size_t _offset, size_t _size) : device_id(_device_id), offset(_offset), size(_size) {}

  int device_id;
  size_t offset;
  size_t size;
};

inline size_t splitWorkLikeHGuided(size_t total, size_t min_worksize, size_t lws, float comp_power) {
  auto ret = total * comp_power / 2;  // computing power of device, 2 = K
  uint mult = ret / lws;
  uint rem = static_cast<size_t>(ret) % lws;
  if (rem) {
    mult++;
  }
  ret = mult * lws;
  if (ret < min_worksize) {
    ret = min_worksize;  // min_worksize should be multiple of lws
  }
  if (total < ret) {
    ret = total;
  }
  return ret;
  // return ret < lws ? lws : ret;
}

#endif  // CLBALANCER_WORK_HPP
