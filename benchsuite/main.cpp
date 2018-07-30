#include "semaphore.hpp"
// #include "device.hpp"
// #include "scheduler.hpp"

// #include "runtime.hpp"

#include <chrono>
#include <iostream>
#include <string>

// #include "assign.hpp"
#include "binomial.hpp"
// #include "convolution.hpp"
#include "vecAllocator.hpp"
//#include "assign.hpp"
//#include "gaussian.hpp"
//#include "mandelbrot.hpp"
//#include "nbody.hpp"
//#include "ray.hpp"
//#include "saxpy.hpp"
// #include "taylor.hpp"
// #include "vecadd.hpp"
// #include "matrix_multiplication.hpp"
//#include "vecadd.hpp"
#include "mersenne.hpp"
#include "sobel.hpp"
#include "watermarking.hpp"
#include "matrix_mult.hpp"
#include "aes_decrypt.hpp"
#include "gsm.hpp"
#include "nw.hpp"
#include "nn.hpp"
// #include "binomial.hpp"

// #include <omp.h>

// #include "fmt/format.h"
// #include "rang/rang.hpp"
// #include "spdlog/spdlog.h"
#include "utils/io.hpp"

#include <thread>

#include "main.hpp"
// using namespace std::chrono_literals;
// using namespace std;

using std::stof;
using std::stoi;

void
usage()
{
  std::cout << "needs (scheduler = 0,1,2) (devices = 0,1,2) (bench = 0,1,2,3,4) (check "
               "0,1) (size * 128|size * "
               "128|image_width) (chunksize * "
               "128|min chunksize) (prop .f) (filter_width)\n"
               "  - scheduler: 0 (static), 1 (dynamic), 2 (hguided)\n"
               "  - devices: 0 (first, maybe CPU), 1 (second, maybe GPU), 2 (2 "
               "devices, first and second)\n"
               "  - bench: 0 (assign), 1 (vecadd), 2 (gaussian), 3 (ray), 4 (binomial)\n"
               "  - check: 0 (no), 1 (yes)"
               "  - problem size (use a multiple of 128 for gaussian)\n"
               "  - chunk size (use a multiple of 128 for gaussian)\n"
               "  - proportion (proportion for the first device between 0.0 and 1.0)\n"
               "  - filter width (for gaussian)\n";
}

