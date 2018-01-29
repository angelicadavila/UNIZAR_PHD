#include "vecadd.hpp"

auto
check_vecadd(vector<int> in1, vector<int> in2, vector<int> out, int size)
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
do_vecadd(int tscheduler, int tdevices, bool check, int wsize, int chunksize, float prop)
{
  string kernel = R"(
__kernel void
#if CL_SUPPORT_KERNEL_OFFSET == 1
vecadd(__global int* in1, __global int* in2, __global int* out, int size){
  int idx = get_global_id(0);
#else
vecadd(__global int* in1, __global int* in2, __global int* out, int size, uint offset){
  int idx = get_global_id(0) + offset;
#endif
  if (idx >= 0 && idx < size){
    out[idx] = in1[idx] + in2[idx];
  }
}
)";

  string kernelPlain1 = R"(
__kernel void
#if CL_SUPPORT_KERNEL_OFFSET == 1
vecadd(__global int* in1, __global int* in2, __global int* out, int size){
  int idx = get_global_id(0);
#else
vecadd(__global int* in1, __global int* in2, __global int* out, int size, uint offset){
  int idx = get_global_id(0) + offset;
#endif
  if (idx >= 0 && idx < size){
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

  auto in1_array = make_shared<vector<int>>(size, 1);
  auto in2_array = make_shared<vector<int>>(size, 2);
  auto out_array = make_shared<vector<int>>(size, 0);

  vector<clb::Device> devices;
  auto platform = 0;
  if (tdevices == 0) { // CPU
    clb::Device device(platform, 1);
    // device.setKernel(kernelPlain1);
    device.setKernel(kernelBin1);
    devices.push_back(move(device));
  } else if (tdevices == 1) { // GPU
    clb::Device device2(platform, 0);
    device2.setKernel(kernelBin0);
    devices.push_back(move(device2));
  } else { // CPU + GPU
    clb::Device device(platform, 1);
    clb::Device device2(platform, 0);
    // device.setKernel(kernel2, false);
    device.setKernel(kernelBin1);
    device2.setKernel(kernelBin0);
    devices.push_back(move(device));
    devices.push_back(move(device2));
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
  runtime.setKernel(kernel, "vecadd");

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
      cout << "Success (" << time << ")\n";
    } else {
      cout << "Failure (" << time << " in pos " << pos << ")\n";
    }
  } else {
    cout << "Done\n";
  }
}
