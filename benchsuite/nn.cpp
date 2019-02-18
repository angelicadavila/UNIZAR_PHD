#include "nn.hpp"
#include <iostream>
#include <fstream>

#define GROUP 64
//#define FRAME 5
#define FRAME 10
//#define FRAME 20
void
Nearest::init_data(uint dim)
{
  srand(0);
  for (uint i=0 ; i<dim ; i++){
     _input_lat[i]=rand()*100;
  }
  
}


string
Nearest::get_kernel_str()
{
   return "";
}

void
do_nearest(int tscheduler,
            int tdevices,
            bool check,
            uint N_data,
            int chunksize,
            float prop,
            float prop2
           )
{
  
  //distance to..
  float lat=10.5;
  float lng=50.4;
  //size_t frame=4;
  size_t frame=4;
  N_data=49000;
  N_data=N_data*4096;
  int worksize = chunksize;
  Nearest nearest(N_data*frame);

  //string kernel = file_read("support/kernels/nearest_kernel_box.cl");
  string kernel = file_read("support/kernels/nn.cl");

//#pragma GCC diagnostic ignored "-Wignored-attributes"
 auto input = shared_ptr<vector<float,vecAllocator<float>>>(&nearest._input_lat);
 auto output = shared_ptr<vector<float,vecAllocator<float>>>(&nearest._distance);
 auto output_aux = shared_ptr<vector<float,vecAllocator<float>>>(&nearest._distance_aux);
//#pragma GCC diagnostic pop
  
  size_t problem_size = 4014080000;
  //size_t problem_size = 1003520000*frame;
  //size_t problem_size = N_data*frame;

//1949696000;
//1130496000;
 //nearest._total_size*FRAME*(2);
  cout<<"problem size"<<problem_size<<"\n";
  vector<ecl::Device> devices;
//exit(0);
  auto platform_cpu = ECL_CPU;
  auto platform_gpu = ECL_GPU;
  auto platform_fpga= ECL_FPGA;
  auto cmp_cpu =0x01;  
  auto cmp_gpu =0x02;  
  auto cmp_fpga=0x04;  

  auto num_in_buff=3;
 // Mersenne twister is a unitary kernel pipeline or Task execution

  vector <char> binary_file;
  vector <size_t>gws=vector <size_t>(3,1);
  gws[0]=GROUP;
 
 if (tdevices &cmp_fpga){  
    ecl::Device device2(platform_fpga,0);
    binary_file =file_read_binary("./benchsuite/altera_kernel/nn.aocx"); 
//    binary_file =file_read_binary("./benchsuite/altera_kernel/nn_prof.aocx"); 
    device2.setKernel(binary_file,gws,gws);
    size_t mem_lim=static_cast<size_t>(ECL_mem_FPGA)*1000000/num_in_buff; 
    cout<<"mem_lim "<< mem_lim<<"\n";
    mem_lim=ceil(mem_lim/401536)*401536;
    cout<<"mem_lim round "<< mem_lim<<"\n";
    //exit(0);
    device2.setLimMemory(mem_lim);
   	//device2.setChunkSize(67108864);
    devices.push_back(move(device2));
  }


 if (tdevices &cmp_cpu){  
    ecl::Device device(platform_cpu,0);
    device.setKernel(kernel,gws,gws);
    size_t mem_lim=static_cast<size_t>(ECL_mem_CPU)*1000000/num_in_buff; 
  	device.setLimMemory (mem_lim);
   	//device.setChunkSize(2097152);
    devices.push_back(move(device));
  }
  if (tdevices &cmp_gpu){  
    ecl::Device device1(platform_gpu,0);
    device1.setKernel(kernel,gws,gws);
    size_t mem_lim=static_cast<size_t>(ECL_mem_GPU)*1000000/num_in_buff; 
    device1.setLimMemory (mem_lim);
   	//device1.setChunkSize(33554432);
    devices.push_back(move(device1));
  }
  ecl::StaticLongScheduler stSched;
  ecl::DynamicScheduler dynSched;
  ecl::HGuidedScheduler hgSched;
cout<<"Manual proportions!";
  ecl::Runtime runtime(move(devices), problem_size);
  if (tscheduler == 0) {
    runtime.setScheduler(&stSched);
    stSched.setRawProportions({ prop,(float)20.0-(prop+prop2),prop2});
   if (tdevices==7)
    stSched.setRawProportions({ 0.2,0.35,0.45});
   // stSched.setRawProportions({ prop2,20.0-(prop2+prop),prop});
   else if(tdevices==3)
    stSched.setRawProportions({ 5.10, 0.92});
   else if(tdevices==5)
    stSched.setRawProportions({ 1.19, 5.1});
   else if(tdevices==6)
    stSched.setRawProportions({ 1.19, 0.92});
   else
    stSched.setRawProportions({ 1.0});
  } else if (tscheduler == 1) {
    runtime.setScheduler(&dynSched);
    dynSched.setWorkSize(worksize,FRAME);
  } else { // tscheduler == 2
    runtime.setScheduler(&hgSched);
    hgSched.setWorkSize(worksize,FRAME);
   if (tdevices==7)
      //hgSched.setRawProportions({0.47 ,0.38 ,0.16});
      hgSched.setRawProportions({0.16,0.47 ,0.38});
   else if (tdevices==3)
      hgSched.setRawProportions({ 5.10,0.92});
   else if (tdevices==5)
      hgSched.setRawProportions({ 1.19, 5.10});
   else if (tdevices==6)
      hgSched.setRawProportions({ 1.19,0.92});
  }
  runtime.setInBuffer(input);
  runtime.setOutBuffer(output);
  runtime.setOutAuxBuffer(output_aux);
  runtime.setKernel(kernel, "NearestNeighbor");//the default is gws(0,1,1) when gws[0]=chunk_size. lws(128,1,1)

  runtime.setKernelArg(0, input);//in
  runtime.setKernelArg(1, output);//out
  
//  runtime.setKernelArg(2,(int) problem_size);//in
  runtime.setKernelArg(2,(int)65536);//in
  runtime.setKernelArg(3,lat);//in
  runtime.setKernelArg(4,lng);//in

  runtime.setInternalChunk(1);
  runtime.run();

  runtime.printStats();

  if (check) {
    auto out = *output.get();

//    auto ok = nearest.compare();

    auto time = 0;
/*    if (ok) {
      cout << "Success (" << time << ")\n";
    } else {
      cout << "Failure (" << time << ")\n";
    }
  */  //file to save and compare data results
      std::ofstream myfile;
      myfile.open ("nearest.txt");
      for(int dat=0; dat<(int)out.size(); dat++)
          myfile<<out[dat]<<"\n";
      myfile.close();

  } else {
    cout << "Done nearest neighbor \n";
  }

  exit(0);
}
