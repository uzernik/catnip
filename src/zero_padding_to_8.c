#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
/** create the normalized, 0-padded number for the media file in .sh *******/
int main(int ac, char **av) {
  char ttt[60];
  if (ac != 2) {
    fprintf(stderr,"Error: takes doc_id\n");
    exit(0);
  }
  sprintf(ttt,"%08d",atoi(av[1]));
  printf("%s",ttt);
  return 0;
}
