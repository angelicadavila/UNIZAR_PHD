#include "mandelbrot.hpp"

using namespace std::chrono;

// ./build/release/EngineCL 0 2 51 1024 256 0.5 512 512

//
// bench=6; size=$(( 1 * 1024 )); its=1; for i in `seq 1 1 $its`; do /usr/bin/time
// ./build/release/EngineCL 0 2 $bench 1 $size 256 0.5 512 512; done

// dev=1; bench=61; size=$(( 8 * 1024 )); its=3; for i in `seq 1 1 $its`; do /usr/bin/time
// ./build/release/EngineCL 0 $dev $bench 1 $size 512 0.5 2>&1 | pcregrep -o1 '(\d+\.\d+)elapsed';
// done

#include <cmath>

using std::ostream;

inline ostream&
operator<<(ostream& os, cl_uchar4& t)
{
  os << "(" << (int)t.s[0] << "," << (int)t.s[1] << "," << (int)t.s[2] << "," << (int)t.s[3] << ")";
  return os;
}

union uchar4
{
  struct __uchar_four
  {
    unsigned char s0;
    unsigned char s1;
    unsigned char s2;
    unsigned char s3;
  } ch;
  cl_uint num;
};

struct int4;
struct float4
{
  float s0;
  float s1;
  float s2;
  float s3;

  float4 operator*(float4& fl)
  {
    float4 temp;
    temp.s0 = (this->s0) * fl.s0;
    temp.s1 = (this->s1) * fl.s1;
    temp.s2 = (this->s2) * fl.s2;
    temp.s3 = (this->s3) * fl.s3;
    return temp;
  }

  float4 operator*(float scalar)
  {
    float4 temp;
    temp.s0 = (this->s0) * scalar;
    temp.s1 = (this->s1) * scalar;
    temp.s2 = (this->s2) * scalar;
    temp.s3 = (this->s3) * scalar;
    return temp;
  }

  float4 operator+(float4& fl)
  {
    float4 temp;
    temp.s0 = (this->s0) + fl.s0;
    temp.s1 = (this->s1) + fl.s1;
    temp.s2 = (this->s2) + fl.s2;
    temp.s3 = (this->s3) + fl.s3;
    return temp;
  }

  float4 operator-(float4 fl)
  {
    float4 temp;
    temp.s0 = (this->s0) - fl.s0;
    temp.s1 = (this->s1) - fl.s1;
    temp.s2 = (this->s2) - fl.s2;
    temp.s3 = (this->s3) - fl.s3;
    return temp;
  }

  friend float4 operator*(float scalar, float4& fl);
  friend float4 convert_float4(int4 i);
};

float4 operator*(float scalar, float4& fl)
{
  float4 temp;
  temp.s0 = fl.s0 * scalar;
  temp.s1 = fl.s1 * scalar;
  temp.s2 = fl.s2 * scalar;
  temp.s3 = fl.s3 * scalar;
  return temp;
}

struct double4
{
  double s0;
  double s1;
  double s2;
  double s3;

  double4 operator*(double4& fl)
  {
    double4 temp;
    temp.s0 = (this->s0) * fl.s0;
    temp.s1 = (this->s1) * fl.s1;
    temp.s2 = (this->s2) * fl.s2;
    temp.s3 = (this->s3) * fl.s3;
    return temp;
  }

  double4 operator*(double scalar)
  {
    double4 temp;
    temp.s0 = (this->s0) * scalar;
    temp.s1 = (this->s1) * scalar;
    temp.s2 = (this->s2) * scalar;
    temp.s3 = (this->s3) * scalar;
    return temp;
  }

  double4 operator+(double4& fl)
  {
    double4 temp;
    temp.s0 = (this->s0) + fl.s0;
    temp.s1 = (this->s1) + fl.s1;
    temp.s2 = (this->s2) + fl.s2;
    temp.s3 = (this->s3) + fl.s3;
    return temp;
  }

  double4 operator-(double4 fl)
  {
    double4 temp;
    temp.s0 = (this->s0) - fl.s0;
    temp.s1 = (this->s1) - fl.s1;
    temp.s2 = (this->s2) - fl.s2;
    temp.s3 = (this->s3) - fl.s3;
    return temp;
  }

  friend double4 operator*(double scalar, double4& fl);
  friend double4 convert_double4(int4 i);
};

double4 operator*(double scalar, double4& fl)
{
  double4 temp;
  temp.s0 = fl.s0 * scalar;
  temp.s1 = fl.s1 * scalar;
  temp.s2 = fl.s2 * scalar;
  temp.s3 = fl.s3 * scalar;
  return temp;
}

struct int4
{
  int s0;
  int s1;
  int s2;
  int s3;

  int4 operator*(int4& fl)
  {
    int4 temp;
    temp.s0 = (this->s0) * fl.s0;
    temp.s1 = (this->s1) * fl.s1;
    temp.s2 = (this->s2) * fl.s2;
    temp.s3 = (this->s3) * fl.s3;
    return temp;
  }

  int4 operator*(int scalar)
  {
    int4 temp;
    temp.s0 = (this->s0) * scalar;
    temp.s1 = (this->s1) * scalar;
    temp.s2 = (this->s2) * scalar;
    temp.s3 = (this->s3) * scalar;
    return temp;
  }

