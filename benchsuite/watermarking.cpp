// This version works with one per chunk

#include "watermarking.hpp"
#include "parse_ppm.hpp"
#include <iostream>
#include <fstream>

#define ROWS 1080
#define COLS 1920
void
Watermarking::init_image()
{
    std::string imageFilename = "butterflies.ppm";
    
  if (!parse_ppm(imageFilename.c_str(),COLS,ROWS,(unsigned char *)_input_img.data())) {
        std::cerr << "Error: could not load " << std::endl;
    }
}

string
Watermarking::get_kernel_str()
{

}

// Dump frame data in PPM format.
void 
Watermarking::outFrame(unsigned *frameData) {
  char fname[256];
  sprintf(fname, "fwatermarking.ppm");
  printf("Dumping %s\n", fname);

  FILE *f = fopen(fname, "wb");
  fprintf(f, "P6 %d %d %d\n", COLS, ROWS, 255);
  for(unsigned y = 0; y < ROWS; ++y) {
    for(unsigned x = 0; x < COLS; ++x) {
      // This assumes byte-order is little-endian.
      unsigned pixel = frameData[y * COLS + x];
      fwrite(&pixel, 1, 3, f);
    }
    fprintf(f, "\n");
  }
  fclose(f);
}


void
do_watermarking(int tscheduler,
            int tdevices,
            bool check,
            uint image_size,
            int chunksize,
            float prop
           )
{

  int worksize = chunksize;

  Watermarking watermarking(COLS*ROWS*8);//+256

  #if CLB_KERNEL_TASK == 0 
    string kernel = file_read("support/kernels/watermarking_simd.cl");
  #else
    string kernel = file_read("support/kernels/watermarking_off.cl");
    cout<<"Task"<<"\n";
  #endif

#pragma GCC diagnostic ignored "-Wignored-attributes"
 auto input = shared_ptr<vector<int,vecAllocator<int>>>(&watermarking._input_img);
 auto output = shared_ptr<vector<int,vecAllocator<int>>>(&watermarking._out);
#pragma GCC diagnostic pop
  
  int problem_size =129536*8;//(watermarking._total_size/16);

  vector<clb::Device> devices;

  auto platform_cpu = 0;
  auto platform_gpu = 1;
  auto platform_fpga= 2;

  vector <char> binary_file;
  if (tdevices &0x04){  
    clb::Device device2(platform_fpga,0);
  #if CLB_KERNEL_TASK == 0 
    binary_file	=file_read_binary("./benchsuite/altera_kernel/watermarking_simd.aocx"); 
  #else
    binary_file	=file_read_binary("./benchsuite/altera_kernel/watermarking_off.aocx"); 
  #endif  
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
   hgSched.setRawProportions({0.9, 0.2});
  }
  runtime.setInBuffer(input);
  runtime.setOutBuffer(output);
  runtime.setKernel(kernel, "apply_watermark");
  runtime.setKernelArg(0, input);//in
  runtime.setKernelArg(1, output);//out
  runtime.setKernelArg(2, COLS);// width
  runtime.setKernelArg(3, ROWS);//height
  //works with 1 per chunk in this version 
  runtime.setInternalChunk(16); //procesing the size of watermark
  runtime.run();

  runtime.printStats();

  if (check) {
    auto out = *output.get();
    watermarking.outFrame((unsigned int*)out.data());

  } else {
    cout << "Done\n";
  }

  exit(0);
}



