%x spp srest sbod sddd remove_x
%{
  /*
CALLING:  (../bin/new_index 39 ../tmp/Index/index_file_39.out 0 1 localhost  dealthing  root imaof3 < ../tmp/Xxx/aaa7 > bbb) &> uuu

UDEV1
CALLING (../bin/new_index 71815 ../tmp/Index/index_file_71815.out 0 1  udev1-from-prod-feb26.cu0erabygff3.us-west-2.rds.amazonaws.com dealthing  root imaof333 < aaa9 > bbb) &> uuu

TOC:
we read the individual TOC pages from DEALS_PAGE2SUMMARY
enter into PAGE_TOC_ARRAY
do clustering of toc pages.  enter cluster_start and cluster_end into page_toc_array.
when inputting into SQL, check and don't input when in_toc_cluster
  */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include <ctype.h>
#include "entity_functions.h"

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
  char * prog;
  int my_mode; // 0: add <H and <a name; 2: add wrapper, <DD>1.1</DD>; 1: add wrapper and  <H> (w/o title and section -- only the clean_head
  FILE *index_file;
  char index_name[200];
  char *file_name;
  char *real_name;  
  char *path;
  int doc_id;
  int debug = 1;
  int cata = 1;
  int dd_flag = 0;

  int in_table;
  char *prog;
  int curr_pn;

  char *db_IP;
  char *db_name;
  char *db_user_name;
  char *db_pwd;
  char line_query[500000];

  int line_no = 0;
  char vpp[100000];
  char vrest[100000];
  int curr_item;

  int div_id = 0;
  int last_lev = -1;
  int inserted_div = 0; // indicates a DEV was inserted and <X is legit
  /****************************************************************/
  #define MAX_SEQ 2000
  struct TOC_Seq { // we allow titles to be only in areas not covered by sequences. 
    int first_pid;
    int last_pid;
    int no_of_items;
    int first_enum;
    int last_enum;
    char *clean_header;
    int lev;
  } toc_seq_array[MAX_SEQ];
  int largest_seq_no = -1;
#define MAX_PID 20000
#define MAX_DIV 20000
  struct TOC_Div { // we allow titles to be only in areas not covered by sequences. 
    int first_pid;
    int last_pid;
    int seq;
    int lev; 
    char *name; 
  } toc_div_array[MAX_DIV];
  int largest_div_no;
  int pid_div_cover_array[MAX_PID]; // how many divs cover this para (in general we want a title to be covered by 0)
  int pid_start_pream_array[MAX_PID]; // is this para the first in a preamble div? b/c then the previous para is allowed to be a title
  int pid_end_pream_array[MAX_PID]; // is this para the last in a pream

  #define MAX_LEV 50
  int level_stack[MAX_LEV];
  
  struct Index_Item {
    int line_no;
    int page_no;    
    int pid; // for MUP
    int epid;
    int my_enum; // new 4/30/13
    int lev_no;
    int seq_no;
    int grp_no;
    char *section; //4.5.6
    char *title; // EXHIBIT
    char *header; // INTRODUCTION
    char *section_s[5]; // the string "5"
    int section_v[5]; // the value (D is 4)
    char section_m[5]; // the marker ("0" for none)
    char *clean_header; // chopped and cleaned
    int indent;
    int too_long;
    int center;
    int is_special;

    int toc_page_no; // "23", "123", "xvii"
    int toc_page_no_type; // roman = 1, arab = 2, none = 0,3,4
    int toc_page_no_coord; // 523123

    int deleted;

  }; // index_item
  #define MAX_FILE 50
  #define MAX_TOC 5000
  #define MAX_HEADER 1500
  int max_file_no = 0;
  int max_item_no[MAX_FILE];

  struct Index_Item item_array[MAX_TOC];
  int total_index = 0;
  /****************************************************************/
  int process_LPN(int pid);

#define MAX_PAGE_NO 1000
struct Page_Toc {
  int score;
  int new_score;  
  int page_no;
  int no_of_words; // on page
  int no_of_junk_words; // no of words with less than 2 alnum chars
  int no_of_dict_words; // no of words with more than 2 alnum chars and found in dictionary
  int taken; // taken by sorting
  int cluster_start; // selected as cluster start
  int cluster_end; // selected as cluster end  
  int taken1;  // taken by clustering
} page_toc_array[MAX_PAGE_NO];
int page_toc_no = 0;

int print_page_toc() {
  int ii;
  for (ii = 0; ii < page_toc_no; ii++) {
    // fprintf(stderr,"TOC_PAGE:%d:%d:\n",ii,page_toc_array[ii].score);
  }
  return 0;
}

#define CLUSTER_SIZE 40
struct Cluster {
  int page_score;  // the score
  int page_no; // the page_no itself
  int page_ii; // the ii in the page_array
  int taken; // this cluster is already taken
  int no_of_items; // how many items belong here
  int item_array[CLUSTER_SIZE];
  int first_page; // first page_ii in this cluster
  int last_page;// last page_ii in this cluster
} cluster_array[MAX_PAGE_NO];
int cluster_no = 0;

  
 
%}
tag \<[^\>]*\>
title (Paragraph|PARAGRAPH|Section|SECTION|Article|ARTICLE|Exhibit|EXHIBIT|Attachment|ATTACHMENT|Appendix|APPENDIX|Schedule|SCHEDULE|Part|PART|Glossary|GLOSSARY) 
%%
("<BODY")|("<body") ECHO; BEGIN sbod;
<sbod>\> {
  ECHO;
  printf("<DIV class=\"H1\" lev=\"1\" name=\"_\">\n");
  //  printf("%s<DIV ID=\"top_div\"><UL ID=\"top_ul\"></UL>",yytext); // now in add_niki_notes.lex
  BEGIN 0;
}
"<table"|"<TABLE" {
  in_table = 1;
  if (0 &&debug) fprintf(stderr,"IN_TABLE\n");
  ECHO;
}

"</table"|"</TABLE" {
  in_table = 0;
  if (0 && debug) fprintf(stderr,"OUT_TABLE\n");
  ECHO;
}

("</body")|("</BODY>") {
  int ii;
  //  printf("</MIV  %d %d>\n",item_array[curr_item-1].lev_no,item_array[0].lev_no);
  for (ii = 0; ii <= item_array[curr_item-1].lev_no - item_array[0].lev_no; ii++) {
    printf("</DIV>\n"/*,item_array[curr_item-1].lev_no,item_array[0].lev_no*/);
  }
  ECHO;
  printf("</DIV>\n");
  // printf("</DIV ID=\"top_div\">%s",yytext);
}

<*>\n {
  line_no++; 
  if (YYSTATE == spp) {
    strcat(vpp,yytext);
  } else if (YYSTATE == srest) {
    strcat(vrest,yytext);
  } else {
    ECHO;
  }
}

