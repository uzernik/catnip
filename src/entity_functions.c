#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include <ctype.h>
#include <unistd.h>
#include "entity_functions.h"

#define INTEGER 5
#define TEXT 3
#define COMPLEX 2
#define DATE 4
#define FLOAT 6

int debug_entity1 = 0;
// delete the instance and all its children fields
int delete_complex_field(MYSQL *conn, int doc_id, char *complex_name) { // remove the complex field and all it's simple fields
  MYSQL_RES *sql_res;
  MYSQL_ROW sql_row;

  static char query[50000];
  if (debug_entity1) fprintf(stderr,"Info: Start delete_complex_field: %d:%s:\n",doc_id, complex_name);

  sprintf(query,"select id  \n\
                     from deals_entity \n\
                     where name = \'%s\' "
	  ,complex_name);

  if (debug_entity1) fprintf(stderr,"QUERY0=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }
  total_query++;

  sql_res = mysql_store_result(conn);
  int nn = mysql_num_rows(sql_res);
  int complex_id;
  if (nn == 0) {
    if (debug_entity1) fprintf(stderr,"Info: no such complex field :%s: (nothing inserted yet)\n",complex_name);
    return 0;
  } else {
    sql_row = mysql_fetch_row(sql_res);
    complex_id = atoi(sql_row[0]);
  }

  /*********** need to insert all 4 ********************/
  delete_any_field_by_complex(conn,"text", doc_id,complex_id);
  delete_any_field_by_complex(conn,"integer", doc_id,complex_id);
  delete_any_field_by_complex(conn,"date", doc_id,complex_id);
  delete_any_field_by_complex(conn,"float", doc_id,complex_id);
  /*********** need to insert all 4 ********************/

  delete_complex_field_by_id(conn,doc_id,complex_id);

  return 0;
}

// delete the instance and all its children fields by doc_id
int delete_all_complex_fields_by_doc_id(MYSQL *conn, int doc_id) { // remove the complex field and all it's simple fields

  if (debug_entity1) fprintf(stderr,"Info: Start delete_complex_field: %d:\n",doc_id);

  static char query[50000];

  sprintf(query,"delete \n\
                     from deals_entity_all \n\
                     where doc_id = \'%d\' "
	  ,doc_id);

  if (debug_entity1) fprintf(stderr,"QUERY0=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }

  /*

  sprintf(query,"delete \n\
                     from deals_chartmembership \n\
                     where doc_id = \'%d\' "
	  ,doc_id);

  if (debug_entity1) fprintf(stderr,"QUERY0=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }
  total_query++;
  */

  /*********** need to insert all 4 ********************/
  delete_any_field_by_doc_id(conn,"text", doc_id);
  delete_any_field_by_doc_id(conn,"integer", doc_id);
  delete_any_field_by_doc_id(conn,"date", doc_id);
  delete_any_field_by_doc_id(conn,"float", doc_id);
  /*********** need to insert all 4 ********************/

  delete_complex_field_by_doc_id(conn,doc_id);

  return 0;
}


int delete_complex_field_by_doc_id(MYSQL *conn, int doc_id) { // e.g., "preamble"
  static char query[10000];
  if (debug_entity1) fprintf(stderr,"Info: Start delete_complex_field_by_id: %d:\n",doc_id);

  sprintf(query,"delete  \n\
                          from deals_entity_complex \n\
                          where doc_id='%d' "
   	  ,doc_id);

  if (debug_entity1) fprintf(stderr,"QUERY4=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
  }
  total_query++;
  int affected = mysql_affected_rows(conn);
  if (debug_entity1) fprintf(stderr,"Info: Done delete_complex_field_by_id: %d: affected=%d:\n",doc_id, affected);
  return 0;
} // delete complex by Doc Id

int delete_any_field_by_doc_id(MYSQL *conn, char *field_type, int doc_id) { 
  static char query[10000];
  if (debug_entity1) fprintf(stderr,"Info: Start delete_any_field_by_complex: %s:%d:\n",field_type, doc_id);

  sprintf(query,"delete  \n\
                          from deals_entity_%s \n\
                          where doc_id='%d' "
   	  ,field_type, doc_id);

  if (debug_entity1) fprintf(stderr,"QUERY4=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
  }
  total_query++;
  int affected = mysql_affected_rows(conn);
  if (debug_entity1) fprintf(stderr,"Info: Done delete_any_field_by_complex: %d: affected=%d:\n",doc_id, affected);
  return 0;
} // delete any 



// delete a field instance
int delete_any_field(MYSQL *conn, char *table_type, int doc_id, char *field_name) { // e.g., "preamble_text"
  MYSQL_RES *sql_res;
  MYSQL_ROW sql_row;

  static char query[10000];
  if (debug_entity1) fprintf(stderr,"Info: Start delete_any_field: %s:%d:%s:\n",table_type,doc_id, field_name);

  sprintf(query,"select id  \n\
                     from deals_entity \n\
                     where name = \'%s\' "
	  ,field_name);

  if (debug_entity1) fprintf(stderr,"QUERY0=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }
  total_query++;
  sql_res = mysql_store_result(conn);
  int nn = mysql_num_rows(sql_res);
  int field_id;
  if (nn == 0) {
    if (debug_entity1) fprintf(stderr,"Info: no such field :%s: (nothing inserted yet)\n",field_name);
    return 0;
  } else {
    sql_row = mysql_fetch_row(sql_res);
    field_id = atoi(sql_row[0]);
  }
  delete_any_field_by_id(conn,table_type, doc_id,field_id);

  if (debug_entity1) fprintf(stderr,"Info: Done delete_any_field: %d:%s:\n",doc_id, field_name);
  return 0;
} // delete_any_field()

int delete_recitals_feature(MYSQL *conn, int doc_id) { 
  static char query[10000];
  if (debug_entity1) fprintf(stderr,"Info: Start delete_recitals_feature: %d:\n",doc_id);

  sprintf(query,"delete  \n\
                          from deals_recitals_feature \n\
                          where doc_id = '%d' "
   	  ,doc_id);

  if (debug_entity1) fprintf(stderr,"QUERY6=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
  }
  total_query++;
  int affected = mysql_affected_rows(conn);
  if (debug_entity1) fprintf(stderr,"Info: Done delete_recitals_feature: %d: affected=%d:\n",doc_id, affected);
  return 0;
} // delete recitals_feature


int delete_complex_field_by_id(MYSQL *conn, int doc_id, int field_id) { // e.g., "preamble"
  static char query[10000];
  if (debug_entity1) fprintf(stderr,"Info: Start delete_complex_field_by_id: %d:%d:\n",doc_id, field_id);

  sprintf(query,"delete  \n\
                          from deals_entity_complex \n\
                          where name_id = '%d'\n\
                             and doc_id='%d' "
   	  ,field_id,doc_id);

  if (debug_entity1) fprintf(stderr,"QUERY4=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
  }
  total_query++;
  int affected = mysql_affected_rows(conn);
  if (debug_entity1) fprintf(stderr,"Info: Done delete_complex_field_by_id: %d:%d: affected=%d:\n",doc_id, field_id, affected);
  return 0;
} // delete complex by Id

int delete_any_field_by_id(MYSQL *conn, char *table_type, int doc_id, int field_id) { // e.g., "preamble_text"
  static char query[10000];
  if (debug_entity1) fprintf(stderr,"Info: Start delete_any_field_by_id: %s:%d:%d:\n",table_type, doc_id, field_id);

  sprintf(query,"delete  \n\
                          from deals_entity_%s \n\
                          where name_id = '%d'\n\
                             and doc_id='%d' "
   	  ,table_type,field_id,doc_id);

  if (debug_entity1) fprintf(stderr,"QUERY4=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
  }

  total_query++;
  /*
  sprintf(query,"delete  \n\
                          from deals_entity_all \n\
                          where name_id = '%d'\n\
                             and doc_id='%d' and table_type='%s' "
   	  ,field_id,doc_id, table_type);

  if (debug_entity1) fprintf(stderr,"QUERY444=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
  }

  total_query++;

  */

  int affected = mysql_affected_rows(conn);
  if (debug_entity1) fprintf(stderr,"Info: Done delete_any_field_by_id: %d:%d: affected=%d:\n",doc_id, field_id, affected);
  return 0;
} // delete any by Id

int delete_any_field_by_complex(MYSQL *conn, char *field_type, int doc_id, int complex_id) { // e.g., "preamble"
  static char query[10000];
  if (debug_entity1) fprintf(stderr,"Info: Start delete_any_field_by_complex: %s:%d:%d:\n",field_type, doc_id, complex_id);

  sprintf(query,"delete  \n\
                          from deals_entity_%s \n\
                          where belongs_in_name_id = '%d'\n\
                             and doc_id='%d' "
   	  ,field_type, complex_id,doc_id);

  if (debug_entity1) fprintf(stderr,"QUERY4=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
  }
  total_query++;
  /*
  sprintf(query,"delete  \n\
                          from deals_entity_all \n\
                          where belongs_in_name_id = '%d'\n\
                             and doc_id='%d' and table_type='%s' "
   	  ,complex_id,doc_id,field_type);

  if (1 || debug_entity1) fprintf(stderr,"QUERY445=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
  }
  total_query++;
  */
  int affected = mysql_affected_rows(conn);
  if (debug_entity1) fprintf(stderr,"Info: Done delete_any_field_by_complex: %d:%d: affected=%d:\n",doc_id, complex_id, affected);
  return 0;
} // delete any by complex

int insert_complex_field(MYSQL *conn, int doc_id, int deal_id, int org_id, int para_id, char *name, int complex_id_name[], int score, int order_no, char *hd, int show_sub_hd) { // get the umbrella entity 
  if (debug_entity1) fprintf(stderr,"Info: Start insert_complex_field: %d:%d:%d:%s:\n",doc_id,deal_id,org_id,name);
  MYSQL_RES *sql_res;
  MYSQL_ROW sql_row;

  /******************* insert Entity and get complex_id_name ***********************/
  static char query[10000];
  sprintf(query,"select id  \n\
                     from deals_entity \n\
                     where name = \'%s\' "
	  ,name);

  if (debug_entity1) fprintf(stderr,"QUERY0=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }
  total_query++;
  sql_res = mysql_store_result(conn);
  int nn = mysql_num_rows(sql_res);
  
  if (nn == 0) {
    sprintf(query,"replace  \n\
                          into deals_entity \n\
                          set name = '%s'\n\
	                  , fieldtype = '%d'"
	    ,name, COMPLEX);

    if (1 ||debug_entity1) fprintf(stderr,"QUERY47=%s\n",query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      exit(1);
    }
    total_query++;
    complex_id_name[0] = mysql_insert_id(conn);
  } else {
    sql_row = mysql_fetch_row(sql_res);
    complex_id_name[0] = atoi(sql_row[0]);
  }

  /******************** Insert Properties *******************************/
  // org_id and name_id are unique_together
  /*
    sprintf(query,"replace  \n\
                          into deals_entity_properties \n\
                          set name_id = '%d'\n\
	                  , heading = '%s' \n\
	                  , organization_id = '%d' \n\
	                  , b_show_sub_hd = '%d' \n\
	                  , order_no = '%d' \n\
	                  , b_show_pid = '%d'"
	    ,complex_id_name[0], hd, org_id, show_sub_hd ,order_no, 1);

    if (1 || debug_entity1) fprintf(stderr,"QUERY1=%s\n",query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      exit(1);
    }
    total_query++;
  */
  /******************* insert instance ***********************/
  sprintf(query,"replace  \n\
                          into deals_entity_complex \n\
                          set name_id = '%d'\n\
                          , doc_id='%d' \n\
                          , folder_id='%d' \n\
                          , organization_id='%d' \n\
                          , para_id='%d' \n\
                          , score='%d' \n\
                          , program='%s' \n\
                          , insertion_date = now()"
	  ,complex_id_name[0],doc_id,deal_id,org_id,para_id,score,"uri");

  if (1 || debug_entity1) fprintf(stderr,"QUERY4=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
  }
  total_query++;
  int field_id = mysql_insert_id(conn);
  if (debug_entity1) fprintf(stderr,"Info: Done insert_complex_field: did_oid %d:%d:%d:%s:--%d:%d:\n",doc_id,deal_id,org_id,name,complex_id_name[0],field_id);
  return field_id;
}
 
int get_did_oid(MYSQL *conn, int doc_id, int oid[]) {
  MYSQL_RES *sql_res;
  MYSQL_ROW sql_row;

  static char query[10000];
  sprintf(query,"select folder_id, organization_id  \n\
                          from deals_document \n\
                          where id = \'%d\' "
	  ,doc_id);

  if (debug_entity1) fprintf(stderr,"QUERY0=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }
  total_query++;
  sql_res = mysql_store_result(conn);
  int nn = mysql_num_rows(sql_res);
  if (debug_entity1) fprintf(stderr,"NUM_ROWS0=%d\n",nn);
  int fid;
  if (nn != 1) {
    fprintf(stderr,"Error: doc_id %d not found in SQL\n",doc_id);
    fid = 0;
    oid[0] = 0;
  } else {
    //	char row[2][20];
    sql_row = mysql_fetch_row(sql_res);
    if (!sql_row[0]) {
      fprintf(stderr,"Error: doc_id %d not found: Can't nest doc under org\n",doc_id);
      exit(0);
    }
    fid = atoi(sql_row[0]);    
    oid[0] = atoi(sql_row[1]);
  }
  if (debug_entity1) fprintf(stderr,"Info: Done get_did_oid %d:%d:%d:\n",doc_id,fid,oid[0]);
  return fid;
} // get_did_oid()


// table_type is: "text", "float", "integer", or "date"
// val is the value. date, integer and float must be converted first into text
// type_no is the type of entity, it is found in the preceding function (insert_complex_field).
// complex_id is the umbrella complex entity (instance) -- provided by the preceding function
// complex_name_id is the complex entity name (e.g., NAME, DATE, ADDRESS, etc) -- provided by the preceding function
// name is the name of this field
// order_no is where it fits in the complex entity
// type_no is INTEGER, DATE, FLOAT, TEXT
// hd is the sub_heading
int insert_any_field(MYSQL *conn, char *table_type, int doc_id, int deal_id, int org_id, int para_id, int complex_id, int complex_name_id, char *name, char orig_text[], char val[], int order_no, int score, char *hd) { // enter one sub-field first_item,2
  MYSQL_RES *sql_res;
  MYSQL_ROW sql_row;

  static char query[10000];
  if (debug_entity1) fprintf(stderr,"Info: Start insert_any_field: %d:%d:%d:%s:%d:\n",doc_id,deal_id,org_id,name,complex_id);
  /**************** Insert Entity and get field_id **********************/
  sprintf(query,"select id  \n\
                     from deals_entity \n\
                     where name = \'%s\' "
	  ,name);

  if (debug_entity1) fprintf(stderr,"QUERY0=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }
  total_query++;
  sql_res = mysql_store_result(conn);
  int nn = mysql_num_rows(sql_res);
  int field_id;
  int type_no; 

  if (strcmp(table_type,"integer") == 0) type_no = 5;
  else if (strcmp(table_type,"float") == 0) type_no = 6;
  else if (strcmp(table_type,"text") == 0) type_no = 3;
  else if (strcmp(table_type,"date") == 0) type_no = 4;

  if (nn == 0) {
    sprintf(query,"replace  \n\
                          into deals_entity \n\
                          set name = '%s'\n\
	                  , fieldtype = '%d'"
	    ,name, type_no);

    if (1 || debug_entity1) fprintf(stderr,"QUERY1=%s\n",query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      exit(1);
    }
    total_query++;
    field_id = mysql_insert_id(conn);      

  } else {
    sql_row = mysql_fetch_row(sql_res);
    field_id = atoi(sql_row[0]);
  }

  /******************** Insert Properties *******************************/
  // unique_together by org_id and name_id
  /*
    sprintf(query,"replace  \n\
                          into deals_entity_properties \n\
                          set name_id = '%d'\n\
	                  , heading = '%s' \n\
	                  , organization_id = '%d'"
	    ,field_id, hd, org_id);

    if (1 || debug_entity1) fprintf(stderr,"QUERY11=%s\n",query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      exit(1);
    }
    total_query++;
  */
  /************************* Insert Value **************************/
  sprintf(query,"replace  \n\
                          into deals_entity_%s \n\
                          set name_id = '%d'\n\
                          , doc_id='%d' \n\
                          , folder_id='%d' \n\
                          , organization_id='%d' \n\
                          , para_id='%d' \n\
                          , program='%s' \n\
                          , insertion_date = now() \n\
                          , orig_text = '%s' \n\
                          , value = '%s' \n\
                          , belongs_in_id = '%d' \n\
                          , belongs_in_name_id = '%d' \n\
                          , order_no = '%d' \n\
                          , score = '%d' "
	  ,table_type,field_id,doc_id,deal_id,org_id,para_id,"uri", orig_text, val,complex_id,complex_name_id,order_no,score);

  if (1 || debug_entity1) fprintf(stderr,"QUERY21=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
  }

  total_query++;
  /************************* Insert Value into the All table**************************/
  /*
  int tix = 0;
  int num_tokens = 0;
  char *my_orig = (strcmp(table_type,"integer") == 0 ||strcmp(table_type,"text") == 0) ? val : orig_text;
  sprintf(query,"replace  \n\
                          into deals_entity_all \n\
                          set name_id = '%d'\n\
                          , doc_id='%d' \n\
                          , folder_id='%d' \n\
                          , organization_id='%d' \n\
                          , para_id='%d' \n\
                          , program='%s' \n\
                          , insertion_date = now() \n\
                          , orig_text = '%s' \n\
                          , value = '%s' \n\
                          , belongs_in_id = '%d' \n\
                          , belongs_in_name_id = '%d' \n\
                          , order_no = '%d' \n\
                          , first_token = '%d' \n\
                          , num_tokens = '%d' \n\
                          , table_type = '%s' \n\
                          , score = '%d' "
	                    
	  ,field_id,doc_id,deal_id,org_id,para_id,"uri", my_orig, val,complex_id,complex_name_id,order_no,tix,num_tokens,table_type,score);

  if (debug_entity1) fprintf(stderr,"QUERY2=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
  }

  total_query++;

  */


  return mysql_insert_id(conn);
} // insert_any_field



int insert_recitals_feature(MYSQL *conn,char *recitals_feature, int fid, int doc_id, int deal_id, int org_id) {
  static char query[10000];
  sprintf(query,"insert  \n\
                     into deals_recitals_feature \n\
                     set doc_id = \'%d\' \n\
                      , folder_id = \'%d\' \n\
                      , organization_id = \'%d\' \n\
                      , feature = \'%s\' \n\
                      , insertion_date = now() \n\
                      , instance_id = \'%d\' "
	  ,doc_id, deal_id, org_id, recitals_feature, fid);

  if (1 || debug_entity1) fprintf(stderr,"QUERY55=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }

  total_query++;
  return mysql_insert_id(conn);
}

int chart_membership(MYSQL *conn, int doc_id, int chart_id, int entity_id, int row_id) {
  return 0;
  /* 
  static char query[10000];
  if (debug_entity1) fprintf(stderr,"Info: chartmembership:%d:%d:%d:\n",chart_id,entity_id,row_id);
  // **************** Insert Entity and get field_id **********************
  sprintf(query,"insert  \n\
                     into deals_chartmembership \n\
                     set chart_id = \'%d\' \n\
                      , doc_id = \'%d\' \n\
                      , complex_id = \'%d\' \n\
                      , row_id = \'%d\' "
	  ,chart_id, doc_id, entity_id, row_id);

  if (debug_entity1) fprintf(stderr,"QUERY0=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }
  int id = mysql_insert_id(conn);
  return id;
*/
}

// delete a chart and its membership
int delete_charts(MYSQL *conn, int doc_id) { 
  static char query[10000];
  if (debug_entity1) fprintf(stderr,"Info: Start delete_charts\n");


  sprintf(query,"delete from deals_pdd_entity_chart_tikun \n\
                     where doc_id = \'%d\' "
	  , doc_id);

  if (debug_entity1) fprintf(stderr,"QUERY349=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY349=%s\n",query);
  }

  sprintf(query,"delete  \n\
                     from deals_entity_all \n\
                     where doc_id = \'%d\' "
	  ,doc_id);

  if (debug_entity1) fprintf(stderr,"QUERY21=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }

  /*
  sprintf(query,"delete  \n\
                     from deals_chartmembership \n\
                     where doc_id = \'%d\' "
	  ,doc_id);

  if (debug_entity1) fprintf(stderr,"QUERY0=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }
  total_query++;
  int affected1 = mysql_affected_rows(conn);
  */

  sprintf(query,"delete  \n\
                     from deals_chartheader \n\
                     where doc_id = \'%d\' "
	  ,doc_id);

  if (debug_entity1) fprintf(stderr,"QUERY21=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }
  total_query++;
  //int affected3 = mysql_affected_rows(conn);




  sprintf(query,"delete  \n\
                     from deals_entity_all \n\
                     where doc_id = \'%d\' "
	  ,doc_id);

  if (debug_entity1) fprintf(stderr,"QUERY21=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }


  sprintf(query,"delete  \n\
                     from deals_chartmembership \n\
                     where doc_id = \'%d\' "
	  ,doc_id);

  if (debug_entity1) fprintf(stderr,"QUERY21=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }



  sprintf(query,"delete  \n\
                     from deals_chart \n\
                     where doc_id = \'%d\' "
	  ,doc_id);

  if (debug_entity1) fprintf(stderr,"QUERY1=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }

  if (debug_entity1) fprintf(stderr,"QUERY11=%s\n",query);
  total_query++;
  //int affected2 = mysql_affected_rows(conn);

  //if (debug_entity1) fprintf(stderr,"Info: Start delete_charts affected=%d:%d:%d:\n", affected1, affected2, affected3);
  return 0;
} // delete_charts()

int insert_chart_header(MYSQL *conn, int doc_id, int chart_id,int col_no,char *hdr) {
  static char query[10000];
  /**************** Insert Entity and get field_id **********************/
  sprintf(query,"replace \n\
                     into deals_chartheader \n\
                     set header = \'%s\' \n\
                         ,doc_id = \'%d\' \n\
                         ,col_no = \'%d\' \n\
                         ,chart_id = \'%d\' "
	  ,hdr, doc_id, col_no, chart_id);

  if (debug_entity1) fprintf(stderr,"QUERY0=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }
  total_query++;
  return 0;
}

int delete_existing_pid(MYSQL *conn, int doc_id) { 
  static char query[10000];

  sprintf(query,"delete  \n\
                          from deals_paragraph \n\
                          where doc_id = '%d' "
   	  ,doc_id);

  if (1 || debug_entity1) fprintf(stderr,"QUERY4=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
  }

  int affected = mysql_affected_rows(conn);
  if (debug_entity1) fprintf(stderr,"Info: Done delete pid: doc_id=%d: affected=%d:\n",doc_id, affected);
  return 0;
} // delete_pid

int get_my_dataset_id(MYSQL *conn) {
  MYSQL_RES *sql_res;
  MYSQL_ROW sql_row;
  int ret = 0;
  static char query[200000];

  sprintf(query,"select id from deals_dataset \n\
                     where description like '%c__uri/prog__%c' "
	  ,'%', '%');

  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
  }

  sql_res = mysql_store_result(conn);
  int nn = mysql_num_rows(sql_res);
  
  if (nn == 1) {
    sql_row = mysql_fetch_row(sql_res);
    ret = atoi(sql_row[0]);
  } else if (nn > 1) {
    fprintf(stderr,"More than 1 \"__uri__\" ???");
  } else {
    sprintf(query,"insert into \n\
                     deals_dataset \n					\
                     set description = '__uri/prog__' "
	    );
    
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
    }
    ret = mysql_insert_id(conn);
  }
  return ret;
} // get_my_dataset_id()
