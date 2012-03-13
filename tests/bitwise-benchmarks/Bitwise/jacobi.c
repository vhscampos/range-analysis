/****************************************************************************
 *
 *  Jacobi
 *
 ***************************************************************************/


#define ITER  32
#define SIZEX  64
#define SIZEY  64
#define WIDTH   8

int main() {
  int r;
  int i,j,a2;
  int a[SIZEX][SIZEY];
  int b[SIZEX][SIZEY];
  int checksum;
/*  int ans = 131072;*/
  int ans = 557568;

  /* Initialize data array */

  a2=0;
  for (i=0;i<SIZEX;i++) {
    a2++;
    for (j=0;j<SIZEY;j++) {

      /* an abitrary boundary condition of the appropriate WIDTH */	  
      a[i][j] = 1 << (WIDTH-1);
      b[i][j] = 1 << (WIDTH-1);

    }
  }
  
  for (r=0;r<ITER;r+=2) {
    for (i=1;i<SIZEX-1;i++) {
      for (j=1;j<SIZEY-1;j++)
	{
	  a[i][j] = (b[i-1][j] + b[i+1][j] + b[i][j-1] + b[i][j+1])>>2;
	}
    }
    for (i=1;i<SIZEX-1;i++) {
      for (j=1;j<SIZEY-1;j++)
	{
	  b[i][j] = (a[i-1][j] + a[i+1][j] + a[i][j-1] + a[i][j+1])>>2;
	}
    }
  }
  
  /* check sum */
  checksum = 0;
  for (i=0;i<SIZEX;i++)
    for (j=0;j<SIZEY;j++)
      checksum += b[i][j];
  
#ifdef PRINTOK
  printf("Checksum = %d\n", checksum);
#endif
  
  if(checksum==ans)
    return 195935983; 
  else
    return 0;

return checksum; /*195935983*/
}
