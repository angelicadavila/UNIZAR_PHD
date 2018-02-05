/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of clbalancer which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef CLBALANCER_BUFFER_HPP
#define CLBALANCER_BUFFER_HPP 1

#include <memory>
#include <typeinfo>
#include <vector>

#include <CL/cl.h>

using std::shared_ptr;
using std::vector;

namespace clb {

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

  Direction direction();
  size_t size();
  size_t itemSize();
  size_t bytes();

  template<typename T>
  std::string type_name();

  template<typename T>
  void set(shared_ptr<vector<T>> in)
  {
    vector<T>* v = in.get();
    m_item_size = sizeof(T);
    m_size = v->size();
    m_bytes = sizeof(T) * m_size;
    m_data = reinterpret_cast<void*>(v->data());
    m_address = reinterpret_cast<void*>(v);
  }

  void* get();
  void* data();
  void* dataWithOffset(size_t offset);

  size_t byBytes(size_t size);

private:
  Direction m_direction;
  size_t m_item_size;
  size_t m_size;
  size_t m_bytes;
  void* m_data;
  void* m_address;
};

} // namespace clb

#endif /* CLBALANCER_BUFFER_HPP */
