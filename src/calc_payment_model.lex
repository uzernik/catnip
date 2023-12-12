%x stext 
%{
  /* insert into SQL all ZZ predictions
  ** also insert the PP-token table
  ** each token has a span. no other spans in the file
  **  CALL: ../bin/calc_payment_model "localhost"  "dealthing" "root"   "imaof3" 1
  */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>	/* for bsearch() */
#include <mysql.h>
#include <ctype.h>
#include "entity_functions.h"
#define MIN(a,b) (a<b)?a:b
#define MAX(a,b) (a>b)?a:b

  int tenant_no = 0;

  int debug;
  char *prog;
  char *db_IP, *db_name, *db_pwd, *db_user_name; // for DB config
  int org_id[2];
  
  int in_zz = 0; // to distinguish <ZZ> from <ZZ name=


  /************** SQL ********************/
  //char *server = "localhost";
  //char *server = "54.241.17.226";
  //char *user = "root";
  //char *password = "imaof3";
  //char *database = "dealthing";
  MYSQL *conn;
  MYSQL_RES *sql_res;
  MYSQL_ROW sql_row;

  /************** END SQL ****************/
  #define MAX_TENANT 50
  struct Tenant {
    char *tenant_name;
    int tenant_id;
  } tenant_array[MAX_TENANT];

void remove_last_comma(char text[]) {
  int ii, found;
  for (found = 0, ii = strlen(text); found == 0 && ii > 0; ii--) {
    if (text[ii] == ',') {
      text[ii] = '\0'; found = 1;
    } else if (isalnum(text[ii]) > 0) {
      found = 1;
    } else {
      ;
    }
  }
}

int delete_payment_model() { // remove old tables
  static char query[50000];

  sprintf(query,"delete  \n\
                     from deals_annotation \n\
                     where source_program = 'payment_model' "
	  );

  if (debug) fprintf(stderr,"QUERYX20=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERYX20=%s\n",mysql_error(conn));
    exit(1);
 }

  sprintf(query,"delete  \n\
                     from deals_annotationleaseproperty \n\
                     where source_program = 'payment_model' "
	  );
  if (debug) fprintf(stderr,"QUERYX21=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERYX21=%s\n",mysql_error(conn));
    exit(1);
 }

  return 0;
}

int insert_calc_annotations_into_sql() {
  int ii;
  static char buff[5000];
  static char query[500000];

  strcpy(query,"insert \n\
                                 into deals_annotationleaseproperty \n\
                                 (tenant_id, property_name, source_program, insertion_date) \n\
                                 values ");

  for (ii = 0; ii < tenant_no; ii++) {
    sprintf(buff,"('%d', 'payment_model', 'payment_model', now()), "
	    , tenant_array[ii].tenant_id); 
    strcat(query,buff);
  }

  remove_last_comma(query);
  if (debug) fprintf(stderr,"QUERY104=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY104:%s:%s:\n",mysql_error(conn),query);
    exit(1);
  }

  fprintf(stderr,"Info:  Inserted  %d period_properties \n", ii);
  int my_p_id = mysql_insert_id(conn);

  fprintf(stderr,"MY_P_ID:%d total=%d:\n",my_p_id,ii);

  strcpy(query,"insert \n\
                                 into deals_annotation \n\
                                 (tenant_id, orig, norm_text, atype, source_program, lease_property_id, insertion_date) \n\
                                 values ");
              
  for (ii = 0; ii < tenant_no; ii++) {
    static char buff[5000];
    char *ttt;
    if (strcasecmp(tenant_array[ii].tenant_name,"Hallmark") == 0) {
      ttt = "percent_sales";
    } else if (strcasecmp(tenant_array[ii].tenant_name,"Belk") == 0) {
      ttt = "percent_sales";
    } else if (strncasecmp(tenant_array[ii].tenant_name,"Classic",7) == 0) {
      ttt = "percent_cpi";
    } else if (strncasecmp(tenant_array[ii].tenant_name,"Rack",4) == 0) {
      ttt = "fixed";
    } else {
      ttt = "other";
    }

    sprintf(buff,"('%d', '%s', '%s', 1, 'payment_model', '%d', now()), "
	    ,tenant_array[ii].tenant_id, ttt ,ttt, my_p_id++);
    strcat(query,buff);

  }

  remove_last_comma(query);

  if (debug) fprintf(stderr,"QUERY105=%s\n",query);

  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY105:%s:%s:\n",mysql_error(conn),query);
    exit(1);
    }

  fprintf(stderr,"Info:  Inserted  %d annotations \n", tenant_no);
  return 0;
} // insert_calc_annotations_into_sql()


%}

%%

%%

int main(int ac, char**av) {
  debug_entity = debug = 0;
  if (ac != 6) {
    fprintf(stderr,"Error: %s takes 5 args (%d): db_IP, db_name, db_user_name, db_pwd, debug\n",av[0],ac);
    exit(0);
  }
  fprintf(stderr,"Info: (%s) got 5 args: IP=%s: name=%s: user_name=%s: pwd=%s: debug=%s: \n",av[0],av[1],av[2],av[3],av[4],av[5]);
  prog = av[0];
  db_IP = av[1];
  db_name = av[2];
  db_user_name = av[3]; // for DB config
  db_pwd = av[4];
  debug = atoi(av[5]);

  conn = mysql_init(NULL);
  
  /* now connect to database */
  if (!mysql_real_connect(conn,db_IP,db_user_name,db_pwd,db_name,0,NULL,0)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }

  delete_payment_model();

  static char query[5000];    
  sprintf(query,"select tenant_folder_id, name from deals_annotationrentrolltenant \n\
                     where 1 "
	  );

  if (debug) fprintf(stderr,"QUERY79 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY79 (%s): query=%s:\n",prog,query);
  }
  fprintf(stderr,"done QUERY79 (%s): query=%s:\n",prog,query);
  sql_res = mysql_store_result(conn);
  
  tenant_no = 0;
  int done_flag = 0;

  while (sql_row = mysql_fetch_row(sql_res)) {
    if (done_flag == 0) {
      tenant_array[tenant_no].tenant_id = atoi(sql_row[0]);
      tenant_array[tenant_no].tenant_name = strdup(sql_row[1]);

      tenant_no++;
      if (tenant_no > MAX_TENANT -1) {
	fprintf(stderr,"MAX_TENANT exceeded\n");
	done_flag = 1;
      }
    }
    fprintf(stderr,"FOUND1 %d tenants:%s:%s:\n",tenant_no, sql_row[0],sql_row[1]);
  }

  fprintf(stderr,"FOUND %d tenants\n",tenant_no);

  insert_calc_annotations_into_sql();
  return 1;
}