"<p"[ ]+(id|sn)=\"[0-9]+ {
  int pid = 0;
  char *pp = strstr(yytext,"sn=\"");
  if (pp) {
    pid = atoi(pp+4);
  } else {
    pp = strstr(yytext,"id=\"");
    if (pp) {
      pid = atoi(pp+4);
    }
  }

  BEGIN spp;
  strcpy(vpp,yytext);
  process_LPN(pid);
  if (debug) fprintf(stderr,"LPN=%d:%d:\n",line_no,pid);
}

<spp>[^\>] {
  strcat(vpp,yytext);
}

<spp>\> {
  strcat(vpp,yytext);
  printf("%s",vpp);

  BEGIN srest;
  strcpy(vrest,"");
  inserted_div = 0; // reset sinx <X not found after DIV
}

  <spp>">"/"\n<"(X|MARK) {
  if (debug) fprintf(stderr,"FOUND X %d\n",inserted_div);
  strcat(vpp,yytext);
  printf("%s",vpp);

  if (inserted_div == 0) {  //REMOVE is no good DIV inserted
    BEGIN remove_x;
  } else {
    BEGIN srest;
  }
  strcpy(vrest,"");
  inserted_div = 0;
}

  <remove_x>("<X"[^\>]*\>)|("<MARK"[^\>]*\>) {
  if (debug) fprintf(stderr,"Info (%s): removed :%s:\n",prog,yytext);
  BEGIN srest;
}

<srest>({title}[\ ]+)?[A-Za-z0-9\.\)\(]+ {  // now we are inside a PP
  strcat(vrest,yytext);
  if (dd_flag) {
    strcat(vrest,"</DD>");
  }
  dd_flag = 0;
}
    
    
<srest>"<table"|"<TABLE" {
  strcat(vrest,yytext);
  in_table = 1;
  if (0 &&debug) fprintf(stderr,"IN_TABLE\n");
}

<srest>"</table"|"</TABLE" {
  strcat(vrest,yytext);
  in_table = 0;
  if (0 && debug) fprintf(stderr,"OUT_TABLE\n");
}


<srest>. {
  strcat(vrest,yytext);
}

<srest>"<\p>" {
  strcat(vrest,yytext);
  printf("%s",vrest);
  BEGIN 0;
}

<srest>"<p"[ ]+(id|sn)=\"[0-9]+ {
  int pid = 0;
  char *pp = strstr(yytext,"sn=\"");
  if (pp) {
    pid = atoi(pp+4);
  } else {
    pp = strstr(yytext,"id=\"");
    if (pp) {
      pid = atoi(pp+4);
    }
  }
  printf("%s",vrest);
  process_LPN(pid);
  //if (debug) fprintf(stderr,"LPNR=%d:%d:%d:\n",line_no,pid,curr_item);


  BEGIN spp;
  strcpy(vpp,yytext);
}

<srest>"</body>" {
  printf("%s",vrest);
  int ii;
  //  printf("</MIV  %d %d>\n",item_array[curr_item-1].lev_no,item_array[0].lev_no);
  for (ii = 0; ii <= item_array[curr_item-1].lev_no - item_array[0].lev_no; ii++) {
    printf("</DIV>\n"/*,item_array[curr_item-1].lev_no,item_array[0].lev_no*/);
  }
  ECHO;
  BEGIN 0;
}

  <srest>("<X"[0-9][^\>]*\>)|("<MARK"[^\>]*\>) {
  strcat(vrest,yytext);
}
    
<srest><<eof>> {
  printf("%s",vrest);

  BEGIN spp;
  strcpy(vpp,yytext);
}

<srest>"<HR pn="[0-9]+\> {

  int nn = sscanf(yytext,"<HR pn=%d",&curr_pn);
  strcat(vrest,yytext);
  if (page_toc_array[curr_pn].cluster_start == 1) {
    strcat(vrest,"\n<UTABLE>\n");    
  }
  if (page_toc_array[curr_pn].cluster_end == 1) {
    strcat(vrest,"\n</UTABLE>\n");    
  }
}

%%
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

char *clean_quotes(char *text) {
  int ii, jj;
  static char bext[6000];
  for (ii = 0, jj = 0; ii <strlen(text); ii++) {
    if (text[ii] != '\'' && text[ii] != '\"' && text[ii] != '<' && text[ii] != '>') {
      bext[jj++] = text[ii];
    }
  }
  bext[jj++] = '\0'; 
  return strdup(bext);
}

// updating paragraphlen by select/delete/insert
int do_para_len_sql(MYSQL *conn, int doc_id) {
  char query[500000];

  /************************** PARA LEN ***************************/
  // in order to update each of the rows, we select them, delete them, and the re-insert them
  sprintf(query,"select doc_id,organization_id,para_no,len,all_upper \n\
                   from deals_paragraphlen \n\
                   where doc_id = '%d' "
	  , doc_id);

  if (debug) fprintf(stderr,"QUERY48=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"Error: QUERY48=%s\n",query);
  }

  sql_res = mysql_store_result(conn);
  int nn = mysql_num_rows(sql_res);
  //fprintf(stderr,"FOUNDDDD: %d PAR_LEN results for doc=%d\n",nn,doc_id);
  int pid_no = 0;
  static char paralen_query[500000];
  strcpy(paralen_query,"");
  while (sql_row = mysql_fetch_row(sql_res)) {
    int my_pid = atoi(sql_row[2]);

    static char buff[500];

    sprintf(buff,"('%s','%s','%s','%s','%s','%d', '%d', '%d'), \n"
	    , sql_row[0],sql_row[1],sql_row[2],sql_row[3],sql_row[4],pid_div_cover_array[my_pid],pid_start_pream_array[my_pid],pid_end_pream_array[my_pid]);

    strcat(paralen_query,buff);
    pid_no++;
  }
  if (debug) fprintf(stderr,"PARALEN_QUERY: pids=%d, len = %ld:\n",pid_no,strlen(paralen_query));
  remove_last_comma(paralen_query);

  sprintf(query,"delete from deals_paragraphlen \n\
                   where doc_id = '%d' "
	  , doc_id);

  if (debug) fprintf(stderr,"QUERY49=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"Error: QUERY49=%s\n",query);
  }

  sprintf(query,"delete from deals_page_toc \n\
                   where doc_id = '%d' "
	  , doc_id);

  if (debug) fprintf(stderr,"QUERY47=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"Error: QUERY47=%s\n",query);
  }

  sprintf(query,"insert \n\
                   into deals_paragraphlen \n\
                   (doc_id, organization_id, para_no, len, all_upper,no_of_covering_seqs, start_pream, end_pream) \n\
                   values %s ",
	  paralen_query);

  if (debug) fprintf(stderr,"QUERY50=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"Error: QUERY50=%s\n",query);
  }



  /*********************************************************/
  sprintf(paralen_query,"insert \n\
                   into deals_page_toc \n\
                   (doc_id, first_page, last_page) \n\
                   values \n " );


  int cc;
  int first_page, last_page;

  for (cc = 0; cc < cluster_no; cc++) {
    if (cluster_array[cc].no_of_items >0) {
      first_page = cluster_array[cc].first_page;
      last_page = cluster_array[cc].last_page;
      static char buff[100];
      sprintf(buff,"(%d, %d, %d), \n", doc_id, first_page, last_page);
      strcat(paralen_query,buff);
    }
  }
  remove_last_comma(paralen_query);

  if (debug) fprintf(stderr,"QUERY57=%s\n",paralen_query);
  if (mysql_query(conn, paralen_query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"Error: QUERY57=%s\n",paralen_query);
  }

  /*********************************/
  return 0;
} //do_para_len_sql



