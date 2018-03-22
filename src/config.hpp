#ifndef CLBALANCER_CONFIG_H
#define CLBALANCER_CONFIG_H

//mersenne twiste
//sobel
//watermarking
//aes
//uncomment this:
//#define CLB_KERNEL_TASK 1
//comente when use:
//vecadd
//gauss
//matrix_mult

#define BLOCK_SIZE_X 64
#define BLOCK_SIZE_Y 64

#define CLB_PROFILING 1

// uncomment to use blocking calls:
#define CLB_OPERATION_BLOCKING_READ 1
#ifndef CLB_OPERATION_BLOCKING_READ
#define CLB_OPERATION_BLOCKING_READ 0
#endif

// uncomment to force unsupport:
#define CLB_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED 0
#ifndef CLB_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED
// enable if CL_VERSION_1_1 or higher
#if CL_VERSION_1_1 == 1
#define CLB_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED 1
#else // CL_VERSION_1_0 == 1
#define CLB_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED 0
#endif
#endif // CLB_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED defined

#endif /* CLBALANCER_CONFIG_H */
