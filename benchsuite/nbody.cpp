#include "nbody.hpp"

#define GROUP_SIZE 64

// ./build/release/EngineCL 1 2 62 0 512000 4000 0.5

using namespace std::chrono;

inline ostream&
operator<<(ostream& os, cl_float4& t)
{
  os << "(" << (float)t.s[0] << "," << (float)t.s[1] << "," << (float)t.s[2] << "," << (float)t.s[3]
     << ")";
  return os;
}

void
nBodyCPUReference(uint numBodies,
                  float delT,
                  float espSqr,
                  float* currentPos,
                  float* currentVel,
                  float* newPos,
                  float* newVel)
{
  // Iterate for all samples
  for (cl_uint i = 0; i < numBodies; ++i) {
    int myIndex = 4 * i;
    float acc[3] = { 0.0f, 0.0f, 0.0f };
    for (cl_uint j = 0; j < numBodies; ++j) {
      float r[3];
      int index = 4 * j;

      float distSqr = 0.0f;
      for (int k = 0; k < 3; ++k) {
        r[k] = currentPos[index + k] - currentPos[myIndex + k];

        distSqr += r[k] * r[k];
      }

      float invDist = 1.0f / sqrt(distSqr + espSqr);
      float invDistCube = invDist * invDist * invDist;
      float s = currentPos[index + 3] * invDistCube;

      for (int k = 0; k < 3; ++k) {
        acc[k] += s * r[k];
      }
    }

    for (int k = 0; k < 3; ++k) {
      newPos[myIndex + k] =
        currentPos[myIndex + k] + currentVel[myIndex + k] * delT + 0.5f * acc[k] * delT * delT;
      newVel[myIndex + k] = currentVel[myIndex + k] + acc[k] * delT;
    }
    newPos[myIndex + 3] = currentPos[myIndex + 3];
  }
}

bool
do_nbody_check(uint num_bodies,
               float delT,
               float espSqr,
               float* pos_in_ptr,
               float* vel_in_ptr,
               float* pos_out_ptr,
               float* vel_out_ptr,
               float threshold)
{
  float* pos_out = (float*)malloc(num_bodies * sizeof(cl_float4));
  float* vel_out = (float*)malloc(num_bodies * sizeof(cl_float4));
  nBodyCPUReference(num_bodies, delT, espSqr, pos_in_ptr, vel_in_ptr, pos_out, vel_out);

  bool ok = true;
  uint buffer_size = num_bodies * 4;
  for (uint i = 0; i < buffer_size; ++i) {

    float vorig = pos_out_ptr[i];
    float v = pos_out[i];
    auto diff = abs(vorig - v);
    if (diff >= threshold) {
      IF_LOGGING(cout << "i: " << i << " value: " << vorig << " calculated: " << v << "\n");
      ok = false;
      break;
    }
  }
  free(pos_out);
  free(vel_out);
  return ok;
}

