// This version works blocks

#include "mandelbrot.hpp"
#include <iostream>
#include <fstream>


// Randomly generate a floating-point number between -10 and 10.
//float rand_float() {
//  return float(rand()) / float(RAND_MAX) * 20.0f - 10.0f;
//  }

void
Mandelbrot::init_matrix()
{

}

string
Mandelbrot::get_kernel_str()
{
   return 0;
 }

// Dump txt.
void 
Mandelbrot::verify_out(unsigned *frameData) {


}


void
do_mandelbrot(int tscheduler,
            int tdevices,
            bool check,
            uint m_height,
            int chunksize,
            float prop,
            uint m_width
           )
{

//  m_height=2048;
  double x0=-2.0;
  double stepSize=10;
  int  maxIterations=1000;
  int windowWidth=m_height;
  double aStartY=1.15;
  double aScale=0.0035;
  

  int worksize = chunksize;
  int _size=m_height*m_height;
  Mandelbrot mandelbrot(_size);

  
  string kernel = file_read("support/kernels/mandelbrot_kernel.cl");

#pragma GCC diagnostic ignored "-Wignored-attributes"
 auto output = shared_ptr<vector<uint,vecAllocator<uint>>>(&mandelbrot._out);
 auto output_aux = shared_ptr<vector<uint,vecAllocator<uint>>>(&mandelbrot._out_aux); 
#pragma GCC diagnostic pop
 //rows of the matrix 
  int problem_size =(m_height);

  vector<ecl::Device> devices;

  #if ECL_GRENDEL == 1 
  auto platform_cpu = 2;
  auto platform_gpu = 0;
  auto platform_fpga= 1;
  auto cmp_cpu  =0x04;  
  auto cmp_gpu  =0x01;  
  auto cmp_fpga=0x02;  
#else
  auto platform_cpu = 3;
  auto platform_gpu = 1;
  auto platform_fpga= 2;
  auto cmp_cpu =0x01;  
  auto cmp_gpu =0x02;  
  auto cmp_fpga=0x04;  
  #endif


// 64 its the block size
// C=A*B
  vector <size_t>gws=vector <size_t>(3,1);
  gws[1]=0; //size_divided
  gws[0]=m_height;//B_size

  vector <size_t>lws=vector <size_t>(3,1);
//  lws[0]=nullptr; lws[1]=nullptr, lws[2]=nullptr;

  vector <char> binary_file;
  if (tdevices &cmp_fpga){  
    ecl::Device device2(platform_fpga,0);
    binary_file =file_read_binary("./benchsuite/altera_kernel/mandelbrot_kernel.aocx"); 
    device2.setKernel(binary_file,"hw_mandelbrot_frame",gws,lws);
    device2.setLimMemory(2000000000);
//    device2.setKernel(binary_file,gws,lws); 
    devices.push_back(move(device2));
  }

  if (tdevices &cmp_cpu){  
    lws[0]=32; lws[1]=32;
    ecl::Device device(platform_cpu,0);
    device.setKernel(kernel, gws, lws);
    devices.push_back(move(device));
  }
  if (tdevices &cmp_gpu){  
    lws[0]=32; lws[1]=32;
    ecl::Device device1(platform_gpu,0);
    device1.setKernel(kernel,gws,lws);
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
    stSched.setRawProportions({ prop });
  } else if (tscheduler == 1) {
    runtime.setScheduler(&dynSched);
    dynSched.setWorkSize(worksize);
  } else if( tscheduler == 2){
    runtime.setScheduler(&hgSched);
    hgSched.setWorkSize(worksize);
   hgSched.setRawProportions({0.9, 0.2});
  }
   else if( tscheduler == 3){
   // runtime.setScheduler(&propSched);
   // propSched.setWorkSize(worksize);
  }
 
  runtime.setOutBuffer(output);
  runtime.setOutAuxBuffer(output_aux); 
  runtime.setKernel(kernel, "hw_mandelbrot_frame");
  runtime.setKernelArg(0,x0) ;//out
  runtime.setKernelArg(1,stepSize );//in
  runtime.setKernelArg(2, maxIterations);//in
  runtime.setKernelArg(3, output);// width
  runtime.setKernelArg(4, windowWidth);//height
  runtime.setKernelArg(5, aStartY);//height
  runtime.setKernelArg(6, aScale);//height

  //works with 1 per chunk in this version 
  runtime.setInternalChunk(m_height); //procesing the per rows
  runtime.run();

  runtime.printStats();

  if (check) {
    auto out = *output.get();
    mandelbrot.verify_out((unsigned int*)out.data());

  } else {
    cout << "Done Mandelbrot\n";
  }

  exit(0);
}


