__kernel void
kernel1(__global int* a, __global int* b, __global int* c, int size)
{
  int idx = get_global_id(0);
  if (idx >= 0 && idx < size) {
    c[idx] = a[idx] + b[idx];
  }
}
