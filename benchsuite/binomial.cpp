#include "binomial.hpp"

// ./build/release/EngineCL 0 1 41 1 128 1020 0.5 128

// ./build/release/EngineCL 0 2 42 0 $(( 8 * 128 * 128 * 128 )) $(( 32 * 1024 * 255 )) 0.5 128

#include <cmath>
/**
 * \RISKFREE 0.02f
 * \brief risk free interest rate.
 */
#define RISKFREE 0.02f

/**
 * \VOLATILITY 0.30f
 * \brief Volatility factor for Binomial Option Pricing.
 */
#define VOLATILITY 0.30f

float*
binomial_option(float* randArray, int samplesPerVectorWidth, int numSamples, int numSteps)
{
  float* stepsArray = (float*)malloc((numSteps + 1) * sizeof(cl_float4));

  float* refOutput = (float*)malloc(samplesPerVectorWidth * sizeof(cl_float4));

  // Iterate for all samples
  for (int bid = 0; bid < numSamples; ++bid) {
    float s[4];
    float x[4];
    float vsdt[4];
    float puByr[4];
    float pdByr[4];
    float optionYears[4];

    float inRand[4];

    for (int i = 0; i < 4; ++i) {
      inRand[i] = randArray[bid + i];
      s[i] = (1.0f - inRand[i]) * 5.0f + inRand[i] * 30.f;
      x[i] = (1.0f - inRand[i]) * 1.0f + inRand[i] * 100.f;
      optionYears[i] = (1.0f - inRand[i]) * 0.25f + inRand[i] * 10.f;
      float dt = optionYears[i] * (1.0f / (float)numSteps);
      vsdt[i] = VOLATILITY * sqrtf(dt);
      float rdt = RISKFREE * dt;
      float r = expf(rdt);
      float rInv = 1.0f / r;
      float u = expf(vsdt[i]);
      float d = 1.0f / u;
      float pu = (r - d) / (u - d);
      float pd = 1.0f - pu;
      puByr[i] = pu * rInv;
      pdByr[i] = pd * rInv;
    }
    /**
     * Compute values at expiration date:
     * Call option value at period end is v(t) = s(t) - x
     * If s(t) is greater than x, or zero otherwise...
     * The computation is similar for put options...
     */
    for (int j = 0; j <= numSteps; j++) {
      for (int i = 0; i < 4; ++i) {
        float profit = s[i] * expf(vsdt[i] * (2.0f * j - numSteps)) - x[i];
        stepsArray[j * 4 + i] = profit > 0.0f ? profit : 0.0f;
      }
    }

    /**
     * walk backwards up on the binomial tree of depth numSteps
     * Reduce the price step by step
     */
    for (int j = numSteps; j > 0; --j) {
      for (int k = 0; k <= j - 1; ++k) {
        for (int i = 0; i < 4; ++i) {
          int index_k = k * 4 + i;
          int index_k_1 = (k + 1) * 4 + i;
          stepsArray[index_k] = pdByr[i] * stepsArray[index_k_1] + puByr[i] * stepsArray[index_k];
        }
      }
    }

    // Copy the root to result
    refOutput[bid] = stepsArray[0];
  }

  free(stepsArray);

  return refOutput;
}

auto
check_binomial(float* in_ptr,
               float* out_ptr,
               int samplesPerVectorWidth,
               int samples,
               int steps,
               float threshold)
{
  float* res = binomial_option(in_ptr, samplesPerVectorWidth, samples, steps);

  if (ECL_LOGGING) {
    std::cout << "in:\n";
    for (uint i = 0; i < samples; ++i) {
      std::cout << in_ptr[i] << " ";
    }
    std::cout << "\n";
    std::cout << "out:\n";
    for (uint i = 0; i < samples; ++i) {
      std::cout << out_ptr[i] << " ";
    }
    std::cout << "\n";

    std::cout << "res:\n";
    for (uint i = 0; i < samples; ++i) {
      std::cout << res[i] << " ";
    }
    std::cout << "\n";
  }
  auto pos = -1;
  for (auto i = 0; i < samples; ++i) {

    auto diff = abs(res[i] - out_ptr[i]);
    if (std::isnan(out_ptr[i]) || diff >= threshold) {
      cout << "diff: " << i << " ver[i]: " << res[i] << " out[i]: " << out_ptr[i] << "\n";
      pos = i;
      break;
    }
  }
  free(res);
  return pos;
}

