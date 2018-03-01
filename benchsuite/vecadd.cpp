#include "vecadd.hpp"

auto
check_vecadd(vector<int,vecAllocator<int>> in1, vector<int,vecAllocator<int>> in2, vector<int,vecAllocator<int>> out, int size)
{
  auto pos = -1;
  for (auto i = 0; i < size; ++i) {
    if ((in1[i] + in2[i]) != out[i]) {
      pos = i;
      break;
    }
  }
  return pos;
}

void
do_vecadd_base(bool check, int wsize)
{
  string m_kernel_str = R"(
__kernel void
#if CLB_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED == 1
vecadd_kernel(__global int* in1, __global int* in2, __global int* out, int size){
  int idx = get_global_id(0);
#else
vecadd(__global int* in1, __global int* in2, __global int* out, int size, uint offset){
  int idx = get_global_id(0) + offset;
#endif
  if (idx < size){
    out[idx] = in1[idx] + in2[idx];
  }
}
)";

  int size = 128 * wsize;
  // int worksize = 128 * chunksize;

  auto in1_array = make_shared<vector<int,vecAllocator<int>>>(size, 1);
  auto in2_array = make_shared<vector<int,vecAllocator<int>>>(size, 2);
  auto out_array = make_shared<vector<int,vecAllocator<int>>>(size, 0);

  auto sel_platform = 0;
  auto sel_device = 0;

  vector<cl::Platform> m_platforms;
  vector<vector<cl::Device>> m_platform_devices;
  cl::Device m_device;

  cout << "discoverDevices\n";
  cl::Platform::get(&m_platforms);
  cout << "platforms: " << m_platforms.size() << "\n";
  auto i = 0;
  for (auto& platform : m_platforms) {
    vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
    cout << "platform: " << i++ << " devices: " << devices.size() << "\n";
    m_platform_devices.push_back(move(devices));
  }

  auto last_platform = m_platforms.size() - 1;
  if (sel_platform > last_platform) {
    throw runtime_error("invalid platform selected");
  }

  auto last_device = m_platform_devices[sel_platform].size() - 1;
  if (sel_device > last_device) {
    throw runtime_error("invalid device selected");
  }

  m_device = move(m_platform_devices[sel_platform][sel_device]);

  cl_int cl_err = CL_SUCCESS;
  cl::Context m_context(m_device);

  cl::CommandQueue m_queue(m_context, m_device, 0, &cl_err);
  CL_CHECK_ERROR(cl_err, "CommandQueue queue");

  cout << "initBuffers\n";

  cl_int buffer_in_flags = CL_MEM_READ_WRITE;
  cl_int buffer_out_flags = CL_MEM_READ_WRITE;

  cl::Buffer in1_buffer(m_context, buffer_in_flags, sizeof(int) * in1_array.get()->size(), NULL);
  CL_CHECK_ERROR(cl_err, "in1 buffer ");
  cl::Buffer in2_buffer(m_context, buffer_in_flags, sizeof(int) * in2_array.get()->size(), NULL);
  CL_CHECK_ERROR(cl_err, "in2 buffer ");
  cl::Buffer out_buffer(m_context, buffer_out_flags, sizeof(int) * out_array.get()->size(), NULL);
  CL_CHECK_ERROR(cl_err, "out buffer ");

  CL_CHECK_ERROR(m_queue.enqueueWriteBuffer(
    in1_buffer, CL_FALSE, 0, sizeof(int) * in1_array.get()->size(), in1_array.get()->data(), NULL));

  CL_CHECK_ERROR(m_queue.enqueueWriteBuffer(
    in2_buffer, CL_FALSE, 0, sizeof(int) * in2_array.get()->size(), in2_array.get()->data(), NULL));

  cout << "initKernel\n";

  cl::Program::Sources sources;
  cl::Program::Binaries binaries;

  sources.push_back({ m_kernel_str.c_str(), m_kernel_str.length() });
  cl::Program m_program(m_context, sources);

  string options;
  options.reserve(32);
  options += "-DCLB_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED=" +
             to_string(CLB_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED);

  cl_err = m_program.build({ m_device }, options.c_str());
  if (cl_err != CL_SUCCESS) {
    cout << " Error building: " << m_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_device) << "\n";
    CL_CHECK_ERROR(cl_err);
  }

  cl::Kernel m_kernel(m_program, "vecadd_kernel", &cl_err);
  CL_CHECK_ERROR(cl_err, "kernel");

  cl_err = m_kernel.setArg(0, in1_buffer);
  CL_CHECK_ERROR(cl_err, "kernel arg 0 in1 buffer");

  cl_err = m_kernel.setArg(1, in2_buffer);
  CL_CHECK_ERROR(cl_err, "kernel arg 1 in2 buffer");

  cl_err = m_kernel.setArg(2, out_buffer);
  CL_CHECK_ERROR(cl_err, "kernel arg 2 out buffer");

  cl_err = m_kernel.setArg(3, size);
  CL_CHECK_ERROR(cl_err, "kernel arg 3 size");

  // cl_int cl_err;
  cl::UserEvent end(m_context, &cl_err);
  CL_CHECK_ERROR(cl_err, "user event end");

  cl::Event evkernel;

  auto lws = 128;

  auto offset = 0;
  auto gws = size;
  m_queue.enqueueNDRangeKernel(
    m_kernel, cl::NDRange(offset), cl::NDRange(gws), cl::NDRange(lws), NULL, NULL);

  cl::Event evread;
  vector<cl::Event> events({ evkernel });

  m_queue.enqueueReadBuffer(out_buffer, CL_TRUE, 0, sizeof(int) * size, out_array.get()->data());

  if (check) {
    auto in1 = *in1_array.get();
    auto in2 = *in2_array.get();
    auto out = *out_array.get();

    auto pos = check_vecadd(in1, in2, out, size);
    auto ok = pos == -1;

    auto time = 0;
    if (ok) {
      cout << "Success (" << time << ")\n";
    } else {
      cout << "Failure (" << time << " in pos " << pos << ")\n";
    }
  } else {
    cout << "Done\n";
  }
}

