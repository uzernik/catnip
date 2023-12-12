
/*
add column into document table only if does not exist
CALLING: ../bin/does_field_exist  -D1 -P mysql -N dealthing -U root -W imaof333 -F tenant_path_in_media 
*/

#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <mysql.h>
#include <stdlib.h>	/* for bsearch() */

#define MAX_LEN 100

char *prog;
int debug;

char *db_IP, *db_name, *db_pwd, *db_user_name; // for DB config 

MYSQL *conn;
MYSQL_RES *sql_res;
MYSQL_ROW sql_row;

char *field_name;


int add_field(MYSQL *conn, char *field_name) {

  MYSQL_RES *sql_res;
  MYSQL_ROW sql_row;
  static char query[10000];

  if (strlen(field_name) >= MAX_LEN) {
    fprintf(stderr, "Error: Field %s is :%d: chars long, longer than MAX_LEN (%d)", field_name, (int)(strlen(field_name)), MAX_LEN);
    exit(-1);
  }

  sprintf(query, "SELECT * FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = 'dealthing' AND TABLE_NAME = 'deals_document' AND COLUMN_NAME = '%s'",field_name);
  if (debug) fprintf(stderr,"QUERY4=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY4=%s\n",mysql_error(conn));
    exit(1);
  }
  sql_res = mysql_store_result(conn);
  sql_row = mysql_fetch_row(sql_res);  
  if (sql_row == NULL) {
    sprintf(query,"ALTER TABLE %s \n\
                   ADD %s VARCHAR(%d) NOT NULL DEFAULT 'NA'"
	    , "deals_document", field_name, MAX_LEN );

    if (debug) fprintf(stderr,"QUERY5=%s\n",query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"QUERY5=%s\n",mysql_error(conn));
      exit(1);
    } 
  } else {
    fprintf(stderr,"Field :%s: already exists, not added\n", field_name);	    
  }

  return 0;
} // add_field()

int main(int argc, char **argv) {
  int get_opt_index;
  int c_getopt;
  prog = argv[0];
  opterr = 0;
  debug = 0;
  while ((c_getopt = getopt (argc, argv, "d:P:N:U:W:D:K:S:F:")) != -1) {
    switch (c_getopt) {
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
      field_name = optarg;
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

  if (debug) fprintf (stderr,"%s took: db_IP =%s, db_name =%s, db_user_name =%s, pwd=%s, field_name=%s:\n",
	   argv[0], db_IP, db_name, db_user_name, db_pwd, field_name);

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
  
  add_field(conn, field_name);
  return 0;
} // main()

