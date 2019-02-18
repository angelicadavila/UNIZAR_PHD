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

 // m_height =40960;
 // m_width =m_height;
  m_height=20480;
  m_width = 8192;
//  m_height =20480;
//  m_width = 81920;
  double x0=-0.5;
//  double x0=-0.7;
//  double stepSize=0.1;
  int  maxIterations=1000;
  int windowWidth=m_width;
  double aStartY=0.0;
//  double aStartY=0.0;
//  double aScale=0.35;//to 2048
   // double aScale=0.0000035;
  //double aScale=0.000015;
  double aScale=0.00015;
  

  int worksize = chunksize;
  int _size=m_width * m_height;
  Mandelbrot mandelbrot(_size);

  
  string kernel = file_read("support/kernels/mandelbrot_kernel.cl");

//#pragma GCC diagnostic ignored "-Wignored-attributes"
 auto output = shared_ptr<vector<uint,vecAllocator<uint>>>(&mandelbrot._out);
 auto output_aux = shared_ptr<vector<uint,vecAllocator<uint>>>(&mandelbrot._out_aux); 
//#pragma GCC diagnostic pop
 //rows of the matrix 
  int problem_size =(m_height);

  vector<ecl::Device> devices;

  auto platform_cpu = ECL_CPU;
  auto platform_gpu = ECL_GPU;
  auto platform_fpga= ECL_FPGA;
  auto cmp_cpu  =0x01;  
  auto cmp_gpu  =0x02;  
  auto cmp_fpga=0x04;  

  auto num_in_buff=2;
// 64 its the block size
// C=A*B
  vector <size_t>gws=vector <size_t>(3,1);
  gws[1]=0; //size_divided
  gws[0]=m_width;//B_size
  gws[2]=1;//B_size

  vector <size_t>lws=vector <size_t>(3,1);
//  lws[0]=nullptr; lws[1]=nullptr, lws[2]=nullptr;

  vector <char> binary_file;
  if (tdevices &cmp_fpga){  
    ecl::Device device2(platform_fpga,0);
    //binary_file =file_read_binary("./benchsuite/altera_kernel/mandelbrot_kernel.aocx"); 
    binary_file =file_read_binary("./benchsuite/altera_kernel/mandelbrot_prof.aocx"); 
    device2.setKernel(binary_file,gws,lws);
    size_t mem_lim=static_cast<size_t>(ECL_mem_FPGA)*1000000/num_in_buff;
    device2.setLimMemory(mem_lim);
    device2.setGwsDim(1);
//    device2.setKernel(binary_file,gws,lws); 
    devices.push_back(move(device2));
  }

  if (tdevices &cmp_cpu){  
//    lws[0]=32; lws[1]=32;
    ecl::Device device(platform_cpu,0);
    //device.setLimMemory(6000000000);
    size_t mem_lim=static_cast<size_t>(ECL_mem_CPU)*1000000/num_in_buff;
    device.setLimMemory(mem_lim);
    device.setKernel(kernel, gws, lws);
    device.setGwsDim(1);
    devices.push_back(move(device));
  }
  if (tdevices &cmp_gpu){  
//    lws[0]=32; lws[1]=32;
    ecl::Device device1(platform_gpu,0);
    size_t mem_lim=static_cast<size_t>(ECL_mem_GPU)*1000000/num_in_buff;
    device1.setLimMemory(mem_lim);
    device1.setKernel(kernel,gws,lws);
    device1.setGwsDim(1);
    devices.push_back(move(device1));
  }

  ecl::StaticScheduler stSched;
  ecl::DynamicScheduler dynSched;
  ecl::HGuidedScheduler hgSched;
  ecl::ProportionalScheduler propSched;
  
  cout<<"Manual proportions!";
  
  ecl::Runtime runtime(move(devices), problem_size);
  if (tscheduler == 0) {
    runtime.setScheduler(&stSched);
    stSched.setRawProportions({ prop });
  } else if (tscheduler == 1) {
    runtime.setScheduler(&dynSched);
    dynSched.setWorkSize(worksize,1);
  } else if( tscheduler == 2){
    runtime.setScheduler(&hgSched);
    hgSched.setWorkSize(worksize,1);
    hgSched.setRawProportions({0.237, 0.434, 0.329});
//   hgSched.setRawProportions({0.5});
  }
   else if( tscheduler == 3){
    runtime.setScheduler(&propSched);
    propSched.setWorkSize(worksize);
  }
  runtime.setOutBuffer(output);
  runtime.setOutAuxBuffer(output_aux); 
  runtime.setKernel(kernel, "hw_mandelbrot_frame");
  runtime.setKernelArg(0,x0) ;//out
  runtime.setKernelArg(1,aScale );//in
  runtime.setKernelArg(2, maxIterations);//in
  runtime.setKernelArg(3, output);// width
  runtime.setKernelArg(4, windowWidth);//height
  runtime.setKernelArg(5, aStartY);//height
  runtime.setKernelArg(6, aScale);//height

  //works with 1 per chunk in this version 
  runtime.setInternalChunk(m_width); //procesing the per rows
  runtime.run();

  runtime.printStats();

  if (check) {
    auto out = *output.get();
//    mandelbrot.verify_out((unsigned int*)out.data())
//    ;
//
     std::ofstream myfile;
     myfile.open ("mandelbrot.txt"); 
     for(int dat=0; dat<(int)out.size(); dat++) 
  	 myfile<<out[dat]<<"\n";
     myfile.close();
  } else {
    cout << "Done Mandelbrot\n";
  }

  exit(0);
}


