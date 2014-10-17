#include <stdio.h>
/* Still the funniest code ever written IMHO. */
float Q_rsqrt(float number) {
  const float threehalfs = 1.5F;
  float x2 = number * 0.5F;
  float y = number;
  long i = *(long *) &y; // evil floating point bit level hacking
  i = 0x5f3759df - (i >> 1); // what the fuck?
  y = *(float *) &i;
  y = y * ( threehalfs - (x2*y*y)); // 1st iteration
  // y = y * (threehalfs - (x2*y*y)); // 2nd iteration, this can be removed
  return y;
}
int main() {
  printf("Result is %.6f\n", Q_rsqrt(0.15625F));
}
