/****************************************************************************
 *
 *  This levdurb filter was taken from Intel's MMX website.
 *
 ***************************************************************************/

#include <limits.h>

void main ()
{
  long i;
  long m;
  long b[11];
  long Rn;
  long Rd;
  long temp;
  long r[11];
  long a[11];
  long k[11];
  

  a[0] = 8192;
  for (i = 1; i < 10; i++) {
    a[i] = rand() & 0xff;
    r[i] = a[i];
  }

  for (m = 1; m < 11; m++)
    {
      Rn = Rd = 0;
      
      for (i = 0; i < m; i++)
        {
          Rn = Rn + (r[m-1] * (long)a[i]);
          Rd = Rd + (r[i] * (long)a[i]);
        }
      
      k[m] = -Rn / ((Rd + 0x4000) >> 15);
      k[m] = (((long)k[m] * 0x7ff8) + 0x4000) >> 15;
      
      b[m] = (k[m] + 0x2) >> 2;
      
      for (i = 1; i < m; i++)
        b[m] = (((long)a[i] << 15) + (k[m] * (long)a[m-i]) + 0x4000) >> 15;
  
      for ( i = 1; i < m+1; i++)
        a[i] = b[i];
    }
}