int
main(int argc, char* argv[])
{
  if (argc <= 7) {
    usage();
    throw runtime_error("wrong number of arguments");
  }

  auto tscheduler = stoi(argv[1]);

  auto tdevices = stoi(argv[2]);

  auto tbench = stoi(argv[3]);

  auto check = stoi(argv[4]);

  auto size = stoi(argv[5]);

  auto chunksize = stoi(argv[6]);

  auto prop = stof(argv[7]);
  auto prop2 = stof(argv[8]);
  auto props = 0;//string_to_proportions(argv[7]);

  if (tbench == 0) {
    // do_assign(tscheduler, tdevices, check, size, chunksize, prop);
  //  do_vecadd_base(check, size);
  } else if (tbench == 1) {
//    do_vecadd(tscheduler, tdevices, check, size, chunksize, prop);
    // thread t1(do_vecadd, tscheduler, tdevices, check, size, chunksize, prop);
    // thread t2(do_vecadd, tscheduler, tdevices, check, size, chunksize, prop);
    // t1.join();
    // t2.join();

  } else if (tbench == 2) {
    auto image_width = size;
    auto filter_width = atoi(argv[8]);

 //   do_gaussian(tscheduler, tdevices, check, image_width, chunksize, prop, filter_width);
  } else if (tbench == 3) {
    auto image_width = size;
    auto scene_path = string(argv[8]);
//    do_ray_base(tscheduler, tdevices, check, image_width, chunksize, props, move(scene_path));
  } else if (tbench == 32) {
    auto image_width = size;
    auto scene_path = string(argv[8]);
  // do_ray(tscheduler, tdevices, check, image_width, chunksize, props, move(scene_path));
  } else if (tbench == 41) {
    auto samples = size;
    //do_binomial_base(tscheduler, tdevices, check, samples, chunksize, props);
  } else if (tbench == 42) {
    auto samples = size;
    //do_binomial(tscheduler, tdevices, check, samples, chunksize, props);
  } else if (tbench == 51) {
    // auto points = atoi(argv[8]);
    // auto width = atoi(argv[8]);
    // auto height = atoi(argv[9]);
    auto width = size;
    auto height = size;
    // 256
    // 256
    auto xstep = (4.0 / (double)width);
    auto ystep = (-4.0 / (double)height);
    auto xpos = 0.0;
    auto ypos = 0.0;
    auto max_iterations = atoi(argv[8]);
    //do_mandelbrot_base(tscheduler,
   //                    tdevices,
   //                    check,
   //                    chunksize,
   //                    props,
   //                    width,
   //                    height,
   //                    xstep,
   //                    ystep,
   //                    xpos,
   //                    ypos,
   //                    max_iterations);
  } else if (tbench == 52) {
    // auto points = atoi(argv[8]);
    // auto width = atoi(argv[8]);
    // auto height = atoi(argv[9]);
    auto width = size;
    auto height = size;
    // 256
    // 256
    auto xstep = (4.0 / (double)width);
    auto ystep = (-4.0 / (double)height);
    auto xpos = 0.0;
    auto ypos = 0.0;
    auto max_iterations = atoi(argv[8]);
   // do_mandelbrot(tscheduler,
   //               tdevices,
   //               check,
   //               chunksize,
   //               props,
   //               width,
   //               height,
   //               xstep,
   //               ystep,
   //               xpos,
   //               ypos,
   //               max_iterations);
  } else if (tbench == 61) {
    auto num_particles = size;
   // do_nbody_base(tscheduler, tdevices, check, num_particles, chunksize, props);
  } else if (tbench == 62) {
    auto num_particles = size;
   // do_nbody(tscheduler, tdevices, check, num_particles, chunksize, props);
    // } else if (tbench == 81) {
    //   auto width = size;
    //   auto height = size;
    //   auto mask = atoi(argv[8]);
    //   do_convolution_base(tscheduler, tdevices, check, chunksize, props, width, height, mask);
    // } else if (tbench == 82) {
    //   auto width = size;
    //   auto height = size;
    //   auto mask = atoi(argv[8]);
    //   do_convolution(tscheduler, tdevices, check, chunksize, props, width, height, mask);
    // } else if (tbench == 5) {
    //   auto points = atoi(argv[8]);
    //   do_taylor(tscheduler, tdevices, check, size, chunksize, props, points);
  //  //do_ray(tscheduler, tdevices, check, image_width, chunksize, prop, move(scene_path));
    // // }else if (tbench == 4){
    // //   auto samples = size;
    // //   samples = (samples / 4) ? (samples / 4) * 4 : 4; // convierte a multiplo de 4
    // //   auto steps = atoi(argv[8]);
    // //   do_binomial(tscheduler, tdevices, check, samples, chunksize, prop, steps;
  }else if (tbench == 4){
   auto N_rand = size;
   do_mersenne(tscheduler, tdevices, check, N_rand, chunksize, prop,prop2);
  }else if (tbench == 5){
   do_sobel(tscheduler, tdevices, check, 1, chunksize, prop,prop2);
  }else if (tbench == 6){
   do_watermarking(tscheduler,tdevices,check,1,chunksize,prop,prop2);
  }else if (tbench ==7){
   do_matrixMult(tscheduler, tdevices, check, size, chunksize, prop, atoi(argv[8]));
  }
  else if (tbench ==8){
   do_aesdecrypt(tscheduler, tdevices, check, size, chunksize, prop,prop2 );
  }
  else if (tbench == 9){
   do_gsm(tscheduler,tdevices,check,size,chunksize,prop);
  }
  else if (tbench == 10){
   do_needleman(tscheduler, tdevices, check, size, chunksize, prop);
  }
  else if (tbench == 11){
   do_nearest(tscheduler, tdevices, check, size, chunksize, prop,stof(argv[8]));
  }
  return 0;
}
