#ifndef ENGINECL_BENCHS_MANDELBROT_HPP
#define ENGINECL_BENCHS_MANDELBROT_HPP

#include <memory>
#include <string>
#include <vector>

#include "utils/io.hpp"

#include "EngineCL.hpp"

#include <ostream>

#include "main.hpp"
// typedef cl_uchar4 Pixel;

// #define PIXEL_BIT_DEPTH 24
// #define BITMAP_HEADER_SIZE 14
// #define BITMAP_INFO_HEADER_SIZE 40

// typedef struct bmp_magic
// {
//   unsigned char magic[2];
// } bmp_magic_t;

// typedef struct
// {
//   uint32_t filesz;
//   uint16_t creator1;
//   uint16_t creator2;
//   uint32_t bmp_offset;
// } BMP_HEADER;

// typedef struct
// {
//   uint32_t header_sz;
//   int32_t width;
//   int32_t height;
//   uint16_t nplanes;
//   uint16_t bitspp;
//   uint32_t compress_type;
//   uint32_t bmp_bytesz;
//   int32_t hres;
//   int32_t vres;
//   uint32_t ncolors;
//   uint32_t nimpcolors;
// } BMP_INFO_HEADER;
// using std::ostream;

// inline ostream&
// operator<<(ostream& os, cl_uchar4& t)
// {
//   os << "(" << (int)t.s[0] << "," << (int)t.s[1] << "," << (int)t.s[2] << "," << (int)t.s[3] <<
//   ")"; return os;
// }
void
do_mandelbrot_base(int tscheduler,
                   int tdevices,
                   uint check,
                   int chunksize,
                   vector<float>& props,
                   int width,
                   int height,
                   double xpos,
                   double ypos,
                   double xstep,
                   double ystep,
                   uint max_iterations);

void
do_mandelbrot(int tscheduler,
              int tdevices,
              uint check,
              int chunksize,
              vector<float>& props,
              int width,
              int height,
              double xpos,
              double ypos,
              double xstep,
              double ystep,
              uint max_iterations);

#endif /* ENGINECL_BENCHS_MANDELBROT_HPP */
