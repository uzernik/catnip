%x spp
%{

  // find DEFINITIONS, INTERPRETATION, COVER PAGE, LASE SUMMARY, etc
  
#include <math.h>
#include <mysql.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <mysql.h>
#include <stdlib.h>	/* for bsearch() */
#include "./calc_page_params.h"



  int page;

%}



%%

"pn="/[0-9]+                   BEGIN spp;
<spp>[0-9]+                    page = atoi(yytext); BEGIN 0;

definitions  summary_point_page_array[page] = 1;  
interpretation  summary_point_page_array[page] = 1;  
"basic lease information"   summary_point_page_array[page] = 1;  
"basic lease provisions"    summary_point_page_array[page] = 1;  
"fundamental lease provisions"   summary_point_page_array[page] = 1;  
"basic lease terms"   summary_point_page_array[page] = 1;  
"lease summary sheet"   summary_point_page_array[page] = 1;  
"document cover sheet"   summary_point_page_array[page] = 1;  
"lease cover sheet"   summary_point_page_array[page] = 1;  
"contract cover sheet"   summary_point_page_array[page] = 1;  
"agreement cover sheet"                 summary_point_page_array[page] = 1;  
"summary page"   summary_point_page_array[page] = 1;  
"cover sheet"   summary_point_page_array[page] = 1;  
"lease face page"   summary_point_page_array[page] = 1;  
"lease data"   summary_point_page_array[page] = 1;  
"basic data"   summary_point_page_array[page] = 1;
"particulars"   summary_point_page_array[page] = 1;  


.|\n ;

%%

int get_summary_page() {
  char text_buffer[1000000];
  char query[5000];
  sprintf(query, "select \n\
                     page, text \n\
                     from deals_ocrtoken \n\
                     where (doc_id = %d) \n\
                          order by sn_in_doc "
	  ,doc_id);

  if (debug) fprintf(stderr,"QUERY73 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY73 (%s): query=%s:\n",prog,query);
  }
  sql_res = mysql_store_result(conn);
  int prev_pp = 0;
  int pp = 0;
  int words = 0;
  while ((sql_row = mysql_fetch_row(sql_res) )) {
    pp = atoi(sql_row[0]);
    if (pp > prev_pp) {
      static char buff[5000];
      sprintf(buff, "\n\n<HR pn=%d> ", pp);
      strcat(text_buffer, buff);    
    }
    strcat(text_buffer, sql_row[1]);
    strcat(text_buffer, " ");    
    words++;
    prev_pp = pp;
  }

  fprintf(stderr,"Found %d pages; %d words;\n", pp, words);

  yy_scan_string(text_buffer);
  if (debug) fprintf(stderr,"DOING yylex0:%d:\n",doc_id);

  yylex();

  return 0;
} 
