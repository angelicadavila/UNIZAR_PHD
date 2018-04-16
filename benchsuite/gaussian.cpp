#include "gaussian.hpp"
#include <iostream>
#include <fstream>

// bench=22; size=$(( 1 * 1024 )); its=1; for i in `seq 1 1 $its`; do /usr/bin/time
// ./build/release/EngineCL 2 2 $bench 1 $size 128 0.5 21; done

using namespace std::chrono;

inline ostream&
operator<<(ostream& os, cl_uchar4& t)
{
  os << "(" << (int)t.s[0] << "," << (int)t.s[1] << "," << (int)t.s[2] << "," << (int)t.s[3] << ")";
  return os;
}

void
Gaussian::fill_image()
{
  srand(0);

  int channels = 4;
  auto total = _total_size * channels;
  // #pragma omp parallel for num_threads(omp_get_max_threads())
  for (auto i = 0; i < total; i++) {
    int mod = i % channels;
    int value = rand() % 256;
    switch (mod) {
      case 0:
        _a[i / channels].s[0] = value;
        break;
      case 1:
        _a[i / channels].s[1] = value;
        break;
      case 2:
        _a[i / channels].s[2] = value;
        break;
      case 3:
        _a[i / channels].s[3] = 0;
        break;
    }
    // if ( i < 15){
    //   cout << i << " " << mod << " " << " " << (uint8_t)value << "\n";
    //   // cout << i << " " << mod << " " << (_total_size % 256) << " " <<
    //   value << "\n"; cout << "fill_image: " << _a.at(i) << "\n";
    // }
  }
  // cout << _a[0] << "\n";
  // cout << _a[0].s[0] << "\n";
  // cout << _a[1] << "\n";
  // cout << _a[2] << "\n";
  // cout << _a[3] << "\n";
}

void
Gaussian::fill_blurred(vector<cl_uchar4,vecAllocator<cl_uchar4>>& blurred) 
{
  int channels = 4;
  auto total = _total_size * channels;
#pragma omp parallel for num_threads(omp_get_max_threads())
  for (auto i = 0; i < total; i++) {
    int mod = i % channels;
    switch (mod) {
      case 0:
        blurred[i / channels].s[0] = 0;
        break;
      case 1:
        blurred[i / channels].s[1] = 0;
        break;
      case 2:
        blurred[i / channels].s[2] = 0;
        break;
      case 3:
        blurred[i / channels].s[3] = 0;
        break;
    }
  }
}

void
Gaussian::fill_filter()
{
  const float sigma = 2.f;

  const int half = _filter_width / 2;
  float sum = 0.f;

  // vector<cl_float>* res = new vector<cl_float>(width * width);
  // vector<cl_float> res(width * width);
  // float* res = (float*)malloc(width * width * sizeof(float));

  int r;
  for (r = -half; r <= half; ++r) {
    int c;
    for (c = -half; c <= half; ++c) {
      float weight = expf(-(float)(c * c + r * r) / (2.0f * sigma * sigma));
      // float weight = expf(-(float)(c * c + r * r) / (2.f * sigma * sigma));
      int idx = (r + half) * _filter_width + c + half;

      _b[idx] = weight;
      // res.at(idx) = weight;
      sum += weight;
    }
  }

  float normal = 1.0f / sum;

  for (r = -half; r <= half; ++r) {
    int c;
    for (c = -half; c <= half; ++c) {
      int idx = (r + half) * _filter_width + c + half;

      _b[idx] *= normal;
      // res[idx] *= normal;
    }
  }
  // return move(res);

  // cout << "filter:\n";
  // for (auto i=0; i<_filter_total_size; i++){
  //   cout << _b[i] << " ";
  // }
  // cout << "\n";
}

