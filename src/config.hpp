#ifndef CLBALANCER_CONFIG_H
#define CLBALANCER_CONFIG_H

#define CLB_KERNEL_TASK 1


#define CLB_INTERNAL_CHUNK 1
//use with mersenne twister kernel
//#define CLB_INTERNAL_CHUNK 64
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