void
do_vecadd(int tscheduler, int tdevices, bool check, int wsize, int chunksize, float prop)
{
  string kernel = R"(
__kernel void
#if CLB_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED == 1
vecadd_kernel(__global int* in1, __global int* in2, __global int* out, int size){
  int idx = get_global_id(0);
#else
vecadd_kernel(__global int* in1, __global int* in2, __global int* out, int size, uint offset){
  int idx = get_global_id(0) + offset;
#endif
  if (idx < size){
    out[idx] = in1[idx] + in2[idx];
  }
}
)";

  string kernelPlain1 = R"(
__kernel void
#if CLB_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED == 1
vecadd(__global int* in1, __global int* in2, __global int* out, int size){
  int idx = get_global_id(0);
#else
vecadd(__global int* in1, __global int* in2, __global int* out, int size, uint offset){
  int idx = get_global_id(0) + offset;
#endif
  if (idx < size){
    out[idx] = in1[idx] + in2[idx];
  }
}
)";

  string kernel1_file = "support/kernels/vecadd_offset_station:0:1.cl.bin";
  // vector<char> kernel1_bin = get_file_binary_contents(kernel1_file);
  string kernel0_file = "support/kernels/vecadd_offset_station:0:0.cl.bin";
  // vector<char> kernel0_bin = get_file_binary_contents(kernel0_file);

  vector<char> kernelBin1;
  vector<char> kernelBin0;
  string plain;
  try {
    // auto binary = file_read_binary("vecadd_offset_lhp:0:0.cl.bin");
    kernelBin1 = file_read_binary(kernel1_file);
    kernelBin0 = file_read_binary(kernel0_file);
    // plain = file_read("vecadd_offset.cl");
    // plain = file_read("support/kernels/vecadd_offset.cl");
  } catch (std::ios::failure& e) {
    cout << "io failure: " << e.what() << "\n";
  }

  // cout << plain << "\n";

  // cout << "kernelBin1: " << kernelBin1.size() << "\n";
  // cout << "kernelBin0: " << kernelBin0.size() << "\n";
  // cout << (int)kernelBin1[0] << "\n";
  // cout << (int)kernelBin0[0] << "\n";
  // cout << (int)kernelBin1[1] << "\n";
  // cout << (int)kernelBin0[1] << "\n";
  // // return;

  int size = 128 * wsize;
  int worksize = 128 * chunksize;

  auto in1_array = make_shared<vector<int,vecAllocator<int>>>(size, 1);
  auto in2_array = make_shared<vector<int,vecAllocator<int>>>(size, 2);
  auto out_array = make_shared<vector<int,vecAllocator<int>>>(size, 0);

  vector<clb::Device> devices;
  int platform_fpga=2;
  int platform_cpu=0;
  int platform_gpu=1;  
  vector <char> binary_file;
  if (tdevices &0x04){  
    clb::Device device2(platform_fpga,0);
    //available kernes:   vecadd_l    -one compute unit
    //                    vecadd_cu16 -16 compute units. performance aprox. 1/2 cpu
    //                    vecadd_cu32 -32 compute units
    binary_file =file_read_binary("./benchsuite/altera_kernel/vecadd_l.aocx");
    device2.setKernel(binary_file);
    devices.push_back(move(device2));
  }

  if (tdevices &0x01){  
    clb::Device device(platform_cpu,0);
     devices.push_back(move(device));
  }
  if (tdevices &0x02){  
    clb::Device device1(platform_gpu,0);
    devices.push_back(move(device1));
  }

  clb::StaticScheduler stSched;
  clb::DynamicScheduler dynSched;
  clb::HGuidedScheduler hgSched;

  clb::Runtime runtime(move(devices), size);
  if (tscheduler == 0) {
    runtime.setScheduler(&stSched);
    stSched.setRawProportions({ prop });
  } else if (tscheduler == 1) {
    runtime.setScheduler(&dynSched);
    dynSched.setWorkSize(worksize);
  } else { // tscheduler == 2
    runtime.setScheduler(&hgSched);
    hgSched.setWorkSize(worksize);
    hgSched.setRawProportions({ prop });
  }
  runtime.setInBuffer(in1_array);
  runtime.setInBuffer(in2_array);
  runtime.setOutBuffer(out_array);
  runtime.setKernel(kernel, "vecadd_kernel");

  runtime.setKernelArg(0, in1_array);
  runtime.setKernelArg(1, in2_array);
  runtime.setKernelArg(2, out_array);
  runtime.setKernelArg(3, size);

  runtime.run();

  runtime.printStats();

  if (check) {
    auto in1 = *in1_array.get();
    auto in2 = *in2_array.get();
    auto out = *out_array.get();

    auto pos = check_vecadd(in1, in2, out, size);
    auto ok = pos == -1;

    auto time = 0;
    if (ok) {
      cout << "Success Vecadd2 (" << time << ")\n";
    } else {
      cout << "Failure (" << time << " in pos " << pos << ")\n";
    }
  } else {
    cout << "Done\n";
  }

  
}
