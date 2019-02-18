#include "mersenne.hpp"
#include <iostream>
#include <fstream>

#define FRAMES  10
void
Mersenne::init_seed()
{
  srand(0);
  for (auto i=0 ; i<MT_N; i++){
     _input_seed[i]=rand()+2;
     _out[i]=0;
  }

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
            float prop,
            float prop2
           )
{

  int worksize = chunksize;
  //JSC
  //
  size_t frames=4;
//  size_t frames=1;
  size_t dim=static_cast<int>(N_rand);
  Mersenne mersenne(N_rand*64*frames);

  //string kernel = file_read("support/kernels/mersenne_kernel.cl");
  string kernel = file_read("support/kernels/mersenne_kernel_box.cl");
//#pragma GCC diagnostic ignored "-Wignored-attributes"
 auto input = shared_ptr<vector<int,vecAllocator<int>>>(&mersenne._input_seed);
 auto output = shared_ptr<vector<float,vecAllocator<float>>>(&mersenne._out);
 auto output_aux = shared_ptr<vector<float,vecAllocator<float>>>(&mersenne._out_aux);
//#pragma GCC diagnostic pop
  //size_t problem_size =(mersenne._total_size/(size_t)64)*frames*FRAMES;
  size_t problem_size =dim*frames*FRAMES;
  
  vector<ecl::Device> devices;

  auto platform_cpu = ECL_CPU;
  auto platform_gpu = ECL_GPU;
  auto platform_fpga= ECL_FPGA;
  auto cmp_cpu =0x01;  
  auto cmp_gpu =0x02;  
  auto cmp_fpga=0x04;  

  auto num_in_buff=2;

 // Mersenne twister is a unitary kernel pipeline or Task execution

  vector <char> binary_file;
  vector <size_t>gws=vector <size_t>(3,1);
  if (tdevices &cmp_fpga){  
    ecl::Device device2(platform_fpga,0);
    //binary_file	=file_read_binary("./benchsuite/altera_kernel/mersenne_kernel_fpr.aocx"); 
    binary_file	=file_read_binary("./benchsuite/altera_kernel/mersenne_kernel_box.aocx"); 
    device2.setKernel(binary_file,gws,gws); 
    size_t mem_lim=static_cast<size_t>(ECL_mem_FPGA)*1000000/num_in_buff;
    device2.setLimMemory(mem_lim);
    devices.push_back(move(device2));
  }
  if (tdevices &cmp_cpu){  
    ecl::Device device(platform_cpu,0);
    device.setKernel(kernel,gws,gws);
    size_t mem_lim=static_cast<size_t>(ECL_mem_CPU)*1000000/num_in_buff;
    device.setLimMemory (mem_lim);
    devices.push_back(move(device));
  }
  if (tdevices &cmp_gpu){  
    ecl::Device device1(platform_gpu,0);
    device1.setKernel(kernel,gws,gws);
    size_t mem_lim=static_cast<size_t>(ECL_mem_GPU)*1000000/num_in_buff;
    device1.setLimMemory (mem_lim);
    devices.push_back(move(device1));
  }

  ecl::StaticLongScheduler stSched;
  ecl::DynamicScheduler dynSched;
  ecl::HGuidedScheduler hgSched;
cout<<"Manual proportions!";
  ecl::Runtime runtime(move(devices), problem_size);
  if (tscheduler == 0) {
    runtime.setScheduler(&stSched);
    //stSched.setRawProportions({ prop });
    //stSched.setRawProportions({ prop,20.0-(prop+prop2),prop2 });
    stSched.setRawProportions({ 0.4,0.55,0.05 });
  } else if (tscheduler == 1) {
    runtime.setScheduler(&dynSched);
    dynSched.setWorkSize(worksize,FRAMES);
  } else { // tscheduler == 2
    runtime.setScheduler(&hgSched);
    hgSched.setWorkSize(worksize);
   hgSched.setRawProportions({0.32, 0.62,0.06});
  }
  runtime.setInBuffer(input,1);
  runtime.setOutBuffer(output);
  runtime.setOutAuxBuffer(output_aux);
  //Mersenne its specialized in every device gws(1,1,1) lws(1,1,1)
  runtime.setKernel(kernel, "mersenne_twister_generate");//the default is gws(0,1,1) when gws[0]=chunk_size. lws(128,1,1)

  runtime.setKernelArg(0, input);//in
  runtime.setKernelArg(1, output);//out
  
  runtime.setInternalChunk(32);
  //runtime.setInternalChunk(64);
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