void
do_nbody_base(int tscheduler,
              int tdevices,
              uint check,
              uint num_particles,
              int chunksize,
              vector<float>& props)
{
  bool use_binaries = (check >= 10) ? true : false;
  check = (check >= 10) ? check - 10 : check;

  auto group_size = GROUP_SIZE;

  cl_float delT = DEL_T;
  cl_float espSqr = ESP_SQR;

  int worksize = chunksize;

  string kernel_str = file_read("support/kernels/nbody.cl");

  num_particles = (uint)(((size_t)num_particles < group_size) ? group_size : num_particles);
  num_particles = (uint)((num_particles / group_size) * group_size);

  uint num_bodies = num_particles;

  auto pos_in_array = make_shared<vector<cl_float4>>(num_bodies);
  auto vel_in_array = make_shared<vector<cl_float4>>(num_bodies);
  auto pos_out_array = make_shared<vector<cl_float4>>(num_bodies);
  auto vel_out_array = make_shared<vector<cl_float4>>(num_bodies);

  // auto pos_in_ptr = *pos_in_array.get();
  // auto vel_in_ptr = *vel_in_array.get();
  // auto pos_out_ptr = *pos_out_array.get();
  // auto vel_out_ptr = *vel_out_array.get();
  cl_float4* pos_in_ptr = reinterpret_cast<cl_float4*>(pos_in_array.get()->data());
  cl_float4* vel_in_ptr = reinterpret_cast<cl_float4*>(vel_in_array.get()->data());
  cl_float4* pos_out_ptr = reinterpret_cast<cl_float4*>(pos_out_array.get()->data());
  cl_float4* vel_out_ptr = reinterpret_cast<cl_float4*>(vel_out_array.get()->data());

  float* pos_in = reinterpret_cast<float*>(pos_in_ptr);
  float* vel_in = reinterpret_cast<float*>(vel_in_ptr);
  float* pos_out = reinterpret_cast<float*>(pos_out_ptr);
  float* vel_out = reinterpret_cast<float*>(vel_out_ptr);

  srand(0);
  for (uint i = 0; i < num_bodies; ++i) {
    int index = 4 * i;

    // First 3 values are position in x,y and z direction
    for (int j = 0; j < 3; ++j) {
      pos_in[index + j] = random(3, 50);
    }

    // Mass value
    pos_in[index + 3] = random(1, 1000);

    for (int j = 0; j < 4; ++j) {
      // init to 0
      vel_in[index + j] = 0.0f;
    }
  }

  auto lws = group_size;
  auto gws = num_bodies;

  auto sel_platform = PLATFORM;
  auto sel_device = tdevices == 0 ? 1 : 0; // invert, tdevices: 0 = CPU, 1 = GPU

  vector<char> kernel_bin;
  if (use_binaries) {
    switch (tdevices) {
      case 200:
        kernel_bin = file_read_binary("support/kernels/nbody_sapu:0:1.cl.bin");
        break;
      case 201:
        kernel_bin = file_read_binary("support/kernels/nbody_sapu:0:0.cl.bin");
        break;
      case 300:
        kernel_bin = file_read_binary("support/kernels/nbody_batel:1:0.cl.bin");
        break;
      case 301:
        kernel_bin = file_read_binary("support/kernels/nbody_batel:1:1.cl.bin");
        break;
      case 310:
        kernel_bin = file_read_binary("support/kernels/nbody_batel:0:0.cl.bin");
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
  vector<cl::Device>tmp_device(1,device);
  cl::Context context(tmp_device);

  cl::CommandQueue queue(context, device, 0, &cl_err);
  CL_CHECK_ERROR(cl_err, "CommandQueue queue");

  IF_LOGGING(cout << "initBuffers\n");

  cl_int buffer_in_flags = CL_MEM_READ_WRITE;
  cl_int buffer_out_flags = CL_MEM_READ_WRITE;

  size_t buffer_size = num_bodies * sizeof(cl_float4);

  cl::Buffer pos_in_buffer(context, buffer_in_flags, buffer_size, 0, &cl_err);
  CL_CHECK_ERROR(cl_err, "pos in1 buffer ");
  cl::Buffer pos_out_buffer(context, buffer_out_flags, buffer_size, 0, &cl_err);
  CL_CHECK_ERROR(cl_err, "pos out1 buffer ");
  cl::Buffer vel_in_buffer(context, buffer_in_flags, buffer_size, 0, &cl_err);
  CL_CHECK_ERROR(cl_err, "vel in1 buffer ");
  cl::Buffer vel_out_buffer(context, buffer_out_flags, buffer_size, 0, &cl_err);
  CL_CHECK_ERROR(cl_err, "vel out1 buffer ");

  IF_LOGGING(cout << "x\n");
  CL_CHECK_ERROR(
    queue.enqueueWriteBuffer(pos_in_buffer, CL_FALSE, 0, buffer_size, pos_in_ptr, NULL, NULL));

  CL_CHECK_ERROR(
    queue.enqueueWriteBuffer(vel_in_buffer, CL_FALSE, 0, buffer_size, vel_in_ptr, NULL, NULL));

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

  cl::Kernel kernel(program, "nbody_sim", &cl_err);

  cl_err = kernel.setArg(0, pos_in_buffer);
  CL_CHECK_ERROR(cl_err, "kernel arg 0 size");

  cl_err = kernel.setArg(1, vel_in_buffer);
  CL_CHECK_ERROR(cl_err, "kernel arg 1 in2 buffer");

  cl_err = kernel.setArg(2, num_bodies);
  CL_CHECK_ERROR(cl_err, "kernel arg 2 in1 buffer");

  cl_err = kernel.setArg(3, delT);
  CL_CHECK_ERROR(cl_err, "kernel arg 3 in2 buffer");

  cl_err = kernel.setArg(4, espSqr);
  CL_CHECK_ERROR(cl_err, "kernel arg 4 out buffer");

  cl_err = kernel.setArg(5, pos_out_buffer);
  CL_CHECK_ERROR(cl_err, "kernel arg 5 size");

  cl_err = kernel.setArg(6, vel_out_buffer);
  CL_CHECK_ERROR(cl_err, "kernel arg 6 size");

  // cl_int cl_err;
  // cl::UserEvent end(context, &cl_err);
  // CL_CHECK_ERROR(cl_err, "user event end");

  // cl::Event evkernel;
  if (ECL_LOGGING) {
    cout << "pos in:\n";
    for (uint i = 0; i < 10; ++i) {
      cout << pos_in_ptr[i] << " ";
    }
    cout << "\n";
    for (uint i = num_bodies - 10; i < num_bodies; ++i) {
      cout << pos_in_ptr[i] << " ";
    }
    cout << "\n";
    cout << "vel in:\n";
    for (uint i = 0; i < 10; ++i) {
      cout << vel_in_ptr[i] << " ";
    }
    cout << "\n";
    for (uint i = num_bodies - 10; i < num_bodies; ++i) {
      cout << vel_in_ptr[i] << " ";
    }
    cout << "\n";
  }

  auto offset = 0;
  queue.enqueueNDRangeKernel(
    kernel, cl::NDRange(offset), cl::NDRange(gws), cl::NDRange(lws), NULL, NULL);

  // cl::Event evread;
  // vector<cl::Event> events({ evkernel });

  CL_CHECK_ERROR(
    queue.enqueueReadBuffer(pos_out_buffer, CL_TRUE, 0, buffer_size, pos_out_ptr, NULL, NULL));
  CL_CHECK_ERROR(
    queue.enqueueReadBuffer(vel_out_buffer, CL_TRUE, 0, buffer_size, vel_out_ptr, NULL, NULL));

  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - time_init).count();

  cout << "time: " << diff_ms << "\n";
  // auto in1 = *a_array.get();
  // auto in2 = *b_array.get();
  // auto out = *c_array.get();

  // auto img = write_bmp_file(in1.data(), image_width, image_height, "g_base.bmp");
  // IF_LOGGING(cout << img << "\n");
  // img = write_bmp_file(out.data(), image_width, image_height, "gf_base.bmp");
  // IF_LOGGING(cout << img << "\n");

  if (ECL_LOGGING) {
    cout << "pos out:\n";
    for (uint i = 0; i < 10; ++i) {
      cout << pos_out_ptr[i] << " ";
    }
    cout << "\n";
    for (uint i = num_bodies - 10; i < num_bodies; ++i) {
      cout << pos_out_ptr[i] << " ";
    }
    cout << "\n";
    cout << "vel out:\n";
    for (uint i = 0; i < 10; ++i) {
      cout << vel_out_ptr[i] << " ";
    }
    cout << "\n";
    for (uint i = num_bodies - 10; i < num_bodies; ++i) {
      cout << vel_out_ptr[i] << " ";
    }
    cout << "\n";
  }
  if (check) {
    // pos_in_ptr[3].s[0] = 0.33f;

    auto threshold = 0.001f;
    auto ok = do_nbody_check(num_bodies, delT, espSqr, pos_in, vel_in, pos_out, vel_out, threshold);

    if (ok) {
      success(diff_ms);
    } else {
      failure(diff_ms);
    }
  } else {
    cout << "Done\n";
  }
  exit(0);
}

void
do_nbody(int tscheduler,
         int tdevices,
         uint check,
         uint num_particles,
         int chunksize,
         vector<float>& props)
{
  bool use_binaries = (check >= 10) ? true : false;
  check = (check >= 10) ? check - 10 : check;

  auto group_size = GROUP_SIZE;

  cl_float delT = DEL_T;
  cl_float espSqr = ESP_SQR;

  int worksize = chunksize;

  string kernel_str = file_read("support/kernels/nbody.cl");

  num_particles = (uint)(((size_t)num_particles < group_size) ? group_size : num_particles);
  num_particles = (uint)((num_particles / group_size) * group_size);

  uint num_bodies = num_particles;

  auto pos_in_array = make_shared<vector<cl_float4>>(num_bodies);
  auto vel_in_array = make_shared<vector<cl_float4>>(num_bodies);
  auto pos_out_array = make_shared<vector<cl_float4>>(num_bodies);
  auto vel_out_array = make_shared<vector<cl_float4>>(num_bodies);

  // vector<cl_float4> pos_in_ptr = *pos_in_array.get();
  // vector<cl_float4> vel_in_ptr = *vel_in_array.get();
  // vector<cl_float4> pos_out_ptr = *pos_out_array.get();
  // vector<cl_float4> vel_out_ptr = *vel_out_array.get();

  cl_float4* pos_in_ptr = reinterpret_cast<cl_float4*>(pos_in_array.get()->data());
  cl_float4* vel_in_ptr = reinterpret_cast<cl_float4*>(vel_in_array.get()->data());
  cl_float4* pos_out_ptr = reinterpret_cast<cl_float4*>(pos_out_array.get()->data());
  cl_float4* vel_out_ptr = reinterpret_cast<cl_float4*>(vel_out_array.get()->data());

  float* pos_in = reinterpret_cast<float*>(pos_in_ptr);
  float* vel_in = reinterpret_cast<float*>(vel_in_ptr);
  float* pos_out = reinterpret_cast<float*>(pos_out_ptr);
  float* vel_out = reinterpret_cast<float*>(vel_out_ptr);

  srand(0);
  for (uint i = 0; i < num_bodies; ++i) {
    int index = 4 * i;

    // First 3 values are position in x,y and z direction
    for (int j = 0; j < 3; ++j) {
      pos_in[index + j] = random(3, 50);
    }

    // Mass value
    pos_in[index + 3] = random(1, 1000);

    for (int j = 0; j < 4; ++j) {
      // init to 0
      vel_in[index + j] = 0.0f;
    }
  }

  if (ECL_LOGGING) {
    cout << "pos in:\n";
    for (uint i = 0; i < 10; ++i) {
      cout << pos_in_ptr[i] << " ";
    }
    cout << "\n";
    for (uint i = num_bodies - 10; i < num_bodies; ++i) {
      cout << pos_in_ptr[i] << " ";
    }
    cout << "\n";
    cout << "vel in:\n";
    for (uint i = 0; i < 10; ++i) {
      cout << vel_in_ptr[i] << " ";
    }
    cout << "\n";
    for (uint i = num_bodies - 10; i < num_bodies; ++i) {
      cout << vel_in_ptr[i] << " ";
    }
    cout << "\n";
  }

  auto lws = group_size;
  auto gws = num_bodies;

  auto platform = PLATFORM;

  vector<char> kernel_bin;
  if (use_binaries) {
    switch (tdevices) {
      case 200:
        kernel_bin = file_read_binary("support/kernels/nbody_sapu:0:1.cl.bin");
        break;
      case 201:
        kernel_bin = file_read_binary("support/kernels/nbody_sapu:0:0.cl.bin");
        break;
      case 300:
        kernel_bin = file_read_binary("support/kernels/nbody_batel:1:0.cl.bin");
        break;
      case 301:
        kernel_bin = file_read_binary("support/kernels/nbody_batel:1:1.cl.bin");
        break;
      case 310:
        kernel_bin = file_read_binary("support/kernels/nbody_batel:0:0.cl.bin");
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

  ecl::Runtime runtime(move(devices), gws, lws);
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
  runtime.setInBuffer(pos_in_array);
  runtime.setInBuffer(vel_in_array);
  runtime.setOutBuffer(pos_out_array);
  runtime.setOutBuffer(vel_out_array);
  runtime.setKernel(kernel_str, "nbody_sim");

  runtime.setKernelArg(0, pos_in_array);
  runtime.setKernelArg(1, vel_in_array);
  runtime.setKernelArg(2, num_bodies);
  runtime.setKernelArg(3, delT);
  runtime.setKernelArg(4, espSqr);
  runtime.setKernelArg(5, pos_out_array);
  runtime.setKernelArg(6, vel_out_array);

  runtime.run();

  auto t2 = std::chrono::system_clock::now().time_since_epoch();
  size_t diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - time_init).count();

  cout << "time: " << diff_ms << "\n";

  runtime.printStats();

  // auto ptr = reinterpret_cast<cl_uchar4*>(b.data());
  // IF_LOGGING(cout << ptr[0].s[0] << "\n");
  // IF_LOGGING(cout << ptr[121].s[0] << "\n");
  // auto pos_in_ptr = *pos_in_array.get();
  // auto vel_in_ptr = *vel_in_array.get();
  // auto pos_out_ptr = *pos_out_array.get();
  // auto vel_out_ptr = *vel_out_array.get();

  if (ECL_LOGGING) {
    cout << "pos out:\n";
    for (uint i = 0; i < 10; ++i) {
      cout << pos_out_ptr[i] << " ";
    }
    cout << "\n";
    for (uint i = num_bodies - 10; i < num_bodies; ++i) {
      cout << pos_out_ptr[i] << " ";
    }
    cout << "\n";
    cout << "vel out:\n";
    for (uint i = 0; i < 10; ++i) {
      cout << vel_out_ptr[i] << " ";
    }
    cout << "\n";
    for (uint i = num_bodies - 10; i < num_bodies; ++i) {
      cout << vel_out_ptr[i] << " ";
    }
    cout << "\n";
  }

  if (check) {
    // out[33] = 0;
    // vector<int> steps = {0,1,2,9,10,11,121,122,123};
    // for (auto i : steps){
    //   IF_LOGGING(cout << "in1[" << i << "]: " << in1[i] << "\n");
    //   IF_LOGGING(cout << "in2[" << i << "]: " << in2[i] << "\n");
    //   IF_LOGGING(cout << "out[" << i << "]: " << out[i] << "\n");
    // }
    auto threshold = 0.001f;
    auto ok = do_nbody_check(num_bodies, delT, espSqr, pos_in, vel_in, pos_out, vel_out, threshold);

    if (ok) {
      success(diff_ms);
    } else {
      failure(diff_ms);
    }
  } else {
    cout << "Done\n";
  }

  exit(0);
}
