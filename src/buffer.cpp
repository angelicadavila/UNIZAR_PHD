/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of clbalancer which is released under MIT License.
 * See file LICENSE for full license details.
 */
#include "buffer.hpp"

#include <memory>
#include <vector>

namespace clb {

Buffer::Buffer(Direction direction)
  : m_direction(direction)
  , m_type(Type::Null)
{}

Direction
Buffer::direction()
{
  return m_direction;
}

Type
Buffer::type()
{
  return m_type;
}

size_t
Buffer::size()
{
  return m_size;
}

size_t
Buffer::bytes()
{
  return m_bytes;
}

size_t
Buffer::itemSize()
{
  return m_item_size;
}

void*
Buffer::data()
{
  return m_data;
}

void*
Buffer::dataWithOffset(size_t offset)
{
  return reinterpret_cast<char*>(m_data) + byBytes(offset);
}

void*
Buffer::get()
{
  return m_address;
}

void
Buffer::set(shared_ptr<vector<int>>& in)
{
  vector<int>* v = in.get();
  m_item_size = sizeof(int);
  m_type = Type::Int;
  m_size = v->size();
  m_bytes = sizeof(int) * m_size;
  m_data = reinterpret_cast<void*>(v->data());
  m_address = reinterpret_cast<void*>(v);
}

void
Buffer::set(shared_ptr<vector<cl_uchar4>>& in)
{
  vector<cl_uchar4>* v = in.get();
  m_item_size = sizeof(cl_uchar4);
  m_type = Type::ClUchar4;
  m_size = v->size();
  m_bytes = sizeof(cl_uchar4) * m_size;
  m_data = reinterpret_cast<void*>(v->data());
  m_address = reinterpret_cast<void*>(v);
}

void
Buffer::set(shared_ptr<vector<float>>& in)
{
  vector<float>* v = in.get(); // NOTE ignoring attributes cl_float -> float
  m_item_size = sizeof(float);
  m_type = Type::Float;
  m_size = v->size();
  m_bytes = sizeof(float) * m_size;
  m_data = reinterpret_cast<void*>(v->data());
  m_address = reinterpret_cast<void*>(v);
}

size_t
Buffer::byBytes(size_t size)
{
  return m_item_size * size;
}

} // namespace clb