int delete_toc_sql(MYSQL *conn, int doc_id) {

  char query[200000];
  /************************* DELETE ****************************/

  /********** ROEE disable delete 09/02/2019 **************************
  sprintf(query,"DELETE RF \n\
                 FROM deals_reportfact as RF \n\
                 LEFT JOIN deals_ts_template_line_value as LV ON (LV.id = RF.ann_lv_id) \n\
                 WHERE LV.doc_id = %d \n\
                   AND LV.toc_item_id is not null  \n\
                   AND RF.doc_id = %d "
	  , doc_id, doc_id);

  if (debug) fprintf(stderr,"QUERY411=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"Error: QUERY411=%s\n",query);
  }


  sprintf(query,"delete LV \n\
                     from deals_ts_template_line_value as LV \n\
                     left join deals_labelinstance as LI on (LI.id = LV.LI_id) \n\
                     where LV.doc_id = \'%d\' \n\
                     and LI.toc_item_id is not null "
	  , doc_id);

  if (debug) fprintf(stderr,"QUERY412=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"Error: QUERY412=%s\n",query);
  }

  sprintf(query,"delete from deals_labelinstance \n\
                     where document_id = \'%d\' \n\
                     and toc_item_id is not null "
	  , doc_id);

  if (debug) fprintf(stderr,"QUERY417=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"Error: QUERY417=%s\n",query);
  }
  *********** ROEE end disable delete 09/02/2019 ***********************/

  // set parent_ids to NULL to deal with toc_item constraints recursion
  sprintf(query,"update deals_toc_item \n\
                     set parent_id = NULL , \n\
                         orig_parent_id = NULL \n\
                     where doc_id = \'%d\' "
	  , doc_id);

  if (debug) fprintf(stderr,"QUERY413A=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"Error: QUERY413A=%s\n",query);
  }

  sprintf(query,"delete from deals_toc_item \n\
                     where doc_id = \'%d\' "
	  , doc_id);

  if (debug) fprintf(stderr,"QUERY413=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"Error: QUERY413=%s\n",query);
  }

  if (debug) fprintf(stderr,"Info: Done delete toc_item: doc_id=%d:\n",doc_id);

  return 0;
} // delete_sql();


int get_last_insert_id(MYSQL *conn) { // get the last_insert_id so we can insert correct IDs as parents
  static char query[150];
  sprintf(query,"insert into deals_toc_item \n\
                     set doc_id=NULL, parent_id = NULL ");

  if (debug) fprintf(stderr,"QUERY09=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"Error: QUERY09=%s\n",query);
  }
  int ret = mysql_insert_id(conn);
  return ret;
}

int collect_pid_cover() { // determine how many seqs cover each pid
  int ii; 
  int max_pid = 0;

  if (debug) fprintf(stderr,"DIVVV0: larg=%d::\n",largest_div_no);
  for (ii = 0; ii <= largest_div_no; ii++) {
    int jj;
    int seq = toc_div_array[ii].seq;

    if (debug) fprintf(stderr,"MY_DIV:%d: s=%d l=%d: b=%d e=%d n=%s:\n"
	    ,ii,seq,toc_div_array[ii].lev,toc_div_array[ii].first_pid,toc_div_array[ii].last_pid
		       ,toc_div_array[ii].name);

    for (jj = toc_div_array[ii].first_pid; jj <= toc_div_array[ii].last_pid; jj++) { // allow the last pid of the div go "free" since the title might be (incorrectly) at the last para of the prev did

      if (toc_div_array[ii].name && toc_div_array[ii].first_pid  == jj && strcmp(toc_div_array[ii].name,"Preamble") == 0) {
	pid_start_pream_array[jj] = 1;
      }

      if (toc_div_array[ii].name && toc_div_array[ii].last_pid  == jj && strcmp(toc_div_array[ii].name,"Preamble") == 0) {
	pid_end_pream_array[jj] = 1;
      }


      if (toc_div_array[ii].name && toc_div_array[ii].lev > 2 && strcmp(toc_div_array[ii].name,"Preamble") != 0) { // preamble is a kosher cover so don't count it, EXHIBIT is a kosher level so don't count it
	pid_div_cover_array[jj]++;
      }
      if (max_pid < jj) max_pid = jj;
    }

  } // for ii


  int jj;
  if (debug) fprintf(stderr,"MAX_PID: max_pid=%d:\n",max_pid);
  for (jj = 0; jj <= max_pid; jj++) {
    if (debug) fprintf(stderr,"PID COVERED: pid=%d max_pid=%d: cover=%d:\n",jj,max_pid,pid_div_cover_array[jj]); 
  }
  if (debug) fprintf(stderr,"DIVVV1: larg=%d::\n",largest_div_no);  
  return 0;
} //  collect_pid_cover

char *my_escape(char text[]) { // not doing mysql_real_escape since it's EXPENSIVE!!!
  static char bext[5000];
  int jj, ii;
  for (ii = 0, jj = 0; ii < strlen(text); ii++) {
    if (text[ii] == '\'' || text[ii] == '\"'  || text[ii] == '\\') {
      bext[jj++] = '\\';
    }
    bext[jj++] = text[ii];
  }

  bext[jj++] = '\0';
  if (strcmp(bext,"0") == 0) bext[0] = '\0';
  return strdup(bext);
}


