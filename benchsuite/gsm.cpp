// This version works with 160 Samples per chunk
//Algorithm base in CHO gsm comunication
#include "gsm.hpp"
#include "parse_ppm.hpp"
#include <iostream>
#include <fstream>

void
Gsm::init_voice()
{
//sample vector example that its copied to evaluate the benchmark
 short inData[] =
  { 81, 10854, 1893, -10291, 7614, 29718, 20475, -29215, -18949, -29806,
  -32017, 1596, 15744, -3088, -17413, -22123, 6798, -13276, 3819, -16273,
    -1573, -12523, -27103,
  -193, -25588, 4698, -30436, 15264, -1393, 11418, 11370, 4986, 7869, -1903,
    9123, -31726,
  -25237, -14155, 17982, 32427, -12439, -15931, -21622, 7896, 1689, 28113,
    3615, 22131, -5572,
  -20110, 12387, 9177, -24544, 12480, 21546, -17842, -13645, 20277, 9987,
    17652, -11464, -17326,
  -10552, -27100, 207, 27612, 2517, 7167, -29734, -22441, 30039, -2368, 12813,
    300, -25555, 9087,
  29022, -6559, -20311, -14347, -7555, -21709, -3676, -30082, -3190, -30979,
    8580, 27126, 3414,
  -4603, -22303, -17143, 13788, -1096, -14617, 22071, -13552, 32646, 16689,
    -8473, -12733, 10503,
  20745, 6696, -26842, -31015, 3792, -19864, -20431, -30307, 32421, -13237,
    9006, 18249, 2403,
  -7996, -14827, -5860, 7122, 29817, -31894, 17955, 28836, -31297, 31821,
    -27502, 12276, -5587,
  -22105, 9192, -22549, 15675, -12265, 7212, -23749, -12856, -5857, 7521,
    17349, 13773, -3091,
  -17812, -9655, 26667, 7902, 2487, 3177, 29412, -20224, -2776, 24084, -7963,
    -10438, -11938,
  -14833, -6658, 32058, 4020, 10461, 15159
};
// for(int i=0; i<_total_size; i++)
  vector <short>_input(inData,inData+160);
}

string
Gsm::get_kernel_str()
{
   return 0;
}

// Dump frame data in PPM format.
void 
Gsm::outVoice(unsigned *frameData) {


}


void
do_gsm(int tscheduler,
            int tdevices,
            bool check,
            uint in_size,
            int chunksize,
            float prop
           )
{

  int worksize = chunksize;

  Gsm gsm(in_size);//+256

    string kernel = file_read("support/kernels/gsm.cl");

//#pragma GCC diagnostic ignored "-Wignored-attributes"
 auto input = shared_ptr<vector<short,vecAllocator<short>>>(&gsm._input);
 auto output1 = shared_ptr<vector<short,vecAllocator<short>>>(&gsm._out1);
 auto output2 = shared_ptr<vector<short,vecAllocator<short>>>(&gsm._out2);
//#pragma GCC diagnostic pop
  
  int problem_size =in_size;//gsm._total_size;
cout<<"problem_size"<<problem_size<<"\n";
cout<<"vector input size"<<gsm._total_size<<"\n";
cout<<"type data"<<sizeof(short)<<"\n";
  vector<ecl::Device> devices;

  auto platform_cpu = 2;
  auto platform_gpu = 0;
  auto platform_fpga= 1;

  vector <char> binary_file;
  if (tdevices &0x02){  
    ecl::Device device2(platform_fpga,0);
    binary_file	=file_read_binary("./benchsuite/altera_kernel/gsm.aocx");
    //vector of kernel dimension. Task Kernel gws==lws
    vector <size_t>gws=vector <size_t>(3,1);
    device2.setKernel(binary_file,gws,gws); 
    devices.push_back(move(device2));
  }

  if (tdevices &0x04){  
    ecl::Device device(platform_cpu,0);
  
    vector <size_t>gws=vector <size_t>(3,1);
    device.setKernel(kernel,gws,gws); 
    devices.push_back(move(device));
  }
  if (tdevices &0x01){  
    ecl::Device device1(platform_gpu,0);
    vector <size_t>gws=vector <size_t>(3,1);
    device1.setKernel(kernel,gws,gws); 
    devices.push_back(move(device1));
  }

  ecl::StaticScheduler stSched;
  ecl::DynamicScheduler dynSched;
  ecl::HGuidedScheduler hgSched;
  
  cout<<"Manual proportions!";
  
  ecl::Runtime runtime(move(devices), problem_size);
  if (tscheduler == 0) {
    runtime.setScheduler(&stSched);
    stSched.setRawProportions({ prop, 0.26 });
  } else if (tscheduler == 1) {
    runtime.setScheduler(&dynSched);
    dynSched.setWorkSize(worksize);
  } else { // tscheduler == 2
    runtime.setScheduler(&hgSched);
    hgSched.setWorkSize(worksize);
   hgSched.setRawProportions({0.3,0.6});
  }
  runtime.setInBuffer(input);
  runtime.setOutBuffer(output1);
  runtime.setOutBuffer(output2);
  runtime.setKernel(kernel, "gsm_main");
  runtime.setKernelArg(0, input);//in
  runtime.setKernelArg(1, output1);//out
  runtime.setKernelArg(2, output2);//LARs
  //works with 1 per chunk in this version 
  runtime.setInternalChunk(160); //procesing the size of watermark
  runtime.run();

  runtime.printStats();

  if (check) {
//    auto out = *output.get();
 //   gsm.outFrame((unsigned int*)out.data());

  } else {
    cout << "Done\n";
  }

  exit(0);
}