void
do_binomial_base(int tscheduler,
                 int tdevices,
                 uint check,
                 int samples,
                 int chunksize,
                 vector<float>& props)
{
  bool use_binaries = (check >= 10) ? true : false;
  check = (check >= 10) ? check - 10 : check;

  string kernel_str;
  try {
    kernel_str = file_read("support/kernels/binomial.cl");
  } catch (std::ios::failure& e) {
    cout << "io failure: " << e.what() << "\n";
  }

  uint steps = 254;

  samples = (samples / 4) ? (samples / 4) * 4 : 4; // convierte a multiplo de 4

  auto steps1 = steps + 1;
  int samplesPerVectorWidth = samples / 4;
  // int size = steps1 * samplesPerVectorWidth;
  size_t gws = steps1 * samplesPerVectorWidth;
  size_t out_size = samplesPerVectorWidth;

  int worksize = chunksize;

  // typedef std::vector<cl_float4, aligned_allocator<cl_float4, sizeof(cl_float4)>>
  //   aligned_vector_cl_float4;

  auto in_size = samplesPerVectorWidth;
  auto in_array = make_shared<vector<cl_float4>>(in_size);
  float* in_ptr = reinterpret_cast<float*>(in_array.get()->data());
  for (uint i = 0; i < samples; ++i) {
    float f = (float)rand() / (float)RAND_MAX; // rand
    in_ptr[i] = f;
  }
  // float* in_ptr = reinterpret_cast<float*>((*in1_array.get()).data());

  auto out_array = make_shared<vector<cl_float4>>(out_size);
  float* out_ptr = reinterpret_cast<float*>(out_array.get()->data());
  for (uint i = 0; i < samples; ++i) {
    out_ptr[i] = 0.0f;
  }

  auto lws = steps1;

  auto sel_platform = PLATFORM;
  auto sel_device = tdevices == 0 ? 1 : 0; // invert, tdevices: 0 = CPU, 1 = GPU

  vector<char> kernel_bin;
  if (use_binaries) {
    switch (tdevices) {
      case 200:
        kernel_bin = file_read_binary("support/kernels/binomial_sapu:0:1.cl.bin");
        break;
      case 201:
        kernel_bin = file_read_binary("support/kernels/binomial_sapu:0:0.cl.bin");
        break;
      case 300:
        kernel_bin = file_read_binary("support/kernels/binomial_batel:1:0.cl.bin");
        break;
      case 301:
        kernel_bin = file_read_binary("support/kernels/binomial_batel:1:1.cl.bin");
        break;
      case 310:
        kernel_bin = file_read_binary("support/kernels/binomial_batel:0:0.cl.bin");
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

  auto in_bytes = in_size * sizeof(cl_float4);
  auto out_bytes = out_size * sizeof(cl_float4);

  vector<cl::Platform> platforms;
  vector<vector<cl::Device>> platfordevices;
  cl::Device device;

  IF_LOGGING(cout << "discoverDevices\n");
  cl::Platform::get(&platforms);
  IF_LOGGING(cout << "platforms: " << platforms.size() << "\n");
  auto i = 0;
  for (auto& platform : platforms) {
    vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    IF_LOGGING(cout << "platform: " << i++ << " devices: " << devices.size() << "\n");
    platfordevices.push_back(move(devices));
  }

  auto last_platform = platforms.size() - 1;
  if (sel_platform > last_platform) {
    throw runtime_error("invalid platform selected");
  }

  auto last_device = platfordevices[sel_platform].size() - 1;
  if (sel_device > last_device) {
    throw runtime_error("invalid device selected");
  }

  device = move(platfordevices[sel_platform][sel_device]);

  cl_int cl_err = CL_SUCCESS;
  cl::Context context(device);

  cl::CommandQueue queue(context, device, 0, &cl_err);
  CL_CHECK_ERROR(cl_err, "CommandQueue queue");

  IF_LOGGING(cout << "initBuffers\n");

  cl_int buffer_in_flags = CL_MEM_READ_WRITE;
  cl_int buffer_out_flags = CL_MEM_READ_WRITE;

  cl::Buffer in_buffer(context, buffer_in_flags, in_bytes, NULL);
  CL_CHECK_ERROR(cl_err, "in buffer ");
  cl::Buffer out_buffer(context, buffer_out_flags, out_bytes, NULL);
  CL_CHECK_ERROR(cl_err, "out buffer ");

  CL_CHECK_ERROR(queue.enqueueWriteBuffer(in_buffer, CL_FALSE, 0, in_bytes, in_ptr, NULL));

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

  cl::Kernel kernel(program, "binomial_options", &cl_err);

  cl_err = kernel.setArg(0, steps);
  CL_CHECK_ERROR(cl_err, "kernel arg 0 in1 buffer");

  cl_err = kernel.setArg(1, in_buffer);
  CL_CHECK_ERROR(cl_err, "kernel arg 0 in1 buffer");

  cl_err = kernel.setArg(2, out_buffer);
  CL_CHECK_ERROR(cl_err, "kernel arg 1 in2 buffer");

  cl_err = kernel.setArg(3, steps1 * sizeof(cl_float4), NULL);
  CL_CHECK_ERROR(cl_err, "kernel arg 2 out buffer");

  cl_err = kernel.setArg(4, steps * sizeof(cl_float4), NULL);
  CL_CHECK_ERROR(cl_err, "kernel arg 3 size");

  auto offset = 0;
  cl_err = queue.enqueueNDRangeKernel(
    kernel, cl::NDRange(offset), cl::NDRange(gws), cl::NDRange(lws), NULL, NULL);
  CL_CHECK_ERROR(cl_err, "enqueue kernel");

  cl_err = queue.enqueueReadBuffer(out_buffer, CL_TRUE, 0, out_bytes, out_ptr);
  CL_CHECK_ERROR(cl_err, "read buffer");

  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - time_init).count();

  cout << "time: " << diff_ms << "\n";

  // auto img = write_bmp_file(in1.data(), image_width, image_height, "g_base.bmp");
  // IF_LOGGING(cout << img << "\n");
  // img = write_bmp_file(out.data(), image_width, image_height, "gf_base.bmp");
  // IF_LOGGING(cout << img << "\n");

  if (check) {
    auto threshold = 0.01f;
    auto pos = check_binomial(in_ptr, out_ptr, samplesPerVectorWidth, samples, steps, threshold);
    auto ok = pos == -1;

    if (ok) {
      success(diff_ms);
    } else {
      failure(diff_ms);
    }
  } else {
    cout << "Done\n";
  }
}

void
do_binomial(int tscheduler,
            int tdevices,
            uint check,
            int samples,
            int chunksize,
            vector<float>& props)
{
  bool use_binaries = (check >= 10) ? true : false;
  check = (check >= 10) ? check - 10 : check;

  string kernel_str;
  try {
    kernel_str = file_read("support/kernels/binomial.cl");
  } catch (std::ios::failure& e) {
    cout << "io failure: " << e.what() << "\n";
  }

  uint steps = 254;

  samples = (samples / 4) ? (samples / 4) * 4 : 4; // convierte a multiplo de 4

  auto steps1 = steps + 1;
  int samplesPerVectorWidth = samples / 4;
  // int size = steps1 * samplesPerVectorWidth;
  size_t gws = steps1 * samplesPerVectorWidth;
  size_t out_size = samplesPerVectorWidth;

  int worksize = chunksize;

  // typedef std::vector<cl_float4, aligned_allocator<cl_float4, sizeof(cl_float4)>>
  //   aligned_vector_cl_float4;

  auto in_size = samplesPerVectorWidth;
  auto in_array = make_shared<vector<cl_float4>>(in_size);
  float* in_ptr = reinterpret_cast<float*>(in_array.get()->data());
  for (uint i = 0; i < samples; ++i) {
    float f = (float)rand() / (float)RAND_MAX; // rand
    in_ptr[i] = f;
  }
  // float* in_ptr = reinterpret_cast<float*>((*in1_array.get()).data());

  auto out_array = make_shared<vector<cl_float4>>(out_size);
  float* out_ptr = reinterpret_cast<float*>(out_array.get()->data());
  for (uint i = 0; i < samples; ++i) {
    out_ptr[i] = 0.0f;
  }

  // cout << "gws: " << gws << " steps " << steps
  //      << " samples per vector width: " << samplesPerVectorWidth << "\n";
  // for (uint i = 0; i < samples; ++i) {
  //   cout << array_ptr[i] << ":" << oarray_ptr[i] << " ";
  // }
  // cout << "\n";

  // auto in1_empty = make_shared<vector<cl_float4>>(steps1);
  // auto in2_empty = make_shared<vector<cl_float4>>(steps);

  auto lws = steps1;
  auto relation = 1.0f / steps1;

  auto platform = PLATFORM;

  vector<char> kernel_bin;
  if (use_binaries) {
    switch (tdevices) {
      case 200:
        kernel_bin = file_read_binary("support/kernels/binomial_sapu:0:1.cl.bin");
        break;
      case 201:
        kernel_bin = file_read_binary("support/kernels/binomial_sapu:0:0.cl.bin");
        break;
      case 300:
        kernel_bin = file_read_binary("support/kernels/binomial_batel:1:0.cl.bin");
        break;
      case 301:
        kernel_bin = file_read_binary("support/kernels/binomial_batel:1:1.cl.bin");
        break;
      case 310:
        kernel_bin = file_read_binary("support/kernels/binomial_batel:0:0.cl.bin");
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

  ecl::StaticScheduler stSched;
  ecl::DynamicScheduler dynSched;
  // ecl::HGuidedScheduler hgSched;

  // ecl::Runtime runtime(move(devices), size, steps1, steps1);
  ecl::Runtime runtime(move(devices), gws, lws, relation);
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
  runtime.setInBuffer(in_array);
  runtime.setOutBuffer(out_array);
  runtime.setKernel(kernel_str, "binomial_options");

  runtime.setKernelArg(0, steps);
  runtime.setKernelArg(1, in_array);
  runtime.setKernelArg(2, out_array);
  // runtime.setKernelArg(3, in1_empty);
  // runtime.setKernelArg(4, in2_empty);
  runtime.setKernelArgLocalAlloc(3, steps1 * sizeof(cl_float4));
  runtime.setKernelArgLocalAlloc(4, steps * sizeof(cl_float4));

  runtime.run();

  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - time_init).count();

  cout << "time: " << diff_ms << "\n";

  runtime.printStats();

  if (check) {
    auto threshold = 0.01f;
    auto pos = check_binomial(in_ptr, out_ptr, samplesPerVectorWidth, samples, steps, threshold);
    auto ok = pos == -1;

    if (ok) {
      success(diff_ms);
    } else {
      failure(diff_ms);
    }
  } else {
    cout << "Done\n";
  }
}