int read_index(MYSQL *conn, FILE *index_file, int doc_id) {
  int toc_stack[50]; // keep current nesting stack
  toc_stack[0] = toc_stack[1] = toc_stack[2] = -1; // those are dummy levels that have no items
  static char line[4000];
  if (debug) fprintf(stderr,"READING INDEX\n");
  int in = 0;

  //strcpy(line_query,"");
  while (fgets(line,800,index_file)) {
    int seqn, pid, ln, lev_no, gn, my_enum, indent, center, too_long, page_no, line_no, is_special, my_loc;
    static char article[100];
    static char section[100];
    static char clean_header[1000];
    int a1, a2, a3;
    int toc_page_no, toc_page_no_type, toc_page_no_coord;
    int nn = sscanf(line,"%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%[^\t\n]\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n"
	   ,&seqn,&my_enum,&ln,&gn,&pid,&lev_no,article,section,clean_header, &indent, &too_long, &center, &my_loc, &line_no, &page_no, &a1, &a2, &a3, &is_special
	   , &toc_page_no, &toc_page_no_type, &toc_page_no_coord);


    if (debug) fprintf(stderr,"MM ind=%d: too=%d: cen=%d:%d: lp=%d:%d: a=%d:%d:%d: is=:%d: \nLINE=%s:\n", indent, too_long, center, my_loc, line_no, page_no, a1,a2,a3,is_special,line);



    if (debug) fprintf(stderr,"LL(%d:%d:)=%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t:%s:\t%d\t%d\t%d\t\tln=%d\tpn=%d\t%d\t\t%d\t%d\t%d\n"
		       , nn, in
		       ,seqn,my_enum,ln,gn,pid,lev_no,article,section,clean_header, indent, too_long, center
		       , line_no, page_no, is_special
		       , toc_page_no, toc_page_no_type, toc_page_no_coord		       
		       );
    int same_as_before = ((in > 0)  // make sure identical lines are not inserted (a bug in get_preamble inserts some lines twice)
			  && (ln == item_array[in-1].line_no 
			      && my_enum == item_array[in-1].my_enum
			      && pid == item_array[in-1].pid));

    if (lev_no != 10 && same_as_before == 0) {
      char *my_clean_header = strdup(clean_quotes(clean_header));

      item_array[in].indent = indent;
      item_array[in].too_long = too_long;      
      //item_array[in].line_no = ln;
      item_array[in].my_enum = my_enum;
      item_array[in].grp_no = gn;
      item_array[in].seq_no = seqn;
      item_array[in].pid = pid;
      if (strcmp(article,"BASIC") == 0 && lev_no == 2) {
	item_array[in].lev_no = 3;
	my_clean_header = "Basic Terms";
      } else {
	item_array[in].lev_no = lev_no;
      }
      item_array[in].title = ((strcmp(article,"-")==0) ? "" : strdup(article));
      item_array[in].section = strdup(section);
      item_array[in].clean_header = my_clean_header;
      item_array[in].indent= indent;
      item_array[in].center = center;
      item_array[in].page_no = page_no;
      item_array[in].line_no = line_no;
      item_array[in].is_special = is_special;	                  


      fprintf(stderr,"MMMMMM=in=%d: lev=%d: hd=%s: art=%s: sec=%s: ln=%d:%d: pg=%d:\n", in, lev_no,my_clean_header,article,section, ln, line_no, page_no);

      item_array[in].toc_page_no = toc_page_no; // "23", "123", "xvii"
      item_array[in].toc_page_no_type = toc_page_no_type; // roman = 1, arab = 2, none = 0,3,4
      item_array[in].toc_page_no_coord = toc_page_no_coord; // 523123

      if (lev_no > 2) {
	toc_stack[lev_no] = in;
	int item_no = 0; // well, we don't have it...
	char *my_title = (strlen(section) == 0 || strcmp(article,"-") == 0 || strcmp(article,"_") == 0) ? "XX" : article;
      }
      if (toc_seq_array[seqn].no_of_items == 0) {
	toc_seq_array[seqn].first_pid = pid;
	toc_seq_array[seqn].first_enum = my_enum;
	toc_seq_array[seqn].lev = lev_no;
	toc_seq_array[seqn].clean_header = item_array[in].clean_header;
      }
      toc_seq_array[seqn].last_pid = pid;
      toc_seq_array[seqn].last_enum = my_enum;
      toc_seq_array[seqn].no_of_items++;
      in++;
      largest_seq_no = seqn;
    }
  } // while
  //remove_last_comma(line_query);
  return in;
} // read_index


int read_page2summary_points() {
  char p2s_query[500000];

  /************************** PARA LEN ***************************/
  sprintf(p2s_query,"select toc_score, page_no, no_of_words, no_of_junk_words, no_of_dict_words, new_toc_score \n\
                   from deals_page2summary_points \n\
                   where doc_id = '%d' "
	  , doc_id);

  if (debug) fprintf(stderr,"QUERY97=%s\n",p2s_query);
  if (mysql_query(conn, p2s_query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"Error: QUERY97=%s\n",p2s_query);
  }

  sql_res = mysql_store_result(conn);
  int page_no = 0;
  while (sql_row = mysql_fetch_row(sql_res)) {
    page_no = atoi(sql_row[1]);
    //page_toc_array[page_no].score = atoi(sql_row[0]);
    page_toc_array[page_no].page_no = page_no;
    page_toc_array[page_no+1].no_of_words = atoi(sql_row[2]);    
    page_toc_array[page_no+1].no_of_junk_words = atoi(sql_row[3]);
    page_toc_array[page_no+1].no_of_dict_words = atoi(sql_row[4]);    
    page_toc_array[page_no].new_score = atoi(sql_row[5]);    
  }
  return page_no+1;
} // read_page2summary_points() {



// 0 is the threshold for NEW_SCORE
#define SEC_SCORE_THRESHOLD 0
// 0 is the threshold for NEW_SCORE
#define PRI_SCORE_THRESHOLD 0


