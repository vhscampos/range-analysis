/*
 *
 * Life benchmark
 *
 * Author: Jonathan Babb           (jbabb@lcs.mit.edu)
 *
 * Written specifically for compilation to hardware.
 *
 * Copyright @ 1999 MIT Laboratory for Computer Science, Cambridge, MA 02129
 */

#define IMAX 32
#define JMAX 32
#define ITER 10
#define ANS 196

#define SIZE IMAX*JMAX
#define WIDTH 1
#ifdef TIME_SOFTWARE
#include "../../../include/time.h"
#endif

int main()
{
  int it,i,j,sum;
  int seed, checksum;
  int data1[IMAX][JMAX],data2[IMAX][JMAX];


#ifdef TIME_SOFTWARE
#include "../../../include/time.begin"
#endif

  /* Fill with zeros */

  for (i=0;i < IMAX; i++) {
    for (j=0;j < JMAX; j++) {
      data1[i][j]=0;
      data2[i][j]=0;
    }
  }

  
  /* Initialize data array */
  
  seed = 74755;

  for(i=1;i<IMAX-1;i++) {
    for(j=1;j<JMAX-1;j++) {
      seed = (seed * 1309 + 13849);
      data1[i][j]=seed & 1;
      data2[i][j]=seed & 1;
    }
  }

  it = 0;
  while(it<ITER) {

    for(i=1;i<IMAX-1;i++) {
      for(j=1;j<JMAX-1;j++) {
	sum=
	  ((data1[i-1][j-1]+
	  data1[i-1][j])+
	  (data1[i-1][j+1]+
	  data1[i][j-1]))+
	  ((data1[i][j+1]+
	  data1[i+1][j-1])+
	  (data1[i+1][j]+
	  data1[i+1][j+1]));
	
	data2[i][j] = (sum==3) | (data1[i][j] & (sum==2));
      }
    }

    for(i=1;i<IMAX-1;i++) {
      for(j=1;j<JMAX-1;j++) {
	sum=
	  ((data2[i-1][j-1]+
	  data2[i-1][j])+
	  (data2[i-1][j+1]+
	  data2[i][j-1]))+
	  ((data2[i][j+1]+
	  data2[i+1][j-1])+
	  (data2[i+1][j]+
	  data2[i+1][j+1]));
	
	data1[i][j] = (sum==3) | (data2[i][j] & (sum==2));
      }
    }
    it+=2;
  }
  
  checksum = 0;
  for(i=0;i<IMAX;i++)
    for(j=0;j<JMAX;j++)
      checksum += data2[i][j];

#ifdef TIME_SOFTWARE
#include "../../../include/time.end"
#endif

#ifdef PRINTOK
  printf("Checksum = %d\n", checksum);
#endif
  
  if(checksum==ANS) {
#ifdef PRINTOK
    printf("RESULT: Success!\n");
    printf("RESULT: Return = %d (%x) \n", 0xbadbeef, 0xbadbeef);
#endif
    return 0xbadbeef;
  }
  else {
#ifdef PRINTOK
    printf("RESULT: Failure!\n");
    printf("RESULT: Return = %d (%x) \n", 0xbad, 0x0bad);
#endif
    return 0xbad;
  }
}
