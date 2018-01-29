/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of clbalancer which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef CLBALANCER_BUFFER_HPP
#define CLBALANCER_BUFFER_HPP 1

#include <memory>
#include <vector>

#include <CL/cl.h>

using std::shared_ptr;
using std::vector;

namespace clb {

enum class Type
{
  Null = 0,
  Int = 1,
  Float = 2,
  ClUchar4 = 3,
  ClFloat = 4,
  // String = 2,
};

enum class Direction
{
  In = 0,
  Out = 1,
  InOut = 2
};

class Buffer
{
public:
  Buffer(Direction direction);

  Buffer(Buffer const&) = delete;
  Buffer& operator=(Buffer const&) = delete;

  Buffer(Buffer&&) = default;
  Buffer& operator=(Buffer&&) = default;

  Type type();
  Direction direction();
  size_t size();
  size_t itemSize();
  size_t bytes();
  void set(shared_ptr<vector<int>>& in);
  void set(shared_ptr<vector<cl_uchar4>>& in);
  void set(shared_ptr<vector<float>>& in);

  void* get();
  void* data();
  void* dataWithOffset(size_t offset);

  size_t byBytes(size_t size);

private:
  Direction m_direction;
  Type m_type;
  size_t m_item_size;
  size_t m_size;
  size_t m_bytes;
  void* m_data;
  void* m_address;
};

} // namespace clb

#endif /* CLBALANCER_BUFFER_HPP */