// finding the best TOC page candidates
int cluster_toc(int page_toc_no) {
  // arrange cluster array by order of descending score
  int ii, jj;
  int max_page_score = -100000;
  int max_page_no = -1;
  int max_page_ii = -1;
    
  /** STEP1: SORTING page_array into cluster_array **/
  for (jj = 0; jj < page_toc_no; jj++) { //sort by score, yes, O(nxn)
    if (debug) fprintf(stderr,"IIIIIII jj=%d:%d:\n",jj,page_toc_array[jj].new_score);
    int max_page_score = -1;
    for (ii = 0; ii < page_toc_no; ii++) {
      if (page_toc_array[ii].taken == 0 && max_page_score < page_toc_array[ii].new_score) {
	max_page_score = page_toc_array[ii].new_score;
	max_page_no = page_toc_array[ii].page_no;
	max_page_ii = ii;
      }
    }
    cluster_array[cluster_no].page_no = max_page_no;
    cluster_array[cluster_no].page_ii = max_page_ii;
    cluster_array[cluster_no].page_score = max_page_score;
    page_toc_array[max_page_ii].taken = 1;
    if (debug) fprintf(stderr,"RRRR: page_ii=%d: cn=%d: score=%d:\n", max_page_ii, cluster_no, max_page_score);
    cluster_no++;
    if (max_page_score < PRI_SCORE_THRESHOLD) break;
  }

  /** STEP2: CLUSTERING */
  if (debug) fprintf(stderr,"\nCL AR:%d:\n",cluster_no);
  for (jj = 0; jj < cluster_no && cluster_array[jj].page_score > PRI_SCORE_THRESHOLD; jj++) {
    int page_ii = cluster_array[jj].item_array[0] = cluster_array[jj].page_ii;
    if (cluster_array[jj].taken == 0 && page_toc_array[page_ii].taken1 == 0) {
      cluster_array[jj].no_of_items++;
      page_toc_array[page_ii].taken1 = 1;
      
      int first_page = cluster_array[jj].page_ii;
      int item_no = cluster_array[jj].no_of_items;

      cluster_array[jj].last_page = first_page;      
      if (debug) fprintf(stderr,"KKK+:jj=%d: fp=%d: item_no=%d: fsc=%d:%d: th=%d:\n",jj, first_page,item_no, /*page_toc_array[first_page+1].score*/-100, page_toc_array[first_page+1].new_score, SEC_SCORE_THRESHOLD);
      for (ii = first_page+1;
	   ii < page_toc_no
	     //&& page_toc_array[ii].score > SEC_SCORE_THRESHOLD //, NOW we look only at NEW_SCORE
	     && page_toc_array[ii].new_score > SEC_SCORE_THRESHOLD // double check
	     && page_toc_array[ii].taken1 == 0;
	   ii++) {
	if (debug) fprintf(stderr,"    BCL+:jj=%d: ii=%d: pii=%d: pnn=%d: score=%d:%d:--\n", jj, ii, cluster_array[jj].page_ii, cluster_array[jj].page_no
			   , /*page_toc_array[ii].score*/ -100, page_toc_array[ii].new_score);	
	cluster_array[jj].item_array[item_no] = ii;
	page_toc_array[ii].taken1 = 1;
	cluster_array[jj].last_page = ii;      	
	item_no++;
      }
      page_toc_array[cluster_array[jj].last_page].cluster_end = 1;  


      if (debug) fprintf(stderr,"KKK-:jj=%d: fp=%d: item_no=%d: fsc=%d:%d: th=%d:\n", jj, first_page,item_no, /*page_toc_array[first_page-1].score*/ -100, page_toc_array[first_page-1].new_score,SEC_SCORE_THRESHOLD);
      cluster_array[jj].first_page = first_page;
      for (ii = first_page-1;
	   ii >= 0
	     //&& page_toc_array[ii].score > SEC_SCORE_THRESHOLD //, NOW we look only at NEW_SCORE
	     && page_toc_array[ii].new_score > SEC_SCORE_THRESHOLD // double check
	     && page_toc_array[ii].taken1 == 0;
	   ii--) {
	if (debug) fprintf(stderr,"    BCL-:jj=%d: ii=%d: pii=%d: pnn=%d: score=%d:%d:--\n", jj, ii, cluster_array[jj].page_ii, cluster_array[jj].page_no
			   , /*page_toc_array[ii].score*/ -100, page_toc_array[ii].new_score);	
	cluster_array[jj].item_array[item_no] = ii;
	page_toc_array[ii].taken1 = 1;
	cluster_array[jj].first_page = ii;	
	item_no++;
      }
      cluster_array[jj].no_of_items = item_no;
      page_toc_array[cluster_array[jj].first_page].cluster_start = 1;
    }
  }
  return 0;
} // cluster_toc()

int para_to_page_array[MAX_PID];
int para_to_page_no = 0;

int print_para_to_page_array() {
  int ii;
  for (ii = 0; ii < para_to_page_no; ii++) {
    if (debug) fprintf(stderr,"PARA_TO_PAGE:%d:%d:\n",ii,para_to_page_array[ii]);
  }
  return 0;
}
  
int read_page2para() {
  char p2s_query[500000];

  /************************** PARA LEN ***************************/
  sprintf(p2s_query,"select page_no, para \n\
                   from deals_page2para \n\
                   where doc_id = '%d' "
	  , doc_id);

  if (debug) fprintf(stderr,"QUERY98=%s\n",p2s_query);
  if (mysql_query(conn, p2s_query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"Error: QUERY98=%s\n",p2s_query);
  }

  sql_res = mysql_store_result(conn);
  int page_no = 0;
  int start_para_no = 0;
  while (sql_row = mysql_fetch_row(sql_res)) {
    static int last_pid = 0;
    static int last_page_no = 0;
    page_no = atoi(sql_row[0]);
    start_para_no = atoi(sql_row[1]);
    int pid;
    for (pid = last_pid; pid < start_para_no; pid++) {
      para_to_page_array[pid] = last_page_no;
      //fprintf(stderr,"    PPPP:page_no=%d: spara_no=%d: last_pid=%d: pid=%d pn=%d:\n", page_no, start_para_no, last_pid, pid, last_page_no);
    }
    last_pid = start_para_no;
    last_page_no = page_no;
  }
  return start_para_no+20;
} // read_page2para() {

// insert cluster_start/cluster_end
int update_page2summary(doc_id) {
  int ii;
  int in_toc = 0;
  for (ii = 0; ii < page_toc_no; ii++) {
    if (page_toc_array[ii].cluster_start == 1) {
      in_toc = 1;
    }

    if (in_toc == 1) {
      static char query[5000];
      sprintf(query, "update deals_page2summary_points \n\
                             set in_toc_by_new_index = 1 \n\
                             where page_no = %d \n\
                                 and doc_id = %d "
	      , ii, doc_id);
      if (debug) fprintf(stderr,"QUERY12=%s\n",query);
      if (mysql_query(conn, query)) {
	fprintf(stderr,"%s\n",mysql_error(conn));
	fprintf(stderr,"Error: QUERY12=%s\n",query);
      }
    } // if in_toc
    
    if (page_toc_array[ii].cluster_end == 1) {
      in_toc = 0;
    }
  } // for ii page
  return 0;
}

int detect_bad_index(int index_no) {
  int ii;
  int found_bad = 0;
  for (ii = 0; ii < index_no; ii++) {
    if (item_array[ii].line_no >= 3 && strncasecmp(item_array[ii].title,"exhib",5) == 0) {
      item_array[ii].deleted = 1;
      found_bad++;
    }
  }
  return found_bad;
}

int update_toc_div_array(int lev, int pid, int curr_item, char *name) {
  toc_div_array[largest_div_no].first_pid = pid;
  toc_div_array[largest_div_no].seq = curr_item;
  toc_div_array[largest_div_no].lev = lev;
  toc_div_array[largest_div_no].name = name;
  //level_stack[lev] = largest_div_no; // THIS WAS INCORRECT-- changed to below 06/18/15
  level_stack[lev] = curr_item;
  largest_div_no++;
  return 0;
}

