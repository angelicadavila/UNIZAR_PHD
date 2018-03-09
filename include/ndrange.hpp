#ifndef ENGINECL_NDRANGE_HPP
#define ENGINECL_NDRANGE_HPP

#include <CL/cl.hpp>

namespace ecl {

class NDRange
{
private:
  size_t m_sizes[3];
  cl_uint m_dimensions;
  size_t m_space;

public:
  //! \brief Default constructor - resulting range has zero dimensions.
  NDRange()
    : m_dimensions(0)
  {
  }

  //! \brief Constructs one-dimensional range.
  NDRange(size_t size0)
    : m_dimensions(1)
  {
    m_sizes[0] = size0;
    m_space = size0;
  }

  //! \brief Constructs two-dimensional range.
  NDRange(size_t size0, size_t size1)
    : m_dimensions(2)
  {
    m_sizes[0] = size0;
    m_sizes[1] = size1;
    m_space = size0 * size1;
  }

  //! \brief Constructs three-dimensional range.
  NDRange(size_t size0, size_t size1, size_t size2)
    : m_dimensions(3)
  {
    m_sizes[0] = size0;
    m_sizes[1] = size1;
    m_sizes[2] = size2;
    m_space = size0 * size1 * size2;
  }

  /*! \brief Conversion operator to const size_t *.
   *
   *  \returns a pointer to the size of the first dimension.
   */
  operator const size_t*() const { return (const size_t*)m_sizes; }

  //! \brief Queries the number of dimensions in the range.
  cl_uint dimensions() const { return m_dimensions; }

  //! \brief Get the total space of the dimensions in the range.
  size_t space() const { return m_space; }

  operator cl::NDRange() const
  {
    switch (m_dimensions) {
      case 0:
        return cl::NDRange();
        break;
      case 1:
        return cl::NDRange(m_sizes[0]);
        break;
      case 2:
        return cl::NDRange(m_sizes[0], m_sizes[1]);
        break;
      default:
        return cl::NDRange(m_sizes[0], m_sizes[1], m_sizes[2]);
    }
  }
};
}

#endif /* ENGINECL_NDRANGE_HPP */
