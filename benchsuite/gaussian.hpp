#ifndef CLBALANCER_BENCHS_GAUSSIAN_HPP
#define CLBALANCER_BENCHS_GAUSSIAN_HPP

#include "clbalancer.hpp"

#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include <cmath>
#include <cstdlib>
#include <omp.h>

using std::ostream;

#define THRESHOLD 0.51

void
do_gaussian(int tscheduler, int tdevices, bool check, uint image_width, int chunksize, float prop, uint filter_width);
// void do_vecadd(int tscheduler, int tdevices, bool check, int wsize, int
// chunksize, float prop);

inline ostream&
operator<<(ostream& os, cl_uchar4& t)
{
  os << "(" << (int)t.s[0] << "," << (int)t.s[1] << "," << (int)t.s[2] << "," << (int)t.s[3] << ")";
  return os;
}

// here it comes the threshold when operating with floats and doing roundings
template<int T>
float
round_to_decimal(float f)
{
  auto inc = pow(10, T);
  return round((f * inc + 0.5) / inc);
}

class Gaussian
{
public:
  Gaussian(int width, int height, int filter_width)
    : _width(width)
    , _height(height)
    , _total_size(width * height)
    , _filter_width(filter_width)
    , _filter_total_size(filter_width * filter_width)
    , _a(_total_size)
    , _b(_filter_total_size)
    , _c(_total_size)
  {
    if (filter_width % 2 == 0) {
      throw runtime_error("filter_width should be odd (1, 3, etc)");
    }
    fill_image();
    fill_blurred(_c);
    fill_filter();
  }

  void fill_image();
  void fill_blurred(vector<cl_uchar4>& blurred);
  void fill_filter();
  void omp_gaussian_blur();
  bool compare_gaussian_blur(float threshold = THRESHOLD);
  bool compare_gaussian_blur_2loops(float threshold = THRESHOLD);
  string get_kernel_str();

  // private:
  int _width;
  int _height;
  int _total_size;
  int _filter_width;
  int _filter_total_size;
#pragma GCC diagnostic ignored "-Wignored-attributes"
  vector<cl_uchar4> _a; // image
  vector<cl_float> _b;  // filter
  vector<cl_uchar4> _c; // blurred
#pragma GCC diagnostic pop
  // shared_ptr<vector<cl_uchar4>> _a; // image
  // shared_ptr<vector<cl_float>> _b; // filter
  // shared_ptr<vector<cl_uchar4>> _c; // blurred
};

#endif /* CLBALANCER_BENCHS_GAUSSIAN_HPP */
