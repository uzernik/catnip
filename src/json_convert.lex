%x wordGeo1 lineGeo1 relate
%{
  int page_no = -1;
  int no_of_pages = -1;
  int page_exists = 0;
  char line_text[5000];
  char block_id[5000];
  char block_ids[50000];  
  float conf, x_coord, y_coord;
%}
%%
.|\n  ;
<wordGeo1>.|\n  ;
<relate>[^\]] ;

Pages": "[0-9]+  {
  if (page_exists == 0) {
    sscanf(yytext,"Pages: %d",&no_of_pages);  
    printf("{\n  \"DocumentMetadata\": {\n    \"Pages\": %d\n  },\n", no_of_pages);
    printf("\"Blocks\": [\n");
    page_exists = 1;
  }
}
"Type: PAGE\nPage: "[0-9]+ {
  sscanf(yytext,"Type: PAGE\nPage: %d", &page_no);
  printf("  {\n    \"BlockType\": \"PAGE\",\n");
  printf("    \"Page\": %d,\n  },\n",page_no);
}

"Type: LINE\nText: "[^\n]+\n"Confidence: "[0-9\.]+"\%"\n"Page: "[0-9]+\n { 
  sscanf(yytext,"Type: LINE\nText: %[^\n]\nConfidence: %f%*s\nPage: %d\n", line_text,&conf,&page_no);
  printf("  {\n    \"BlockType\": \"LINE\",\n");
  printf("    \"Confidence\": %f,\n",conf);
  printf("    \"Text\": \"%s\",\n",line_text);
  //printf("    \"Page\": \"%d\",\n  },\n",page_no);
  BEGIN lineGeo1;  
}

<lineGeo1>"Geometry\n" {
    printf("    \"Geometry\": {\n");  
}

<lineGeo1>"Polygon:" {
    printf("       \"Polygon\":");  
}

<lineGeo1>"["/"{" {
  printf(" [\n");
}

<lineGeo1>"{"/"\'" {
  printf("          {\n");
}

<lineGeo1>"Bounding".* {
  ;
}

<lineGeo1>"\'X\': "[0-9\.]+\, {
  sscanf(yytext,"'X': %f", &x_coord);
  printf("                \"X\": %f,\n",x_coord);
}

<lineGeo1>"\'Y\': "[0-9\.]+"}" {
  sscanf(yytext,"'Y': %f", &y_coord);
  printf("                \"Y\": %f\n          },\n",y_coord);
}

<lineGeo1>"]"/\n {
  printf("\n       ]\n    },\n");
  BEGIN 0;
}

"Block Id:"[\ \t]+[^\ \n\t]+ {
  sscanf(yytext,"Block Id: %[^\n\t\n]", block_id);
}

"Type: WORD\nText: "[^\n]+\n"Confidence: "[0-9\.]+"\%"\n"Page: "[0-9]+\n {  
  sscanf(yytext,"Type: WORD\nText: %[^\n]\nConfidence: %f%*s\nPage: %d\n", line_text,&conf,&page_no);
  printf("  {\n    \"BlockType\": \"WORD\",\n");
  printf("    \"Confidence\": %f,\n",conf);
  printf("    \"Text\": \"%s\",\n",line_text);
  //printf("    \"Page\": \"%d\",\n  },\n",page_no);
  BEGIN wordGeo1;
}

<wordGeo1>"Geometry\n" {
    printf("    \"Geometry\": {\n");  
}

<wordGeo1>"Polygon:" {
    printf("       \"Polygon\":");  
}

<wordGeo1>"["/"{" {
  printf(" [\n");
}

<wordGeo1>"{"/"\'" {
  printf("          {\n");
}

<wordGeo1>"Bounding".* {
  ;
}

<wordGeo1>"\'X\': "[0-9\.]+\, {
  sscanf(yytext,"'X': %f", &x_coord);
  printf("                \"X\": %f,\n",x_coord);
}

<wordGeo1>"\'Y\': "[0-9\.]+"}" {
  sscanf(yytext,"'Y': %f", &y_coord);
  printf("                \"Y\": %f\n          },\n",y_coord);
}

<wordGeo1>"]"/\n {
  printf("\n       ]\n    },\n");
  printf("    \"Id\": \"%s\",\n",block_id);
  //printf("    \"Ids\":%s,\n",block_ids);    strcpy(block_ids,"");
  printf("    \"Page\": %d,\n  },\n",page_no);
  BEGIN 0;
}

"Relationships:"[\ \t\n]+"[" {
  BEGIN relate;
}

<relate>\'[0-9a-z\-]+/\' {
  char temp[500];
  sprintf(temp," \"%s\"", yytext+1);
  strcat(block_ids, temp);
}

<relate>"]" {
  printf("    \"Id\": \"%s\",\n",block_id);
  printf("    \"Ids\": [%s],\n",block_ids);    strcpy(block_ids,"");
  printf("    \"Page\": %d,\n  },\n",page_no);
  BEGIN 0;
}

%%

