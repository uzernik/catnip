#include <string.h>
#include <time.h>
#include <math.h>  
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <mysql.h>
#include <stdlib.h>	/* for bsearch() */
#include "line_token_ocr.h"	/* for coords */

extern int debug;
extern MYSQL *conn;
extern MYSQL_RES *sql_res;
extern MYSQL_ROW sql_row;


int get_tokens(int doc_id, char *source_ocr) {
  char query[10000];
  sprintf(query,"select page_no, text, line_no, x1, x2 \n\
                from deals_token \n					\
                where doc_id = \"%d\" and source_program='%s'",
	  doc_id, source_ocr);

    if (debug) fprintf(stderr,"QUERY6=%s\n",query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY6=%s\n",query);
    }

    sql_res = mysql_store_result(conn);
    int token_no = 0;
    while (sql_row = mysql_fetch_row(sql_res)) {
      token_array[token_no].page_no = atoi(sql_row[0]);
      token_array[token_no].text = strdup(sql_row[1]);
      token_array[token_no].line_no = atoi(sql_row[2]);
      //      token_array[token_no].x1_1000 = (int)(atof(sql_row[3]) *1000.0);
      //      token_array[token_no].x2_1000 = (int)(atof(sql_row[4]) *1000.0);
      token_array[token_no].x1_1000 = (int)(atof(sql_row[3]) *1.0); // for AWS, URI 06/19/20
      token_array[token_no].x2_1000 = (int)(atof(sql_row[4]) *1.0);
      token_no++;
    }
    return token_no;
}