void
Gaussian::omp_gaussian_blur()
{
  int rows = _height;
  int cols = _width;
  int filterWidth = _filter_width;
#pragma GCC diagnostic ignored "-Wignored-attributes"
  vector<cl_uchar4,vecAllocator<cl_uchar4>>& input = _a;
  vector<cl_float,vecAllocator<cl_float>>& filterWeight = _b;
  vector<cl_uchar4,vecAllocator<cl_uchar4>>& blurred = _c;
#pragma GCC diagnostic pop

  int total_size = _total_size;

  // auto num_threads = omp_get_max_threads();
  auto num_threads = 8;
  auto part = total_size / num_threads;

  // ANNOTATE_SITE_BEGIN(Site1);

#pragma omp parallel for num_threads(num_threads) schedule(static, part)
  // #pragma omp parallel for num_threads(num_threads) schedule(dynamic,part)
  // #pragma omp parallel for num_threads(num_threads) schedule(runtime)
  // #pragma omp parallel for num_threads(2) schedule(runtime)
  for (int i = 0; i < total_size; i++) {
    // ANNOTATE_ITERATION_TASK(Task1);
    // int tid = get_global_id(0);
    int tid = i;

    if (tid < total_size) {
      int r = tid / cols; // current row
      int c = tid % cols; // current column

      int middle = filterWidth / 2;
      float blurX = 0.0f; // will contained blurred value
      float blurY = 0.0f; // will contained blurred value
      float blurZ = 0.0f; // will contained blurred value
      int width = cols - 1;
      int height = rows - 1;

      for (int i = -middle; i <= middle; ++i) // rows
      {
        // #pragma omp simd
        for (int j = -middle; j <= middle; ++j) // columns
        {
          // Clamp filter to the image border
          // int h=min(max(r+i, 0), height);
          // int w=min(max(c+j, 0), width);

          int h = r + i;
          int w = c + j;
          if (h > height || h < 0 || w > width || w < 0) {
            continue;
          }

          // Blur is a product of current pixel value and weight of that pixel.
          // Remember that sum of all weights equals to 1, so we are averaging
          // sum of all pixels by their weight.
          int idx = w + cols * h; // current pixel index
          float pixelX = input[idx].s[0];
          float pixelY = input[idx].s[1];
          float pixelZ = input[idx].s[2];

          idx = (i + middle) * filterWidth + j + middle;
          float weight = filterWeight[idx];

          blurX += pixelX * weight;
          blurY += pixelY * weight;
          blurZ += pixelZ * weight;
        }
      }

      // if (tid == 3921){
      // if (tid == 2592){
      //   cout << blurZ << " " << setprecision(10) << blurZ << " " <<
      //   round(blurZ) << " " <<  round_to_decimal(blurZ)
      //   << "\n";
      // }

      // blurred[tid].s[0] = blurX;
      // blurred[tid].s[1] = blurY;
      // blurred[tid].s[2] = blurZ;
      // blurred[tid].s[0] = (unsigned char)round_to_decimal<3>(blurX);
      // blurred[tid].s[1] = (unsigned char)round_to_decimal<3>(blurY);
      // blurred[tid].s[2] = (unsigned char)round_to_decimal<3>(blurZ);
      blurred[tid].s[0] = (unsigned char)round(blurX);
      blurred[tid].s[1] = (unsigned char)round(blurY);
      blurred[tid].s[2] = (unsigned char)round(blurZ);
    }
  } // omp

  // ANNOTATE_SITE_END();
}