int process_LPN(int pid) {
  if (curr_item < total_index && item_array[curr_item].pid <= pid) { // the < is in case that 2 items have the same PID (happens if the first line in a file says "EXHIBIT 8.03".  to prevent starvation
    static char title[500];

    if (strlen(item_array[curr_item].title) > 2) {
      sprintf(title,"%s ",item_array[curr_item].title);
    } else {
      strcpy(title,"");
    }
    if (my_mode == 0) { // add H tags
      //printf("<H%d style=\"visibility:hidden\" class=\"my_hdr\">%s%s %s</H%d>\n",item_array[curr_item].lev_no,title,item_array[curr_item].section,item_array[curr_item].clean_header,item_array[curr_item].lev_no);
      last_lev = item_array[curr_item-1].lev_no;
      int ii;
      if (in_table) { // a DIV inside an unclosed table totally corrupts the TOC. so we force close the table before the DIV
	if (debug) fprintf(stderr,"Warning (%s): FORCE ETABLE\n",prog);
	in_table = 0;
	printf("</table>");  // 
      }
      for (ii = 0; ii <= last_lev - item_array[curr_item].lev_no; ii++) { // this is for the case we abruptly jump more than 1 lev up
	int kk = last_lev-ii;
	if (debug) fprintf(stderr,"LPN=%d: pid=%d: curr_item:%d: curr_item_pid=%d: last_lev=%d: diff_lev=%d l_item=%d:\n",line_no,pid,curr_item,item_array[curr_item].pid,last_lev,kk, level_stack[kk]);
	toc_div_array[level_stack[kk]].last_pid = pid-1;
	item_array[level_stack[kk]].epid = pid-1; // UZ 050514
	if ((((toc_div_array[level_stack[kk]].name) && strcmp(toc_div_array[level_stack[kk]].name,"Recitals") == 0)
	     || ((toc_div_array[level_stack[kk]].name) && strcmp(toc_div_array[level_stack[kk]].name,"Signature Block") == 0)
	     || ((toc_div_array[level_stack[kk]].name) && strcmp(toc_div_array[level_stack[kk]].name,"Table of Contents") == 0))
	    && (toc_div_array[level_stack[kk]].last_pid - toc_div_array[level_stack[kk]].first_pid > 5)) {
	  toc_div_array[level_stack[kk]].last_pid-=3;
	}
	printf("</DIV %d %d>\n", last_lev-ii,largest_div_no );
      }
      //fprintf(stderr,"PRINTING: <DIV class=\"H\" lev=\"%d\" unit=\"%s\" sec=\"%s\" name=\"%s\" id=\"div_%d\">\n",item_array[curr_item].lev_no,title,item_array[curr_item].section,item_array[curr_item].clean_header,div_id++);
      for (ii = last_lev+1; ii >=3 && ii < item_array[curr_item].lev_no; ii++) {
	update_toc_div_array(ii, pid, curr_item,"");
	static char dbuff[500];
	sprintf(dbuff,"<DIV class=\"H%d\" lev=\"%d\" unit=\"%s\" sec=\"%s\" name=\"%s\" id=\"div_%d\" last_lev=%d>\n"
		,ii,ii
		,""
		,""
		,""
		,pid //div_id++
		, last_lev); 
	printf("\n%s",dbuff);
	//inserted_div = 1;
	//if (debug) fprintf(stderr,"VDIV1: %s\n",dbuff);
      }
      char *clean_header = clean_quotes(item_array[curr_item].clean_header);
      update_toc_div_array(item_array[curr_item].lev_no, pid, curr_item,clean_header);
      static char dbuff[500];
      sprintf(dbuff,"<DIV class=\"H%d\" lev=\"%d\" unit=\"%s\" sec=\"%s\" name=\"%s\" id=\"div_%d\" last_lev=%d>\n"
	     ,item_array[curr_item].lev_no,item_array[curr_item].lev_no
	     ,clean_quotes(title)
	     ,clean_quotes(item_array[curr_item].section)
	     ,clean_header
	     ,pid //div_id++
	     ,last_lev);
      printf("\n%s",dbuff);
      if (debug) fprintf(stderr,"VDIV0:%s:%s:%c\n",dbuff,clean_quotes(item_array[curr_item].section),clean_quotes(item_array[curr_item].section)[0]);
      if (clean_quotes(item_array[curr_item].section)[0] != '0') { // do not insert <X at the beginning of a TOC due to the TOC div
	inserted_div = 1;
	//if (debug) fprintf(stderr,"YES INSERTED DIV\n");
      } else {
	//if (debug) fprintf(stderr,"NOT INSERTED DIV\n");
      }
    } else if (my_mode == 2) { // add DD
      printf("<DD hd=\"%s\">\n",item_array[curr_item].section);
      dd_flag = 1;
    } else if (my_mode == 1) { // add DD and H
      printf("<H%d  color=red class=\"my_hdr\">%s</H%d>",item_array[curr_item].lev_no,clean_quotes(item_array[curr_item].clean_header),item_array[curr_item].lev_no);
      printf("<DD hd=\"%s\">\n",clean_quotes(item_array[curr_item].section));
      dd_flag = 1;
    }
    curr_item++;
  } // if pid == pid
  return 0;
} // process_LPN()


