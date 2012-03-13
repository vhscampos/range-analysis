/* 9/15/99 JWB
 * no floating point, fixed point instead
 *
 */

/* This program enhances a 256-gray-level, 128x128 pixel image by applying
   global histogram equalization.

   This program is based on the routines and algorithms found in the book
   "C Language Algorithms for Disgital Signal Processing" by P. M. Embree
   and B. Kimble.

   Copyright (c) 1992 -- Mazen A.R. Saghir -- University of Toronto */
/* Modified to use arrays - SMP */

/*#include "traps.h" */

#define         N       128
#define         L       256
#define SIZE N*N
#define WIDTH  8
#define ANS 128

#ifdef TIME_SOFTWARE
#include "../../../include/time.h"
#endif

main()
{
  int checksum;
  int cdf,b2;
  int pixels;
  int i,j,ii,b3,foo;

  int image[N][N];
  int histogram[L];
  int gray_level_mapping[L];

#ifdef TIME_SOFTWARE
#include "../../../include/time.begin"
#endif


  /* Get the original image */

  /*  input_dsp(image, N*N, 1);*/
  for (i = 0; i < N; i++)
    for (j = 0; j < N; j++)
      image[i][j]=j;

  /* Initialize the histogram array. */

  for (i = 0; i < L; i++)
    histogram[i] = 0;

  /* Compute the image's histogram */


   for (i = 0; i < N; i++) {
     for (j = 0; j < N; ++j) {
       histogram[image[i][j]] += 1;
     }
   }

  /* Compute the mapping from the old to the new gray levels */
   
   /* fixed point with 20 binary decimal places */

  cdf = 0;
  pixels = N * N;
  for (i = 0; i < L; i++) {
    foo = (histogram[i] << 20);
    cdf += foo / pixels;
    gray_level_mapping[i] = 255 * (cdf >> 20) & 0xff;
  }

  /* Map the old gray levels in the original image to the new gray levels. */

  checksum = 0;
  for (i = 0; i < N; i++) {
    for (j = 0; j < N; ++j) {
        image[i][j] = gray_level_mapping[image[i][j]];
	checksum = (checksum + image[i][j]) & 0xff;
    }
  }

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
