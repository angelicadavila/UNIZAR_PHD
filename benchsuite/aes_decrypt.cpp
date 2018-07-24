// This version works with one per chunk

#include "aes_decrypt.hpp"
#include "parse_ppm.hpp"
#include "aes_proc.hpp"
#include <iostream>
#include <fstream>

#define COLS 25920
#define ROWS 12060
//#define COLS 25920
//#define ROWS 12060
#define FRAME 10
#define ROUNDS 10
void
Aes_decrypt::init_image()
{
  
  //std::string imageFilename = "butterflies.ppm";
  std::string imageFilename = "benchsuite/burmistree.ppm";
    
  if (!parse_ppm(imageFilename.c_str(),COLS,ROWS,(unsigned char *)_input_img.data())) {
        std::cerr << "Error: could not load " << std::endl;
    }
  //128 bit encryption key
  unsigned char key[] = "Xilinx SDAccel  ";
  unsigned char roundkey[(ROUNDS + 1) * 16];
  strcpy(((char *) roundkey), ((char *) key));
  KeyExpansion(roundkey);
  //vector<char,vecAllocator<char>>_round_key((char*)roundkey, (char*)roundkey+(10+1)*16);
 //uncomment this if you have a other test image
  //perform SW encryption
  //Xilinx
 // aesecb_encrypt(key, ((unsigned char *)_input_img.data() ),
  //    ((unsigned char *) _crypt_img.data()), _total_size, ROUNDS);
 // outFrame((unsigned int*)_crypt_img.data());
  //imageFilename = "benchsuite/burmisCript.ppm";
    
  if (!parse_ppm(imageFilename.c_str(),COLS,ROWS,(unsigned char *)_crypt_img.data())) {
        std::cerr << "Error: could not load " << std::endl;
        exit(0);
    }
}

string
Aes_decrypt::get_kernel_str()
{

}

//write the frame bmp resultant
void 
Aes_decrypt::outFrame(unsigned *frameData) {

  char fname[256];
  sprintf(fname, "burmisCript.ppm");
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
do_aesdecrypt(int tscheduler,
            int tdevices,
            bool check,
            uint image_size,
            int chunksize,
            float prop
           )
{

  int worksize = chunksize;
  size_t frames=4;
  Aes_decrypt aes_decrypt(COLS*ROWS*frames);//+256

    string kernel = file_read("support/kernels/aes_decrypt.cl");
#pragma GCC diagnostic ignored "-Wignored-attributes"
 auto input =  shared_ptr<vector<char,vecAllocator<char>>>(&aes_decrypt._crypt_img);
 auto key =    shared_ptr<vector<char,vecAllocator<char>>>(&aes_decrypt._round_key);
 auto output = shared_ptr<vector<char,vecAllocator<char>>>(&aes_decrypt._out);
 auto output_aux = shared_ptr<vector<char,vecAllocator<char>>>(&aes_decrypt._out_aux);
#pragma GCC diagnostic pop
  
  size_t problem_size =COLS*ROWS*frames*FRAME;//(aes_decrypt._total_size)/16;
  //size_t problem_size =19537152;//(aes_decrypt._total_size)/16;
  //19537152;//(aes_decrypt._total_size)/16;

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
  vector <size_t>gws = vector <size_t>(3,1);
  if (tdevices &0x02){  
    ecl::Device device2(platform_fpga,0);
    binary_file	=file_read_binary("./benchsuite/altera_kernel/aes_decrypt.aocx"); 
    device2.setKernel(binary_file,gws,gws); 
   	device2.setLimMemory(1400000000);
    devices.push_back(move(device2));
  }

  if (tdevices &0x04){  
    ecl::Device device(platform_cpu,0);
  //  device.setKernel(kernel,gws,gws);
  	device.setLimMemory (4000000000);
    devices.push_back(move(device));
  }
  if (tdevices &0x01){  
    ecl::Device device1(platform_gpu,0);
  //  device1.setKernel(kernel,gws,gws);
  	device1.setLimMemory (4000000000);
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
   hgSched.setRawProportions({0.10, 0.1});
  }
  runtime.setInBuffer(input);
  runtime.setInBuffer(key);
  runtime.setOutBuffer(output);
  runtime.setOutAuxBuffer(output_aux);
  runtime.setKernel(kernel, "krnl_aes_decrypt");
  runtime.setKernelArg(0, output);//out
  runtime.setKernelArg(1, input);//in
  runtime.setKernelArg(2, key);// width
  //works with 1 per chunk in this version 
  runtime.setInternalChunk(16); //procesing the size of watermark
  runtime.run();

  runtime.printStats();

  if (check) {
    auto out = *output.get();
    aes_decrypt.outFrame((unsigned int*)out.data());

  } else {
    cout << "Done\n";
  }

  exit(0);
}

