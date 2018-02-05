// -*- mode: c -*-

/* #pragma OPENCL EXTENSION cl_khr_select_fprounding_mode : enable */
/* #pragma OPENCL EXTENSION cl_intel_printf : enable */
/* #pragma OPENCL EXTENSION cl_amd_printf : enable */

__kernel void
#if CLB_KERNEL_GLOBAL_WORK_OFFSET_SUPPORTED == 1
gaussian_blur(__global uchar4* blurred,
              __global uchar4* input,
              int rows,
              int cols,
              __global float* filterWeight,
              int filterWidth)
{
  int tid = get_global_id(0);
#else
gaussian_blur(__global uchar4* blurred,
              __global uchar4* input,
              int rows,
              int cols,
              __global float* filterWeight,
              int filterWidth,
              uint offset)
{
  int tid = get_global_id(0) + offset;
#endif

  if (tid < rows * cols) {
    int r = tid / cols; // current row
    int c = tid % cols; // current column

    int middle = filterWidth / 2;
    float blurX = 0.f; // will contained blurred value
    float blurY = 0.f; // will contained blurred value
    float blurZ = 0.f; // will contained blurred value
    int width = cols - 1;
    int height = rows - 1;

    for (int i = -middle; i <= middle; ++i) // rows
    {
      for (int j = -middle; j <= middle; ++j) // columns
      {
        // Clamp filter to the image border
        // int h=min(max(r+i, 0), height);
        // int w=min(max(c+j, 0), width);

        int h = r + i;
        int w = c + j;
        if (h > height || h < 0 || w > width || w < 0) {
          continue;
        }

        // Blur is a product of current pixel value and weight of that pixel.
        // Remember that sum of all weights equals to 1, so we are averaging sum
        // of all pixels by their weight.
        int idx = w + cols * h; // current pixel index
        float pixelX = (input[idx].x);
        float pixelY = (input[idx].y);
        float pixelZ = (input[idx].z);

        idx = (i + middle) * filterWidth + j + middle;
        float weight = filterWeight[idx];

        blurX += pixelX * weight;
        blurY += pixelY * weight;
        blurZ += pixelZ * weight;
      }
    }

    // if (tid == 2592){
    //    printf("%f %f %d %d\n", blurZ, round(blurZ), (unsigned char)round(blurZ),
    //    (int)round(blurZ)); printf("%f %d\n", blurZ, convert_uchar_rte(blurZ)); printf("%f %d\n",
    //    blurZ, convert_uchar_rtz(blurZ)); printf("%f %d\n", blurZ, convert_uchar_rtp(blurZ));
    // }

    // blurred[tid].x = (unsigned char)(blurX);
    // blurred[tid].y = (unsigned char)(blurY);
    // blurred[tid].z = (unsigned char)(blurZ);

    blurred[tid].x = (unsigned char)round(blurX);
    blurred[tid].y = (unsigned char)round(blurY);
    blurred[tid].z = (unsigned char)round(blurZ);
  }
}
