#include "rand.hpp"

float
random(float rand_max, float rand_min)
{
  float result;
  result = (float)rand() / (float)RAND_MAX;

  return ((1.0f - result) * rand_min + result * rand_max);
}
