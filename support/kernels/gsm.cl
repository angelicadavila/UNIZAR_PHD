typedef short word;    /* 16 bit signed int    */
typedef long longword;    /* 32 bit signed int    */

#define MIN_WORD  ((-32767)-1)
#define MAX_WORD  ( 32767)

#define SASR(x, by) ((x) >> (by))

#define GSM_MULT_R(a, b)  gsm_mult_r(a, b)
#define GSM_MULT(a, b)    gsm_mult(a, b)
#define GSM_ADD(a, b)   gsm_add(a, b)
#define GSM_ABS(a)    gsm_abs(a)

#define N 160
#define M 8

#define saturate(x)     \
    ((x) < MIN_WORD ? MIN_WORD : (x) > MAX_WORD ? MAX_WORD: (x))

word
gsm_add (word a, word b)
{
  longword sum;
  sum = (longword) a + (longword) b;
  return saturate (sum);
}

word
gsm_mult (word a, word b)
{
  if (a == MIN_WORD && b == MIN_WORD)
    return MAX_WORD;
  else
    return SASR ((longword) a * (longword) b, 15);
}

word
gsm_mult_r (word a, word b)
{
  longword prod;
  if (b == MIN_WORD && a == MIN_WORD)
    return MAX_WORD;
  else
    {
      prod = (longword) a *(longword) b + 16384;
      prod >>= 15;
      return prod & 0xFFFF;
    }
}

word
gsm_abs (word a)
{
  return a < 0 ? (a == MIN_WORD ? MAX_WORD : -a) : a;
}

