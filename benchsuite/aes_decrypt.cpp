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
  vector<char,vecAllocator<char>>_round_key((char*)key, (char*)key+16);
  KeyExpansion((unsigned char *)_round_key.data());  
  //perform SW encryption
  //Xilinx
  aesecb_encrypt(key, ((unsigned char *)_input_img.data() ),
      ((unsigned char *) _crypt_img.data()), _total_size, ROUNDS);

}

string
Aes_decrypt::get_kernel_str()
{

}

//write the frame bmp resultant
void 
Aes_decrypt::outFrame(unsigned *frameData) {



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

  Aes_decrypt aes_decrypt(COLS*ROWS);//+256

    string kernel = file_read("support/kernels/aes_decrypt.cl");
#pragma GCC diagnostic ignored "-Wignored-attributes"
 auto input =  shared_ptr<vector<char,vecAllocator<char>>>(&aes_decrypt._crypt_img);
 auto key =    shared_ptr<vector<char,vecAllocator<char>>>(&aes_decrypt._round_key);
 auto output = shared_ptr<vector<char,vecAllocator<char>>>(&aes_decrypt._out);
#pragma GCC diagnostic pop
  
  int problem_size =(aes_decrypt._total_size)/16;
  //19537152;//(aes_decrypt._total_size)/16;

  vector<clb::Device> devices;

  auto platform_cpu = 0;
  auto platform_gpu = 1;
  auto platform_fpga= 2;

  vector <char> binary_file;
  vector <size_t>gws = vector <size_t>(3,1);
  if (tdevices &0x04){  
    clb::Device device2(platform_fpga,0);
    binary_file	=file_read_binary("./benchsuite/altera_kernel/aes_decrypt.aocx"); 
    device2.setKernel(binary_file,gws,gws); 
    devices.push_back(move(device2));
  }

  if (tdevices &0x01){  
    clb::Device device(platform_cpu,0);
  //  device.setKernel(kernel,gws,gws);
    devices.push_back(move(device));
  }
  if (tdevices &0x02){  
    clb::Device device1(platform_gpu,0);
  //  device1.setKernel(kernel,gws,gws);
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
  runtime.setInBuffer(key);
  runtime.setOutBuffer(output);
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