bool
Gaussian::compare_gaussian_blur(float threshold)
{
  int rows = _height;
  int cols = _width;
  int filterWidth = _filter_width;
#pragma GCC diagnostic ignored "-Wignored-attributes"
  vector<cl_uchar4,vecAllocator<cl_uchar4>>& input = _a;
  vector<cl_float,vecAllocator<cl_float>>& filterWeight = _b;
  vector<cl_uchar4,vecAllocator<cl_uchar4>>& blurred = _c;
#pragma GCC diagnostic pop

  int total_size = _total_size;

  auto num_threads = omp_get_max_threads();
  auto ok = true;
  vector<bool> oks(num_threads, true);

#pragma omp parallel num_threads(num_threads)
  {
    bool showable = true;
#pragma omp for reduction(& : ok)
    for (int i = 0; i < total_size; i++) {
      // int tid = get_global_id(0);
      int tid = i;

      if (tid < total_size) {
        int r = tid / cols; // current row
        int c = tid % cols; // current column

        int middle = filterWidth / 2;
        float blurX = 0.0f; // will contained blurred value
        float blurY = 0.0f; // will contained blurred value
        float blurZ = 0.0f; // will contained blurred value
        int width = cols - 1;
        int height = rows - 1;

        for (int i = -middle; i <= middle; ++i) // rows
        {
          for (int j = -middle; j <= middle; ++j) // columns
          {
            // Clamp filter to the image border
            // int h=min(max(r+i, 0), height);
            // int w=min(max(c+j, 0), width);

            int h = r + i;
            int w = c + j;
            if (h > height || h < 0 || w > width || w < 0) {
              continue;
            }

            // Blur is a product of current pixel value and weight of that
            // pixel. Remember that sum of all weights equals to 1, so we are
            // averaging sum of all pixels by their weight.
            int idx = w + cols * h; // current pixel index
            float pixelX = input[idx].s[0];
            float pixelY = input[idx].s[1];
            float pixelZ = input[idx].s[2];

            idx = (i + middle) * filterWidth + j + middle;
            float weight = filterWeight[idx];

            blurX += pixelX * weight;
            blurY += pixelY * weight;
            blurZ += pixelZ * weight;
          }
        }

        // if (tid == 3921){
        // if (tid == 2592){
        //   cout << blurZ << " " << setprecision(10) << blurZ << " " <<
        //   round(blurZ) << " " <<  round_to_decimal(blurZ)
        //   << "\n";
        // }

        auto diffX = abs(blurX - (float)blurred[tid].s[0]);
        auto diffY = abs(blurY - (float)blurred[tid].s[1]);
        auto diffZ = abs(blurZ - (float)blurred[tid].s[2]);

        if (diffX >= threshold || diffY >= threshold || diffZ >= threshold) {
          if (showable) {
#pragma omp critical
            cout << "i: " << tid << " blurred: " << blurred[tid] << " float calculated: (" << blurX
                 << "," << blurY << "," << blurZ << ")\n";
            showable = false;
            ok = ok & false;
          }
        }
      }
    }
  } // omp

  // if (!ok){
  //   cout << "oks: " << oks[0] << " " << oks[1] << " " << oks[2] << " " <<
  //   oks[3] << "\n";
  // }

  return ok;
}

