/*
 *
 * Life benchmark
 *
 * Authors: Rajeev Kumar Barua      (barua@lcs.mit.edu)
 *          Jonathan Babb           (jbabb@lcs.mit.edu)
 *
 * Copyright @ 1997 MIT Laboratory for Computer Science, Cambridge, MA 02129
 */

/* Caveats for Life: Does not take advantage of CPU
   bit twiddling to get improve software performance */

#define SIZEX 1
#define SIZEY 30
#define IMAX  33 /* SIZEX*32+1 */  /* imax=last column (of fake boundary)*/
#define JMAX  31 /* SIZEY+1    */  /* jmax=last row (of fake boundary)*/
#define JMAX1 32 /* JMAX+1     */
#define ITER  20
#define MAXSIZE (SIZEX*32+2)*(SIZEY+2)

/* for software array, indices into unpacked data, and fake boundary */

#define soft_u_ref(i, j) ((j)+(i)*(SIZEY+2))

/* for software array, indices into packed data, k refers to bit
position beginning at msb at leftmost = 0, and fake boundary */

#define soft_p_ref(i, j, k) ((j+1)+((i*32)+k+1)*(SIZEY+2))

#define ANS 191

#define SIZE SIZEX*SIZEY*32
#define WIDTH 1
#ifdef TIME_SOFTWARE
#include "../../../include/time.h"
#endif

int main()
{
  int it,i,j,k,data,sum;
  int seed, checksum;
  int data1[MAXSIZE],data2[MAXSIZE];

#ifdef TIME_SOFTWARE
#include "../../../include/time.begin"
#endif

  /* Initialize data array */
  
  seed = 74755;

  for(i=0;i<SIZEX;i++) {
    for(j=0;j<SIZEY;j++) {

      seed = (seed * 1309 + 13849);;
      data = seed;
	
      /* Initialize software data after unpacking */

      for (k=0;k<32;k++) {
	int tmp = ((data & (1<<(31-k)))!=0);
	data1[soft_p_ref(i,j,k)]=tmp;
	data2[soft_p_ref(i,j,k)]=tmp;
      }
    } 
  }

  /* Fill boundaries with zeros */

  for (j=0;j < JMAX1; j++) {
    data1[soft_u_ref(0,j)] = 0;
    data2[soft_u_ref(0,j)] = 0;
    data1[soft_u_ref(IMAX,j)] = 0;
    data2[soft_u_ref(IMAX,j)] = 0;
  }

  for (i=1;i < IMAX; i++) {
    data1[soft_u_ref(i,0)] = 0;
    data2[soft_u_ref(i,0)] = 0;
    data1[soft_u_ref(i,JMAX)] = 0;
    data2[soft_u_ref(i,JMAX)] = 0;
  }
  
  it = 0;
  while(it<ITER) {
    
    for(i=1;i<IMAX;i++) {
      for(j=1;j<JMAX;j++) {
	sum = data1[soft_u_ref(i-1,j)] + data1[soft_u_ref(i+1,j)] +
	  data1[soft_u_ref(i,j-1)] + data1[soft_u_ref(i,j+1)] +
	  data1[soft_u_ref(i-1,j-1)] + data1[soft_u_ref(i-1,j+1)] +
	  data1[soft_u_ref(i+1,j-1)] + data1[soft_u_ref(i+1,j+1)];
	if (data1[soft_u_ref(i,j)]) 
	{
	  data2[soft_u_ref(i,j)] = ((sum == 2) || (sum == 3)) ? 1:0;
	}
	else
	{
	  data2[soft_u_ref(i,j)] = (sum == 3) ? 1:0;
	}
      }
    }
    
    for(i=1;i<IMAX;i++) {
      for(j=1;j<JMAX;j++) {
	sum = data2[soft_u_ref(i-1,j)] + data2[soft_u_ref(i+1,j)] +
	  data2[soft_u_ref(i,j-1)] + data2[soft_u_ref(i,j+1)] +
	  data2[soft_u_ref(i-1,j-1)] + data2[soft_u_ref(i-1,j+1)] +
	  data2[soft_u_ref(i+1,j-1)] + data2[soft_u_ref(i+1,j+1)];
	if (data2[soft_u_ref(i,j)]) 
	  data1[soft_u_ref(i,j)] = ((sum == 2) || (sum == 3)) ? 1:0;
	else
	  data1[soft_u_ref(i,j)] = (sum == 3) ? 1:0;
      }
    }
    it += 2;
  }
  
  checksum = 0;
  for(i=0;i<IMAX;i++)
    for(j=0;j<JMAX;j++)
      checksum += data2[soft_u_ref(i,j)];

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
