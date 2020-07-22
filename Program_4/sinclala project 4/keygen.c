

#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <string.h>
#include <time.h>


int main(int argc, char *argv[])
{

  srand(time(0));
  
  if (argc < 1)
    {
      perror("not enough arguements.");
    }
  char* p;
  
  int amount = strtol(argv[1], &p, 10);
  
  //int *letters = (int *)malloc(sizeof(int)*amount);
  int temp;
  char tempC;
  int i=0;
  for (i; i < amount; i++)
    {
      //generate random numbers between 0 and 26
      //letters[i] = rand() % 27 + 65;
      temp = rand() % 27 + 65;
      if (temp == 91)
        {
          temp = 32;
        }
      tempC = (char)temp;
      printf("%c", tempC);
    }
  printf("\n");
  
  
  
  return 0;
}
