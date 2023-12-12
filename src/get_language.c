
/*
get the country of the document
CALLING: ../bin/get_country -d 1498 -D 1 -P localhost -N dealthing -U root -W imaof3 
*/

#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <mysql.h>
#include <stdlib.h>	/* for bsearch() */


char *prog;
int debug;
int doc_id;
char *db_IP, *db_name, *db_pwd, *db_user_name; // for DB config 
char *which_file;
int doc_id;
int org_id[1]; // organization
int deal_id;

MYSQL *conn;
MYSQL_RES *sql_res;
MYSQL_ROW sql_row;


char *get_default_data_language_from_sysconfig(MYSQL *conn) {
  static char query[200000];
  char *default_data_language = "ENG";

  sprintf(query,"select value from dealthing_sysconfig \n\
                        where param_name='default_data_language' "
  );

  if (debug) fprintf(stderr,"QUERY4 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY4 (%s): query=%s:\n",prog,query);
    return default_data_language;
  }

  sql_res = mysql_store_result(conn);
  int nn = mysql_num_rows(sql_res);
  if (nn > 0) {
      sql_row = mysql_fetch_row(sql_res);
      default_data_language = strdup(sql_row[0]);
  }

  return default_data_language;
}


int get_language(MYSQL *conn, int did) {

  MYSQL_RES *sql_res;
  MYSQL_ROW sql_row;
  static char query[10000];

  sprintf(query,"select ct.data_language, cp.data_language, co.data_language, d.id, t.id, p.id, o.id \n\
                          from deals_document as d  \n\
                          left join deals_folder as t on (t.id = d.folder_id) \n\
                          left join deals_folder as p on (p.id = t.parentfolder_id) \n\
                          left join deals_organization as o on (o.id = p.organization_id) \n\
                          left join deals_org_folder_language as co on (co.org_id = o.id) \n\
                          left join deals_org_folder_language as cp on (cp.folder_id = p.id) \n\
                          left join deals_org_folder_language as ct on (ct.folder_id = t.id) \n\
                          where d.id = %d "
	    , did
	  );

  if (debug) fprintf(stderr,"QUERY5=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY5=%s\n",mysql_error(conn));
    exit(1);
  }
  sql_res = mysql_store_result(conn);

  char *data_language = get_default_data_language_from_sysconfig(conn);
  sql_row = mysql_fetch_row(sql_res);
  if (sql_row) {
    if (sql_row[0]) { // try t language first
      data_language = strdup(sql_row[0]);
    } else if (sql_row[1]) { // try p language next
      data_language = strdup(sql_row[1]);
    } else if (sql_row[2]) { // try o language next
      data_language = strdup(sql_row[2]);
    }
  } else {
    fprintf(stderr,"Error: no sql_row for doc=%d:\n",doc_id);
  }

  if (debug) fprintf(stderr,"LANGUAGE :%s:\n",data_language);
  printf("%s",data_language);
  return 0;
} // get_language()



int main(int argc, char **argv) {
  int get_opt_index;
  int c_getopt;
  prog = argv[0];
  opterr = 0;
  debug = 0;
  while ((c_getopt = getopt (argc, argv, "d:P:N:U:W:D:K:S:F:")) != -1) {
    switch (c_getopt) {
    case 'd':
      doc_id = atoi(optarg);
      break;
    case 'D':
      debug = atoi(optarg);
      break;
    case 'P':
      db_IP = optarg;
      break;
    case 'N':
      db_name = optarg;
      break;
    case 'U':
      db_user_name = optarg;
      break;
    case 'W':
      db_pwd = optarg;
      break;
    case 'F':
      which_file = optarg;
      break;

    case '?':
      if (optopt == 'd')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'D')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'P')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'N')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'U')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'W')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'F')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      else if (isprint (optopt))
	fprintf (stderr, "Unknown option `-%c'.\n", optopt);
      else {
	fprintf (stderr,
		 "Unknown option character `\\x%x'.\n",
		 optopt);
      }
      break;
    default:
      abort ();
    }
  } //while

  if (debug) fprintf (stderr,"%s took: doc_id = %d, db_IP =%s, db_name =%s, db_user_name =%s, db_pwd=%s: which_file=%s\n",
	   argv[0], doc_id, db_IP, db_name, db_user_name, db_pwd, which_file);

  for (get_opt_index = optind; get_opt_index < argc; get_opt_index++) {
    printf ("Non-option argument %s\n", argv[get_opt_index]);
  }

  /************** SQL ********************/
  //char *server = "localhost";
  char *server = db_IP; // "54.241.17.226"
  char *user = db_user_name; // "root";
  char *password = db_pwd; //"imaof3";
  char *database = db_name; // "dealthing";
  /************** END SQL ****************/

  conn = mysql_init(NULL);
  
  /* now connect to database */
  if (!mysql_real_connect(conn,db_IP,db_user_name,db_pwd,db_name,0,NULL,0)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }

  get_language(conn, doc_id);
  return 0;
} // main()

