// This version works with one per chunk

#include "sobel.hpp"
#include "parse_ppm.hpp"
#include <iostream>
#include <fstream>

#define threshold_test 32
//butterflies
//#define ROWS 1080
//#define COLS 1920

//#define COLS 12116
//#define ROWS 6155
#define COLS 25920
#define ROWS 12060

//#define COLS 1024 
//#define ROWS 1024

void
Sobel::init_image()
{
//    std::string imageFilename = "butterflies.ppm";
//    std::string imageFilename = "benchsuite/lake.ppm";
    std::string imageFilename = "benchsuite/burmistree.ppm";
    
  if (!parse_ppm(imageFilename.c_str(),COLS,ROWS,(unsigned char *)_input_img.data())) {
        std::cerr << "Error: could not load " << std::endl;
    }
}

string
Sobel::get_kernel_str()
{

}

// Dump frame data in PPM format.
void 
Sobel::outFrame(unsigned *frameData) {
  char fname[256];
  sprintf(fname, "frame.ppm");
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
do_sobel(int tscheduler,
            int tdevices,
            bool check,
            uint image_size,
            int chunksize,
            float prop
           )
{

  int worksize = chunksize;

  Sobel sobel(COLS*ROWS);

  string kernel = file_read("support/kernels/sobel_med.cl");

#pragma GCC diagnostic ignored "-Wignored-attributes"
 auto input = shared_ptr<vector<int,vecAllocator<int>>>(&sobel._input_img);
 auto output = shared_ptr<vector<int,vecAllocator<int>>>(&sobel._out);
#pragma GCC diagnostic pop
  
  int problem_size = sobel._total_size;

  vector<ecl::Device> devices;

  auto platform_cpu = 0;
  auto platform_gpu = 1;
  auto platform_fpga= 2;

  vector <char> binary_file;
  if (tdevices &0x04){  
    ecl::Device device2(platform_fpga,0);
    binary_file	=file_read_binary("./benchsuite/altera_kernel/sobel_3w.aocx"); 
    vector <size_t>gws=vector <size_t>(3,1);
    device2.setKernel(binary_file,gws,gws);
    devices.push_back(move(device2));
  }

  if (tdevices &0x01){  
    ecl::Device device(platform_cpu,0);
    devices.push_back(move(device));
  }
  if (tdevices &0x02){  
    ecl::Device device1(platform_gpu,0);
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
    stSched.setRawProportions({ prop, 0.26 });
  } else if (tscheduler == 1) {
    runtime.setScheduler(&dynSched);
    dynSched.setWorkSize(worksize);
  } else if (tscheduler ==2){ // tscheduler == 2
    runtime.setScheduler(&hgSched);
    hgSched.setWorkSize(worksize);
   hgSched.setRawProportions({prop, 0.25});
  } else if (tscheduler ==3){ // tscheduler == 2
    runtime.setScheduler(&propSched);
    propSched.setWorkSize(worksize);
    //propSched.setRawProportions({prop, 0.25});
  }
  runtime.setInBuffer(input);
  runtime.setOutBuffer(output);
  runtime.setKernel(kernel, "sobel");
  uint threshold=threshold_test;
  runtime.setKernelArg(0, input);//in
  runtime.setKernelArg(1, output);//out
  runtime.setKernelArg(2, threshold);// 
//works with 1 per chunk in this versiòn 
//  runtime.setInternalChunk(64);
  runtime.run();

  runtime.printStats();

  if (check) {
    auto out = *output.get();
    sobel.outFrame((unsigned int*)out.data());

  } else {
    cout << "Done Sobel\n";
  }

  exit(0);
}



