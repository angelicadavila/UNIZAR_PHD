// This version works with one per chunk

#include "watermarking.hpp"
#include "parse_ppm.hpp"
#include <iostream>
#include <fstream>

#define COLS 25920 
#define ROWS 12060
//for Journal of super
#define FRAMES 10
//#define FRAMES 1
//#define ROWS 256
//#define COLS 128 
//#define ROWS 12060
//#define COLS 1920
    void
Watermarking::init_image()
{
    std::string imageFilename = "benchsuite/burmistree.ppm";

    if (!parse_ppm(imageFilename.c_str(),COLS,ROWS,(unsigned char *)_input_img.data())) {
	std::cerr << "Error: could not load " << std::endl;
    }
}

string
Watermarking::get_kernel_str()
{
    return 0;
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
	float prop,
	float prop2
	)
{

    int worksize = chunksize;
    //int frames=1;
    //JSC 2018
    int frames=6;
    Watermarking watermarking(COLS*ROWS*frames);//+256

    string kernel = file_read("support/kernels/watermarking_simd.cl");

    //  #pragma GCC diagnostic ignored "-Wignored-attributes"
    auto input = shared_ptr<vector<int,vecAllocator<int>>>(&watermarking._input_img);
    auto output = shared_ptr<vector<int,vecAllocator<int>>>(&watermarking._out);
    auto output_aux = shared_ptr<vector<int,vecAllocator<int>>>(&watermarking._out_aux);
    //  #pragma GCC diagnostic pop

    int problem_size =19537152*frames;//(watermarking._total_size/16);
    cout<<"problem Size" << problem_size; 
    vector<ecl::Device> devices;

    auto platform_cpu = ECL_CPU;
    auto platform_gpu = ECL_GPU;
    auto platform_fpga= ECL_FPGA;
    auto cmp_cpu =0x01;  
    auto cmp_gpu =0x02;  
    auto cmp_fpga=0x04;  

    auto num_in_buff=3;

    vector <char> binary_file;
    if (tdevices & cmp_fpga){  
	ecl::Device FPGA(platform_fpga,0);
	binary_file	=file_read_binary("./benchsuite/altera_kernel/watermarking_off.aocx");
	//      binary_file	=file_read_binary("./benchsuite/altera_kernel/watermarking_simd.aocx");
	//      binary_file	=file_read_binary("./benchsuite/altera_kernel/watermark_prof.aocx");
	//vector of kernel dimension. Task Kernel gws==lws
	vector <size_t>gws=vector <size_t>(3,1);
	FPGA.setKernel(binary_file,gws,gws);
	size_t mem_lim=static_cast<size_t>(ECL_mem_FPGA)*1000000/num_in_buff;
	if (chunksize>mem_lim*4){
	    cout<<"chunksize exceeded the memory lim of FPGA device\n";
	    cout<<"shuld be < "<<mem_lim<<"\n";
	    exit(0);
	}

	FPGA.setLimMemory(mem_lim);
	devices.push_back(move(FPGA));
    }
    if (tdevices & cmp_cpu){  
	ecl::Device CPU(platform_cpu,0);
	size_t mem_lim=static_cast<size_t>(ECL_mem_CPU)*1000000/num_in_buff;
	CPU.setLimMemory(mem_lim);
	devices.push_back(move(CPU));
    }
    if (tdevices & cmp_gpu){  
	ecl::Device GPU(platform_gpu,0);
	size_t mem_lim=static_cast<size_t>(ECL_mem_GPU)*1000000/num_in_buff;
	GPU.setLimMemory(mem_lim);
	devices.push_back(move(GPU));
    }

    //ecl::StaticScheduler stSched;
    ecl::StaticLongScheduler stSched;
    ecl::DynamicScheduler dynSched;
    ecl::HGuidedScheduler hgSched;
    //ecl::ProportionalScheduler propSched;
    cout<<"Manual proportions!";
    //problem size global frames 
    ecl::Runtime runtime(move(devices), problem_size*FRAMES);
    if (tscheduler == 0) {
	runtime.setScheduler(&stSched);
	runtime.setScheduler(&stSched);
	stSched.setRawProportions({ prop2, prop,(float)20.0-(prop+prop2) });
	//stSched.setRawProportions({ 0.2, 0.25,0.55 });
    } else if (tscheduler == 1) {
	runtime.setScheduler(&dynSched);
	dynSched.setWorkSize(worksize,FRAMES);
    }
    else if (tscheduler ==2){ // tscheduler == 2
	runtime.setScheduler(&hgSched);
	hgSched.setWorkSize(worksize,FRAMES);
	if(tdevices ==7)
	    //hgSched.setRawProportions({0.4,0.57,0.17});
	    hgSched.setRawProportions({0.18,0.39,0.43});
	else
	    hgSched.setRawProportions({0.1,prop});
    }else if (tscheduler == 3){
	// runtime.setScheduler(&propSched);
	// propSched.setWorkSize(worksize);
    }

    runtime.setInBuffer(input);
    runtime.setOutBuffer(output);
    runtime.setOutAuxBuffer(output_aux);
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