bool
Gaussian::compare_gaussian_blur_2loops(float /* threshold */)
{
#pragma GCC diagnostic ignored "-Wignored-attributes"
  vector<cl_uchar4,vecAllocator<cl_uchar4>> blurred(_total_size);
  fill_blurred(blurred);
  vector<cl_uchar4,vecAllocator<cl_uchar4>>& input = _a;
  vector<cl_float,vecAllocator<cl_float>>& filterWeight = _b;
#pragma GCC diagnostic pop

  // gaussian_blur(__global uchar4* blurred, __global uchar4* input, int rows,
  //               int cols, __global float* filterWeight, int filterWidth)
  int rows = _height;
  int cols = _width;
  int filterWidth = _filter_width;

  int total_size = _total_size;

#pragma omp parallel for num_threads(4)
  for (int i = 0; i < total_size; i++) {
    // int tid = get_global_id(0);
    int tid = i;

    if (tid < total_size) {
      int r = tid / cols; // current row
      int c = tid % cols; // current column

      int middle = filterWidth / 2;
      float blurX = 0.0f; // will contained blurred value
      float blurY = 0.0f; // will contained blurred value
      float blurZ = 0.0f; // will contained blurred value
      int width = cols - 1;
      int height = rows - 1;

      for (int i = -middle; i <= middle; ++i) // rows
      {
        for (int j = -middle; j <= middle; ++j) // columns
        {
          // Clamp filter to the image border
          // int h=min(max(r+i, 0), height);
          // int w=min(max(c+j, 0), width);

          int h = r + i;
          int w = c + j;
          if (h > height || h < 0 || w > width || w < 0) {
            continue;
          }

          // Blur is a product of current pixel value and weight of that pixel.
          // Remember that sum of all weights equals to 1, so we are averaging
          // sum of all pixels by their weight.
          int idx = w + cols * h; // current pixel index
          float pixelX = input[idx].s[0];
          float pixelY = input[idx].s[1];
          float pixelZ = input[idx].s[2];

          idx = (i + middle) * filterWidth + j + middle;
          float weight = filterWeight[idx];

          blurX += pixelX * weight;
          blurY += pixelY * weight;
          blurZ += pixelZ * weight;
        }
      }

      // if (tid == 3921){
      // if (tid == 2592){
      //   cout << blurZ << " " << setprecision(10) << blurZ << " " <<
      //   round(blurZ) << " " <<  round_to_decimal(blurZ)
      //   << "\n";
      // }

      blurred[tid].s[0] = (unsigned char)round_to_decimal<3>(blurX);
      blurred[tid].s[1] = (unsigned char)round_to_decimal<3>(blurY);
      blurred[tid].s[2] = (unsigned char)round_to_decimal<3>(blurZ);
      // blurred[tid].s[0] = (unsigned char)(blurX);
      // blurred[tid].s[1] = (unsigned char)(blurY);
      // blurred[tid].s[2] = (unsigned char)(blurZ);
      // for i = 49 (image size)
      // if (tid == 2031 || tid == 2032 || tid == 2033){
      //   cout << "tid: " << tid << " " << blurred[tid] << " x: " << blurX << "
      //   y: " << blurY << " z: " << blurZ <<
      //   "\n"; cout << "tid: " << tid << " " << blurred[tid] << " x: " <<
      //   (int)blurX << " y: " << (int)blurY << " z: "
      //   << (int)blurZ << "\n";
      // }
    }
  }

  // blurred[3].s[1] = 33;
  // blurred[12577].s[1] = 33;
  // blurred[100].s[1] = 33;
  // blurred[400000].s[1] = 33;
  auto ok = true;
  int channels = 4;
  int threads = 4;
  bool oks[24] = { true, true, true, true };

#pragma omp parallel num_threads(threads)
  {
    int tid = omp_get_thread_num();
#pragma omp critical
    cout << "thread: " << tid << "\n";
    bool showable = true;

#pragma omp for reduction(& : ok)
    for (auto i = 0; i < _total_size * channels; i++) {
      int mod = i % channels;
      int lok = true;
      lok = blurred[i / channels].s[mod] == _c[i / channels].s[mod];
      if (!lok) {
        oks[tid] = false;
#pragma omp critical
        {
          if (showable) {
            // cout << "i: " << (i-1)/channels << " mod: " << (i-1)%channels <<
            // " blurred: " << blurred[(i-1)/channels]
            // << " _c: " << _c[(i-1)/channels] << "\n";
            cout << "i: " << i / channels << " mod: " << mod
                 << " blurred: " << blurred[i / channels] << " _c: " << _c[i / channels] << "\n";
            // cout << "i: " << (i+1)/channels << " mod: " << (i+1)%channels <<
            // " blurred: " << blurred[(i+1)/channels]
            // << " _c: " << _c[(i+1)/channels] << "\n";
            showable = false;
          }
        }
        ok = ok & lok;
      }
    }
  } // omp

  if (!ok) {
    cout << "oks: " << oks[0] << " " << oks[1] << " " << oks[2] << " " << oks[3] << "\n";
  }

  return ok;
}

