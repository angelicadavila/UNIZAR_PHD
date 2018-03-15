// This version works with one per chunk

#include "aes_decrypt.hpp"
#include "parse_ppm.hpp"
#include <iostream>
#include <fstream>

void
Aes_decrypt::init_image()
{
  std::string m_strBitmapFP= "input.bmp";
  int err;
  struct bmp_t inputbmp;
  err = readbmp((char*)m_strBitmapFP.c_str(), &inputbmp);
  if (err != 0) {
    cout<<"Error : failed to read input.bmp\n";
    return false;
  }
  size_t inputbmpsize = inputbmp.height * inputbmp.width * 3;   


    //AES ECB encryption in software
  //encrypted image setup
  struct bmp_t swencryptbmp;
  swencryptbmp.pixels = (uint32_t *) malloc(inputbmpsize);
  swencryptbmp.width = inputbmp.width;
  swencryptbmp.height = inputbmp.height;
  if (swencryptbmp.pixels == NULL) {
    printf(
        "Error : failed to allocate memory for sw encrypted swencrypted.bmp\n");
    return false;
  }

  //128 bit encryption key
  unsigned char key[] = "Xilinx SDAccel  ";
  vector<char,vecAllocator<char>>_round_key((char*)key, (char*)key+16)
  //perform SW encryption
  //Xilinx
  aesecb_encrypt(key, ((unsigned char *) inputbmp.pixels),
      ((unsigned char *) swencryptbmp.pixels), inputbmpsize, ROUNDS);

 //copy to othe input vector
  vector<int,vecAllocator<int>>_input_img((int *)swencryptbmp.pixels, (int *)swencryptbmp.pixels+inputbmpsize);
  _total_size=inputbmpsize;
  //write "swencrypted.bmp"
  char swencryptbmpfile[] = "swencrypt.bmp";
  writebmp(swencryptbmpfile, &swencryptbmp); 
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

  Aes_decrypt aes_decrypt();//+256

    string kernel = file_read("support/kernels/aes_decrypt.cl");
#pragma GCC diagnostic ignored "-Wignored-attributes"
 auto input = shared_ptr<vector<int,vecAllocator<int>>>(&aes_decrypt._input_img);
 auto output = shared_ptr<vector<int,vecAllocator<int>>>(&aes_decrypt._out);
#pragma GCC diagnostic pop
  
  int problem_size =(aes_decrypt._total_size);

  vector<clb::Device> devices;

  auto platform_cpu = 0;
  auto platform_gpu = 1;
  auto platform_fpga= 2;

  vector <char> binary_file;
  if (tdevices &0x04){  
    clb::Device device2(platform_fpga,0);
    binary_file	=file_read_binary("./benchsuite/altera_kernel/aes_decrypt.aocx"); 
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
    aes_decrypt.outFrame((unsigned int*)out.data());

  } else {
    cout << "Done\n";
  }

  exit(0);
}