  int4 operator+(int4& fl)
  {
    int4 temp;
    temp.s0 = (this->s0) + fl.s0;
    temp.s1 = (this->s1) + fl.s1;
    temp.s2 = (this->s2) + fl.s2;
    temp.s3 = (this->s3) + fl.s3;
    return temp;
  }

  int4 operator-(int4 fl)
  {
    int4 temp;
    temp.s0 = (this->s0) - fl.s0;
    temp.s1 = (this->s1) - fl.s1;
    temp.s2 = (this->s2) - fl.s2;
    temp.s3 = (this->s3) - fl.s3;
    return temp;
  }

  int4 operator+=(int4 fl)
  {
    s0 += fl.s0;
    s1 += fl.s1;
    s2 += fl.s2;
    s3 += fl.s3;
    return (*this);
  }

  friend float4 convert_float4(int4 i);
  friend double4 convert_double4(int4 i);
};

float4
convert_float4(int4 i)
{
  float4 temp;
  temp.s0 = (float)i.s0;
  temp.s1 = (float)i.s1;
  temp.s2 = (float)i.s2;
  temp.s3 = (float)i.s3;
  return temp;
}

double4
convert_double4(int4 i)
{
  double4 temp;
  temp.s0 = (double)i.s0;
  temp.s1 = (double)i.s1;
  temp.s2 = (double)i.s2;
  temp.s3 = (double)i.s3;
  return temp;
}

inline float
native_log2(float in)
{
  return log(in) / log(2.0f);
}

inline float
native_cos(float in)
{
  return cos(in);
}

inline double
native_log2(double in)
{
  return log(in) / log(2.0f);
}

inline double
native_cos(double in)
{
  return cos(in);
}

#ifndef min
int
min(int a1, int a2)
{
  return ((a1 < a2) ? a1 : a2);
}
#endif

