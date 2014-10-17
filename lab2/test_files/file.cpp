#include <stdio.h>
#include <stdlib.h>

int main()
{
  FILE *out=fopen("filename","w");
  char ch=0xff;
  if(out==NULL){
    {
      perror("Error on file open");
      exit(EXIT_FAILURE);
    }
    if( !fwrite(&ch,sizeof(char),100,out) || fclose(out)==EOF)
    {
      perror("File I/O failure");
      exit(EXIT_FAILURE);
    }
    return 0;
  }
}
