int main()
{
  int i;
  int j;
  int k;

  i = 0;
  k = 9;
  j = -100; 
  while (i <= 100) //1
  {
    i = i + 1; //2
    while (j < 20) //3
      j = i+j; //4 //5
    k = 4; //6
    while (k <=3) //7
      k = k+1; //8 //9
  } //10
}



