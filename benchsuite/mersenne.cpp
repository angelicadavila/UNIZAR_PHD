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

/*    unsigned int state = 777;
    uint ival[VECTOR];
    #pragma unroll VECTOR
    for (int i=0; i<64; i++) {
       ival[i] = 777;
    }
    for (unsigned int n=0; n<MT_N; n++) {
       #pragma unroll
       for (int i=0; i<VECTOR-1; i++) {
          ival[i] = ival[i+1];
       }
       ival[VECTOR-1] = state;
       state = (1812433253U * (state ^ (state >> 30)) + n) & 0xffffffffUL;
       if ((n & (VECTOR-1)) == 47) {
          vec_uint_ty I0, I1, I2, I3;
          #pragma unroll VECTOR_DIV4
          for (int i=0; i<VECTOR_DIV4; i++) {
             I0[i]=ival[i];
             I1[i]=ival[i+1*VECTOR_DIV4];
             I2[i]=ival[i+2*VECTOR_DIV4];
             I3[i]=ival[i+3*VECTOR_DIV4];
          }
       }
    }
*/
}


string
Mersenne::get_kernel_str()
{
   return "";
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
 auto output = shared_ptr<vector<float,vecAllocator<float>>>(&mersenne._out);
#pragma GCC diagnostic pop
  
  int problem_size = mersenne._total_size/64;

  vector<ecl::Device> devices;

  auto platform_cpu = 2;
  auto platform_gpu = 0;
  auto platform_fpga= 1;
  
 // Mersenne twister is a unitary kernel pipeline or Task execution

  vector <char> binary_file;
  vector <size_t>gws=vector <size_t>(3,1);
  if (tdevices &0x02){  
    ecl::Device device2(platform_fpga,0);
    binary_file	=file_read_binary("./benchsuite/altera_kernel/mersenne_kernel_fpr.aocx"); 
    device2.setKernel(binary_file,gws,gws); 
    devices.push_back(move(device2));
  }
  if (tdevices &0x04){  
    ecl::Device device(platform_cpu,0);
    device.setKernel(kernel,gws,gws);
    devices.push_back(move(device));
  }
  if (tdevices &0x01){  
    ecl::Device device1(platform_gpu,0);
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
    stSched.setRawProportions({ prop });
  } else if (tscheduler == 1) {
    runtime.setScheduler(&dynSched);
    dynSched.setWorkSize(worksize);
  } else { // tscheduler == 2
    runtime.setScheduler(&hgSched);
    hgSched.setWorkSize(worksize);
   hgSched.setRawProportions({0.9, 0.2});
  }
  runtime.setInBuffer(input);
  runtime.setOutBuffer(output);
  //Mersenne its specialized in every device gws(1,1,1) lws(1,1,1)
  runtime.setKernel(kernel, "mersenne_twister_generate");//the default is gws(0,1,1) when gws[0]=chunk_size. lws(128,1,1)

  runtime.setKernelArg(0, input);//in
  runtime.setKernelArg(1, output);//out
  
  runtime.setInternalChunk(64);
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
      for(int dat=0; dat<(int)out.size(); dat++)
      		myfile<<out[dat]<<"\n";
 			myfile.close();

  } else {
    cout << "Done mersenne Twister\n";
  }

  exit(0);
}
