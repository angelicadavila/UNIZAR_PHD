#define BLOCK_SIZE (16)


int maximum(int a, int b, int c)
{
  int k;
  if( a <= b )
    k = b;
  else
    k = a;

  if( k <=c )
    return(c);
  else
    return(k);
}

__kernel void 
nw_kernel1(__global int * restrict reference, 
           __global int volatile * restrict input_itemsets,
           __global int * restrict output_itemsets,           
           uint max_cols,
           uint penalty,
           uint iterations, uint offset) {
  int max_rows = max_cols;
  int bx =get_global_id(0);// init_block +  get_gloabal_id(0);
//  for (int bx= init_block; bx<init_block+iterations; bx++)
{

    int base = offset + BLOCK_SIZE * bx + 1+offset;

    int sr[BLOCK_SIZE];
    
    #pragma unroll
    for (int i = 0; i < BLOCK_SIZE; ++i) {
      sr[i] = input_itemsets[base + i];
    }

    for (int j = 1; j < max_rows - 1; ++j) {
      int diag = input_itemsets[base + max_cols * j - 1 - max_cols];
      int left = input_itemsets[base + max_cols * j - 1];

    #pragma unroll
      for (int i = 0; i < BLOCK_SIZE; ++i) {
        int index = base + i + max_cols * j;
        int above = sr[i];
        int v = 
            maximum(
                diag + reference[index], 
                left - penalty,
                above - penalty);
        diag = above;
        left = v;
        sr[i] = v;
        output_itemsets[index] = v;
      }
    }
  }
}
