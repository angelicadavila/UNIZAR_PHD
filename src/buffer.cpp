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
{
}

Direction
Buffer::direction()
{
  return m_direction;
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

size_t
Buffer::byBytes(size_t size)
{
  return m_item_size * size;
}

} // namespace clb
