#ifndef ENGINECL_UTILS_IO_HPP
#define ENGINECL_UTILS_IO_HPP

#include <string>
#include <vector>

#include <fstream>
#include <iostream>
#include <sstream>

#include <CL/cl.h>

using std::cout;
using std::ifstream;
using std::ios;
using std::move;
using std::string;
using std::stringstream;
using std::vector;

#define TERM_BOLD "\033[1m"
#define TERM_GREEN "\033[32m"
#define TERM_RED "\033[31m"
#define TERM_RESET "\033[0m"

// BEGIN BMP writing

typedef cl_uchar4 Pixel;

#define PIXEL_BIT_DEPTH 24
#define BITMAP_HEADER_SIZE 14
#define BITMAP_INFO_HEADER_SIZE 40

typedef struct bmp_magic
{
  unsigned char magic[2];
} bmp_magic_t;

typedef struct
{
  uint32_t filesz;
  uint16_t creator1;
  uint16_t creator2;
  uint32_t bmp_offset;
} BMP_HEADER;

typedef struct
{
  uint32_t header_sz;
  int32_t width;
  int32_t height;
  uint16_t nplanes;
  uint16_t bitspp;
  uint32_t compress_type;
  uint32_t bmp_bytesz;
  int32_t hres;
  int32_t vres;
  uint32_t ncolors;
  uint32_t nimpcolors;
} BMP_INFO_HEADER;

int
write_bmp_file(Pixel* pixels, int width, int height, const char* filename);

// END BMP writing

vector<char>
file_read_binary(const string& path);
string
file_read(const string& path);

void
success(size_t time = 0, float relerr = -1.0f);
void
failure(size_t time = 0, float relerr = -1.0f);

#include <iterator>
#include <sstream>
#include <string>
#include <vector>

template<typename Out>
void
split(const std::string& s, char delim, Out result)
{
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    *(result++) = item;
  }
}

vector<float>
string_to_proportions(const string& str);

#endif /* ENGINECL_UTILS_IO_HPP */
