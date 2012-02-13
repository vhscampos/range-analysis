int main() 
{
  int i;
  int j;
  int k;
  i=1;
  k=0;
  while (k<1000) {
    while (i<100) 
      {
	j=100;
	while (j>1)
	  j = j-i;
	i=i+1;
      }
    k=k-j;
  }
}
