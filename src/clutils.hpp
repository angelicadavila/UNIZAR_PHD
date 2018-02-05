/**
 * Copyright (c) 2017  Raúl Nozal <raul.nozal@unican.es>
 * This file is part of clbalancer which is released under MIT License.
 * See file LICENSE for full license details.
 */
#ifndef CLUTILS_HPP
#define CLUTILS_HPP 1

#include <CL/cl.hpp>
#include <iostream>
#include <sstream>
#include <string>

using std::cerr;
using std::cout;
using std::ostringstream;
using std::runtime_error;
using std::string;

#define VARGS_(_10, _9, _8, _7, _6, _5, _4, _3, _2, _1, N, ...) N
#define VARGS(...) VARGS_(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

#define CL_CHECK_ERROR_2(error, msg) CLUtils::checkErrorFunc((error), (msg), __FILE__, __LINE__)
#define CL_CHECK_ERROR_1(error) CL_CHECK_ERROR_2(error, "")

#define CL_CHECK_ERROR(...) CONCAT(CL_CHECK_ERROR_, VARGS(__VA_ARGS__))(__VA_ARGS__)

class CLUtils
{
public:
  static bool checkErrorFunc(cl_int error, string msg, const char* file, const int line);

  static string clErrorToString(cl_int error);

  static string clEnumToString(cl_int error);
};

namespace clb {
namespace OpenCL {
struct LocalSpaceArg
{
  ::size_t size_;
};

template<typename T>
struct KernelArgumentHandler
{
  static ::size_t size(const T&) { return sizeof(T); }
  static const T* ptr(const T& value) { return &value; }
};

template<>
struct KernelArgumentHandler<LocalSpaceArg>
{
  static ::size_t size(const LocalSpaceArg& value) { return value.size_; }
  static const void* ptr(const LocalSpaceArg&) { return NULL; }
};

} // namespace OpenCL
} // namespace clb

#endif // CLUTILS_HPP