__constant unsigned char bitoff[256] = {
  8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

word
gsm_norm (longword a)
/*
 * the number of left shifts needed to normalize the 32 bit
 * variable L_var1 for positive values on the interval
 *
 * with minimum of
 * minimum of 1073741824  (01000000000000000000000000000000) and
 * maximum of 2147483647  (01111111111111111111111111111111)
 *
 *
 * and for negative values on the interval with
 * minimum of -2147483648 (-10000000000000000000000000000000) and
 * maximum of -1073741824 ( -1000000000000000000000000000000).
 *
 * in order to normalize the result, the following
 * operation must be done: L_norm_var1 = L_var1 << norm( L_var1 );
 *
 * (That's 'ffs', only from the left, not the right..)
 */
{

  if (a < 0)
    {
      if (a <= -1073741824)
    return 0;
      a = ~a;
    }

  return a & 0xffff0000 ?
    (a & 0xff000000 ? -1 + bitoff[0xFF & (a >> 24)] :
     7 + bitoff[0xFF & (a >> 16)])
    : (a & 0xff00 ? 15 + bitoff[0xFF & (a >> 8)] : 23 + bitoff[0xFF & a]);
}

word
gsm_div (word num, word denum)
{
  longword L_num;
  longword L_denum;
  word div;
  int k;

  L_num = num;
  L_denum = denum;
  div = 0;
  k = 15;
  /* The parameter num sometimes becomes zero.
   * Although this is explicitly guarded against in 4.2.5,
   * we assume that the result should then be zero as well.
   */

  if (num == 0)
    return 0;

  while (k--)
    {
      div <<= 1;
      L_num <<= 1;

      if (L_num >= L_denum)
    {
      L_num -= L_denum;
      div++;
    }
    }

  return div;
}





/*
 *  4.2.4 .. 4.2.7 LPC ANALYSIS SECTION
 */

/* 4.2.4 */


void
Autocorrelation (word * s /* [0..159]     IN/OUT  */ ,
         longword * L_ACF /* [0..8]       OUT     */ )
/*
 *  The goal is to compute the array L_ACF[k].  The signal s[i] must
 *  be scaled in order to avoid an overflow situation.
 */
{
  int k, i;

  word temp;
  word smax;
  word scalauto, n;
  word *sp;
  word sl;

  /*  Search for the maximum.
   */
  smax = 0;
  for (k = 0; k <= 159; k++)
    {
      temp = GSM_ABS (s[k]);
      if (temp > smax)
    smax = temp;
    }

  /*  Computation of the scaling factor.
   */
  if (smax == 0)
    scalauto = 0;
  else
    scalauto = 4 - gsm_norm ((longword) smax << 16);    /* sub(4,..) */

  if (scalauto > 0 && scalauto <= 4)
    {
      n = scalauto;
      for (k = 0; k <= 159; k++)
    s[k] = GSM_MULT_R (s[k], 16384 >> (n - 1));
    }

  /*  Compute the L_ACF[..].
   */
  {
    sp = s;
    sl = *sp;

#define STEP(k)  L_ACF[k] += ((longword)sl * sp[ -(k) ]);

#define NEXTI    sl = *++sp
    for (k = 8; k >= 0; k--)
      L_ACF[k] = 0;

    STEP (0);
    NEXTI;
    STEP (0);
    STEP (1);
    NEXTI;
    STEP (0);
    STEP (1);
    STEP (2);
    NEXTI;
    STEP (0);
    STEP (1);
    STEP (2);
    STEP (3);
    NEXTI;
    STEP (0);
    STEP (1);
    STEP (2);
    STEP (3);
    STEP (4);
    NEXTI;
    STEP (0);
    STEP (1);
    STEP (2);
    STEP (3);
    STEP (4);
    STEP (5);
    NEXTI;
    STEP (0);
    STEP (1);
    STEP (2);
    STEP (3);
    STEP (4);
    STEP (5);
    STEP (6);
    NEXTI;
    STEP (0);
    STEP (1);
    STEP (2);
    STEP (3);
    STEP (4);
    STEP (5);
    STEP (6);
    STEP (7);

    for (i = 8; i <= 159; i++)
      {

    NEXTI;

    STEP (0);
    STEP (1);
    STEP (2);
    STEP (3);
    STEP (4);
    STEP (5);
    STEP (6);
    STEP (7);
    STEP (8);
      }

    for (k = 8; k >= 0; k--)
      L_ACF[k] <<= 1;

  }
  /*   Rescaling of the array s[0..159]
   */
  if (scalauto > 0)
    for (k = 159; k >= 0; k--)
      *s++ <<= scalauto;
}

/* 4.2.5 */

void
Reflection_coefficients (longword * L_ACF /* 0...8        IN      */ ,
             word * r /* 0...7        OUT     */ )
{
  int i, m, n;
  word temp;
  word ACF[9];          /* 0..8 */
  word P[9];            /* 0..8 */
  word K[9];            /* 2..8 */

  /*  Schur recursion with 16 bits arithmetic.
   */

  if (L_ACF[0] == 0)
    {
      for (i = 8; i > 0; i--)
    *r++ = 0;
      return;
    }

  temp = gsm_norm (L_ACF[0]);
  for (i = 0; i <= 8; i++)
    ACF[i] = SASR (L_ACF[i] << temp, 16);

  /*   Initialize array P[..] and K[..] for the recursion.
   */

  for (i = 1; i <= 7; i++)
    K[i] = ACF[i];
  for (i = 0; i <= 8; i++)
    P[i] = ACF[i];

  /*   Compute reflection coefficients
   */
  for (n = 1; n <= 8; n++, r++)
    {

      temp = P[1];
      temp = GSM_ABS (temp);
      if (P[0] < temp)
    {
      for (i = n; i <= 8; i++)
        *r++ = 0;
      return;
    }

      *r = gsm_div (temp, P[0]);

      if (P[1] > 0)
    *r = -*r;       /* r[n] = sub(0, r[n]) */
      if (n == 8)
    return;

      /*  Schur recursion
       */
      temp = GSM_MULT_R (P[1], *r);
      P[0] = GSM_ADD (P[0], temp);

      for (m = 1; m <= 8 - n; m++)
    {
      temp = GSM_MULT_R (K[m], *r);
      P[m] = GSM_ADD (P[m + 1], temp);

      temp = GSM_MULT_R (P[m + 1], *r);
      K[m] = GSM_ADD (K[m], temp);
    }
    }
}

/* 4.2.6 */

void
Transformation_to_Log_Area_Ratios (word * r /* 0..7    IN/OUT */ )
/*
 *  The following scaling for r[..] and LAR[..] has been used:
 *
 *  r[..]   = integer( real_r[..]*32768. ); -1 <= real_r < 1.
 *  LAR[..] = integer( real_LAR[..] * 16384 );
 *  with -1.625 <= real_LAR <= 1.625
 */
{
  word temp;
  int i;


  /* Computation of the LAR[0..7] from the r[0..7]
   */
  for (i = 1; i <= 8; i++, r++)
    {

      temp = *r;
      temp = GSM_ABS (temp);

      if (temp < 22118)
    {
      temp >>= 1;
    }
      else if (temp < 31130)
    {
      temp -= 11059;
    }
      else
    {
      temp -= 26112;
      temp <<= 2;
    }

      *r = *r < 0 ? -temp : temp;
    }
}

/* 4.2.7 */

void
Quantization_and_coding ( word * LAR /* [0..7]       IN/OUT  */ )
{
  word temp;


  /*  This procedure needs four tables; the following equations
   *  give the optimum scaling for the constants:
   *
   *  A[0..7] = integer( real_A[0..7] * 1024 )
   *  B[0..7] = integer( real_B[0..7] *  512 )
   *  MAC[0..7] = maximum of the LARc[0..7]
   *  MIC[0..7] = minimum of the LARc[0..7]
   */

#   undef STEP
#   define  STEP( A, B, MAC, MIC )      \
        temp = GSM_MULT( A,   *LAR );   \
        temp = GSM_ADD(  temp,   B );   \
        temp = GSM_ADD(  temp, 256 );   \
        temp = SASR(     temp,   9 );   \
        *LAR  =  temp>MAC ? MAC - MIC : (temp<MIC ? 0 : temp - MIC); \
        LAR++;

  STEP (20480, 0, 31, -32);
  STEP (20480, 0, 31, -32);
  STEP (20480, 2048, 15, -16);
  STEP (20480, -2560, 15, -16);

  STEP (13964, 94, 7, -8);
  STEP (15360, -1792, 7, -8);
  STEP (8534, -341, 3, -4);
  STEP (9036, -1144, 3, -4);

#   undef   STEP
}

void
Gsm_LPC_Analysis (word * s /* 0..159 signals       IN/OUT  */ ,
          word * LARc /* 0..7   LARc's        OUT     */ )
{
  longword L_ACF[9];

  Autocorrelation (s, L_ACF);
  Reflection_coefficients (L_ACF, LARc);
  Transformation_to_Log_Area_Ratios (LARc);
  Quantization_and_coding (LARc);
}





__kernel
//#ifdef FPGA
//__attribute__((num_compute_units(1)))
//__attribute__((task))
//#endif
//__attribute__((reqd_work_group_size(1,1,1)))
void gsm_main(__global unsigned short* restrict  input_data,  
              __global short* restrict  output_data,
              __global short* restrict  output_data2,
    const unsigned int iterations, 
    const unsigned int offset   
    )
{
    word so[N];
    word LARc[M];
  uint it=get_global_id(0); 
  for(uint it=0; it<iterations; it ++)
  {
    uint index=offset+N*it;
   #pragma unroll
    for (int i = 0; i < N; i++)
    {
        so[i] = input_data[i+index];
    }

    Gsm_LPC_Analysis (so, LARc);

   #pragma unroll
    for (int i = 0; i < N; i++)
    {
       //main_result += (so[i] != outData[i]);
       //printf("[%d] = %d\n",i,so[i]);
       output_data[i+offset] = so[i];
    }
    index=offset+M*it;
   #pragma unroll
    for (int i = 0; i < M; i++)
    {
       //main_result += (so[i] != outData[i]);
       //printf("[%d] = %d\n",i,so[i]);
       output_data2[i+index] = LARc[i];
    }
  }

}