int insert_index_array_into_sql(MYSQL *conn, int doc_id) {
  int ii;
  char my_toc_item_query[500000];
  char query[500000];
  strcpy(query,"");
  strcpy(my_toc_item_query,"");
  int toc_stack[50]; // keep current nesting stack
  toc_stack[0] = toc_stack[1] = toc_stack[2] = -1; // those are dummy levels that have no items

  sprintf(query,"insert into deals_toc_item \n\
                   (doc_id, item_id, para_no, level, orig_level, enum, grp_id, section, title, header \n\
                  , parent_id, orig_parent_id \n\
                  , end_para_no, indent, center \n\
                  , too_long, special, is_entire_document \n\
                  , line_no, page_no, deleted, in_toc_by_new_index, seq_id,   toc_page_no, toc_page_no_type, toc_page_no_coord, source_program) \n \
                   values \n\
	             %s ",
	  my_toc_item_query);


  if (debug) fprintf(stderr,"ITEM_ARRAY:%d:\n",total_index);
  int last_insert_id = get_last_insert_id(conn); // get the last_insert_id so we can insert correct IDs for parent field
  if (debug) fprintf(stderr,"LAST_INSERT_ID:%d:\n",last_insert_id);
  int so_far_inserted_id_array[MAX_TOC]; // has 1 for item that was inserted.  relative to first II
  int skipped_so_far = 0;
  int in_toc_cluster= 0;
  int pid_cluster_start = -1;
  int pid_cluster_end = -1;  
  for (ii = 0; ii < total_index-1; ii++) { // do not insert the last item which is "Lease --"
    so_far_inserted_id_array[ii+1] = 0;
    int pid= item_array[ii].pid;
    int page_id= para_to_page_array[pid]; // the -1 added by UZ 031512
    int toc_score = page_toc_array[page_id].new_score;
    int toc_cluster_start = page_toc_array[page_id].cluster_start;
    int toc_cluster_end = page_toc_array[page_id].cluster_end;    
    int cc;
    in_toc_cluster = 0;
    for (cc = 0; cc < cluster_no; cc++) {
      if (0) fprintf(stderr,"CCC:%d:%d: pid=%d: fl=%d:%d: rfl=%d:%d:\n",cc,cluster_array[cc].no_of_items, page_id
	      , cluster_array[cc].first_page, cluster_array[cc].last_page, page_toc_array[cluster_array[cc].first_page].page_no, page_toc_array[cluster_array[cc].last_page].page_no);
      if (cluster_array[cc].no_of_items > 0) {
	if (page_id >= page_toc_array[cluster_array[cc].first_page].page_no && page_id <= page_toc_array[cluster_array[cc].last_page].page_no) {
	  in_toc_cluster = 1;
	}
      }
    }
    if (debug) fprintf(stderr,"ITEM/CLUSTER: item=%d: pid=%d: page_id=%d: toc_score=%d: cl_start/end=%d:%d:  pid_start/end=%d:%d: in_toc_cluster=%d:\n"
		       , ii
		       , pid
		       , page_id
		       , toc_score
		       , toc_cluster_start, toc_cluster_end
		       , pid_cluster_start, pid_cluster_end
		       , in_toc_cluster
		       );
	   
	    
    /*
     */
    if (debug) fprintf(stderr,"UUUU:toc_score=%d: ii=%d: pid=%d: page=%d: score=%d: cluster_start=%d: cluster_end:%d: in_cluster=%d: doc_len=%d: no_of_clusters=%d:\n"
	    , toc_score
	    , ii
	    , item_array[ii].pid
	    , para_to_page_array[item_array[ii].pid]
	    , page_toc_array[para_to_page_array[item_array[ii].pid]].new_score
	    , toc_cluster_start
	    , toc_cluster_end
	    , in_toc_cluster
		       , page_toc_no
		       , cluster_no
	    );
    //if (toc_score > 80) { // in 3134 the TOC is 375, the next is 10 so we have a large window, let's see// CAN'T DO THS!!! NOs NOT CONSECUTIVE!!!
    int no_of_words = page_toc_array[para_to_page_array[item_array[ii].pid]].no_of_words;
    int no_of_junk_words = page_toc_array[para_to_page_array[item_array[ii].pid]].no_of_junk_words;
    int no_of_dict_words = page_toc_array[para_to_page_array[item_array[ii].pid]].no_of_dict_words;        
    fprintf(stderr,"JUNK WORDS: ii=%d: pid=%d: pg=%d: now=%d: junk=%d: dict=%d:\n"
	    , ii, item_array[ii].pid, para_to_page_array[item_array[ii].pid], no_of_words, no_of_junk_words, no_of_dict_words);
    if (item_array[ii].deleted == 1) {
      skipped_so_far++;      
    } else if (0
	       && (no_of_junk_words * 3 > no_of_words// most of the words on this page are junk words // deletes good items, taken out UZ 122317
		   && no_of_dict_words * 2 < no_of_words
		   && item_array[ii].lev_no > 2
		   )) {  // but allow "Exhibit" (lev 2)
      if (debug) fprintf(stderr,"      WW:%d: ssf=%d: %d-%d: lev=%d: seq=%d: header=%s: page=%d: w=%d:%d:%d:\n",ii, skipped_so_far, item_array[ii].pid, item_array[ii].epid, item_array[ii].lev_no, item_array[ii].seq_no, item_array[ii].clean_header, page_id, no_of_dict_words, no_of_dict_words * 3,  no_of_words);
      skipped_so_far++;
    } else if (0 && (in_toc_cluster == 1 // now we just mark it as deleted
		&& page_toc_no > 4 // doc must be more than 4 pages long to have a toc
		&& ii != 0 // please leave first item, LEASE...
		&& (para_to_page_array[item_array[ii].pid] < 10 || page_toc_no > 90))) {   // toc must be in first 10 pages if doc is shorter than 90 pages
      if (debug) fprintf(stderr,"      JJ:%d: ssf=%d: %d-%d: lev=%d: seq=%d: header=%s:\n",ii, skipped_so_far, item_array[ii].pid, item_array[ii].epid, item_array[ii].lev_no, item_array[ii].seq_no, item_array[ii].clean_header, page_id);
      skipped_so_far++;
    } else {
      int in_toc_new_index = 0;
      if ((in_toc_cluster == 1
	   && page_toc_no > 4 // doc must be more than 4 pages long to have a toc
	   && ii != 0 // please leave first item, LEASE...
	   && (para_to_page_array[item_array[ii].pid] < 12 || page_toc_no > 90))) {   // toc must be in first 10 pages if doc is shorter than 90 pages
	in_toc_new_index = 1;
      }
      if (debug) fprintf(stderr,"      II:%d: ssf=%d: %d-%d: lev=%d: seq=%d: header=%s: in_toc=%d:%d:\n"
			 ,ii, skipped_so_far, item_array[ii].pid, item_array[ii].epid, item_array[ii].lev_no, item_array[ii].seq_no, item_array[ii].clean_header, in_toc_cluster, in_toc_new_index);

      toc_stack[item_array[ii].lev_no] = ii-skipped_so_far;
      int item_no = 0; // well, we don't have it...
      char *my_title = (strlen(item_array[ii].section) == 0 || strcmp(item_array[ii].title,"-") == 0 || strcmp(item_array[ii].title,"_") == 0) ? "" : item_array[ii].title;

      static char buff[500];
      /******** THIS IS PROBLEMATIC what if not consecutive? ***************/
      static char parent_buff[50];
      int my_parent = toc_stack[item_array[ii].lev_no-1];
      int total_parent = my_parent + last_insert_id +1;
      if (0 && debug) fprintf(stderr,"JJ:ii=%d: lev_no=%d:  my_p=%d: lid=%d: tp=%d: \n", ii, item_array[ii].lev_no, my_parent, last_insert_id, total_parent);
      if (my_parent >= 0) {
	sprintf(parent_buff,"%d",total_parent); // what if not consecutive???
      } else {
	sprintf(parent_buff,"NULL");
      }
      //sprintf(parent_buff,"NULL"); // for now don't show the parent
      so_far_inserted_id_array[ii+1] = 1;
      if (debug) fprintf(stderr,"                     BBB:ID=%d:%d: (%d, %d, %d, %d, %d, %d, '%s', '%s', '%s', %s, %d, %d is_spec=%d: page=%d:%d:%d:),\n "
			 , last_insert_id+ii+1
			 , ((strcmp(parent_buff,"NULL") == 0) ? 2 : so_far_inserted_id_array[atoi(parent_buff)-last_insert_id])
			 , doc_id, ii, item_array[ii].pid, item_array[ii].lev_no, item_array[ii].my_enum, item_array[ii].grp_no, my_escape(item_array[ii].section), my_escape(my_title), my_escape(item_array[ii].clean_header),parent_buff, item_array[ii].epid, in_toc_new_index, item_array[ii].is_special	      , item_array[ii].toc_page_no
	      , item_array[ii].toc_page_no_type
	      , item_array[ii].toc_page_no_coord	      

			 );

      int is_special = 0;
      char *clean_header = my_escape(item_array[ii].clean_header);
      int is_entire_document = (strcmp(clean_header,"Lease") == 0) ? 1 : 0;
      char *escape_title = my_escape(my_title);
      sprintf(buff,"(%d, %d, %d, %d, %d, %d, %d, '%s', '%s', '%s', %s, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,     %d, %d, %d, 'aws'),\n "
	      ,doc_id, ii
	      , item_array[ii].pid
	      , item_array[ii].lev_no
	      , item_array[ii].lev_no	      
	      , item_array[ii].my_enum
	      , item_array[ii].grp_no
	      , my_escape(item_array[ii].section)
	      , ((strcmp(escape_title,"_") == 0) ? "" : escape_title)
	      , clean_header
	      , parent_buff
	      , parent_buff	      
	      , item_array[ii].epid
	      , item_array[ii].indent
	      , item_array[ii].center

	      , item_array[ii].too_long
	      , item_array[ii].is_special
	      , is_entire_document

	      , item_array[ii].line_no
	      , item_array[ii].page_no
	      , 0

	      , in_toc_new_index // indicates deleted// changed to 0 UZ 180218
	      , item_array[ii].seq_no

	      , item_array[ii].toc_page_no
	      , item_array[ii].toc_page_no_type
	      , item_array[ii].toc_page_no_coord

	      //, 'aws'
	      );
      strcat(my_toc_item_query,buff);
      //fprintf(stderr, "KKKK:%s:\n",buff);
    }
    //if (toc_cluster_end == 1) in_toc_cluster = 0;    
  } // for ii
  int kk;
  //for (kk = 0; kk < page_toc_no; kk++)
  {
    static char buff[5000];
    int is_entire_document = 0;
    int is_special = 1;    
    if (1 || page_toc_array[kk].cluster_start == 1) { // don't do for now, UZ 082217



      sprintf(buff,"(%d, %d, %d, %d, %d, %d, '%s', '%s', '%s', '%s', %s, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, '%s'),\n "






	      ,doc_id, 0, 0, 70, 0, 0, "_", "_", "Lease", "_", "NULL", "NULL", 0, is_special, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1,     0, 0, 'aws');






      if (0) strcat(my_toc_item_query,buff);

 

    }
  }
  if (0) {
    static char buff[5000];    
    int is_special = 1;
      sprintf(buff,"(%d, %d, %d, %d, %d, %d, '%s', '%s', '%s', '%s', %s, %s, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,     %d, %d, %d, '%s'),\n "
	      ,doc_id, 0, 0, 70, 0, 0            , "_", "_", "Lease", "_", "NULL", "NULL", 0, is_special, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1,     0, 0, 'aws');	      
      strcat(my_toc_item_query,buff);	      
  }
  remove_last_comma(my_toc_item_query);
  strcat(query,my_toc_item_query);

  if (debug) fprintf(stderr,"QUERY42=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"Error: QUERY42=%s\n",query);
  }
  int affected = mysql_affected_rows(conn);
  if (debug) fprintf(stderr,"Info: Done insert toc_item: doc_id=%d: affected=%d:\n",doc_id, affected);
  return 0;
} // insert_index_array_into_sql()

