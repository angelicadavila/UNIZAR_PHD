// This version works with one per chunk

#include "nw.hpp"
#include "parse_ppm.hpp"
#include <iostream>
#include <fstream>

#define BLOCK_SIZE 16
#define penalty 10

#define FRAMES 10
void
Needleman::init_vectors()
{
    int blosum62[24][24] = {
  { 4, -1, -2, -2,  0, -1, -1,  0, -2, -1, -1, -1, -1, -2, -1,  1,  0, -3, -2,  0, -2, -1,  0, -4},
  {-1,  5,  0, -2, -3,  1,  0, -2,  0, -3, -2,  2, -1, -3, -2, -1, -1, -3, -2, -3, -1,  0, -1, -4},
  {-2,  0,  6,  1, -3,  0,  0,  0,  1, -3, -3,  0, -2, -3, -2,  1,  0, -4, -2, -3,  3,  0, -1, -4},
  {-2, -2,  1,  6, -3,  0,  2, -1, -1, -3, -4, -1, -3, -3, -1,  0, -1, -4, -3, -3,  4,  1, -1, -4},
  { 0, -3, -3, -3,  9, -3, -4, -3, -3, -1, -1, -3, -1, -2, -3, -1, -1, -2, -2, -1, -3, -3, -2, -4},
  {-1,  1,  0,  0, -3,  5,  2, -2,  0, -3, -2,  1,  0, -3, -1,  0, -1, -2, -1, -2,  0,  3, -1, -4},
  {-1,  0,  0,  2, -4,  2,  5, -2,  0, -3, -3,  1, -2, -3, -1,  0, -1, -3, -2, -2,  1,  4, -1, -4},
  { 0, -2,  0, -1, -3, -2, -2,  6, -2, -4, -4, -2, -3, -3, -2,  0, -2, -2, -3, -3, -1, -2, -1, -4},
  {-2,  0,  1, -1, -3,  0,  0, -2,  8, -3, -3, -1, -2, -1, -2, -1, -2, -2,  2, -3,  0,  0, -1, -4},
  {-1, -3, -3, -3, -1, -3, -3, -4, -3,  4,  2, -3,  1,  0, -3, -2, -1, -3, -1,  3, -3, -3, -1, -4},
  {-1, -2, -3, -4, -1, -2, -3, -4, -3,  2,  4, -2,  2,  0, -3, -2, -1, -2, -1,  1, -4, -3, -1, -4},
  {-1,  2,  0, -1, -3,  1,  1, -2, -1, -3, -2,  5, -1, -3, -1,  0, -1, -3, -2, -2,  0,  1, -1, -4},
  {-1, -1, -2, -3, -1,  0, -2, -3, -2,  1,  2, -1,  5,  0, -2, -1, -1, -1, -1,  1, -3, -1, -1, -4},
  {-2, -3, -3, -3, -2, -3, -3, -3, -1,  0,  0, -3,  0,  6, -4, -2, -2,  1,  3, -1, -3, -3, -1, -4},
  {-1, -2, -2, -1, -3, -1, -1, -2, -2, -3, -3, -1, -2, -4,  7, -1, -1, -4, -3, -2, -2, -1, -2, -4},
  { 1, -1,  1,  0, -1,  0,  0,  0, -1, -2, -2,  0, -1, -2, -1,  4,  1, -3, -2, -2,  0,  0,  0, -4},
  { 0, -1,  0, -1, -1, -1, -1, -2, -2, -1, -1, -1, -1, -2, -1,  1,  5, -2, -2,  0, -1, -1,  0, -4},
  {-3, -3, -4, -4, -2, -2, -3, -2, -2, -3, -2, -3, -1,  1, -4, -3, -2, 11,  2, -3, -4, -3, -2, -4},
  {-2, -2, -2, -3, -2, -1, -2, -3,  2, -1, -1, -2, -1,  3, -3, -2, -2,  2,  7, -1, -3, -2, -1, -4},
  { 0, -3, -3, -3, -1, -2, -2, -3, -3,  3,  1, -2,  1, -1, -2, -2,  0, -3, -1,  4, -3, -2, -1, -4},
  {-2, -1,  3,  4, -3,  0,  1, -1,  0, -3, -4,  0, -3, -3, -2,  0, -1, -4, -3, -3,  4,  1, -1, -4},
  {-1,  0,  0,  1, -3,  3,  4, -2,  0, -3, -3,  1, -1, -3, -1,  0, -1, -3, -2, -2,  1,  4, -1, -4},
  { 0, -1, -1, -1, -2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2,  0,  0, -2, -1, -1, -1, -1, -1, -4},
  {-4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4,  1}
  };
  
  srand(7);
  
  //initialization 
  for (int i = 0 ; i < _max_cols; i++){
    for (int j = 0 ; j < _max_rows; j++){
      _input_itemsets[i*_max_cols+j] = 0;
    }
  }

  for( int i=1; i< _max_rows ; i++){    //initialize the cols
      _input_itemsets[i*_max_cols] = rand() % 10 + 1;
  }
  
  for( int j=1; j< _max_cols ; j++){    //initialize the rows
      _input_itemsets[j] = rand() % 10 + 1;
  }
  
  for (int i = 1 ; i < _max_cols; i++){
    for (int j = 1 ; j < _max_rows; j++){
       int ref_offset = i*_max_cols+j;
       _reference[ref_offset] = blosum62[_input_itemsets[i*_max_cols]][_input_itemsets[j]];
    }
  }

    

}

