
/*
get a deals_document entry for the document
CALLING: ../bin/set_doc_entry -P mysql -N dealthing -U root -W imaof333 -F "hsgatlin/comcast" -O "Comcast Licnense Agreement"
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
char *path;
char *orig_file;

int deal_id;
int no_of_pages;
int org_id=1; // fictitious for now
int folder_id=8; // fictitious for now
MYSQL *conn;
MYSQL_RES *sql_res;
MYSQL_ROW sql_row;


char *clean_text(char *text) {
    int ii;
    for (ii = 0; ii < strlen(text); ii++) {
      if (text[ii] == ' ') text[ii] = '_';
      else if (text[ii] == '-') text[ii] = '_';
      text[ii] = tolower(text[ii]);
    }
    
    return text;
} // clean_text()


int insert_entry(MYSQL *conn, char *orig_file, int no_of_pages, char *path, int org_id, int folder_id) {

  MYSQL_RES *sql_res;
  MYSQL_ROW sql_row;
  static char query[10000];

  sprintf(query,"insert into deals_document \n\
                 (name, display_name, origfile, htmlfile, dsafile, displayfile, no_of_pages, tenant_path_in_media, organization_id, folder_id, upload_date) \n\
                 values \n\
                 ('%s'           , '%s'                 , 'Dropbox/%s', 'X', 'X', 'X', %d,         '%s',  %d,       %d, now())"
	  , clean_text(orig_file), clean_text(orig_file), orig_file   ,                no_of_pages, path, org_id, folder_id);

  if (debug) fprintf(stderr,"QUERY5=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY5=%s\n",mysql_error(conn));
    exit(1);
  }

  int did =  mysql_insert_id(conn);

  sprintf(query, "UPDATE deals_document SET \n\
                    htmlfile = \"htmlfile_%08d\", dsafile = \"dsafile_%08d\", displayfile = \"displayfile_%08d\" \n\
                    WHERE id = %d "
	  , did, did, did, did);

  if (debug) fprintf(stderr,"QUERY6=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY6=%s\n",mysql_error(conn));
    exit(1);
  }
   
  return did;
} // insert_entry()



int main(int argc, char **argv) {
  int get_opt_index;
  int c_getopt;
  prog = argv[0];
  opterr = 0;
  debug = 0;
  while ((c_getopt = getopt (argc, argv, "P:N:U:W:D:K:S:F:O:n:o:f:")) != -1) {
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
      path = optarg;
      break;
    case 'O':
      orig_file = optarg;
      break;
    case 'n':
      no_of_pages = atoi(optarg);
      break;
    case 'o':
      org_id = atoi(optarg);
      break;
    case 'f':
      folder_id = atoi(optarg);
      break;

    case '?':
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
      if (optopt == 'O')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'F')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'n')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'o')
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

  if (debug) fprintf (stderr,"%s took: doc_id = %d, db_IP =%s, db_name =%s, db_user_name =%s, db_pwd=%s: path=%s: nop=%d: orig_file=%s:\n",
		      argv[0], doc_id, db_IP, db_name, db_user_name, db_pwd, path, no_of_pages, orig_file);

  if (debug) for (get_opt_index = optind; get_opt_index < argc; get_opt_index++) {
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

  int my_did = insert_entry(conn, orig_file, no_of_pages, path, org_id, folder_id);
  printf("%d",my_did);
  return my_did;
} // main()