int main(int ac, char **av) {
  prog = av[0];
  line_no = 0;
  curr_item = 0; // used to be 1
  if (ac != 9) {
    fprintf(stderr,"Error: %s takes 8 params, doc_id, real_name, mode\n",av[0]);
    exit(0);
  }
  prog = av[0];
  fprintf(stderr,"Called %s takes 8 params, doc_id:%s: real_name:%s: mode:%s: debug:%s:\n",av[0],av[1],av[2],av[3],av[4]); 
  doc_id = atoi(av[1]);
  real_name= av[2];
  my_mode = atoi(av[3]);
  debug = atoi(av[4]);
  db_IP = av[5];
  db_name = av[6];
  db_user_name = av[7]; // for DB config
  db_pwd = av[8];

  conn = mysql_init(NULL);
  
  /* now connect to database */
  if (!mysql_real_connect(conn,db_IP,db_user_name,db_pwd,db_name,0,NULL,0)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }

  if (debug)fprintf(stderr,"STEP%d: connect to sql",0);
  sprintf(index_name,"%s",real_name);
  index_file = fopen(real_name,"r");
  if (!index_file) {
    fprintf(stderr,"Error: can't (r) open index_file %s\n",real_name);
    exit(0);
  }

  if (debug) fprintf(stderr,"Opened (r) index_file %s\n",real_name);
  int aa = read_index(conn,index_file, doc_id); // now we populate toc_div_array()
  detect_bad_index(aa);
  total_index = aa;
  if (debug) fprintf(stderr,"STEP%d: done read_index()\n",1);
  delete_toc_sql(conn, doc_id);
  if (debug) fprintf(stderr,"TOTAL_INDEX=%d\n",total_index);
  largest_div_no = 0;
  page_toc_no = read_page2summary_points();
  if (debug)fprintf(stderr,"STEP%d: done page_summary()\n",2);  
  cluster_toc(page_toc_no);
  update_page2summary(doc_id); // insert cluster_start/cluster_end
  if (debug)fprintf(stderr,"STEP%d: done cluster_toc()\n",3);    
  print_page_toc();
  para_to_page_no = read_page2para();
  if (debug)fprintf(stderr,"STEP%d: done read_page2para()\n",3);    
  if (1) print_para_to_page_array();  
  yylex(); // now we populate toc_div_array()
  if (debug)fprintf(stderr,"STEP%d: done yylex()",4);      
  collect_pid_cover();
  do_para_len_sql(conn, doc_id);
  insert_index_array_into_sql(conn,doc_id);
  return 0;
} // main()