void
mandelbrotRefFloat(cl_uint* verificationOutput,
                   cl_float posx,
                   cl_float posy,
                   cl_float stepSizeX,
                   cl_float stepSizeY,
                   cl_int maxIterations,
                   cl_int width,
                   cl_int height,
                   cl_int bench)
{
  int tid;

  for (tid = 0; tid < (height * width / 4); tid++) {
    int i = tid % (width / 4);
    int j = tid / (width / 4);

    int4 veci = { 4 * i, 4 * i + 1, 4 * i + 2, 4 * i + 3 };
    int4 vecj = { j, j, j, j };
    float4 x0;
    x0.s0 = (float)(posx + stepSizeX * (float)veci.s0);
    x0.s1 = (float)(posx + stepSizeX * (float)veci.s1);
    x0.s2 = (float)(posx + stepSizeX * (float)veci.s2);
    x0.s3 = (float)(posx + stepSizeX * (float)veci.s3);
    float4 y0;
    y0.s0 = (float)(posy + stepSizeY * (float)vecj.s0);
    y0.s1 = (float)(posy + stepSizeY * (float)vecj.s1);
    y0.s2 = (float)(posy + stepSizeY * (float)vecj.s2);
    y0.s3 = (float)(posy + stepSizeY * (float)vecj.s3);

    float4 x = x0;
    float4 y = y0;

    cl_int iter = 0;
    float4 tmp;
    int4 stay;
    int4 ccount = { 0, 0, 0, 0 };

    stay.s0 = (x.s0 * x.s0 + y.s0 * y.s0) <= 4.0f;
    stay.s1 = (x.s1 * x.s1 + y.s1 * y.s1) <= 4.0f;
    stay.s2 = (x.s2 * x.s2 + y.s2 * y.s2) <= 4.0f;
    stay.s3 = (x.s3 * x.s3 + y.s3 * y.s3) <= 4.0f;
    float4 savx = x;
    float4 savy = y;

    for (iter = 0; (stay.s0 | stay.s1 | stay.s2 | stay.s3) && (iter < maxIterations); iter += 16) {
      x = savx;
      y = savy;

      // Two iterations
      tmp = x * x + x0 - y * y;
      y = 2.0f * x * y + y0;
      x = tmp * tmp + x0 - y * y;
      y = 2.0f * tmp * y + y0;

      // Two iterations
      tmp = x * x + x0 - y * y;
      y = 2.0f * x * y + y0;
      x = tmp * tmp + x0 - y * y;
      y = 2.0f * tmp * y + y0;

      // Two iterations
      tmp = x * x + x0 - y * y;
      y = 2.0f * x * y + y0;
      x = tmp * tmp + x0 - y * y;
      y = 2.0f * tmp * y + y0;

      // Two iterations
      tmp = x * x + x0 - y * y;
      y = 2.0f * x * y + y0;
      x = tmp * tmp + x0 - y * y;
      y = 2.0f * tmp * y + y0;

      // Two iterations
      tmp = x * x + x0 - y * y;
      y = 2.0f * x * y + y0;
      x = tmp * tmp + x0 - y * y;
      y = 2.0f * tmp * y + y0;

      // Two iterations
      tmp = x * x + x0 - y * y;
      y = 2.0f * x * y + y0;
      x = tmp * tmp + x0 - y * y;
      y = 2.0f * tmp * y + y0;

      // Two iterations
      tmp = x * x + x0 - y * y;
      y = 2.0f * x * y + y0;
      x = tmp * tmp + x0 - y * y;
      y = 2.0f * tmp * y + y0;

      // Two iterations
      tmp = x * x + x0 - y * y;
      y = 2.0f * x * y + y0;
      x = tmp * tmp + x0 - y * y;
      y = 2.0f * tmp * y + y0;

      stay.s0 = (x.s0 * x.s0 + y.s0 * y.s0) <= 4.0f;
      stay.s1 = (x.s1 * x.s1 + y.s1 * y.s1) <= 4.0f;
      stay.s2 = (x.s2 * x.s2 + y.s2 * y.s2) <= 4.0f;
      stay.s3 = (x.s3 * x.s3 + y.s3 * y.s3) <= 4.0f;

      savx.s0 = (stay.s0 ? x.s0 : savx.s0);
      savx.s1 = (stay.s1 ? x.s1 : savx.s1);
      savx.s2 = (stay.s2 ? x.s2 : savx.s2);
      savx.s3 = (stay.s3 ? x.s3 : savx.s3);
      savy.s0 = (stay.s0 ? y.s0 : savy.s0);
      savy.s1 = (stay.s1 ? y.s1 : savy.s1);
      savy.s2 = (stay.s2 ? y.s2 : savy.s2);
      savy.s3 = (stay.s3 ? y.s3 : savy.s3);
      ccount += stay * 16;
    }
    // Handle remainder
    if (!(stay.s0 & stay.s1 & stay.s2 & stay.s3)) {
      iter = 16;
      do {
        x = savx;
        y = savy;
        stay.s0 = ((x.s0 * x.s0 + y.s0 * y.s0) <= 4.0f) && (ccount.s0 < maxIterations);
        stay.s1 = ((x.s1 * x.s1 + y.s1 * y.s1) <= 4.0f) && (ccount.s1 < maxIterations);
        stay.s2 = ((x.s2 * x.s2 + y.s2 * y.s2) <= 4.0f) && (ccount.s2 < maxIterations);
        stay.s3 = ((x.s3 * x.s3 + y.s3 * y.s3) <= 4.0f) && (ccount.s3 < maxIterations);
        tmp = x;
        x = x * x + x0 - y * y;
        y = 2.0f * tmp * y + y0;
        ccount += stay;
        iter--;
        savx.s0 = (stay.s0 ? x.s0 : savx.s0);
        savx.s1 = (stay.s1 ? x.s1 : savx.s1);
        savx.s2 = (stay.s2 ? x.s2 : savx.s2);
        savx.s3 = (stay.s3 ? x.s3 : savx.s3);
        savy.s0 = (stay.s0 ? y.s0 : savy.s0);
        savy.s1 = (stay.s1 ? y.s1 : savy.s1);
        savy.s2 = (stay.s2 ? y.s2 : savy.s2);
        savy.s3 = (stay.s3 ? y.s3 : savy.s3);
      } while ((stay.s0 | stay.s1 | stay.s2 | stay.s3) && iter);
    }
    x = savx;
    y = savy;
    float4 fc = convert_float4(ccount);

    fc.s0 = (float)ccount.s0 + 1 - native_log2(native_log2(x.s0 * x.s0 + y.s0 * y.s0));
    fc.s1 = (float)ccount.s1 + 1 - native_log2(native_log2(x.s1 * x.s1 + y.s1 * y.s1));
    fc.s2 = (float)ccount.s2 + 1 - native_log2(native_log2(x.s2 * x.s2 + y.s2 * y.s2));
    fc.s3 = (float)ccount.s3 + 1 - native_log2(native_log2(x.s3 * x.s3 + y.s3 * y.s3));

    float c = fc.s0 * 2.0f * 3.1416f / 256.0f;
    uchar4 color[4];
    color[0].ch.s0 = (unsigned char)(((1.0f + native_cos(c)) * 0.5f) * 255);
    color[0].ch.s1 =
      (unsigned char)(((1.0f + native_cos(2.0f * c + 2.0f * 3.1416f / 3.0f)) * 0.5f) * 255);
    color[0].ch.s2 = (unsigned char)(((1.0f + native_cos(c - 2.0f * 3.1416f / 3.0f)) * 0.5f) * 255);
    color[0].ch.s3 = 0xff;
    if (ccount.s0 == maxIterations) {
      color[0].ch.s0 = 0;
      color[0].ch.s1 = 0;
      color[0].ch.s2 = 0;
    }
    if (bench) {
      color[0].ch.s0 = ccount.s0 & 0xff;
      color[0].ch.s1 = (ccount.s0 & 0xff00) >> 8;
      color[0].ch.s2 = (ccount.s0 & 0xff0000) >> 16;
      color[0].ch.s3 = (ccount.s0 & 0xff000000) >> 24;
    }
    verificationOutput[4 * tid] = color[0].num;

    c = fc.s1 * 2.0f * 3.1416f / 256.0f;
    color[1].ch.s0 = (unsigned char)(((1.0f + native_cos(c)) * 0.5f) * 255);
    color[1].ch.s1 =
      (unsigned char)(((1.0f + native_cos(2.0f * c + 2.0f * 3.1416f / 3.0f)) * 0.5f) * 255);
    color[1].ch.s2 = (unsigned char)(((1.0f + native_cos(c - 2.0f * 3.1416f / 3.0f)) * 0.5f) * 255);
    color[1].ch.s3 = 0xff;
    if (ccount.s1 == maxIterations) {
      color[1].ch.s0 = 0;
      color[1].ch.s1 = 0;
      color[1].ch.s2 = 0;
    }
    if (bench) {
      color[1].ch.s0 = ccount.s1 & 0xff;
      color[1].ch.s1 = (ccount.s1 & 0xff00) >> 8;
      color[1].ch.s2 = (ccount.s1 & 0xff0000) >> 16;
      color[1].ch.s3 = (ccount.s1 & 0xff000000) >> 24;
    }
    verificationOutput[4 * tid + 1] = color[1].num;

    c = fc.s2 * 2.0f * 3.1416f / 256.0f;
    color[2].ch.s0 = (unsigned char)(((1.0f + native_cos(c)) * 0.5f) * 255);
    color[2].ch.s1 =
      (unsigned char)(((1.0f + native_cos(2.0f * c + 2.0f * 3.1416f / 3.0f)) * 0.5f) * 255);
    color[2].ch.s2 = (unsigned char)(((1.0f + native_cos(c - 2.0f * 3.1416f / 3.0f)) * 0.5f) * 255);
    color[2].ch.s3 = 0xff;
    if (ccount.s2 == maxIterations) {
      color[2].ch.s0 = 0;
      color[2].ch.s1 = 0;
      color[2].ch.s2 = 0;
    }
    if (bench) {
      color[2].ch.s0 = ccount.s2 & 0xff;
      color[2].ch.s1 = (ccount.s2 & 0xff00) >> 8;
      color[2].ch.s2 = (ccount.s2 & 0xff0000) >> 16;
      color[2].ch.s3 = (ccount.s2 & 0xff000000) >> 24;
    }
    verificationOutput[4 * tid + 2] = color[2].num;

    c = fc.s3 * 2.0f * 3.1416f / 256.0f;
    color[3].ch.s0 = (unsigned char)(((1.0f + native_cos(c)) * 0.5f) * 255);
    color[3].ch.s1 =
      (unsigned char)(((1.0f + native_cos(2.0f * c + 2.0f * 3.1416f / 3.0f)) * 0.5f) * 255);
    color[3].ch.s2 = (unsigned char)(((1.0f + native_cos(c - 2.0f * 3.1416f / 3.0f)) * 0.5f) * 255);
    color[3].ch.s3 = 0xff;
    if (ccount.s3 == maxIterations) {
      color[3].ch.s0 = 0;
      color[3].ch.s1 = 0;
      color[3].ch.s2 = 0;
    }
    if (bench) {
      color[3].ch.s0 = ccount.s3 & 0xff;
      color[3].ch.s1 = (ccount.s3 & 0xff00) >> 8;
      color[3].ch.s2 = (ccount.s3 & 0xff0000) >> 16;
      color[3].ch.s3 = (ccount.s3 & 0xff000000) >> 24;
    }
    verificationOutput[4 * tid + 3] = color[3].num;
  }
}

