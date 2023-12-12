%%
.|\n ;
"\nPages:"[^0-9]+[0-9]{1,4} {
  int no_of_pages = 0;
  sscanf(yytext+1,"Pages: %d",&no_of_pages);
  printf("%d",no_of_pages);
  exit(-1);
}
%%
