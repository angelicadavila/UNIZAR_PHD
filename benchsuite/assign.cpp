#include "assign.hpp"

// #include "runtime.hpp"
// #include "scheduler.hpp"
// #include "device.hpp"

auto
check_assign(vector<int> in1, vector<int> out, int size)
{
  auto pos = -1;
  for (auto i = 0; i < size; ++i) {
    if (in1[i] != out[i]) {
      pos = i;
      break;
    }
  }
  return pos;
}

void
do_assign(int tscheduler, int tdevices, bool check, int wsize, int chunksize, float prop)
{
  string kernel = R"(
__kernel void
assign(__global int* in1, __global int* out, int size){
  int idx = get_global_id(0);
  if (idx >= 0 && idx < size){
    out[idx] = in1[idx];
  }
}
)";

  int size = 128 * wsize;
  int worksize = 128 * chunksize;
  // int size = 128 * 1000000;
  // auto worksize = 128 * 100000;
  // int size = 256;
  auto in1_array = make_shared<vector<int>>(size, 1);
  auto out_array = make_shared<vector<int>>(size, 0);

  // clb::Buffer b1;
  // b1.set(in1_array);
  // void* ptr = b1.get();
  // cout << "len: " << b1.size() << "\n";
  // cout << "bytes: " << b1.bytes() << "\n";
  // int* iptr = reinterpret_cast<int*>(ptr);
  // cout << "[0] " << iptr[0] << "\n";
  // cout << "[1] " << iptr[1] << "\n";
  // iptr[1] = 22;
  // cout << "[1] " << iptr[1] << "\n";

  vector<clb::Device> devices;
  // if (tdevices == 0) {
  //   clb::Device device(1, 1);
  //   devices.push_back(move(device));
  // } else if (tdevices == 1) {
  //   clb::Device device2(1, 0);
  //   devices.push_back(move(device2));
  // } else {
  //   clb::Device device(1, 1);
  //   clb::Device device2(1, 0);
  //   devices.push_back(move(device));
  //   devices.push_back(move(device2));
  // }
  auto platform = 0;
  if (tdevices == 0) {
    clb::Device device(platform, 1,0);
    devices.push_back(move(device));
  } else if (tdevices == 1) {
    clb::Device device2(platform, 0,0);
    devices.push_back(move(device2));
  } else {
    clb::Device device(platform, 1,0);
    clb::Device device2(platform, 0,0);
    devices.push_back(move(device));
    devices.push_back(move(device2));
  }
  // clb::Scheduler scheduler;
  // clb::StaticScheduler scheduler;
  clb::StaticScheduler stSched;
  clb::DynamicScheduler dynSched;
  clb::HGuidedScheduler hgSched;
  // clb::Runtime runtime(move(vector<clb::Device>{device, device2}),
  // scheduler); clb::Runtime runtime(move(vector<clb::Device>{{
  //                                                clb::Device(0, 1, 0, size /
  //                                                2 ), clb::Device(0, 0, size
  //                                                / 2, size / 2)
  //       }}), scheduler);
  // clb::Runtime runtime({device, device2}, scheduler);
  // clb::Runtime runtime(device, device2, scheduler);
  clb::Runtime runtime(move(devices), size);
  if (tscheduler == 0) {
    runtime.setScheduler(&stSched);
    // vector<float> props;
    // stSched.setRawProportions({0.45});
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
  runtime.setOutBuffer(out_array);
  runtime.setKernel(kernel, "assign");

  runtime.setKernelArg(0, in1_array);
  runtime.setKernelArg(1, out_array);
  runtime.setKernelArg(2, size);

  runtime.run();

  runtime.printStats();

  if (check) {
    auto in1 = *in1_array.get();
    auto out = *out_array.get();

    // out[33] = 0;
    // cout << "9: " << in[9] << "\n";
    // cout << "9: " << out[9] << "\n";

    auto pos = check_assign(in1, out, size);
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
