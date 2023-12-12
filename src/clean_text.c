#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>	/* for bsearch() */


char *prog;
int debug;
char *origPDF;

char *do_clean_text(char *text) {
    int ii;
    int jj = 0;
    static char bext[5000];
    int first_junk = 0;
    for (ii = 0; ii < strlen(text); ii++) {
      if (isalnum(text[ii])) {
	bext[jj++] = tolower(text[ii]);
	first_junk = 1;
      } else {
	if (first_junk == 1) {
	  bext[jj++] = '_';
	}
	first_junk = 0;
      }
    }
    bext[jj++] = '\0';
    return bext;
} // clean_text()


int main(int argc, char **argv) {
  int get_opt_index;
  int c_getopt;
  prog = argv[0];
  origPDF = argv[1];
  fprintf(stderr,"IN 0=%s: 1=%s\n",argv[0], argv[1]);  
  char *ret=do_clean_text(origPDF);
  printf("%s",ret);
  return 0;
} // main()

