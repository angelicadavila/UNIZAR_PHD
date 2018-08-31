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

#define FRAMES 10
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
            float prop,
            float prop2
           )
{

  int worksize = chunksize;
  size_t frames=4;
  Sobel sobel(COLS*ROWS*frames);

  string kernel = file_read("support/kernels/sobel_med.cl");

#pragma GCC diagnostic ignored "-Wignored-attributes"
 auto input = shared_ptr<vector<int,vecAllocator<int>>>(&sobel._input_img);
 auto output = shared_ptr<vector<int,vecAllocator<int>>>(&sobel._out);
 auto output_aux = shared_ptr<vector<int,vecAllocator<int>>>(&sobel._out_aux);
#pragma GCC diagnostic pop
  
  size_t problem_size = COLS*ROWS*frames*FRAMES;
  cout <<"Problem Size: "<<problem_size<<"\n";
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

  vector <char> binary_file;
  if (tdevices &cmp_fpga){  
    ecl::Device device2(platform_fpga,0);
    binary_file	=file_read_binary("./benchsuite/altera_kernel/sobel_doble.aocx"); 
    vector <size_t>gws=vector <size_t>(3,1);
    device2.setKernel(binary_file,"sobel",gws,gws);
    device2.setKernel(binary_file,"sobel2",gws,gws);
   	device2.setLimMemory(1400000000);
    devices.push_back(move(device2));
  }

  if (tdevices &cmp_cpu){  
    ecl::Device device(platform_cpu,0);
  	device.setLimMemory (4000000000);
    device.setKernel(kernel,"sobel");
    devices.push_back(move(device));
  }
  if (tdevices &cmp_gpu){  
    ecl::Device device1(platform_gpu,0);
  	device1.setLimMemory (4000000000);
    device1.setKernel(kernel,"sobel");
	  devices.push_back(move(device1));
  }

 // ecl::StaticScheduler stSched;
  ecl::StaticLongScheduler stSched;
  ecl::DynamicScheduler dynSched;
  ecl::HGuidedScheduler hgSched;
  //ecl::ProportionalScheduler propSched;
 // ecl::SwarmScheduler smSched;
  
  cout<<"Manual proportions!";
  
  ecl::Runtime runtime(move(devices), problem_size);
  if (tscheduler == 0) {
    runtime.setScheduler(&stSched);
    //stSched.setRawProportions({0.17,0.16, 0.67});
    stSched.setRawProportions({prop,prop2, 20.0-(prop+prop2)});
  // stSched.setRawProportions({0.15,0.05, 0.8});
  } else if (tscheduler == 1) {
    runtime.setScheduler(&dynSched);
    dynSched.setWorkSize(worksize);
  } else if (tscheduler ==2){ // tscheduler == 2
    runtime.setScheduler(&hgSched);
    hgSched.setWorkSize(worksize);
   // hgSched.setRawProportions({0.17,0.43, 0.40});
    hgSched.setRawProportions({0.16,0.16, 0.67});
  } else if (tscheduler ==3){ // tscheduler == 2
   // runtime.setScheduler(&propSched);
   // propSched.setWorkSize(worksize);
    //propSched.setRawProportions({prop, 0.25});
  } else if (tscheduler ==4){ 
  //  runtime.setScheduler(&smSched);
  //  smSched.setWorkSize(worksize);
  }
  runtime.setInBuffer(input);
  runtime.setOutBuffer(output);
  runtime.setOutAuxBuffer(output_aux);
//  runtime.setKernel(kernel, "sobel");
  uint threshold=threshold_test;
  runtime.setKernelArg(0, input);//in
  runtime.setKernelArg(1, output);//out
  runtime.setKernelArg(2, threshold);// 
//works with 1 per chunk in this versiòn 
  runtime.setInternalChunk(1);
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