string
Gaussian::get_kernel_str()
{
  string kernelstr = R"(

// #pragma OPENCL EXTENSION cl_khr_select_fprounding_mode : enable
// #pragma OPENCL EXTENSION cl_intel_printf : enable
#pragma OPENCL EXTENSION cl_amd_printf : enable

__kernel void
#if CL_SUPPORT_KERNEL_OFFSET == 1
gaussian_blur(__global uchar4* blurred, __global uchar4* input, int rows,
              int cols, __global float* filterWeight, int filterWidth){
  int tid = get_global_id(0);
#else
gaussian_blur(__global uchar4* blurred, __global uchar4* input, int rows,
              int cols, __global float* filterWeight, int filterWidth, uint offset){
  int tid = get_global_id(0) + offset;
#endif

  if (tid < rows * cols) {
    int r = tid / cols; // current row
    int c = tid % cols; // current column

    int middle = filterWidth / 2;
    float blurX = 0.f; // will contained blurred value
    float blurY = 0.f; // will contained blurred value
    float blurZ = 0.f; // will contained blurred value
    int width = cols - 1;
    int height = rows - 1;

    for (int i = -middle; i <= middle; ++i) // rows
    {
      for (int j = -middle; j <= middle; ++j) // columns
      {
        // Clamp filter to the image border
        // int h=min(max(r+i, 0), height);
        // int w=min(max(c+j, 0), width);

        int h = r + i;
        int w = c + j;
        if (h > height || h < 0 || w > width || w < 0) {
          continue;
        }

        // Blur is a product of current pixel value and weight of that pixel.
        // Remember that sum of all weights equals to 1, so we are averaging sum
        // of all pixels by their weight.
        int idx = w + cols * h; // current pixel index
        float pixelX = (input[idx].x);
        float pixelY = (input[idx].y);
        float pixelZ = (input[idx].z);

        idx = (i + middle) * filterWidth + j + middle;
        float weight = filterWeight[idx];

        blurX += pixelX * weight;
        blurY += pixelY * weight;
        blurZ += pixelZ * weight;
      }
    }

    // if (tid == 2592){
    //    printf("%f %f %d %d\n", blurZ, round(blurZ), (unsigned char)round(blurZ), (int)round(blurZ));
    //    printf("%f %d\n", blurZ, convert_uchar_rte(blurZ));
    //    printf("%f %d\n", blurZ, convert_uchar_rtz(blurZ));
    //    printf("%f %d\n", blurZ, convert_uchar_rtp(blurZ));
    // }

    // blurred[tid].x = (unsigned char)(blurX);
    // blurred[tid].y = (unsigned char)(blurY);
    // blurred[tid].z = (unsigned char)(blurZ);

    blurred[tid].x = (unsigned char)round(blurX);
    blurred[tid].y = (unsigned char)round(blurY);
    blurred[tid].z = (unsigned char)round(blurZ);
  }
}
)";

  return move(kernelstr);
}

