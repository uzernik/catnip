%{
  // CALLING: ../bin/get_folder_path doc_id db_IP db_name db_user_name db_pwd media Xfile debug
#include <string.h>
#include <stdio.h>
#include <stdlib.h>	/* for bsearch() */
#include <mysql.h>
#include <ctype.h>
  MYSQL *conn;
  MYSQL_RES *sql_res;
  MYSQL_ROW sql_row;

%}
%%

%%
int main(int ac, char**av) {

  if (ac != 9) {
    fprintf(stderr,"Error: %s takes 9 args (%d): doc_id, db_IP, db_name, db_user_name, db_pwd, media, Xfile, debug\n",av[0],ac);

    exit(0);
  }

  char *prog = av[0];
  int doc_id = atoi(av[1]);
  char *db_IP = av[2];
  char *db_name = av[3];
  char *db_user_name = av[4]; // for DB config
  char *db_pwd = av[5];
  char *dt_media = av[6];
  char *Xfile = av[7];
  int debug = atoi(av[8]);

  conn = mysql_init(NULL);
  
  /* now connect to database */
  if (!mysql_real_connect(conn,db_IP,db_user_name,db_pwd,db_name,0,NULL,0)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }

  char query[5000];
  sprintf(query, "select %s \n\
                 from deals_document \n\
                 where id='%d' ",
	  Xfile, doc_id);

  if (debug) fprintf(stderr,"QUERY79 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY79 (%s): query=%s:\n",prog,query);
  }

  sql_res = mysql_store_result(conn);
  if (sql_row = mysql_fetch_row(sql_res)) {
    char *my_path = (sql_row[0]) ? sql_row[0]: "NULL";
    char tmp_path[5000];
    sprintf(tmp_path,"%s/%s",dt_media, my_path);
    char *pp = strrchr(tmp_path,'/');
    // pp[0] = '\0'; no need to chop off the root name
    printf("%s",tmp_path);
  }

  return 0;
} // main