string
Needleman::get_kernel_str()
{
   return 0;
}

// Dump frame data in PPM format.
void 
Needleman::out_vector() {
}


void
do_needleman(int tscheduler,
            int tdevices,
            bool check,
            uint sq_length ,
            int chunksize,
            float prop
           )
{
  //sequence with equal size
  int worksize = chunksize;
  size_t frames=6; 
  sq_length*=BLOCK_SIZE*6; 
  cout<<"length sequence"<<sq_length<<"\n";
  Needleman needleman(sq_length+1);

  string kernel = file_read("support/kernels/nw.cl");

#pragma GCC diagnostic ignored "-Wignored-attributes"
 auto reference       = shared_ptr<vector<int,vecAllocator<int>>>(&needleman._reference);
 auto input_itemsets  = shared_ptr<vector<int,vecAllocator<int>>>(&needleman._input_itemsets);
 auto output_itemsets = shared_ptr<vector<int,vecAllocator<int>>>(&needleman._output_itemsets);
 auto output_itemsets_aux = shared_ptr<vector<int,vecAllocator<int>>>(&needleman._output_itemsets_aux);
#pragma GCC diagnostic pop
  
  int problem_size = ((needleman._max_cols )/BLOCK_SIZE)*10;

  vector<ecl::Device> devices;

  #if ECL_GRENDEL == 1 
  auto platform_cpu = 2;
  auto platform_gpu = 0;
  auto platform_fpga= 1;
  auto cmp_cpu  =0x04;  
  auto cmp_gpu  =0x01;  
  auto cmp_fpga =0x02;  
  #else
  auto platform_cpu = 3;
  auto platform_gpu = 1;
  auto platform_fpga= 2;
  auto cmp_cpu =0x01;  
  auto cmp_gpu =0x02;  
  auto cmp_fpga=0x04;  
  #endif

  vector <char> binary_file;
  vector <size_t>gws=vector <size_t>(3,1);
  vector <size_t>lws=vector <size_t>(3,1);
  if (tdevices &cmp_fpga){  
    ecl::Device device2(platform_fpga,0);
    binary_file	=file_read_binary("./benchsuite/altera_kernel/nw_16.aocx"); 
    device2.setKernel(binary_file,gws,gws);
   	device2.setLimMemory(1000000000);
    devices.push_back(move(device2));
  }

  if (tdevices &cmp_cpu){  
    ecl::Device device(platform_cpu,0);
    gws[0]=0; lws[0]=BLOCK_SIZE;
    device.setKernel(kernel,gws,lws);
  	device.setLimMemory (3000000000);
    devices.push_back(move(device));
  }
  if (tdevices &cmp_gpu){  
    ecl::Device device1(platform_gpu,0);
    gws[0]=0;lws[0]=BLOCK_SIZE;
    device1.setKernel(kernel,gws,lws);
  	device1.setLimMemory (3000000000);
    devices.push_back(move(device1));
  }

  ecl::StaticScheduler stSched;
  ecl::DynamicScheduler dynSched;
  ecl::HGuidedScheduler hgSched;
  //ecl::ProportionalScheduler propSched;
  
  cout<<"Manual proportions!";
  
  ecl::Runtime runtime(move(devices), problem_size);
  if (tscheduler == 0) {
    runtime.setScheduler(&stSched);
    stSched.setRawProportions({ prop, 0.26 });
  } else if (tscheduler == 1) {
    runtime.setScheduler(&dynSched);
    dynSched.setWorkSize(worksize);
  } else if (tscheduler ==2){ // tscheduler == 2
    runtime.setScheduler(&hgSched);
    hgSched.setWorkSize(worksize);
   hgSched.setRawProportions({prop, 0.25});
  } else if (tscheduler ==3){ // tscheduler == 2
   // runtime.setScheduler(&propSched);
   // propSched.setWorkSize(worksize);
    //propSched.setRawProportions({prop, 0.25});
  }
  runtime.setInBuffer(input_itemsets);
  runtime.setInBuffer(reference);
  runtime.setOutBuffer(output_itemsets);
  runtime.setOutAuxBuffer(output_itemsets_aux);
  runtime.setKernel(kernel, "nw_kernel1");
  runtime.setKernelArg(0, reference);//in
  runtime.setKernelArg(1, input_itemsets);//out
  runtime.setKernelArg(2, output_itemsets);// 
  runtime.setKernelArg(3, sq_length);// 
  runtime.setKernelArg(4, penalty);// 
  
  runtime.setInternalChunk(BLOCK_SIZE);
  runtime.run();

  runtime.printStats();

  if (check) {
    auto out = *output_itemsets.get();
    needleman.out_vector();

  } else {
    cout << "Done Needleman\n";
  }

  exit(0);
}