void
do_gaussian_base(int tscheduler,
                 int tdevices,
                 uint check,
                 uint image_width,
                 int chunksize,
                 vector<float>& props,
                 uint filter_width)
{
  bool use_binaries = (check >= 10) ? true : false;
  check = (check >= 10) ? check - 10 : check;

  uint image_height = image_width;

  int worksize = chunksize;

  IF_LOGGING(cout << image_width << "\n");

  Gaussian gaussian(image_width, image_height, filter_width);

  string kernel_str = file_read("support/kernels/gaussian.cl");

  int size = gaussian._total_size;

  auto a_array = shared_ptr<vector<cl_uchar4>>(&gaussian._a);
  auto b_array = shared_ptr<vector<cl_float>>(&gaussian._b);
  auto c_array = shared_ptr<vector<cl_uchar4>>(&gaussian._c);

  auto sel_platform = PLATFORM;
  auto sel_device = tdevices == 0 ? 1 : 0; // invert, tdevices: 0 = CPU, 1 = GPU

  vector<char> kernel_bin;
  if (use_binaries) {
    switch (tdevices) {
      case 200:
        kernel_bin = file_read_binary("support/kernels/gaussian_sapu:0:1.cl.bin");
        break;
      case 201:
        kernel_bin = file_read_binary("support/kernels/gaussian_sapu:0:0.cl.bin");
        break;
      case 300:
        kernel_bin = file_read_binary("support/kernels/gaussian_batel:1:0.cl.bin");
        break;
      case 301:
        kernel_bin = file_read_binary("support/kernels/gaussian_batel:1:1.cl.bin");
        break;
      case 310:
        kernel_bin = file_read_binary("support/kernels/gaussian_batel:0:0.cl.bin");
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

  IF_LOGGING(cout << a_array.get()->size() << "\n");
  IF_LOGGING(cout << b_array.get()->size() << "\n");
  IF_LOGGING(cout << c_array.get()->size() << "\n");

  cl::Buffer a_buffer(context, buffer_in_flags, sizeof(cl_uchar4) * a_array.get()->size(), NULL);
  CL_CHECK_ERROR(cl_err, "in1 buffer ");
  cl::Buffer b_buffer(context, buffer_in_flags, sizeof(cl_float) * b_array.get()->size(), NULL);
  CL_CHECK_ERROR(cl_err, "in2 buffer ");
  cl::Buffer c_buffer(context, buffer_out_flags, sizeof(cl_uchar4) * c_array.get()->size(), NULL);
  CL_CHECK_ERROR(cl_err, "out buffer ");

  IF_LOGGING(cout << "x\n");
  CL_CHECK_ERROR(queue.enqueueWriteBuffer(
    a_buffer, CL_FALSE, 0, sizeof(cl_uchar4) * a_array.get()->size(), a_array.get()->data(), NULL));

  CL_CHECK_ERROR(queue.enqueueWriteBuffer(
    b_buffer, CL_FALSE, 0, sizeof(cl_float) * b_array.get()->size(), b_array.get()->data(), NULL));

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

  cl::Kernel kernel(program, "gaussian_blur", &cl_err);

  cl_err = kernel.setArg(0, c_buffer);
  CL_CHECK_ERROR(cl_err, "kernel arg 0 in1 buffer");

  cl_err = kernel.setArg(1, a_buffer);
  CL_CHECK_ERROR(cl_err, "kernel arg 1 in2 buffer");

  cl_err = kernel.setArg(2, image_height);
  CL_CHECK_ERROR(cl_err, "kernel arg 2 out buffer");

  cl_err = kernel.setArg(3, image_width);
  CL_CHECK_ERROR(cl_err, "kernel arg 3 size");

  cl_err = kernel.setArg(4, b_buffer);
  CL_CHECK_ERROR(cl_err, "kernel arg 1 in2 buffer");

  cl_err = kernel.setArg(5, filter_width);
  CL_CHECK_ERROR(cl_err, "kernel arg 3 size");

  // cl_int cl_err;
  cl::UserEvent end(context, &cl_err);
  CL_CHECK_ERROR(cl_err, "user event end");

  cl::Event evkernel;

  auto lws = 128;

  auto offset = 0;
  auto gws = size;
  queue.enqueueNDRangeKernel(
    kernel, cl::NDRange(offset), cl::NDRange(gws), cl::NDRange(lws), NULL, NULL);

  cl::Event evread;
  vector<cl::Event> events({ evkernel });

  queue.enqueueReadBuffer(
    c_buffer, CL_TRUE, 0, sizeof(cl_uchar4) * c_array.get()->size(), c_array.get()->data());

  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - time_init).count();

  cout << "time: " << diff_ms << "\n";

  auto in1 = *a_array.get();
  auto in2 = *b_array.get();
  auto out = *c_array.get();

  if (check) {

    auto ok = gaussian.compare_gaussian_blur();

    if (ok) {
      success(diff_ms);
    } else {
      failure(diff_ms);
    }
    if (check == 2) {
      auto img = write_bmp_file(out.data(), image_width, image_height, "gaussian_base.bmp");
      cout << "writing gaussian_base.bmp (" << img << ")\n";
    }
  } else {
    cout << "Done\n";
  }
  exit(0);
}

void
do_gaussian(int tscheduler,
            int tdevices,
            bool check,
            uint image_width,
            int chunksize,
            vector<float>& props,
            uint filter_width)
{
  bool use_binaries = (check >= 10) ? true : false;
  check = (check >= 10) ? check - 10 : check;

  uint image_height = image_width;

  int worksize = chunksize;

  IF_LOGGING(cout << "width " << image_width << "\n");
  IF_LOGGING(cout << "filter width " << filter_width << "\n");

  Gaussian gaussian(image_width, image_height, filter_width);

  // string kernel = gaussian.get_kernel_str();
  string kernel = file_read("support/kernels/gaussian.cl");
  // auto a = make_shared<vector<cl_uchar4>>(gaussian._a);
  // auto b = make_shared<vector<cl_float>>(gaussian._b);
  // auto c = make_shared<vector<cl_uchar4>>(gaussian._c);
#pragma GCC diagnostic ignored "-Wignored-attributes"
 auto a = shared_ptr<vector<cl_uchar4,vecAllocator<cl_uchar4>>>(&gaussian._a);
 auto b = shared_ptr<vector<cl_float,vecAllocator<cl_float>>>(&gaussian._b);
 auto c = shared_ptr<vector<cl_uchar4,vecAllocator<cl_uchar4>>>(&gaussian._c);
#pragma GCC diagnostic pop

  int problem_size = gaussian._total_size;

  auto platform = PLATFORM;

  auto platform_cpu = 0;
  auto platform_gpu = 1;
  auto platform_fpga= 2;

  vector <char> binary_file;
  if (tdevices &0x04){  
    ecl::Device device2(platform_fpga,0);
    binary_file	=file_read_binary("./benchsuite/altera_kernel/gaussian_unroll.aocx"); 
    device2.setKernel(binary_file); 
    devices.push_back(move(device2));
  }

  if (tdevices &0x01){  
    ecl::Device device(platform_cpu,0);
    devices.push_back(move(device));
  }
  if (tdevices &0x02){  
    ecl::Device device1(platform_gpu,0);
    devices.push_back(move(device1));
  }

  ecl::StaticScheduler stSched;
  ecl::DynamicScheduler dynSched;
  // ecl::HGuidedScheduler hgSched;

  ecl::Runtime runtime(move(devices), problem_size);
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
  runtime.setInBuffer(a);
  runtime.setInBuffer(b);
  runtime.setOutBuffer(c);
  runtime.setKernel(kernel, "gaussian_blur");

  runtime.setKernelArg(0, c);
  runtime.setKernelArg(1, a);
  runtime.setKernelArg(2, image_height);
  runtime.setKernelArg(3, image_width);
  runtime.setKernelArg(4, b);
  runtime.setKernelArg(5, filter_width);

  runtime.run();

  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - time_init).count();

  cout << "time: " << diff_ms << "\n";

  runtime.printStats();

  // auto ptr = reinterpret_cast<cl_uchar4*>(b.data());
  // IF_LOGGING(cout << ptr[0].s[0] << "\n");
  // IF_LOGGING(cout << ptr[121].s[0] << "\n");

  auto in1 = *a.get();
  auto in2 = *b.get();
  auto out = *c.get();

  if (check) {
    auto in1 = *a.get();
    auto in2 = *b.get();
    auto out = *c.get();

    // out[33] = 0;
    // vector<int> steps = {0,1,2,9,10,11,121,122,123};
    // for (auto i : steps){
    //   cout << "in1[" << i << "]: " << in1[i] << "\n";
    //   cout << "in2[" << i << "]: " << in2[i] << "\n";
    //   cout << "out[" << i << "]: " << out[i] << "\n";
    // }

    auto ok = gaussian.compare_gaussian_blur();

    auto time = 0;
    if (ok) {
      cout << "Success gauss(" << time << ")\n";
    } else {
      cout << "Failure gauss(" << time << ")\n";
    }
    if (check == 2) {
      auto xxx = write_bmp_file(in1.data(), image_width, image_height, "gaussian_original.bmp");
      auto img = write_bmp_file(out.data(), image_width, image_height, "gaussian.bmp");
      cout << "writing gaussian.bmp (" << img << ")\n";
    }
  } else {
    cout << "Done gauss\n";
  }

  exit(0);
}
