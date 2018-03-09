#include "saxpy.hpp"

using namespace std::chrono;

auto
check_saxpy(vector<int> in1, vector<int> in2, vector<int> out, int size, float a)
{
  auto pos = -1;
  for (auto i = 0; i < size; ++i) {
    int v = (a * (float)in1[i]) + in2[i];
    if (v != out[i]) {
      pos = i;
      break;
    }
  }
  return pos;
}

void
do_saxpy_base(int tdevices, uint check, int wsize, float a)
{
  bool use_binaries = (check >= 10) ? true : false;
  check = (check >= 10) ? check - 10 : check;

  string kernel_str;
  try {
    kernel_str = file_read("support/kernels/saxpy.cl");
  } catch (std::ios::failure& e) {
    cout << "io failure: " << e.what() << "\n";
  }

  int size = wsize;
  // int worksize = chunksize;

  auto in1_array = make_shared<vector<int>>(size, 1);
  auto in2_array = make_shared<vector<int>>(size, 2);
  auto out_array = make_shared<vector<int>>(size, 0);

  auto sel_platform = PLATFORM;
  auto sel_device = tdevices == 0 ? 1 : 0; // invert, tdevices: 0 = CPU, 1 = GPU

  auto lws = 128;
  auto gws = size;

  vector<char> kernel_bin;
  if (use_binaries) {
    switch (tdevices) {
      case 200:
        kernel_bin = file_read_binary("support/kernels/saxpy_sapu:0:1.cl.bin");
        break;
      case 201:
        kernel_bin = file_read_binary("support/kernels/saxpy_sapu:0:0.cl.bin");
        break;
      case 300:
        kernel_bin = file_read_binary("support/kernels/saxpy_batel:1:0.cl.bin");
        break;
      case 301:
        kernel_bin = file_read_binary("support/kernels/saxpy_batel:1:1.cl.bin");
        break;
      case 310:
        kernel_bin = file_read_binary("support/kernels/saxpy_batel:0:0.cl.bin");
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

  cl::Buffer in1_buffer(context, buffer_in_flags, sizeof(int) * in1_array.get()->size(), NULL);
  CL_CHECK_ERROR(cl_err, "in1 buffer ");
  cl::Buffer in2_buffer(context, buffer_in_flags, sizeof(int) * in2_array.get()->size(), NULL);
  CL_CHECK_ERROR(cl_err, "in2 buffer ");
  cl::Buffer out_buffer(context, buffer_out_flags, sizeof(int) * out_array.get()->size(), NULL);
  CL_CHECK_ERROR(cl_err, "out buffer ");

  CL_CHECK_ERROR(queue.enqueueWriteBuffer(
    in1_buffer, CL_FALSE, 0, sizeof(int) * in1_array.get()->size(), in1_array.get()->data(), NULL));

  CL_CHECK_ERROR(queue.enqueueWriteBuffer(
    in2_buffer, CL_FALSE, 0, sizeof(int) * in2_array.get()->size(), in2_array.get()->data(), NULL));

  IF_LOGGING(cout << "initKernel\n");

  cl::Program::Sources sources;
  cl::Program::Binaries binaries;
  cl::Program program;
  if (use_binaries) {
    IF_LOGGING(cout << "using binary\n");
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
    cout << " Error building: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << "\n";
    CL_CHECK_ERROR(cl_err);
  }

  cl::Kernel kernel(program, "saxpy", &cl_err);
  CL_CHECK_ERROR(cl_err, "kernel");

  cl_err = kernel.setArg(0, in1_buffer);
  CL_CHECK_ERROR(cl_err, "kernel arg 0 in1 buffer");

  cl_err = kernel.setArg(1, in2_buffer);
  CL_CHECK_ERROR(cl_err, "kernel arg 1 in2 buffer");

  cl_err = kernel.setArg(2, out_buffer);
  CL_CHECK_ERROR(cl_err, "kernel arg 2 out buffer");

  cl_err = kernel.setArg(3, size);
  CL_CHECK_ERROR(cl_err, "kernel arg 3 size");

  cl_err = kernel.setArg(4, a);
  CL_CHECK_ERROR(cl_err, "kernel arg 4 a");

  // cl_int cl_err;
  // cl::UserEvent end(context, &cl_err);
  // CL_CHECK_ERROR(cl_err, "user event end");

  // cl::Event evkernel;

  auto offset = 0;
  cl_err = queue.enqueueNDRangeKernel(
    kernel, cl::NDRange(offset), cl::NDRange(gws), cl::NDRange(lws), NULL, NULL);
  CL_CHECK_ERROR(cl_err, "enqueue kernel");

  // cl::Event evread;
  // vector<cl::Event> events({ evkernel });

  cl_err =
    queue.enqueueReadBuffer(out_buffer, CL_TRUE, 0, sizeof(int) * size, out_array.get()->data());
  CL_CHECK_ERROR(cl_err, "read buffer");

  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - time_init).count();

  cout << "time: " << diff_ms << "\n";

  if (check) {
    auto in1 = *in1_array.get();
    auto in2 = *in2_array.get();
    auto out = *out_array.get();

    auto pos = check_saxpy(in1, in2, out, size, a);
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
do_saxpy(int tscheduler,
         int tdevices,
         uint check,
         int wsize,
         int chunksize,
         vector<float>& props,
         float a)
{
  bool use_binaries = (check >= 10) ? true : false;
  check = (check >= 10) ? check - 10 : check;

  string kernel_str;
  try {
    kernel_str = file_read("support/kernels/saxpy.cl");
  } catch (std::ios::failure& e) {
    cout << "io failure: " << e.what() << "\n";
  }

  int size = wsize;
  int worksize = chunksize;
  size_t lws = 128;
  auto gws = size;

  auto in1_array = make_shared<vector<int>>(size, 1);
  auto in2_array = make_shared<vector<int>>(size, 2);
  auto out_array = make_shared<vector<int>>(size, 0);

  auto platform = PLATFORM;

  vector<char> kernel_bin;
  if (use_binaries) {
    switch (tdevices) {
      case 200:
        kernel_bin = file_read_binary("support/kernels/saxpy_sapu:0:1.cl.bin");
        break;
      case 201:
        kernel_bin = file_read_binary("support/kernels/saxpy_sapu:0:0.cl.bin");
        break;
      case 300:
        kernel_bin = file_read_binary("support/kernels/saxpy_batel:1:0.cl.bin");
        break;
      case 301:
        kernel_bin = file_read_binary("support/kernels/saxpy_batel:1:1.cl.bin");
        break;
      case 310:
        kernel_bin = file_read_binary("support/kernels/saxpy_batel:0:0.cl.bin");
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

  // cl::NDRange ngws(gws);
  // ecl::Runtime runtime(move(devices), ngws, lws);
  // ecl::Runtime runtime(move(devices), gws, lws);
  ecl::Runtime runtime(move(devices), { gws, 0, 0 }, lws);
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
  runtime.setInBuffer(in1_array);
  runtime.setInBuffer(in2_array);
  runtime.setOutBuffer(out_array);
  runtime.setKernel(kernel_str, "saxpy");

  runtime.setKernelArg(0, in1_array);
  runtime.setKernelArg(1, in2_array);
  runtime.setKernelArg(2, out_array);
  runtime.setKernelArg(3, size);
  runtime.setKernelArg(4, a);

  runtime.run();

  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - time_init).count();

  cout << "time: " << diff_ms << "\n";

  runtime.printStats();

  if (check) {
    auto in1 = *in1_array.get();
    auto in2 = *in2_array.get();
    auto out = *out_array.get();

    auto pos = check_saxpy(in1, in2, out, size, a);
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