auto
check_mandelbrot(cl_uchar4* out,
                 float leftxF,
                 float topyF,
                 float xstepF,
                 float ystepF,
                 uint max_iterations,
                 int width,
                 int height,
                 int bench,
                 float threshold)
{

  int max = 10;
  std::cout << "out:\n";
  for (uint i = 0; i < max; ++i) {
    std::cout << out[i] << " ";
  }
  std::cout << "\n";

  auto size_bytes = width * height * sizeof(cl_uchar4);
  uint* res = (uint*)malloc(size_bytes);

  uint* out_ptr = reinterpret_cast<uint*>(out);
  mandelbrotRefFloat(res, leftxF, topyF, xstepF, ystepF, max_iterations, width, height, bench);

  cl_uchar4* res_ptr = reinterpret_cast<cl_uchar4*>(res);

  std::cout << "res:\n";
  for (uint i = 0; i < max; ++i) {
    std::cout << res_ptr[i] << " ";
  }
  std::cout << "\n";

  int i, j;
  int counter = 0;

  auto pos = -1;

  for (j = 0; j < height; j++) {
    for (i = 0; i < width; i++) {
      uchar4 temp_ver, temp_out;
      temp_ver.num = res[j * width + i];
      temp_out.num = out_ptr[j * width + i];

      unsigned char threshold = 2;

      if (((temp_ver.ch.s0 - temp_out.ch.s0) > threshold) ||
          ((temp_out.ch.s0 - temp_ver.ch.s0) > threshold) ||

          ((temp_ver.ch.s1 - temp_out.ch.s1) > threshold) ||
          ((temp_out.ch.s1 - temp_ver.ch.s1) > threshold) ||

          ((temp_ver.ch.s2 - temp_out.ch.s2) > threshold) ||
          ((temp_out.ch.s2 - temp_ver.ch.s2) > threshold) ||

          ((temp_ver.ch.s3 - temp_out.ch.s3) > threshold) ||
          ((temp_out.ch.s3 - temp_ver.ch.s3) > threshold)) {
        if (pos == -1) { // capture the first error
          pos = i + j * width;
        }
        counter++;
      }
    }
  }

  int numPixels = height * width;
  double ratio = (double)counter / numPixels;

  // if( ratio > threshold){
  //   pos =
  // }
  return pos;
}

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
                   uint max_iterations)
{
  bool use_binaries = (check >= 10) ? true : false;
  check = (check >= 10) ? check - 10 : check;

  string kernel_str;
  try {
    kernel_str = file_read("support/kernels/mandelbrot.cl");
  } catch (std::ios::failure& e) {
    cout << "io failure: " << e.what() << "\n";
  }

  // Make sure width is a multiple of 4
  width = (width + 3) & ~(4 - 1);

  // cout << size << "\n";
  IF_LOGGING(cout << width << " h " << height << "\n");

  int size_matrix = width * height;
  auto size = size_matrix;

  auto lws = 256;
  auto gws = size >> 2;

  int worksize = chunksize;

  auto numDevices = 1;
  auto bench = 0;
  auto xsize = 4.0;

  auto out_array = make_shared<vector<cl_uchar4>>(size_matrix);
  cl_uchar4* out_ptr = reinterpret_cast<cl_uchar4*>(out_array.get()->data());

  out_ptr[0].s[0] = 22;
  out_ptr[0].s[2] = 255;

  out_ptr[size_matrix - 2].s[0] = 22;
  out_ptr[size_matrix - 2].s[2] = 33;

  double aspect = (double)width / (double)height;
  xstep = (xsize / (double)width);
  // Adjust for aspect ratio
  double ysize = xsize / aspect;
  ystep = (-(xsize / aspect) / height);
  auto leftx = (xpos - xsize / 2.0);
  // for (int i=0; i<numDevices; i++){
  auto idx = 0;
  auto topy = (ypos + ysize / 2.0 - ((double)idx * ysize) / (double)numDevices);
  // }

  // if (i == 0) {
  //   topy0 = topy;
  // }

  float leftxF = (float)leftx;
  float topyF = (float)topy;
  float xstepF = (float)xstep;
  float ystepF = (float)ystep;

  // cout << width << " " << size_matrix << " " << size << " " << worksize << " " << bench << " "
  //      << max_iterations << " " << xsize << " " << aspect << " " << ysize << " " << leftx << " "
  //      << topy << gws << " " << lws << " " << leftxF << " " << topyF << " " << xstepF << " "
  //      << ystepF << "\n";

  auto sel_platform = PLATFORM;
  auto sel_device = tdevices == 0 ? 1 : 0; // invert, tdevices: 0 = CPU, 1 = GPU

  vector<char> kernel_bin;
  if (use_binaries) {
    switch (tdevices) {
      case 200:
        kernel_bin = file_read_binary("support/kernels/mandelbrot_sapu:0:1.cl.bin");
        break;
      case 201:
        kernel_bin = file_read_binary("support/kernels/mandelbrot_sapu:0:0.cl.bin");
        break;
      case 300:
        kernel_bin = file_read_binary("support/kernels/mandelbrot_batel:1:0.cl.bin");
        break;
      case 301:
        kernel_bin = file_read_binary("support/kernels/mandelbrot_batel:1:1.cl.bin");
        break;
      case 310:
        kernel_bin = file_read_binary("support/kernels/mandelbrot_batel:0:0.cl.bin");
        break;
    }
  }

  auto time_init = std::chrono::system_clock::now().time_since_epoch();

  switch (tdevices) {
    case 200:
      sel_platform = 0;
      sel_device = 1;
      break;
    case 201:
      sel_platform = 0;
      sel_device = 0;
      break;
    case 300:
      sel_platform = 1;
      sel_device = 0;
      break;
    case 301:
      sel_platform = 1;
      sel_device = 1;
      break;
    case 310:
      sel_platform = 0;
      sel_device = 0;
      break;
  }

  vector<cl::Platform> platforms;
  vector<vector<cl::Device>> platform_devices;
  cl::Device device;

  IF_LOGGING(cout << "discoverDevices\n");
  cl::Platform::get(&platforms);
  IF_LOGGING(cout << "platforms: " << platforms.size() << "\n");
  auto i = 0;
  for (auto& platform : platforms) {
    vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    IF_LOGGING(cout << "platform: " << i++ << " devices: " << devices.size() << "\n");
    platform_devices.push_back(move(devices));
  }

  auto last_platform = platforms.size() - 1;
  if (sel_platform > last_platform) {
    throw runtime_error("invalid platform selected");
  }

  auto last_device = platform_devices[sel_platform].size() - 1;
  if (sel_device > last_device) {
    throw runtime_error("invalid device selected");
  }

  device = move(platform_devices[sel_platform][sel_device]);

  cl_int cl_err = CL_SUCCESS;
  cl::Context context(device);

  cl::CommandQueue queue(context, device, 0, &cl_err);
  CL_CHECK_ERROR(cl_err, "CommandQueue queue");

  IF_LOGGING(cout << "initBuffers\n");

  cl_int buffer_in_flags = CL_MEM_READ_WRITE;
  cl_int buffer_out_flags = CL_MEM_READ_WRITE;

  IF_LOGGING(cout << out_array.get()->size() << "\n");

  cl::Buffer out_buffer(
    context, buffer_out_flags, sizeof(cl_uchar4) * out_array.get()->size(), NULL);
  CL_CHECK_ERROR(cl_err, "out buffer ");

  IF_LOGGING(cout << "initKernel\n");

  cl::Program::Sources sources;
  cl::Program::Binaries binaries;
  cl::Program program;
  if (use_binaries) {
    cout << "using binary\n";
    binaries.push_back({ kernel_bin.data(), kernel_bin.size() });
    vector<cl_int> status = { -1 };
    program = std::move(cl::Program(context, { device }, binaries, &status, &cl_err));
    CL_CHECK_ERROR(cl_err, "building program from binary failed for device ");
  } else {
    sources.push_back({ kernel_str.c_str(), kernel_str.length() });
    program = std::move(cl::Program(context, sources));
  }

  string options;
  options.reserve(32);
  options += "-DECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED=" +
             to_string(ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED);

  cl_err = program.build({ device }, options.c_str());
  if (cl_err != CL_SUCCESS) {
    IF_LOGGING(cout << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device)
                    << "\n");
    CL_CHECK_ERROR(cl_err);
  }

  cl::Kernel kernel(program, "mandelbrot_vector_float", &cl_err);

  cl_err = kernel.setArg(0, out_buffer);
  CL_CHECK_ERROR(cl_err, "kernel arg 0 in1 buffer");

  cl_err = kernel.setArg(1, leftxF);
  CL_CHECK_ERROR(cl_err, "kernel arg 1 in2 buffer");

  cl_err = kernel.setArg(2, topyF);
  CL_CHECK_ERROR(cl_err, "kernel arg 2 out buffer");

  cl_err = kernel.setArg(3, xstepF);
  CL_CHECK_ERROR(cl_err, "kernel arg 3 size");

  cl_err = kernel.setArg(4, ystepF);
  CL_CHECK_ERROR(cl_err, "kernel arg 4 in2 buffer");

  cl_err = kernel.setArg(5, max_iterations);
  CL_CHECK_ERROR(cl_err, "kernel arg 5 size");

  cl_err = kernel.setArg(6, width);
  CL_CHECK_ERROR(cl_err, "kernel arg 6 size");

  cl_err = kernel.setArg(7, bench);
  CL_CHECK_ERROR(cl_err, "kernel arg 7 size");
  // cl_int cl_err;
  // cl::UserEvent end(m_context, &cl_err);
  // CL_CHECK_ERROR(cl_err, "user event end");

  // cl::Event evkernel;

  auto offset = 0;
  queue.enqueueNDRangeKernel(
    kernel, cl::NDRange(offset), cl::NDRange(gws), cl::NDRange(lws), NULL, NULL);

  // cl::Event evread;
  // vector<cl::Event> events({ evkernel });

  queue.enqueueReadBuffer(
    out_buffer, CL_TRUE, 0, sizeof(cl_uchar4) * out_array.get()->size(), out_array.get()->data());

  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - time_init).count();

  cout << "time: " << diff_ms << "\n";

  // auto in1 = *a_array.get();
  // auto in2 = *b_array.get();
  auto out = *out_array.get();

  if (ECL_LOGGING) {
    cout << "out:\n";
    for (uint i = 0; i < 10; ++i) {
      cout << out_ptr[i] << " ";
    }
    cout << "\n";
    for (uint i = size_matrix - 10; i < size_matrix; ++i) {
      cout << out_ptr[i] << " ";
    }
    cout << "\n";
  }

  auto image_width = width;
  auto image_height = height;
  // IF_LOGGING(cout << img << "\n");
  // img = write_bmp_file(out.data(), image_width, image_height, "gf_base.bmp");
  // IF_LOGGING(cout << img << "\n");
  if (check) {
    auto threshold = 0.001f;

    auto ok = check_mandelbrot(
      out_ptr, leftxF, topyF, xstepF, ystepF, max_iterations, width, height, bench, threshold);

    // auto time = 0;
    if (ok) {
      // cout << "\033[1;32m"
      //      << "Success"
      //      << "\033[0m"
      //      << "\n";
      success(diff_ms);
      // cout << TERM_GREEN << "Success (" << time << ")\n" << TERM_RESET;
    } else {
      // cout << TERM_RED << "Failure (" << time << ")\n" << TERM_RESET;
      failure(diff_ms);
    }
    if (check == 2) {
      auto img = write_bmp_file(out.data(), width, height, "mandelbrot_base.bmp");
      cout << "writing mandelbrot_base.bmp (" << img << ")\n";
    }
  } else {
    cout << "Done\n";
  }
  exit(0);
}

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
              uint max_iterations)
{
  bool use_binaries = (check >= 10) ? true : false;
  check = (check >= 10) ? check - 10 : check;

  string kernel;
  try {
    kernel = file_read("support/kernels/mandelbrot.cl");
  } catch (std::ios::failure& e) {
    cout << "io failure: " << e.what() << "\n";
  }

  // Make sure width is a multiple of 4
  width = (width + 3) & ~(4 - 1);

  // cout << width << " h " << height << "\n";

  int size_matrix = width * height;
  auto size = size_matrix;

  auto lws = 256;
  auto gws = size_matrix >> 2;
  // int size_dimpoints = points * points;

  int worksize = chunksize;

  auto numDevices = 1;
  auto bench = 0;
  auto xsize = 4.0;

  // typedef std::vector<cl_float4, aligned_allocator<cl_float4, sizeof(cl_float4)>>
  // aligned_vector_cl_float4;

  // auto in1_array = make_shared<aligned_vector_cl_float4>(samplesPerVectorWidth);
  auto out_array = make_shared<vector<cl_uchar4>>(size_matrix);
  cl_uchar4* out_ptr = reinterpret_cast<cl_uchar4*>(out_array.get()->data());

  out_ptr[0].s[0] = 22;
  out_ptr[0].s[2] = 255;

  out_ptr[size_matrix - 2].s[0] = 22;
  out_ptr[size_matrix - 2].s[2] = 33;

  double aspect = (double)width / (double)height;
  xstep = (xsize / (double)width);
  // Adjust for aspect ratio
  double ysize = xsize / aspect;
  ystep = (-(xsize / aspect) / height);
  auto leftx = (xpos - xsize / 2.0);
  // for (int i=0; i<numDevices; i++){
  auto i = 0;
  auto topy = (ypos + ysize / 2.0 - ((double)i * ysize) / (double)numDevices);
  // }

  // if (i == 0) {
  //   topy0 = topy;
  // }

  float leftxF = (float)leftx;
  float topyF = (float)topy;
  float xstepF = (float)xstep;
  float ystepF = (float)ystep;

  // cout << width << " " << size_matrix << " " << size << " " << worksize << " " << bench << " "
  //      << max_iterations << " " << xsize << " " << aspect << " " << ysize << " " << leftx << " "
  //      << topy << gws << " " << lws << " " << leftxF << " " << topyF << " " << xstepF << " "
  //      << ystepF << "\n";
  auto platform = PLATFORM;

  vector<char> kernel_bin;
  if (use_binaries) {
    switch (tdevices) {
      case 200:
        kernel_bin = file_read_binary("support/kernels/mandelbrot_sapu:0:1.cl.bin");
        break;
      case 201:
        kernel_bin = file_read_binary("support/kernels/mandelbrot_sapu:0:0.cl.bin");
        break;
      case 300:
        kernel_bin = file_read_binary("support/kernels/mandelbrot_batel:1:0.cl.bin");
        break;
      case 301:
        kernel_bin = file_read_binary("support/kernels/mandelbrot_batel:1:1.cl.bin");
        break;
      case 310:
        kernel_bin = file_read_binary("support/kernels/mandelbrot_batel:0:0.cl.bin");
        break;
    }
  }

  auto time_init = std::chrono::system_clock::now().time_since_epoch();

  vector<ecl::Device> devices;
  switch (tdevices) {
    case 200: {
      ecl::Device cpu(0, 1);
      if (use_binaries) {
        cpu.setKernel(kernel_bin);
      }
      devices.push_back(move(cpu));
    } break;
    case 201: {
      ecl::Device igpu(0, 0);
      if (use_binaries) {
        igpu.setKernel(kernel_bin);
      }
      devices.push_back(move(igpu));
    } break;
    case 300: {
      ecl::Device cpu(1, 0);
      if (use_binaries) {
        cpu.setKernel(kernel_bin);
      }
      devices.push_back(move(cpu));
    } break;
    case 301: {
      ecl::Device phi(1, 1);
      if (use_binaries) {
        phi.setKernel(kernel_bin);
      }
      devices.push_back(move(phi));
    } break;
    case 310: {
      ecl::Device gpu(0, 0);
      if (use_binaries) {
        gpu.setKernel(kernel_bin);
      }
      devices.push_back(move(gpu));
    } break;
  }

  // if (tdevices == 300) { // BATEL CPU
  // } else if (tdevices == 301) { // BATEL PHI
  //   ecl::Device phi(1, 1);
  //   if (use_binaries) {
  //     phi.setKernel(kernel_bin);
  //   }
  //   devices.push_back(move(phi));
  // } else if (tdevices == 310) { // BATEL GPU
  //   ecl::Device gpu(0, 0);
  //   if (use_binaries) {
  //     gpu.setKernel(kernel_bin);
  //   }
  //   devices.push_back(move(gpu));
  // }
  // } else if (tdevices == 302) { // BATEL CPU + PHI
  //   ecl::Device cpu(1, 0);
  //   if (use_binaries) {
  //     cpu.setKernel(kernel_bin);
  //   }
  //   ecl::Device phi(1, 1);
  //   if (use_binaries) {
  //     phi.setKernel(kernel_bin);
  //   }
  //   devices.push_back(move(cpu));
  //   devices.push_back(move(phi));
  // } else if (tdevices == 311) { // BATEL PHI + GPU
  //   ecl::Device phi(1, 1);
  //   if (use_binaries) {
  //     phi.setKernel(kernel_bin);
  //   }
  //   ecl::Device gpu(0, 0);
  //   if (use_binaries) {
  //     gpu.setKernel(kernel_bin);
  //   }
  //   devices.push_back(move(phi));
  //   devices.push_back(move(gpu));
  // } else if (tdevices == 312) { // BATEL CPU + PHI + GPU
  //   ecl::Device cpu(1, 0);
  //   if (use_binaries) {
  //     cpu.setKernel(file_read_binary("support/kernels/mandelbrot_batel:1:0.cl.bin"));
  //   }
  //   ecl::Device phi(1, 1);
  //   if (use_binaries) {
  //     phi.setKernel(file_read_binary("support/kernels/mandelbrot_batel:1:1.cl.bin"));
  //   }
  //   ecl::Device gpu(0, 0);
  //   if (use_binaries) {
  //     gpu.setKernel(file_read_binary("support/kernels/mandelbrot_batel:0:0.cl.bin"));
  //   }
  //   devices.push_back(move(cpu));
  //   devices.push_back(move(phi));
  //   devices.push_back(move(gpu));
  // } else if (tdevices == 200) { // SAPU CPU
  //   ecl::Device cpu(0, 1);
  //   devices.push_back(move(cpu));
  // } else if (tdevices == 201) { // SAPU IGPU
  //   ecl::Device gpu(0, 0);
  //   devices.push_back(move(gpu));
  // } else if (tdevices == 202) { // SAPU CPU + IGPU
  //   ecl::Device cpu(0, 1);
  //   ecl::Device gpu(0, 0);
  //   devices.push_back(move(cpu));
  //   devices.push_back(move(gpu));
  // } else {
  //   if (tdevices == 0) {
  //     ecl::Device device(platform, 1);
  //     devices.push_back(move(device));
  //   } else if (tdevices == 1) {
  //     ecl::Device device2(platform, 0);
  //     devices.push_back(move(device2));
  //   } else {
  //     ecl::Device device(platform, 1);
  //     ecl::Device device2(platform, 0);
  //     devices.push_back(move(device));
  //     devices.push_back(move(device2));
  //   }
  // }

  ecl::StaticScheduler stSched;
  ecl::DynamicScheduler dynSched;
  // ecl::HGuidedScheduler hgSched;

  // auto lws = 512;
  // runtime(gws, lws)
  // size_matrix / gws = number of outupt values computed by a work-item = 4
  ecl::Runtime runtime(move(devices), gws, lws, size_matrix / gws); // size_matrix >> 2);
  if (tscheduler == 0) {
    runtime.setScheduler(&stSched);
    stSched.setRawProportions(props);
  } else if (tscheduler == 1) {
    runtime.setScheduler(&dynSched);
    dynSched.setWorkSize(worksize);
    // } else { // tscheduler == 2
    //   runtime.setScheduler(&hgSched);
    //   hgSched.setWorkSize(worksize);
    //   hgSched.setRawProportions({ prop });
  }
  // runtime.setInBuffer(a_array);
  // runtime.setInBuffer(v_array);
  // runtime.setInBuffer(x_array);
  // runtime.setInBuffer(y_array);
  runtime.setOutBuffer(out_array);
  runtime.setKernel(kernel, "mandelbrot_vector_float");

  runtime.setKernelArg(0, out_array);
  runtime.setKernelArg(1, leftxF);
  runtime.setKernelArg(2, topyF);
  runtime.setKernelArg(3, xstepF);
  runtime.setKernelArg(4, ystepF);
  runtime.setKernelArg(5, max_iterations);
  runtime.setKernelArg(6, width);
  runtime.setKernelArg(7, bench);

  runtime.run();

  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - time_init).count();

  cout << "time: " << diff_ms << "\n";

  runtime.printStats();

  // cout << "gws: " << size << " steps " << steps
  // << " samples per vector width: " << samplesPerVectorWidth << "\n";
  if (ECL_LOGGING) {
    cout << "out:\n";
    for (uint i = 0; i < 10; ++i) {
      cout << out_ptr[i] << " ";
    }
    cout << "\n";
    for (uint i = size_matrix - 10; i < size_matrix; ++i) {
      cout << out_ptr[i] << " ";
    }
    cout << "\n";
  }

  // using namespace std::chrono;

  // std::this_thread::sleep_for(1s);

  // cout << x;
  if (check) {

    auto threshold = 0.001f;

    auto ok = check_mandelbrot(
      out_ptr, leftxF, topyF, xstepF, ystepF, max_iterations, width, height, bench, threshold);

    auto time = 0;
    if (ok) {
      cout << "Success (" << time << ")\n";
    } else {
      cout << "Failure (" << time << ")\n";
    }
    if (check == 2) {
      auto img = write_bmp_file(out_ptr, width, height, "mandelbrot.bmp");
      cout << "writing mandelbrot.bmp (" << img << ")\n";
    }
  } else {
    cout << "Done\n";
  }
  exit(0);
}
