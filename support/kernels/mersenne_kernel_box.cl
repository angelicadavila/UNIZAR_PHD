#define VECTOR 32 //number of random numbers per cycle
#define VECTOR_DIV2 16 
#define VECTOR_DIV4 8 
//parameters of Mersenne twister based on FPGA implementation
#define   MT_RNG_COUNT 4096
#define          MT_MM 9
#define          MT_NN 19
#define       MT_WMASK 0xFFFFFFFFU
#define       MT_UMASK 0xFFFFFFFEU
#define       MT_LMASK 0x1U
#define      MT_SHIFT0 12
#define      MT_SHIFTB 7
#define      MT_SHIFTC 15
#define      MT_SHIFT1 18
#define PI 3.14159265358979f

// Mersenne twister constants
#define MT_M 397 
#define MT_N 624 
#define MATRIX_A   0x9908b0dfUL 
#define UPPER_MASK 0x80000000UL 
#define LOWER_MASK 0x7fffffffUL 

// Used to ensure that the uniformly generated random numbers are in the range (0,1)
#define CLAMP_ZERO 0x1.0p-126f 
#define CLAMP_ONE  0x1.fffffep-1f

float2 box_muller(float a, float b);


float2 box_muller(float a, float b)
{
   float radius = sqrt(-2.0f * log(a));
   float angle = 2.0f*b;
   float2 result;
   result.x = radius*cospi(angle);
   result.y = radius*sinpi(angle);
   return result;
}

__kernel void mersenne_twister_generate(__global uint* restrict y_in,
							    __global float* restrict U_out, uint N, uint offset)
{
   unsigned int mt[MT_N];
  
   bool read_from_initialization = true;
   ushort num_initializers_read = 0;
   uint next_chunk=offset; //for VECTOR=64
   for (ulong n=0; n<N/VECTOR+(MT_N/VECTOR+1); n++) {
      uint y[VECTOR];
      bool write_output=false;
      if (read_from_initialization) {
         #pragma unroll VECTOR_DIV4
         for (int i=0; i<VECTOR_DIV4; i++) {
            y[i]=y_in[i+next_chunk];
            y[i+1*VECTOR_DIV4]=y_in[i+1*VECTOR_DIV4+next_chunk];
            y[i+2*VECTOR_DIV4]=y_in[i+2*VECTOR_DIV4+next_chunk];
            y[i+3*VECTOR_DIV4]=y_in[i+3*VECTOR_DIV4+next_chunk];
         }
         if (++num_initializers_read == MT_N/VECTOR+1) read_from_initialization=false;
      } else {
         // You'll notice quite alot of this design pattern in this particular example
         // We unroll inner loops fully as much as possible. This technique will generally
         // lead to the best performance on the FPGA as long as the resulting pipelined
         // implementation can fit within the avilable resources.
         //
         #pragma unroll VECTOR
         for (int i=0; i<VECTOR; i++) {
            y[i] = (mt[i]&UPPER_MASK)|(mt[i+1]&LOWER_MASK);
            y[i] = mt[i+MT_M] ^ (y[i] >> 1) ^ (y[i] & 0x1UL ? MATRIX_A : 0x0UL);
         }
         write_output=true;
      }

      #pragma unroll
      for (int i=0; i<MT_N-VECTOR; i++) {
         mt[i]=mt[i+VECTOR];
      }

      float U[VECTOR], Z[VECTOR];
      #pragma unroll VECTOR
      for (int i=0; i<VECTOR; i++) {
         mt[MT_N-VECTOR+i]=y[i];

         // Tempering
         y[i] ^= (y[i] >> 11);
         y[i] ^= (y[i] << 7) & 0x9d2c5680UL;
         y[i] ^= (y[i] << 15) & 0xefc60000UL;
         y[i] ^= (y[i] >> 18);

         U[i] = (float)y[i] / 4294967296.0f;

         if (U[i] == 0.0f) U[i] = CLAMP_ZERO;
         if (U[i] == 1.0f) U[i] = CLAMP_ONE;
      }
      if(write_output){

         #pragma unroll VECTOR_DIV2 
         for (int i=0; i<VECTOR_DIV2; i++) {
            float2 z = box_muller(U[2*i], U[2*i+1]);
            Z[2*i] = z.x;
            Z[2*i+1] = z.y;
         }

         #pragma unroll VECTOR_DIV4
         for (int i=0; i<VECTOR_DIV4; i++) {
            U_out[i+next_chunk]=Z[i];
            U_out[i+1*VECTOR_DIV4+next_chunk]=Z[i+1*VECTOR_DIV4];
            U_out[i+2*VECTOR_DIV4+next_chunk]=Z[i+2*VECTOR_DIV4];
            U_out[i+3*VECTOR_DIV4+next_chunk]=Z[i+3*VECTOR_DIV4];
         }         
        next_chunk+=VECTOR; 
      }
    }
}



