%{
  /* PARAMS:  it takes a flex input for yylex()
     1.  the C program in a *.blex file, it has a #BLEX "blabla.blex" directive
     2.  the flex code in a blabla.h file
     INPUT: the blabla.blex file
     OUTPUT: a blabla.lex file
     SYNTAXL
     the included file should have 
---------------------
     %x
     definitions
     %%
     RULES
     %%
---------------------
   */
  #define MAX_BUFFER 5000
  int found_blex = 0;
%}

file_name [^\"\n]+
%%
"#BLEX"[\ ]+\"{file_name}\"[\ ]*/\n {
  found_blex = 1;
  printf("\n       /*-----------------START INCLUDED------------- */\n");
  char *pp = strchr(yytext,'\"');
  if (pp) {
    static char file_name[500] ;
    sscanf(pp+1,"%[^\"]\"",file_name);
    FILE *included_file = fopen(file_name,"r");
    if (included_file) {
      static char line_buff[MAX_BUFFER];
      while (fgets(line_buff, MAX_BUFFER, included_file)) {
	printf("%s",line_buff);
      }
    } else {
      fprintf(stderr,"ERROR:Could not open file:%s:\n",file_name);
    }
  } else {
    fprintf(stderr,"ERROR:  Incorrect syntax: :%s:\n",yytext);
  }
  printf("\n       /*-----------------END   INCLUDED------------- */\n");
}
%%

int main() {
  found_blex = 0;
  yylex();
  if (found_blex == 0) {
    fprintf(stderr,"ERROR: not found a blex include statement\n");
  }
  return 0;
}
