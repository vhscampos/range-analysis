/****************************************************************************
 *
 *  A convolution algorithm.
 *
 ***************************************************************************/

#define NTAPS   16
#define SIZE   256
#define WIDTH   16
#define ANS  16128

#ifdef TIME_SOFTWARE
#include "../../../include/time.h"
#endif

int main()
{
  int A[SIZE];
  int B[SIZE];
  int taps[NTAPS];
  int checksum;
  int i;

#ifdef TIME_SOFTWARE
#include "../../../include/time.begin"
#endif

  for(i = 0; i < NTAPS;i++)
    taps[i] = (1 << WIDTH) - 1;

  for(i = 0; i < SIZE; i++)
    A[i] = (1 << WIDTH) - 1;

  for(i = 0; i < (SIZE - NTAPS); i++) {
    B[i] =
      A[i] * taps[0] +
      A[i+1] * taps[1] +
      A[i+2] * taps[2] +
      A[i+3] * taps[3] +
      A[i+4] * taps[4] +
      A[i+5] * taps[5] +
      A[i+6] * taps[6] +
      A[i+7] * taps[7] +
      A[i+8] * taps[8] +
      A[i+9] * taps[9] +
      A[i+10] * taps[10] +
      A[i+11] * taps[11] +
      A[i+12] * taps[12] +
      A[i+13] * taps[13] +
      A[i+14] * taps[14] +
      A[i+15] * taps[15];
  }

  checksum = 0;
  for (i=0;i<SIZE;i++)
    checksum += A[i] & 0x3f;

#ifdef TIME_SOFTWARE
#include "../../../include/time.end"
#endif

#ifdef PRINTOK
  printf("RESULT: Checksum %d\n", checksum);
#endif
  
  if(checksum==ANS) {
#ifdef PRINTOK
    printf("RESULT: Success!\n");
    printf("RESULT: Return-value %d (%x) \n", 0xbadbeef, 0xbadbeef);
#endif
    return 0xbadbeef;
  }
  else {
#ifdef PRINTOK
    printf("RESULT: Failure!\n");
    printf("RESULT: Return-value %d (%x) \n", 0xbad, 0x0bad);
#endif
    return 0xbad;
  }
}
