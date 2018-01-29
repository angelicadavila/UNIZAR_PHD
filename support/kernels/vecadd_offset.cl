// -*- mode: c -*-

__kernel void
#if CL_SUPPORT_KERNEL_OFFSET == 1
vecadd(__global int* in1, __global int* in2, __global int* out, int size){
  int idx = get_global_id(0);
#else
vecadd(__global int* in1, __global int* in2, __global int* out, int size, uint offset){
  int idx = get_global_id(0) + offset;
#endif

  if (idx >= 0 && idx < size){
    out[idx] = in1[idx] + in2[idx];
  }

}
