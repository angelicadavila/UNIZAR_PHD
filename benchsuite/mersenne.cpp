#include "mersenne.hpp"
#include <iostream>
#include <fstream>

void
Mersenne::init_seed()
{
  srand(0);
  for (auto i=0 ; i<_total_size; i++){
     _input_seed[i]=rand();
     _out[i]=0;
  }
}


string
Mersenne::get_kernel_str()
{

}

void
do_mersenne(int tscheduler,
            int tdevices,
            bool check,
            uint N_rand,
            int chunksize,
            float prop
           )
{

  int worksize = chunksize;

  Mersenne mersenne(N_rand);

  string kernel = file_read("support/kernels/mersenne_kernel.cl");
#pragma GCC diagnostic ignored "-Wignored-attributes"
 auto input = shared_ptr<vector<int,vecAllocator<int>>>(&mersenne._input_seed);
 auto output = shared_ptr<vector<int,vecAllocator<int>>>(&mersenne._out);
#pragma GCC diagnostic pop
  
  int problem_size = mersenne._total_size/64;

  vector<clb::Device> devices;

  auto platform_cpu = 0;
  auto platform_gpu = 1;
  auto platform_fpga= 2;

  vector <char> binary_file;
  if (tdevices &0x04){  
    clb::Device device2(platform_fpga,0);
    binary_file	=file_read_binary("./benchsuite/mersenne_kernel.aocx"); 
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
cout<<"Manual proportions!";
  clb::Runtime runtime(move(devices), problem_size);
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
  runtime.setInBuffer(input);
  runtime.setOutBuffer(output);
  runtime.setKernel(kernel, "mersenne_twister_generate");

  runtime.setKernelArg(0, input);//in
  runtime.setKernelArg(1, output);//out

  runtime.run();

  runtime.printStats();

  if (check) {
    auto out = *output.get();

//    auto ok = mersenne.compare();

    auto time = 0;
/*    if (ok) {
      cout << "Success (" << time << ")\n";
    } else {
      cout << "Failure (" << time << ")\n";
    }
  */  //file to save and compare data results
      std::ofstream myfile;
      myfile.open ("mersenne.txt");
      for(int dat=0; dat<out.size(); dat++)
      		myfile<<out[dat]<<"\n";
 			myfile.close();

  } else {
    cout << "Done\n";
  }

  exit(0);
}
