#ifndef ENGINECL_CONFIG_HPP
#define ENGINECL_CONFIG_HPP

#define CL_LWS 128

#define ECL_GRENDEL 1
#define ECL_NOOVERLAP 1
#define ECL_PROFILING 1  
// uncomment to use blocking calls:
#define ECL_OPERATION_BLOCKING_READ 1
#ifndef ECL_OPERATION_BLOCKING_READ
#define ECL_OPERATION_BLOCKING_READ 0
#endif

// uncomment to force unsupport:
 #define ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED 0
//#ifndef ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED
// enable if CL_VERSION_1_1 or higher
//#if CL_VERSION_1_1 == 1
//#define ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED 1
//#else // CL_VERSION_1_0 == 1
//#define ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED 0
//#endif
//#endif // ECL_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED defined

// uncomment to enable logging
// #define ECL_LOGGING 1
#ifndef ECL_LOGGING
#define ECL_LOGGING 1
#endif
#if ECL_LOGGING
#define IF_LOGGING(x) (x)
#else
#define IF_LOGGING(x)
#endif

// #define ECL_RUNTIME_WAIT_ALL_READY 1
#define ECL_RUNTIME_WAIT_ALL_READY 0

#endif /* ENGINECL_CONFIG_HPP */
