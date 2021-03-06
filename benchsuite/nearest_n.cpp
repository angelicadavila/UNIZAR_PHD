#include "nearest_n.hpp"
#include <iostream>
#include <fstream>

#define GROUP 64
void
Nearest::init_data(uint dim)
{
  srand(0);
  for (auto i=0 ; i<dim*2 ; i++){
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

  N_data=N_data*4096;
  int worksize = chunksize;
  Nearest nearest(N_data);

  //string kernel = file_read("support/kernels/nearest_kernel_box.cl");
  string kernel = file_read("support/kernels/nn.cl");
#pragma GCC diagnostic ignored "-Wignored-attributes"
 auto input = shared_ptr<vector<float,vecAllocator<float>>>(&nearest._input_lat);
 auto output = shared_ptr<vector<float,vecAllocator<float>>>(&nearest._distance);
#pragma GCC diagnostic pop
  
  int problem_size = nearest._total_size;
  cout<<"problem size"<<problem_size<<"\n";
  vector<ecl::Device> devices;

  auto platform_cpu = 0;
  auto platform_gpu = 1;
  auto platform_fpga= 2;
  
 // Mersenne twister is a unitary kernel pipeline or Task execution

  vector <char> binary_file;
  vector <size_t>gws=vector <size_t>(3,1);
  gws[0]=GROUP;
  if (tdevices &0x01){  
    ecl::Device device(platform_cpu,0);
    device.setKernel(kernel,gws,gws);
    devices.push_back(move(device));
  }
  if (tdevices &0x02){  
    ecl::Device device1(platform_gpu,0);
    device1.setKernel(kernel,gws,gws);
    devices.push_back(move(device1));
  }
  if (tdevices &0x04){  
    ecl::Device device2(platform_fpga,0);
    binary_file	=file_read_binary("./benchsuite/altera_kernel/nn.aocx"); 
    device2.setKernel(binary_file,gws,gws); 
    devices.push_back(move(device2));
  }

  ecl::StaticScheduler stSched;
  ecl::DynamicScheduler dynSched;
  ecl::HGuidedScheduler hgSched;
cout<<"Manual proportions!";
  ecl::Runtime runtime(move(devices), problem_size);
  if (tscheduler == 0) {
    runtime.setScheduler(&stSched);
   // stSched.setRawProportions({ 0.21, 0.82});
   if (tdevices==7)
    stSched.setRawProportions({ 0.2,0.55,0.25});
    //stSched.setRawProportions({ prop2,20.0-(prop2+prop),prop});
   else if(tdevices==3)
    stSched.setRawProportions({ 5.10, 0.92});
   else if(tdevices==5)
    //stSched.setRawProportions({ 1.19, 5.1});
    stSched.setRawProportions({ prop2,prop});
   else if(tdevices==6)
    stSched.setRawProportions({ 1.19, 0.92});
   else
    stSched.setRawProportions({ 1.0});
  } else if (tscheduler == 1) {
    runtime.setScheduler(&dynSched);
    dynSched.setWorkSize(worksize);
  } else { // tscheduler == 2
    runtime.setScheduler(&hgSched);
   // hgSched.setWorkSize(worksize);
   if (tdevices==7)
      hgSched.setRawProportions({ 1.96, 2.94,1});
   else if (tdevices==3)
      hgSched.setRawProportions({ 5.10,0.92});
   else if (tdevices==5)
      //hgSched.setRawProportions({ 1.19, 5.10});
      hgSched.setRawProportions({ 1, 1});
   else if (tdevices==6)
      hgSched.setRawProportions({ 1.19,0.92});
  }
  runtime.setInBuffer(input);
  runtime.setOutBuffer(output);
  runtime.setKernel(kernel, "NearestNeighbor");//the default is gws(0,1,1) when gws[0]=chunk_size. lws(128,1,1)

  runtime.setKernelArg(0, input);//in
  runtime.setKernelArg(1, output);//out
  
  runtime.setKernelArg(2, problem_size);//in
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
