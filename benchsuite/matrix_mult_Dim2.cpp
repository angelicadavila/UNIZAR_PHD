// This version works blocks

#include "matrix_mult.hpp"
#include <iostream>
#include <fstream>

#define ROWS 1080
#define COLS 1920

#define FRAMES 1
// Randomly generate a floating-point number between -10 and 10.
float rand_float() {
  return float(rand()) / float(RAND_MAX) * 20.0f - 10.0f;
  }

void
MatrixMult::init_matrix()
{
 for(int j = 0; j < _total_size; ++j) {
       _input_A[j] = rand_float();
       _input_B[j] = rand_float();
       _out[j]=0;
  }

}

string
MatrixMult::get_kernel_str()
{
   return 0;
 }

// Dump txt.
void 
MatrixMult::verify_out(unsigned *frameData) {
 
  auto dim=sqrt(_total_size);
  vector<float> very(_total_size,0);
 for (int i = 0; i <dim ; i++) {
        for (int j = 0; j <dim; j++) {
            float sum = 0.0;
            for (int k = 0; k < dim; k++)
                sum = sum + _input_A[i * dim + k] * _input_B[k * dim + j];
            very[i * dim + j] = sum;
            //cout<<"[i] "<<i<<" [j] "<<j<<" ver: "<<sum<<" out:"<<_out[i*dim+j]<<"\n";
            if (sum-_out[i*dim+j]>0.1){
            cout<<"[i] "<<i<<" [j] "<<j<<" ver: "<<sum<<" out:"<<_out[i*dim+j]<<"\n";
            cout<<"Problem!\n";
            exit(0);  
            }
        }

    }
   cout<<"element 0: "<<_out[0]-very[0]<<"\n";
   //cout<<"element 0: "<<_out[_total_size-1]<<"\n";


}


void
do_matrixMult(int tscheduler,
            int tdevices,
            bool check,
            uint m_height,
            int chunksize,
            float prop,
            uint m_width
           )
{

  int worksize = chunksize;
 // cout<<"size H*W*BLOCKSIZE^2- H:"<<m_height << " W:"<<m_width<<" BLCK:64\n";
  m_height*=64;
  int _size=m_height*m_height;
  MatrixMult matrixMult(_size);

  cout<<"warning: m_width was ignored. Only square Matrix\n "<<_size<<"\n"; 
  
  string kernel = file_read("support/kernels/matrix_mult.cl");

//#pragma GCC diagnostic ignored "-Wignored-attributes"
 auto input_A = shared_ptr<vector<float,vecAllocator<float>>>(&matrixMult._input_A);
 auto input_B = shared_ptr<vector<float,vecAllocator<float>>>(&matrixMult._input_B);
 auto output = shared_ptr<vector<float,vecAllocator<float>>>(&matrixMult._out);
//#pragma GCC diagnostic pop
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
  gws[0]=0; //size_divided
  gws[1]=m_height;//B_size

  vector <size_t>lws=vector <size_t>(3,1);
  lws[0]=64; lws[1]=64;

  vector <char> binary_file;
  if (tdevices &cmp_fpga){  
    ecl::Device device2(platform_fpga,0);
    binary_file =file_read_binary("./benchsuite/altera_kernel/matrix_mult_Dim2.aocx"); 
    device2.setKernel(binary_file,gws,lws); 
    device2.setGwsDim(1);
    devices.push_back(move(device2));
  }

  if (tdevices &cmp_cpu){  
    lws[0]=32; lws[1]=32;
    ecl::Device device(platform_cpu,0);
    device.setKernel(kernel, gws, lws);
    device.serGwsDim(1);
    devices.push_back(move(device));
  }
  if (tdevices &cmp_gpu){  
    lws[0]=32; lws[1]=32;
    ecl::Device device1(platform_gpu,0);
    device1.setKernel(kernel,gws,lws);
    device.setGwsDim(1);
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
    dynSched.setWorkSize(worksize,FRAMES);
  } else if( tscheduler == 2){
    runtime.setScheduler(&hgSched);
    hgSched.setWorkSize(worksize);
   hgSched.setRawProportions({0.9, 0.2});
  }
   else if( tscheduler == 3){
   // runtime.setScheduler(&propSched);
   // propSched.setWorkSize(worksize);
  }
  
  runtime.setInBuffer(input_A);
  runtime.setInBuffer(input_B);
  runtime.setOutBuffer(output);
  runtime.setKernel(kernel, "matrixMult");
  runtime.setKernelArg(0, output);//out
  runtime.setKernelArg(1, input_A);//in
  runtime.setKernelArg(2, input_B);//in
  runtime.setKernelArg(3, m_height);// width
  runtime.setKernelArg(4, m_height);//height
  //works with 1 per chunk in this version 
  runtime.setInternalChunk(m_height); //procesing the per rows
  runtime.run();

  runtime.printStats();

  if (check) {
    auto out = *output.get();
    matrixMult.verify_out((unsigned int*)out.data());

  } else {
    cout << "Done\n";
  }

  exit(0);
}


