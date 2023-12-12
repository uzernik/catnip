#include <string.h>  
#include <stdio.h>
#include <stdlib.h>	/* for bsearch() */
#include <mysql.h>
#include <ctype.h>
#include <time.h>
#include "entity_functions.h"
#include "my_import_functions.h"
#include "design_convert_functions.h"

#define MIN(a,b) (a<b)?a:b
#define MAX(a,b) (a>b)?a:b


char *my_escape(char *text) {
    static char bext[5000];
    int jj, ii;
    for (ii = 0, jj = 0; ii < strlen(text); ii++) {
      if (text[ii] == '\n') {
	bext[jj++] = ' ';
      } else if (text[ii] != '\"' && text[ii] != '\'' && text[ii] != '!' && text[ii] != '\\') { // ! will be used as a field separator
	bext[jj++] = text[ii];
      }
    }
    bext[jj++] = '\0';
    return strdup(bext);
} // my_escape


int convert_para2tok_to_tok2para() {
  int ii, jj;
  int last_par_tok = -1;
  for (jj = 0; jj < par_tok_no_array; jj++) { // go over all the paras
    for (ii = last_par_tok; ii < par_tok_array[jj].tok_id; ii++) { // for each para mark up all it's toks
      tok2para_array[ii] = jj-1;
    }
    last_par_tok = MAX(last_par_tok,par_tok_array[jj].tok_id);
  }
  for (ii = last_par_tok; ii < last_par_tok+200; ii++) { // do the paras of the last page, we don't know where it ends...
    tok2para_array[ii] = jj-1;
  }
  return last_par_tok+200;
} // convert_para2tok_to_tok2para() {


//bbb
int read_label_type_array() { 
  /************************/
  static char query[5000];
  sprintf(query,"select name, id from deals_label \n\
                     where 1 "
	  );

  if (debug) fprintf(stderr,"QUERY79 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY79 (%s): query=%s:\n",prog,query);
  }

  sql_res = mysql_store_result(conn);
  int nn = mysql_num_rows(sql_res);
  fprintf(stderr,"READ LABELS0:%d:\n",nn);  
  label_type_no = 0;
  while (NULL != (sql_row = mysql_fetch_row(sql_res))) {
    label_type_array[label_type_no].label_type_id = atoi(sql_row[1]);
    label_type_array[label_type_no].label_type_name = strdup(sql_row[0]);
    label_type_no++;
  }
  sort_array_by_word();
  fprintf(stderr,"READ LABELS:%d:\n",label_type_no);
  return label_type_no;
} // read_label_type_array() {

  // originally, attributes were associated with labels 
int read_label_attribute_array() {
  /************************/
  static char query[5000];
  sprintf(query,"select label_id, attribute from deals_ts_label_attribute \n\
                     where 1 "
	  );

  if (debug) fprintf(stderr,"QUERY799 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY799 (%s): query=%s:\n",prog,query);
  }

  sql_res = mysql_store_result(conn);
  //int nn = mysql_num_rows(sql_res);
  
  label_attribute_no = 0;
  while (NULL != (sql_row = mysql_fetch_row(sql_res))) {
    label_attribute_array[label_attribute_no].label_id = atoi(sql_row[0]);
    label_attribute_array[label_attribute_no].attribute_name = strdup(sql_row[1]);
    label_attribute_no++;
  }

  fprintf(stderr,"READ ATTS:%d:\n",label_attribute_no);
  return label_attribute_no;
} // read_label_attribute_array()


int ppp1_cmp(const void *a, const void *b) { 
  char *x1 = (((struct Label_Type *)a)->label_type_name);
  char *y1 = (((struct Label_Type *)b)->label_type_name);
  //fprintf(stderr,"\nComparing <%s> <%s> <%d>\n",x1,y1,strncmp(x1,y1,strlen(x1)));
  return(strcasecmp(x1,y1));
}

int sort_array_by_word() {
  qsort((void *)&label_type_array[0],label_type_no,sizeof(struct Label_Type),ppp1_cmp);
  return 0;
}

int find_fname1(char *text) {
  struct Label_Type *p;
  struct Label_Type mmm;
  struct Label_Type *triple1 = &mmm;

  triple1->label_type_name = text;
  p = (struct Label_Type *)bsearch((void *)triple1,
				(void *)&label_type_array[0], label_type_no,
				sizeof(struct Label_Type), 
				ppp1_cmp);
  int ret = -1;
  if (p != NULL) {
    ret = (p)->label_type_id;
  } else {
    ret = -1;
  }

 return ret;
} // find_fname2 -- label_id

int find_fname2(char *text) {

  struct Template_Line *p;
  struct Template_Line mmm;
  struct Template_Line *triple1 = &mmm;

  triple1->template_line_name = text;
  p = (struct Template_Line *)bsearch((void *)triple1,
				(void *)&template_line_array[0], template_line_no,
				sizeof(struct Template_Line), 
				ppp1_cmp);

  int ret = -1;
  if (p != NULL) {
    ret = (p)->template_line_id;
  } else {
    ret = -1;
  }

 return ret;
} // find_fname2 -- template_lne_id


void copy_little_foot(char *pp) {
  char *pp1 = NULL; // delete everything after "ft"
  pp1 = strstr(pp, "foot");
  if (pp1) {
    strcpy(pp+2,"ft");
  } else {
    pp1 = strstr(pp, "feet");
    if (pp1) {
      strcpy(pp+2,"ft");
    } else {
      pp1 = strstr(pp, "ft");
      if (pp1) {
	strcpy(pp+2,"ft");
      } else {
	;
      }
    }
  }
  return;
}


char *standardize_name(char *name) {
  /*  
      1.  lower case
      2.  replace non-alnum, single '_'
      3.  replace "square foot" by sqft
   */
  static char bext0[500];

  int ii, jj;
  for (jj = 0, ii = 0; ii < strlen(name); ii++) { // remove non-alnum
    if (isalnum(name[ii]) != 0) { // convert non-alnum to '_' and tolower
      bext0[jj++] = tolower(name[ii]);
    } else {
      bext0[jj++] = '_';
    }
  }
  bext0[jj++] = '\0';

  if (strcmp(name,"sqf") == 0) strcpy(bext0,"sqft"); // change "sqf" into "sqft"
  else {
    char *pp = NULL;   // identify square and sq
    pp = strstr(bext0, "square");
    if (pp) {  // we found "square"
      pp[2] = '_';
      strcpy(pp+3,pp+6);
      copy_little_foot(pp);
    } else {
      pp = strstr(bext0, "sq");
      if (pp) {
	copy_little_foot(pp);      
      } else { // if not found sq or square then take care of toot and feet
	char *qq = strstr(bext0, "foot");
	if (!qq) qq = strstr(bext0, "feet");
	if (qq) {
	  qq[1] = 't';
	  qq[2] = '_';
	  strcpy(qq+3,qq+4);
	}
      }
    }
  }

  /****************************** cleanup ********************************/
  static char bext1[500];  
  int in_under;
  for (in_under = 0, jj = 0, ii = 0; ii < strlen(bext0); ii++) { // remove non-alnum
    if (bext0[ii] == '_') { 
      if (in_under == 0) { // first underscore in a chain
	bext1[jj++] = bext0[ii];
	in_under++;
      } else { // in_under > 0, delete the '_'
	;
      }
    } else { // a normal alnum
	bext1[jj++] = bext0[ii];
	in_under = 0;
    }      
  }
  bext1[jj++] = '\0';

  if (bext1[0] == '_') { // trim
    strcpy(bext1, bext1+1);
  }

  if (bext1[strlen(bext1)-1] == '_') { // trim
    bext1[strlen(bext1)-1] = '\0';
  }
  /****************************** done cleanup ********************************/  
  return bext1;
} // standardize_name()

int get_or_create_label_type_id(char *name, char *type, char *source_prog) {
  if (label_type_no == 0) {
    fprintf(stderr,"ERROR: somebody forgot to download the LABEL_TYPE_ARRAY, so new labels will be created again and again...\n");
    exit(-1);
  }
  // type can be "feature", "phrase", "bigram", "clause"
  int ret = -1;
  char *my_name = standardize_name(name);
  if (0) fprintf(stderr,"UUUUUUUUUUUUUU0:%s:%s:\n",name,type);
  ret = find_fname1(my_name);
  if (0) fprintf(stderr,"UUUUUUUUUUUUUU1:%d:\n",ret);
  if (my_name && strlen(my_name) && ret <= 0) {
    if (0) fprintf(stderr,"UUUUUUUUUUUUUU2\n");
    char query[1000];
    sprintf(query,"insert into deals_label \n\
                set \n\
                  type = '%s' , \n\
                  source_prog = '%s' , \n\
                  name = '%s' ",
	    type, source_prog, my_name);

    if (debug) fprintf(stderr,"QUERY57=%s\n",query);    
    if (mysql_query(conn, query)) {
      fprintf(stderr,"QUERY57=%s\n",mysql_error(conn));
    }

    ret = mysql_insert_id(conn);
    label_type_array[label_type_no].label_type_id = ret;    
    label_type_array[label_type_no].label_type_name = strdup(my_name);    
    label_type_no++;
    sort_array_by_word();
#define NOT_EXPIRED 0
  } else if (NOT_EXPIRED
	     && ret > 0 
	     && (strcmp(type,"bigram") == 0 || strcmp(type,"phrase") == 0)) { // for BW compatibility
    if (0) fprintf(stderr,"UUUUUUUUUUUUUU3\n");
    char query[1000];
    sprintf(query,"update deals_label \n\
                set type = '%s' \n\
                where id = '%d' "
	    , type, ret);
    if (debug) fprintf(stderr,"QUERY55=%s\n",query);    
    if (mysql_query(conn, query)) {
      fprintf(stderr,"QUERY55=%s\n",mysql_error(conn));
    }
    if (0) fprintf(stderr,"UUUUUUUUUUUUUU4\n");
  }
  if (0) fprintf(stderr,"UUUUUUUUUUUUUU5:%d:\n",ret);
  return ret;
} //get_or_create_label_type_id()



int check_PLP_exists(MYSQL *conn, int doc_id) {
  static char query[5000];
  sprintf(query,"select id \n\
                        from deals_page_line_properties as plp \n\
                        where \n\
                          plp.doc_id = %d "
	  , doc_id);

  if (debug) fprintf(stderr,"QUERY79_6 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY79_6 (%s): query=%s:\n",prog,query);
  }

  sql_res = mysql_store_result(conn);
  int nn = mysql_num_rows(sql_res);
  return nn;
} // check_PLP_exists(MYSQL *conn, int doc_id) {



 int print_tokens(int token_no) {
   int ii;
   fprintf(stderr,"PRINTING TOKENS:%d:\n",token_no);
   for (ii = 0; ii < token_no; ii++) {
     if (debug) fprintf(stderr,"TOKEN:sn=%d:x1=%s:x2=%s:y1=%s:y2=%s:pl=%d:%d: sn_on_line=%d:\n"
			,token_array[ii].sn
			,token_array[ii].x1
			,token_array[ii].x2
			,token_array[ii].y1
			,token_array[ii].y2
			,token_array[ii].page_no
			,token_array[ii].line_no
			,token_array[ii].sn_on_line
			);
   }
   return 0;
 }

int read_tokens_into_array(MYSQL *conn, int doc_id) {
  static char query[5000];
  sprintf(query,"select tok.sn, tok.x1, tok.x2, tok.y1, tok.y2, tok.page_no, tok.id, tok.text, tok.line_no, plp.center, plp.left_X, tok.para_no \n\
                        from deals_token as tok \n\
                        left join deals_page_line_properties as plp on (plp.page_no = tok.page_no && plp.line_no = tok.line_no) \n\
                        where tok.doc_id = %d \n\
                          and plp.doc_id = %d ",
	  doc_id, doc_id);

  if (debug) fprintf(stderr,"QUERY79_2 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY79_2 (%s): query=%s:\n",prog,query);
  }

  sql_res = mysql_store_result(conn);

  token_no = 0;
  int sn_on_line = -1;
  while (NULL != (sql_row = mysql_fetch_row(sql_res))) {
//    if (debug) fprintf(stderr,"TTT0:%d: \n",token_no);
    int sn = atoi(sql_row[0]);
    static int line_no;
    static int page_no;
    static int prev_line_no = -1;
    token_array[sn].sn = sn;
    token_array[sn].x1 = strdup(sql_row[1]);
    token_array[sn].x2 = strdup(sql_row[2]);
    token_array[sn].y1 = strdup(sql_row[3]);
    token_array[sn].y2 = strdup(sql_row[4]);
    token_array[sn].page_no = page_no = atoi(sql_row[5]);
    token_array[sn].text = strdup(sql_row[7]);
    prev_line_no = line_no;
    token_array[sn].line_no = line_no = atoi(sql_row[8]);
    token_array[sn].center = atoi(sql_row[9]);
    token_array[sn].left_X = atoi(sql_row[10]);
    token_array[sn].pid = atoi(sql_row[11]);   ;
    // calculate the position of this word on the line
    if (line_no != prev_line_no) { // line no's can go up and then down to 0
      token_array[sn].sn_on_line = sn_on_line = 0;
    } else {
      token_array[sn].sn_on_line = sn_on_line = sn_on_line+1;
    }
//    if (debug) fprintf(stderr,"TTT1:%d: pln=%d:%d:%d: sol=%d: \n",token_no, page_no, line_no, prev_line_no, sn_on_line);
    token_no++;
  }

  fprintf(stderr,"READ TOKENS:%d:\n",token_no);
  return token_no;
} // read_tokens_into_array(MYSQL *conn, int doc_id) {



int read_tokens_into_array_for_eo_only(MYSQL *conn, int doc_id) {
  static char query[5000];
  sprintf(query,"select tok.sn, tok.x1, tok.x2, tok.y1, tok.y2, tok.page_no, tok.id, tok.text, tok.line_no /*, plp.center, plp.left_X*/, 0,0 \n\
                        from deals_token as tok \n\
                        /* left join deals_page_line_properties as plp on (plp.page_no = tok.page_no && plp.line_no = tok.line_no)*/ \n	\
                        where tok.doc_id = %d \n\
                          /* and plp.doc_id = %d */ ",
	  doc_id, doc_id);

  if (debug) fprintf(stderr,"QUERY79_3 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY79_3 (%s): query=%s:\n",prog,query);
  }

  sql_res = mysql_store_result(conn);

  token_no = 0;
  int sn_on_line = -1;
  while (NULL != (sql_row = mysql_fetch_row(sql_res))) {
    int sn = atoi(sql_row[0]);
    static int line_no;
    static int page_no;
    static int prev_line_no = -1;
    token_array[sn].sn = sn;
    token_array[sn].x1 = strdup(sql_row[1]);
    token_array[sn].x2 = strdup(sql_row[2]);
    token_array[sn].y1 = strdup(sql_row[3]);
    token_array[sn].y2 = strdup(sql_row[4]);
    token_array[sn].page_no = page_no = atoi(sql_row[5]);
    token_array[sn].text = strdup(sql_row[7]);
    prev_line_no = line_no;
    token_array[sn].line_no = line_no = atoi(sql_row[8]);
    token_array[sn].center = atoi(sql_row[9]);
    token_array[sn].left_X = atoi(sql_row[10]);
    // calculate the position of this word on the line
    if (line_no != prev_line_no) { // line no's can go up and then down to 0
      token_array[sn].sn_on_line = sn_on_line = 0;
    } else {
      token_array[sn].sn_on_line = sn_on_line = sn_on_line+1;
    }
    //fprintf(stderr,"TTT:%d: pln=%d:%d:%d: sol=%d: \n",token_no, page_no, line_no, prev_line_no, sn_on_line);
    token_no++;
  }

  fprintf(stderr,"READ TOKENS:%d:\n",token_no);
  return token_no;
} // read_tokens_into_array_for_eo_only(MYSQL *conn, int doc_id)

int get_page_line_properties() {
  char query[10000];
  sprintf(query,"select page_no, line_no, center, no_of_words, no_of_first_cap_words, no_of_all_cap_words, left_X \n\
                from deals_page_line_properties \n\
                where  \n\
                   doc_id = '%d'  "
	  , doc_id);

    if (debug) fprintf(stderr,"QUERY8.1=%s\n",query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY8.1=%s\n",query);
    }

    sql_res = mysql_store_result(conn);
    int prev_page_id = -1;
    int line_id = -1;
    int page_id = -1;
    int mm = 0;
    while ((sql_row = mysql_fetch_row(sql_res))) {
      page_id = atoi(sql_row[0]);
      if (page_id != prev_page_id && prev_page_id >= 0) {
	page_line_array[prev_page_id].no_of_lines = line_id;
      }
      prev_page_id = page_id;
      line_id = atoi(sql_row[1]);
      page_line_properties_array[page_id][line_id].center = atoi(sql_row[2]);
      page_line_properties_array[page_id][line_id].no_of_words = atoi(sql_row[3]);
      page_line_properties_array[page_id][line_id].no_of_first_cap_words = atoi(sql_row[4]);
      page_line_properties_array[page_id][line_id].no_of_all_cap_words = atoi(sql_row[5]);
      page_line_properties_array[page_id][line_id].left_X = atoi(sql_row[6]);
      // fprintf(stderr,"WWWWWWWW:%d:%d:%d:\n",page_id,line_id,page_line_properties_array[page_id][line_id].no_of_words);
      mm++;
    }
    return page_id+1;
    if (debug) fprintf(stderr,"READ %d page_lines on %d pages\n",mm,page_id+1);
} // get_page_line_properties(

int my_import_load(MYSQL *conn, int doc_id, int org_id[]) {
  int fid = get_did_oid(conn, doc_id, org_id);
  fprintf(stderr,"START LOAD:%d:\n",fid);
  if (fid) {
    get_page_line_properties();
    int exists = check_PLP_exists(conn, doc_id);
    if (exists == 0) {
      token_no = read_tokens_into_array_for_eo_only(conn, doc_id); // in case PLP does not exists for some retro reason
    } else {
      token_no = read_tokens_into_array(conn, doc_id);
    }
    read_label_attribute_array();
    //get_token_array(doc_id);
    get_par_tok_from_sql(conn, doc_id);
    tok2para_no = convert_para2tok_to_tok2para();
    read_label_type_array();
    get_toc_item_from_sql1(conn, doc_id); // where each toc_item starts (in PID units)
    fprintf(stderr,"DONE LOAD\n");
  }
  fprintf(stderr,"DONE LOAD:%d:\n",fid);
  return fid;
}




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


int which_toc_item_is_this_token_in(int tix, int did, int my_pid[]) { // given the tix of the label, find TOC
  int found_toc_item = -1;
  int found = 0;
  my_pid[0] = -1;
  //fprintf(stderr,"FFFF1:%d:\n",tix);
  for (found = 0, pid = 0; found == 0 && pid < par_tok_no_array; pid++) {
    if (tix <= par_tok_array[pid].tok_id) { // find the PID
      found = 1;
      int jj, found;
      for (jj = 0, found = 0; found == 0 && jj < toc_no_array; jj++) {
	//fprintf(stderr,"KKK jj=%d: pid-1=%d: ppid=%d:\n",jj,pid-1,toc_array[did][jj].pid);
	if (pid-1 < toc_array[jj].pid) {
	  found_toc_item = jj;
	  my_pid[0] = pid-1;
	  found = 1;
	  //fprintf(stderr,"KKK1 jj-1=%d: pid-1=%d: ppid=%d: hdr=%s:\n",jj-1,pid-1,toc_array[did][jj-1].pid,toc_array[did][jj-1].header);
	}
      } // jj
      if (found==0) found_toc_item = jj;
      //printf("B:%d:%d:%d:%d:--%d:B ", par_tok_array[did][pid].tok_id,did,tix,pid-1,found_toc_item);
    }
  }
  // if (whp_flag) fprintf(stderr,"FFFF2:pid=%d: tix=%d: toc=%d:\n",pid-2,tix,found_toc_item-1);
  return found_toc_item-1;
} // which_toc_item_is_this_token_in



int stick_dont_delete(MYSQL *conn, int doc_id, char *source_prog) {
  char query[2000000];

  /************ STICK DONT_DELETE= in LV1: select= LV where parent UD_select_option exists OR selected=True *****************************/
  sprintf(query,"select lv.id, lv.* \n\
                     from deals_ts_template_line_value as lv \n\
                     left join deals_ud_selected_option as ud on (lv.id = ud.option_id) \n\
                     where lv.doc_id = %d \n\
                        and (ud.id is not NULL \n\
                            or lv.selected is True) "
	  , doc_id);

  if (debug) fprintf(stderr,"QUERY68 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY68 (%s): query=%s:\n",prog,query);
  }

  sql_res = mysql_store_result(conn);

  int total_found_LV = 0;
  while (NULL != (sql_row = mysql_fetch_row(sql_res))) {
    static char query1[5000];
    sprintf(query1,"update deals_ts_template_line_value \n\
                       set dont_delete = True \n\
                       where id = '%s' "
	    , sql_row[0]);

    if (debug) fprintf(stderr,"QUERY69 (%s): query=%s:\n",prog,query1);
    if (mysql_query(conn, query1)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY69 (%s): query=%s:\n",prog,query1);
    }
    total_found_LV++;
  } // while
  fprintf(stderr,"Set dont_delete= in %d LVs with parent UD_select_option or selected=True \n",total_found_LV);

  /************ STICK DONT_DELETE in LV2: select= inner LV where part of selected or GT group *****************************/
  sprintf(query,"select inner_lv.id, inner_lv.* \n\
                     from deals_ts_template_line_value as inner_lv \n\
                     left join deals_addressgroup ag on inner_lv.belongs_in_address_group_id = ag.id \n\
                     left join deals_labelinstance li on ag.group_LI_id = li.id \n\
                     left join deals_ts_template_line_value outer_lv on outer_lv.LI_id = li.id \n\
                     where outer_lv.doc_id = %d \n\
                        and (outer_lv.selected is True \n\
                            or outer_lv.source_program like 'ground_truth%%') "
	  , doc_id);

  if (debug) fprintf(stderr,"QUERY965 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY965 (%s): query=%s:\n",prog,query);  // no idea what query number I should use, just wrote one
  }

  sql_res = mysql_store_result(conn);

  int total_found_LV2 = 0;
  while (NULL != (sql_row = mysql_fetch_row(sql_res))) {
    static char query1[5000];
    sprintf(query1,"update deals_ts_template_line_value \n\
                       set dont_delete = True \n\
                       where id = '%s' "
	    , sql_row[0]);

    if (debug) fprintf(stderr,"QUERY966 (%s): query=%s:\n",prog,query1);
    if (mysql_query(conn, query1)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY966 (%s): query=%s:\n",prog,query1);
    }
    total_found_LV2++;
  } // while
  fprintf(stderr,"Set dont_delete= in %d inner (group) LVs part of selected or GT group \n",total_found_LV2);

  /************ STICK DONT_DELETE= in LI1: select= LI where parent LV is GT *****************************/
  sprintf(query,"select li.id, li.* \n\
                     from deals_labelinstance as li \n\
                     left join deals_ts_template_line_value as lv on (li.id = lv.li_id) \n\
                     left join deals_ud_selected_option as ud on (lv.id = ud.option_id) \n\
                     where li.document_id = %d \n\
                        and (lv.source_program like 'ground_truth%c' \n\
                            or lv.dont_delete is True \n\
                            or ud.id is not NULL )"
	  , doc_id, '%');

  if (debug) fprintf(stderr,"QUERY70 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY70 (%s): query=%s:\n",prog,query);
  }

  sql_res = mysql_store_result(conn);

  int total_found_LI = 0;
  while (NULL != (sql_row = mysql_fetch_row(sql_res))) {
    static char query1[5000];
    sprintf(query1,"update deals_labelinstance \n\
                     set dont_delete = True \n\
                     where id = '%s' "
	    , sql_row[0]);

    if (debug) fprintf(stderr,"QUERY71 (%s): query=%s:\n",prog,query1);
    if (mysql_query(conn, query1)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY71 (%s): query=%s:\n",prog,query1);
    }
    total_found_LI++;
  } // while
  fprintf(stderr,"Set dont_delete in %d LIs with GT in parent LV \n",total_found_LI);

  /************ STICK DONT_DELETE in LIF: select= LIF where parent LI is GT or Dont_Delete *****************************/
  sprintf(query,"select lif.id, lif.* \n\
                     from deals_labelinstancefield as lif \n\
                     left join deals_labelinstance as li on (li.id = lif.li_id) \n\
                     where lif.document_id = %d \n\
                        and (li.source_program like 'ground_truth%c' \n\
                           or li.dont_delete is True) "
	  , doc_id, '%');

  if (debug) fprintf(stderr,"QUERY72 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY72 (%s): query=%s:\n",prog,query);
  }

  sql_res = mysql_store_result(conn);

  int total_found_LIF = 0;
  while (NULL != (sql_row = mysql_fetch_row(sql_res))) {
    static char query1[5000];
    sprintf(query1,"update deals_labelinstancefield \n\
                       set dont_delete = True \n\
                       where id = '%s' "
	    , sql_row[0]);

    if (debug) fprintf(stderr,"QUERY73 (%s): query=%s:\n",prog,query1);
    if (mysql_query(conn, query1)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY73 (%s): query=%s:\n",prog,query1);
    }
    total_found_LIF++;
  } // while
  fprintf(stderr,"Set dont_delete in %d LIFs with GT in parent LI \n",total_found_LIF);

  /************ STICK DONT_DELETE in LI2: select= LIF where parent LI is GT or Dont_Delete *****************************/
  sprintf(query,"select li.id, li.* \n\
                     from deals_labelinstance as li \n\
                     left join deals_labelinstancefield as lif on (li.id = lif.my_li_id) \n\
                     where li.document_id = %d \n\
                        and (lif.source_program like 'ground_truth%c' \n\
                           or lif.dont_delete is True) "
	  , doc_id, '%');

  if (debug) fprintf(stderr,"QUERY74 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY74 (%s): query=%s:\n",prog,query);
  }

  sql_res = mysql_store_result(conn);

  int total_found_LI2 = 0;
  while (NULL != (sql_row = mysql_fetch_row(sql_res))) {
    static char query1[5000];
    sprintf(query1,"update deals_labelinstance \n\
                       set dont_delete = True \n\
                       where id = '%s' "
	    , sql_row[0]);

    if (debug) fprintf(stderr,"QUERY75 (%s): query=%s:\n",prog,query1);
    if (mysql_query(conn, query1)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY75 (%s): query=%s:\n",prog,query1);
    }
    total_found_LI2++;
  } // while
  fprintf(stderr,"Set dont_delete in %d LI2s with GT in parent LIF \n",total_found_LI2);
  return 0;
} // stick_dont_delete()

int delete_old_folder_from_sql(int folder_id, char *source_prog) {
  static char query[5000];
  sprintf(query,"select id \n\
                     from deals_document \n\
                     where folder_id = %d "
	  , folder_id);

  if (debug) fprintf(stderr,"QUERY743 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY743 (%s): query=%s:\n",prog,query);
  }

  sql_res = mysql_store_result(conn);

  int total_found_docs = 0;
  while (NULL != (sql_row = mysql_fetch_row(sql_res))) {
    int doc_id = atoi(sql_row[0]);
    delete_old_from_sql(conn, doc_id, source_prog, NULL, debug);
  }
  total_found_docs++;
  fprintf(stderr,"Found %d docs in folder %d \n",total_found_docs, folder_id);
  return 0;
} // delete_old_folder_from_sql


int delete_old_from_sql(MYSQL *conn, int doc_id, char *source_prog, char *yyy, int debug) {
    return delete_old_from_sql__only_by_source_program(conn, doc_id, source_prog, yyy, debug, 0);
}

int delete_old_from_sql__only_by_source_program(MYSQL *conn, int doc_id, char *source_prog, char *yyy, int debug, int only_by_source_program) {
    // only_by_source_program:
    //      0: deletes entries also from other places (sp "collect%", all namecluster, and more) -- original behaviour of this function
    //      1: deletes entries only from the given source_program

    int affected;
    char query[2000000];
    char error_query[2000];
    if (!source_prog) source_prog = "%";
    /*********************** delete= extracted labels **********************/

    /*sprintf(query,"delete from deals_reportfact \n\
                     where doc_id = %d "
      , doc_id);*/
    //

    // reportfact by given source_program
    sprintf(query,"delete r \n\
                     from deals_reportfact as r\n\
                     join deals_ts_template_line_value as LV on(r.ann_lv_id = LV.id) \n\
                     left join deals_ud_selected_option as UD on (LV.id = UD.option_id) \n\
                     where \n\
                       LV.doc_id = %d \n\
                       and UD.id is NULL \n\
                       and LV.dont_delete is FALSE \n\
                       and LV.source_program like '%s' "
      , doc_id, source_prog);

    if (debug) fprintf(stderr,"QUERY411=%s\n",query);
    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
        fprintf(stderr,"Error: QUERY411=%s\n",query);
    }


    if (!only_by_source_program) {
        // reportfact by source_program "collect%"
        sprintf(query,"delete r \n\
                         from deals_reportfact as r\n\
                         join deals_ts_template_line_value as LV on(r.ann_lv_id = LV.id) \n\
                         left join deals_ud_selected_option as UD on (LV.id = UD.option_id) \n\
                         where \n\
                           LV.doc_id = %d \n\
                           and UD.id is NULL \n\
                           and LV.dont_delete is FALSE \n\
                           and LV.source_program like '%s%c' "
            , doc_id, "collect",'%');

        if (debug) fprintf(stderr,"QUERY412=%s\n",query);
        if (mysql_query(conn, query)) {
            fprintf(stderr,"%s\n",mysql_error(conn));
            fprintf(stderr,"Error: QUERY412=%s\n",query);
        }

        // LV by source_program "collect%"
        sprintf(query,"delete LV \n\
                         from deals_ts_template_line_value as LV \n\
                         left join deals_ud_selected_option as UD on (LV.id = UD.option_id) \n\
                         where \n\
                           LV.doc_id = %d \n\
                           and UD.id is NULL \n\
                           and LV.dont_delete is FALSE \n\
                           and LV.source_program like '%s%c' "
          , doc_id,"collect",'%');

        if (debug) fprintf(stderr,"QUERY771 (%s): query=%s:\n",prog,query);
        if (mysql_query(conn, query)) {
            fprintf(stderr,"%s\n",mysql_error(conn));
            fprintf(stderr,"QUERY771 (%s): query=%s:\n",prog,query);
        }
    }

    // LV by given source_program
    sprintf(query,"delete LV \n\
                     from deals_ts_template_line_value as LV \n\
                     left join deals_ud_selected_option as UD on (LV.id = UD.option_id) \n\
                     where \n\
                       LV.doc_id = %d \n\
                       and UD.id is NULL \n\
                       and LV.dont_delete is FALSE \n\
                       and LV.source_program like '%s' "
      , doc_id,source_prog);

    if (debug) fprintf(stderr,"QUERY772 (%s): query=%s:\n",prog,query);
    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
        fprintf(stderr,"QUERY772 (%s): query=%s:\n",prog,query);
    }

    if (!only_by_source_program) {
        // all partynameli2namecluster
        sprintf(query,"delete \n\
                         from deals_partynameli2namecluster \n\
                         where \n\
                           doc_id = %d "
          , doc_id);

        if (debug) fprintf(stderr,"QUERY778 (%s): query=%s:\n",prog,query);
        if (mysql_query(conn, query)) {
            fprintf(stderr,"%s\n",mysql_error(conn));
            fprintf(stderr,"QUERY778 (%s): query=%s:\n",prog,query);
        }

        // all namecluster
        sprintf(query,"delete \n\
                         from deals_namecluster \n\
                         where \n\
                           doc_id = %d "
          , doc_id);

        if (debug) fprintf(stderr,"QUERY779 (%s): query=%s:\n",prog,query);
        if (mysql_query(conn, query)) {
            fprintf(stderr,"%s\n",mysql_error(conn));
            fprintf(stderr,"QUERY779 (%s): query=%s:\n",prog,query);
        }

        // all addressgroup
        sprintf(query,"delete LV \n\
                         from deals_ts_template_line_value as LV \n\
                         left join deals_addressgroup as AG on (AG.id = LV.belongs_in_address_group_id)\n\
                         where \n\
                           AG.doc_id = %d \n\
                           and LV.dont_delete = 0 "
          , doc_id);

        if (debug) fprintf(stderr,"QUERY7740 (%s): query=%s:\n",prog,query);
        if (mysql_query(conn, query)) {
            fprintf(stderr,"%s\n",mysql_error(conn));
            fprintf(stderr,"QUERY7740 (%s): query=%s:\n",prog,query);
        }


        // all addressgroup
        sprintf(query,"delete AG \n\
                         from deals_addressgroup as AG \n\
                         left join deals_labelinstance as LI on (LI.id = AG.group_LI_id) \n\
                         where \n\
                           AG.doc_id = %d \n\
                           and AG.group_type like 'address' \n\
                           and LI.dont_delete = 0 "
          , doc_id);

        if (debug) fprintf(stderr,"QUERY774 (%s): query=%s:\n",prog,query);
        if (mysql_query(conn, query)) {
            fprintf(stderr,"%s\n",mysql_error(conn));
            fprintf(stderr,"QUERY774 (%s): query=%s:\n",prog,query);
        }
    }


    // labelinstancefield by given source_program
    sprintf(query,"delete da \n\
                     from deals_labelinstancefield as da \n\
                     left join deals_labelinstance as li on (li.id = da.my_li_id) \n\
                     where \n\
                      da.document_id = %d \n\
                      and li.dont_delete=0 \n\
                      and li.source_program not like '%cground%c' \n\
                      and li.source_program like '%s' "
      , doc_id, '%', '%', source_prog);

    if (debug) fprintf(stderr,"QUERY76 : query=%s:\n",query);
    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
        fprintf(stderr,"QUERY76: query=%s:\n",query);

        sprintf(error_query,"update \n\
                     deals_documentstatus \n\
                     set extractdelete = -1 \n\
                     where \n\
                      document_id = %d "
          , doc_id);

        if (debug) fprintf(stderr,"QUERY01: query=%s:\n",error_query);
        if (mysql_query(conn, error_query)) {
            fprintf(stderr,"%s\n",mysql_error(conn));
            fprintf(stderr,"QUERY01: query=%s:\n",error_query);
        }
    }
    affected = mysql_affected_rows(conn);

    if (debug) fprintf(stderr,"Info: Done delete= LabelInstanceField: doc_id=%d: affected=%d:\n",doc_id, affected);

    // labelinstancefield by given source_program
    sprintf(query,"delete da \n\
                     from deals_labelinstancefield as da \n\
                     left join deals_labelinstance as li on (li.id = da.li_id) \n\
                     where \n\
                      da.document_id = %d \n\
                      and li.dont_delete=0 \n\
                      and li.source_program not like '%cground%c' \n\
                      and li.source_program like '%s' "
      , doc_id, '%', '%', source_prog);

    if (debug) fprintf(stderr,"QUERY77 : query=%s:\n",query);
    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
        fprintf(stderr,"QUERY77: query=%s:\n",query);

        sprintf(error_query,"update \n\
                     deals_documentstatus \n\
                     set extractdelete = -2 \n\
                     where \n\
                      document_id = %d "
          , doc_id);

        if (debug) fprintf(stderr,"QUERY02: query=%s:\n",error_query);
        if (mysql_query(conn, error_query)) {
            fprintf(stderr,"%s\n",mysql_error(conn));
            fprintf(stderr,"QUERY02: query=%s:\n",error_query);
        }
    }
    affected = mysql_affected_rows(conn);

    if (debug) fprintf(stderr,"Info: Done delete= LabelInstanceField: doc_id=%d: affected=%d:\n",doc_id, affected);

    /***********************/
    // labelinstance2chartperiodproperty by given source_program
    sprintf(query,"delete lv \n\
                     from deals_labelinstance2chartperiodproperty as lv \n\
                     join deals_labelinstance as li on (li.id = lv.LI_id) \n\
                     where lv.document_id = %d \n\
                       and li.source_program not like '%cground%c' \n\
                       and li.source_program like '%s' "
      , doc_id, '%', '%', source_prog);

    fprintf(stderr,"QUERY81B (%s): query=%s:\n",prog,query);
    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
        fprintf(stderr,"QUERY81B (%s): query=%s:\n",prog,query);
    }

    affected = mysql_affected_rows(conn);
    if (debug) fprintf(stderr,"Info: Done delete= LI2Chart: doc_id=%d: affected=%d:\n",doc_id, affected);


    // reportfact by given source_program
    sprintf(query,"delete r \n\
                     from deals_reportfact as r\n\
                     join deals_ts_template_line_value as lv on(r.ann_lv_id = lv.id) \n\
                     join deals_labelinstance as li on (li.id = lv.li_id) \n\
                     where lv.doc_id = %d \n\
                       and lv.dont_delete=0 \n\
                       and li.source_program not like '%cground%c' \n\
                       and li.source_program like '%s' "
      , doc_id, '%', '%', source_prog);

    if (debug) fprintf(stderr,"QUERY81F=%s\n",query);
    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
        fprintf(stderr,"Error: QUERY81F=%s\n",query);
    }

    // LV by given source_program
    sprintf(query,"delete lv \n\
                     from deals_ts_template_line_value as lv \n\
                     join deals_labelinstance as li on (li.id = lv.li_id) \n\
                     where lv.doc_id = %d \n\
                       and lv.dont_delete=0 \n\
                       and li.source_program not like '%cground%c' \n\
                       and li.source_program like '%s' "
      , doc_id, '%', '%', source_prog);

    fprintf(stderr,"QUERY81 (%s): query=%s:\n",prog,query);
    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
        fprintf(stderr,"QUERY81 (%s): query=%s:\n",prog,query);

        sprintf(error_query,"update \n\
                     deals_documentstatus \n\
                     set extractdelete = -3 \n\
                     where \n\
                      document_id = %d "
          , doc_id);

        if (debug) fprintf(stderr,"QUERY03: query=%s:\n",error_query);
        if (mysql_query(conn, error_query)) {
            fprintf(stderr,"%s\n",mysql_error(conn));
            fprintf(stderr,"QUERY03: query=%s:\n",error_query);
        }
    }

    affected = mysql_affected_rows(conn);
    if (debug) fprintf(stderr,"Info: Done delete= LineValue: doc_id=%d: affected=%d:\n",doc_id, affected);
    /*****************************************/
    /* LI1 goes directly to LV1; LI1 goes to LV2 via LIF and LI2 */

   // partynameli2namecluster by given source_program
    sprintf(query,"delete p2n \n\
                     from deals_partynameli2namecluster as p2n \n\
                     left join deals_labelinstance as LI on (LI.id = p2n.LI_id) \n\
                     where LI.document_id = %d \n\
                        and LI.dont_delete=0 \n\
                        and LI.source_program not like '%cground%c' \n\
                        and LI.source_program like '%s' \n\
                    and (p2n.doc_id = %d or p2n.doc_id is NULL) "
      , doc_id, '%', '%', source_prog, doc_id);

    if (debug) fprintf(stderr,"QUERY78A (%s): query=%s:\n",prog,query);
    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
        fprintf(stderr,"QUERY78A (%s): query=%s:\n",prog,query);
    }

  /********* delete hirsch connectors ************/
  sprintf(query,"delete \n\
                 from deals_clustertolabelinstancerelation \n\
                 where tenant_id = %d "
	  // and (selected != 1 or selected is null)"
	  , tenant_id);

  if (debug) fprintf(stderr,"QUERYG154=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERYG154=%s\n",mysql_error(conn));
    exit(1);
  }
  fprintf(stderr,"DG154 Records deleted: %lld\n", mysql_affected_rows(conn));


    // LI by given source_program
    sprintf(query,"delete AG \n\
                     from deals_addressgroup as AG \n\
                     left join deals_labelinstance as LI on (LI.id = AG.party_name_LI_id) \n\
                     where AG.doc_id = %d \n\
                        and LI.document_id = %d \n\
                        and LI.dont_delete=0 \n\
                        and LI.source_program not like '%cground%c' \n\
                        and LI.source_program like '%s' "
      , doc_id, doc_id, '%', '%', source_prog);

    if (debug) fprintf(stderr,"QUERY784 (%s): query=%s:\n",prog,query);
    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
        fprintf(stderr,"QUERY784 (%s): query=%s:\n",prog,query);
    }


    // LI by given source_program
    sprintf(query,"delete PSM0 \n\
                     from deals_property_similarity_matrix_for_clustering_lis as PSM0 \n\
                     left join deals_labelinstance as LI on (LI.id = PSM0.LI0_id) \n\
                     where PSM0.doc0_id = %d \n\
                        and LI.document_id = %d \n\
                        and LI.dont_delete=0 \n\
                        and LI.source_program not like '%cground%c' \n\
                        and LI.source_program like '%s' "
      , doc_id, doc_id, '%', '%', source_prog);

    if (debug) fprintf(stderr,"QUERY789 (%s): query=%s:\n",prog,query);
    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
        fprintf(stderr,"QUERY789 (%s): query=%s:\n",prog,query);
    }


    // LI by given source_program
    sprintf(query,"delete PSM1 \n\
                     from deals_property_similarity_matrix_for_clustering_lis as PSM1 \n\
                     left join deals_labelinstance as LI on (LI.id = PSM1.LI1_id) \n\
                     where PSM1.doc1_id = %d \n\
                        and LI.document_id = %d \n\
                        and LI.dont_delete=0 \n\
                        and LI.source_program not like '%cground%c' \n\
                        and LI.source_program like '%s' "
      , doc_id, doc_id, '%', '%', source_prog);

    if (debug) fprintf(stderr,"QUERY788 (%s): query=%s:\n",prog,query);
    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
        fprintf(stderr,"QUERY788 (%s): query=%s:\n",prog,query);
    }




    /*********************************/

    sprintf(query,"delete PPP \n\
                     from deals_clusterleasestatsextension as PPP \n\
                     left join deals_labelinstance as LI on (LI.id = PPP.original_LI_id_of_centroid_id) \n\
                     where 1 \n\
                        and LI.document_id = %d \n\
                        and LI.dont_delete=0 \n\
                        and LI.source_program not like '%cground%c' \n\
                        and LI.source_program like '%s' "
      , doc_id, '%', '%', source_prog);

    if (debug) fprintf(stderr,"QUERY784 (%s): query=%s:\n",prog,query);
    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
        fprintf(stderr,"QUERY784 (%s): query=%s:\n",prog,query);
    }


    /*********************************/


    // LI by given source_program
    sprintf(query,"delete \n\
                     from deals_labelinstance \n\
                     where document_id = %d \n\
                        and dont_delete=0 \n\
                        and source_program not like '%cground%c' \n\
                        and source_program like '%s' "
      , doc_id, '%', '%', source_prog);

    if (debug) fprintf(stderr,"QUERY78 (%s): query=%s:\n",prog,query);
    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
        fprintf(stderr,"QUERY78 (%s): query=%s:\n",prog,query);
        sprintf(error_query,"update \n\
                     deals_documentstatus \n\
                     set extractdelete = -4 \n\
                     where \n\
                      doc_id = %d "
          , doc_id);

        if (debug) fprintf(stderr,"QUERY04: query=%s:\n",error_query);
        if (mysql_query(conn, error_query)) {
            fprintf(stderr,"%s\n",mysql_error(conn));
            fprintf(stderr,"QUERY04: query=%s:\n",error_query);
        }
    }

    affected = mysql_affected_rows(conn);

    if (debug) fprintf(stderr,"Info: Done delete= LabelInstance: doc_id=%d: affected=%d:\n",doc_id, affected);

    return 0;
} // delete_old_from_sql() {




int delete_old_from_sql__with_source_program(MYSQL *conn, int doc_id, char *source_prog, char *yyy, int debug) {
  int affected;
  char query[2000000];
  char error_query[2000];
  if (!source_prog) source_prog = "%";
  /*********************** delete= extracted labels **********************/

    sprintf(query,"delete r \n\
                     from deals_reportfact as r\n\
                     join deals_ts_template_line_value as LV on(r.ann_lv_id = LV.id) \n\
                     left join deals_ud_selected_option as UD on (LV.id = UD.option_id) \n\
                     where \n\
                       LV.doc_id = %d \n\
                       and UD.id is NULL \n\
                       and LV.dont_delete is FALSE \n\
                       and LV.source_program like '%s' "
	  , doc_id, source_prog);

  if (debug) fprintf(stderr,"QUERY4115=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"Error: QUERY4115=%s\n",query);
  }

  sprintf(query,"delete LV \n\
                     from deals_ts_template_line_value as LV \n\
                     left join deals_ud_selected_option as UD on (LV.id = UD.option_id) \n\
                     where \n\
                       LV.doc_id = %d \n\
                       and UD.id is NULL \n\
                       and LV.dont_delete is FALSE \n\
                       and LV.source_program like '%s' "
	  , doc_id, source_prog);

  if (debug) fprintf(stderr,"QUERY7715 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY7715 (%s): query=%s:\n",prog,query);
  }



  sprintf(query,"delete \n\
                     from deals_partynameli2namecluster \n\
                     where \n\
                       doc_id = %d "
	  , doc_id);

  if (debug) fprintf(stderr,"QUERY778 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY778 (%s): query=%s:\n",prog,query);
  }

  sprintf(query,"delete \n\
                     from deals_namecluster \n\
                     where \n\
                       doc_id = %d "
	  , doc_id);

  if (debug) fprintf(stderr,"QUERY779 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY779 (%s): query=%s:\n",prog,query);
  }


  sprintf(query,"delete AG \n\
                     from deals_addressgroup as AG \n\
                     where \n\
                       AG.doc_id = %d "
	  , doc_id);

  if (debug) fprintf(stderr,"QUERY774 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY774 (%s): query=%s:\n",prog,query);
  }




  sprintf(query,"delete da \n\
                     from deals_labelinstancefield as da \n\
                     left join deals_labelinstance as li on (li.id = da.my_li_id) \n\
                     where \n\
                      da.document_id = %d \n\
                      and li.dont_delete=0 \n\
                      and li.source_program not like '%cground%c' \n\
                      and li.source_program like '%s' "
	  , doc_id, '%', '%', source_prog);

  if (debug) fprintf(stderr,"QUERY76 : query=%s:\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY76: query=%s:\n",query);

    {
      sprintf(error_query,"update \n\
                     deals_documentstatus \n\
                     set extractdelete = -1 \n\
                     where \n\
                      document_id = %d "
	      , doc_id);

      if (debug) fprintf(stderr,"QUERY01: query=%s:\n",error_query);
      if (mysql_query(conn, error_query)) {
	fprintf(stderr,"%s\n",mysql_error(conn));
	fprintf(stderr,"QUERY01: query=%s:\n",error_query);
      }
    }
  }
  affected = mysql_affected_rows(conn);

  if (debug) fprintf(stderr,"Info: Done delete= LabelInstanceField: doc_id=%d: affected=%d:\n",doc_id, affected);

  sprintf(query,"delete da \n\
                     from deals_labelinstancefield as da \n\
                     left join deals_labelinstance as li on (li.id = da.li_id) \n\
                     where \n\
                      da.document_id = %d \n\
                      and li.dont_delete=0 \n\
                      and li.source_program not like '%cground%c' \n\
                      and li.source_program like '%s' "
	  , doc_id, '%', '%', source_prog);


  if (debug) fprintf(stderr,"QUERY77 : query=%s:\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY77: query=%s:\n",query);
    {
      sprintf(error_query,"update \n\
                     deals_documentstatus \n\
                     set extractdelete = -2 \n\
                     where \n\
                      document_id = %d "
	      , doc_id);

      if (debug) fprintf(stderr,"QUERY02: query=%s:\n",error_query);
      if (mysql_query(conn, error_query)) {
	fprintf(stderr,"%s\n",mysql_error(conn));
	fprintf(stderr,"QUERY02: query=%s:\n",error_query);
      }
    }

  }
  affected = mysql_affected_rows(conn);

  if (debug) fprintf(stderr,"Info: Done delete= LabelInstanceField: doc_id=%d: affected=%d:\n",doc_id, affected);

  /***********************/
  sprintf(query,"delete lv \n\
                     from deals_labelinstance2chartperiodproperty as lv \n\
                     join deals_labelinstance as li on (li.id = lv.LI_id) \n\
                     where lv.document_id = %d \n\
                       and li.source_program not like '%cground%c' \n\
                       and li.source_program like '%s' "
	  , doc_id, '%', '%', source_prog);

  fprintf(stderr,"QUERY81B (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY81B (%s): query=%s:\n",prog,query);
  }

  affected = mysql_affected_rows(conn);
  if (debug) fprintf(stderr,"Info: Done delete= LI2Chart: doc_id=%d: affected=%d:\n",doc_id, affected);

  sprintf(query,"delete lv \n\
                     from deals_ts_template_line_value as lv \n\
                     join deals_labelinstance as li on (li.id = lv.li_id) \n\
                     where lv.doc_id = %d \n\
                       and lv.dont_delete=0 \n\
                       and li.source_program not like '%cground%c' \n\
                       and li.source_program like '%s' "
	  , doc_id, '%', '%', source_prog);

  fprintf(stderr,"QUERY81 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY81 (%s): query=%s:\n",prog,query);
    {
      sprintf(error_query,"update \n\
                     deals_documentstatus \n\
                     set extractdelete = -3 \n\
                     where \n\
                      document_id = %d "
	      , doc_id);

      if (debug) fprintf(stderr,"QUERY03: query=%s:\n",error_query);
      if (mysql_query(conn, error_query)) {
	fprintf(stderr,"%s\n",mysql_error(conn));
	fprintf(stderr,"QUERY03: query=%s:\n",error_query);
      }
    }

  }

  affected = mysql_affected_rows(conn);
  if (debug) fprintf(stderr,"Info: Done delete= LineValue: doc_id=%d: affected=%d:\n",doc_id, affected);
  /*****************************************/
  /* LI1 goes directly to LV1; LI1 goes to LV2 via LIF and LI2 */
  sprintf(query,"delete \n\
                     from deals_labelinstance \n\
                     where document_id = %d \n\
                        and dont_delete=0 \n\
                        and source_program not like '%cground%c' \n\
                        and source_program like '%s' "
	  , doc_id, '%', '%', source_prog);

  if (debug) fprintf(stderr,"QUERY78B (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {

    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY78B (%s): query=%s:\n",prog,query);

    {
      sprintf(error_query,"update \n\
                     deals_documentstatus \n\
                     set extractdelete = -4 \n\
                     where \n\
                      doc_id = %d "
	      , doc_id);

      if (debug) fprintf(stderr,"QUERY041: query=%s:\n",error_query);
      if (mysql_query(conn, error_query)) {
	fprintf(stderr,"%s\n",mysql_error(conn));
	fprintf(stderr,"QUERY041: query=%s:\n",error_query);
      }
    }

  }

  affected = mysql_affected_rows(conn);

  if (debug) fprintf(stderr,"Info: Done delete= LabelInstance: doc_id=%d: affected=%d:\n",doc_id, affected);

  return 0;
} // delete_old_from_sql() {


#define BUFF 20
int handle_float(double norm_float, char real_display_text[]) {
  static char norm_text[40];
  static char display_text[500];
  sprintf(norm_text,"%6.2f",norm_float);

  int len = strlen(norm_text);
  int ii, jj, kk;
  for (ii = 0; ii <= BUFF; ii++) {
    display_text[ii] = 'L';
  }

  display_text[BUFF] = '\0';
  for (kk = -1, jj = 0,ii = 0; ii <= len; ii++) { // kk counts three's from the "."
    if (norm_text[len-ii] == '.') { // encountered the "."
      display_text[BUFF-jj++] = norm_text[len-ii];

      kk = 0;
    } else if (kk < 0) { // before we encountered the "." just copy
      display_text[BUFF-jj++] = norm_text[len-ii];

    } else if (kk != 0 && kk % 3 == 0) {
      display_text[BUFF-jj++] = ',';
      display_text[BUFF-jj++] = norm_text[len-ii];
      kk++;

    } else {
      display_text[BUFF-jj++] = norm_text[len-ii];
      kk++;

    }
  }

  strcpy(real_display_text,display_text+BUFF-jj+1);

  return 0;
} // handle_int()

#define BUFF 20
int handle_int(int norm_int, char real_display_text[]) {
  static char norm_text[40];
  static char display_text[500];
  sprintf(norm_text,"%d",norm_int);

  int len = strlen(norm_text);
  int ii, jj, kk;
  for (ii = 0; ii <= BUFF; ii++) {
    display_text[ii] = 'L';
  }

  display_text[BUFF] = '\0';
  for (kk = -1, jj = 0,ii = 0; ii <= len; ii++) { // kk counts three's from the "."
    if (kk != 0 && kk % 3 == 0) {
      display_text[BUFF-jj++] = ',';
      display_text[BUFF-jj++] = norm_text[len-ii];
      kk++;

    } else {
      display_text[BUFF-jj++] = norm_text[len-ii];
      kk++;

    }
  }

  strcpy(real_display_text,display_text+BUFF-jj+1);

  return 0;
} // handle_int()


char *create_display_text(double norm_float, char *buff2, int norm_int, char *norm_date, int atype, char *norm_text) {
  static char display_text[5000];
  if (atype == 3) { // date
    int yy,mm,dd;
    int nn = sscanf(norm_date, "%d-%d-%d",&yy,&mm,&dd);
    if (nn == 3) {
      sprintf(display_text,"%02d/%02d/%04d",mm,dd,yy);
    } else {
      strcpy(display_text,norm_date); // in case date not valid then show it
    }
  } else if (atype == 0) { // int
    handle_int(norm_int, display_text);
  } else if (atype == 2) { // float
    handle_float(norm_float, display_text);
  } else if (atype == 1 && norm_text != NULL) { // text
    sprintf(display_text,"%s",norm_text);
  }else { // text, we need to decide what to do based on LABEL
    sprintf(display_text,"%s",buff2);
  }

  return display_text;
}

// type is: phrase / feature / bigram
int insert_label_instance(MYSQL *conn, int doc_id, int folder_id, int label_no, char *type, char *source_prog) {
  fprintf(stderr,"InsertLabelInstance\n");
  static char query[500000];
  int affected;
  char clean_toc_query[2000000];
  int ii;

  /********************************** INSTANCE *************************************************/
  sprintf(query,"insert into deals_labelinstance \n\
           (document_id, folder_id, start_pid, end_pid, label_id, label_name \n\
            , token_index, num_tokens, source, norm_date, norm_float, norm_int \n\
            , source_program, score_100, display_text, text, atype, annot \n\
            , no_year, no_month, no_day, insertion_date, norm_text) \n\
           values \n");
  strcpy(clean_toc_query,"");
  if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMMX: %d:\n", label_no);
  for (ii = 0; ii < label_no; ii++) {
    if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMMY:ii=%d: :%s:\n", ii, instance_array[ii].label_name);
    if (!(instance_array[ii].label_name && strlen(instance_array[ii].label_name) > 2 && strlen(instance_array[ii].label_name) < 30)) {
      fprintf(stderr,"\n\nError: incorrect label_name:%s:\n\n",instance_array[ii].label_name);
    } else {
      // char cc = (ii == label_no-1) ? ' ' : ',';

      int token_type_id = get_or_create_label_type_id(instance_array[ii].label_name, type, source_prog);
      if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMM0:ii=%d: %d:\n", ii, token_type_id);
      int token_index = instance_array[ii].span_no;
      if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMM1: %d:\n", token_type_id);
      int num_tokens = instance_array[ii].last_span_no - instance_array[ii].span_no + 1;
      if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMM2: %d:\n", token_type_id);
      static char buff[50000];
      char *buff2;
      if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMM3: %d:\n", token_type_id);
      buff2 = (instance_array[ii].text) ? my_escape(instance_array[ii].text) : "___";
      if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMM4: %d:\n", token_type_id);
      int my_pid;
      int end_pid;
      if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMM5:ii=%d: cpid=%d: tx=%d:%d: tcpid=%d: tcepid=%d:\n"
	      , ii
	      , instance_array[ii].pid
	      , token_index, num_tokens
	      , tok2para_array[token_index]
	      , tok2para_array[token_index+num_tokens-1]
	      );

      if (0 && instance_array[ii].pid) { // if there is a PID, take it for both
	my_pid = instance_array[ii].pid;
	end_pid = instance_array[ii].pid;
	if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMM6: tcpid=%d: tcepid=%d:\n" , my_pid,end_pid);

	//fprintf(stderr, "labelinstance inserted with pid: pid=%d\n", my_pid);
      } else { // if there is no PID then take it the right way, from the tokens
	my_pid = tok2para_array[token_index];
	end_pid = tok2para_array[token_index+num_tokens-1];
	if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMM7: tcpid=%d: tcepid=%d:\n" , my_pid,end_pid);
      }
      if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMM8: %d:\n", token_type_id);
      // dummies for now
      int atype = instance_array[ii].atype;
      char *annot = instance_array[ii].annot;
      int norm_int = instance_array[ii].norm_int;
      double norm_float = instance_array[ii].norm_float;
      char *norm_text = instance_array[ii].norm_text;
      if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMM9: %d: norm_int=%d:%s:\n", token_type_id, norm_int, norm_text);
      static char norm_date[500];
      if (instance_array[ii].date_norm == NULL || (strstr(instance_array[ii].date_norm,"null")!= NULL)) {
	sprintf(norm_date,"%s","NULL");
      } else {
	sprintf(norm_date,"%s",instance_array[ii].date_norm);
      }
      static char *display_text;
      int no_year = 0, no_month = 0, no_day = 0;
      char vy[10], vm[10], vd[10];
      if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMMA: %d:\n", token_type_id);
      if (atype == 3) {
	sscanf(norm_date,"%[0-9]-%[0-9]-%[0-9]",vy,vm,vd);
	if (atoi(vy) == 0) {
	  no_year = 1;
	  norm_date[0] = '1';norm_date[1] = '9';norm_date[2] = '7';norm_date[3] = '0';
	}
	if (atoi(vm) == 0) {
	  no_month = 1;
	  norm_date[5] = '0';norm_date[6] = '1';
	}
	if (atoi(vd) == 0) {
	  no_day = 1;
	  norm_date[8] = '0';norm_date[9] = '1';
	}
      }
      if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMMB: %d:\n", token_type_id);
      display_text = create_display_text(norm_float, buff2, norm_int, norm_date, atype, norm_text);
      if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMMB1: %d:\n", ii);
      //display_text = create_display_text1(norm_float, atype);


      if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMMB4: %d:\n", token_type_id);

      sprintf(buff,"(%d, %d, %d, %d, %d, '%s',       %d, %d, '%s', '%s', %2.2f, %d,       '%s', %d, '%s', '%s', %d, '%s',       %d, %d, %d, now(), '%s'), \n "
	      , doc_id
	      , folder_id
	      , my_pid
	      , end_pid
	      , token_type_id
	      , instance_array[ii].label_name

	      , token_index
	      , num_tokens
	      , "uri/prog"
	      , norm_date
	      , norm_float
	      , norm_int


	      , source_prog
	      , instance_array[ii].score
	      , my_escape(display_text)
	      , buff2
	      , atype
	      , annot

	      , no_year
	      , no_month
	      , no_day
	      , norm_text
 	      );
      if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMMB2: %d:\n", token_type_id);
      strcat(clean_toc_query,buff);
      if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMMC: %d:\n", token_type_id);
    }
    if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMMD: %d:\n", 5);
  } // for ii
  if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMME: %d:\n", 5);
  strcat(query,clean_toc_query);
  if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMMF: %d:\n", 5);
  remove_last_comma(query);

  if (debug) fprintf(stderr,"QUERY532=%s\n",query);
  if (label_no == 0) {
    fprintf(stderr,"Warning: No Instances Found!?\n");
    affected = 0;
  } else {
    if (mysql_query(conn, query)) {
      fprintf(stderr,"QUERY532=%s\n",mysql_error(conn));
    }
    affected = mysql_affected_rows(conn);
  }

  fprintf(stderr,"Info: Ddone insert LabelInstances: doc_id=%d: affected=%d:\n",doc_id, affected);

  int first_id = mysql_insert_id(conn);

  if (0) fprintf(stderr,"MMMMMMMMMMMMMMMMMMG: %d:\n", 5);
  return first_id;
} //  insert_label_instance()

int insert_line_value(MYSQL *conn, int doc_id, int label_no, int first_id, char *source_prog) {
  static char query[500000];
  int affected;
  char clean_toc_query[2000000];
  int ii;

  /********************************** INSTANCE *************************************************/
  sprintf(query,"insert into deals_ts_template_line_value \n\
           (score, insertion_date, LI_id, doc_id, organization_id, template_id, template_line_id, source_program) \n\
           values ");
  strcpy(clean_toc_query,"");

  for (ii = 0; ii < label_no; ii++) {
    if (instance_array[ii].label_name) {
      char cc = (ii == label_no-1) ? ' ' : ',';

      int token_type_id = get_or_create_label_type_id(instance_array[ii].label_name, "", source_prog);

      int token_index = instance_array[ii].span_no;
      char *x1 = (token_index >=0) ? token_array[token_index].x1 : "0.0";
      char *x2 = (token_index >=0) ? token_array[token_index].x2 : "0.0";
      int p1 = token_array[token_index].page_no;
      int num_tokens = instance_array[ii].last_span_no - instance_array[ii].span_no + 2;
      int last_token_index = instance_array[ii].last_span_no;
      char *y1 = (last_token_index >=0) ? token_array[last_token_index].y1 : "0.0";
      char *y2 = (last_token_index >=0) ? token_array[last_token_index].y2 : "0.0";
      int p2 = token_array[last_token_index].page_no;
      char *desig = instance_array[ii].desig;

      static char buff[50000];
      //static char buff2[500000];
      //strcpy(buff2,(instance_array[ii].text) ? my_escape(instance_array[ii].text) : "___");
      char *buff2;
      buff2 = (instance_array[ii].text) ? my_escape(instance_array[ii].text) : "___";

      int my_pid = tok2para_array[token_index];
      //fprintf(stderr,"MMMMMMMMMM:%d-->%d:\n",token_index,my_pid);
      sprintf(buff,"(%d, %d, %d, '%s', %d, %d, '%s', '%s', '%s', '%s', '%s', %d, '%s', '%s', '%s', '%s', %d, %d, now())%c "
	      ,doc_id, my_pid, token_type_id, instance_array[ii].label_name, token_index, num_tokens, "uri/prog", desig, buff2
	      , instance_array[ii].date_norm, source_prog, instance_array[ii].score, x1, x2, y1, y2, p1, p2, cc);
      strcat(clean_toc_query,buff);
    }
  }
  strcat(query,clean_toc_query);
  if (debug) fprintf(stderr,"QUERY53=%s\n",query);
  if (label_no == 0) {
    fprintf(stderr,"Warning: No Instances Found!?\n");
    affected = 0;
  } else {
    if (mysql_query(conn, query)) {
      fprintf(stderr,"QUERY53=%s\n",mysql_error(conn));
    }
    affected = mysql_affected_rows(conn);
  }
  fprintf(stderr,"Info: Done insert predicted instance: doc_id=%d: affected=%d:\n",doc_id, affected);
  return 0;
} // insert_line_value(MYSQL *conn, int doc_id, int label_no, int first_id) {



int get_folder_from_document(int doc_id) {
  static char query[5000];
  int ret = -1;
  sprintf(query,"select folder_id \n\
                     from deals_document \n\
                     where id = %d "
	  , doc_id);

  if (debug) fprintf(stderr,"QUERY743 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY743 (%s): query=%s:\n",prog,query);
  }

  sql_res = mysql_store_result(conn);
  sql_row = mysql_fetch_row(sql_res);
  if (sql_row) {
    ret = atoi(sql_row[0]);
  }
  return ret;
} // get_folder_from_document(int doc_id)

int enter_instance_into_sql(MYSQL *conn, int doc_id, int label_no, int generate_lv_for_labeling, char *type, char *source_prog) {
  // generate_lv_for_labeling is for showing the instance in the labeling system as if it was selected by the user, as in titles
  int folder_id = get_folder_from_document(doc_id);
  int first_id = insert_label_instance(conn, doc_id, folder_id, label_no, type, source_prog); // first_id is the ID of the first label_instance in the block
  if (generate_lv_for_labeling == 1) {
    insert_line_value(conn, doc_id, label_no, first_id, source_prog);
  }
  return 0;
} //enter_into_sql


int get_par_tok_from_sql(MYSQL *conn, int did) {
  MYSQL_RES *sql_res;
  MYSQL_ROW sql_row;
  static char query[10000];
  int nn;

  sprintf(query,"select para_no, token_id, num_tokens, doc_id \n\
                          from deals_paragraphtoken as dd \n\
                          where doc_id = '%d' "
	  , did
	  );
  if (debug) fprintf(stderr,"QUERY6=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }

  sql_res = mysql_store_result(conn);
  nn = mysql_num_rows(sql_res);
  if (debug) fprintf(stderr,"NUM_ROWS6=%d\n",nn);
  int ii = 0;
  if (nn == 0) {
    if (debug) fprintf(stderr,"Error: PARA_LEN item not found in SQL\n");
  } else {
    while (ii < nn) {
      sql_row = mysql_fetch_row(sql_res);

      int pid = atoi(sql_row[0]);
      //int doc_id = atoi(sql_row[3]);
      par_tok_array[pid].tok_id = atoi(sql_row[1]);
      par_tok_array[pid].tok_num = atoi(sql_row[2]);
      ii++;
      par_tok_no_array = pid+1;
      //fprintf(stderr,"PID_NO=%d\n",pid+1);
    }
  }
  return ii;
} // get_par_tok_from_sql()

char *get_doc_ocr_engine(int doc_id) {
  static char query[5000];
  char* ret = NULL;
  sprintf(query,"select ocr_engine \n\
                     from deals_document \n\
                     where id = %d "
	  , doc_id);

  if (debug) fprintf(stderr,"QUERY (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY (%s): query=%s:\n",prog,query);
  }

  sql_res = mysql_store_result(conn);
  sql_row = mysql_fetch_row(sql_res);
  if (sql_row && sql_row[0]) {
    ret = strdup(sql_row[0]);
  } else {
    ret = "AWS";
  }
  /*  
  if (sql_row) {
    ret = strdup(sql_row[0]);
  }
  */
  return ret;
} // get_doc_ocr_engine(int doc_id)

// should replace the one above after testing
int get_toc_item_from_sql1(MYSQL *conn, int did) {
  MYSQL_RES *sql_res;
  MYSQL_ROW sql_row;
  static char query[10000];
  int nn;
  //int toc_stack[50];

  char aws_ocr_filter[] = " \n\
                            and p2t.source_program='aws' \n\
                            and toc.source_program='aws' \n\
                            and tok.source_program='aws' \n\
                            ";
  char* ocr_engine = get_doc_ocr_engine(did);
  int is_abbyy = strcmp(ocr_engine, "ABBYY") == 0 || ocr_engine == NULL;
  char* current_ocr_filter = is_abbyy ? "" : aws_ocr_filter;

  //int max_toc_pid; // the max PID we ran into so far in the TOC
  sprintf(query,"select toc.id, toc.doc_id, toc.para_no, toc.level, toc.item_id, toc.enum, toc.section, toc.title, toc.header, toc.end_para_no, tok.page_no, tok.line_no, tok.sn, toc.special, seq_id, grp_id \n\
                          from deals_toc_item as toc \n\
                          left join deals_paragraphtoken as p2t on (toc.para_no = p2t.para_no) \n\
                          left join deals_token as tok on (tok.sn = p2t.token_id and tok.doc_id = '%d') \n\
                          where p2t.doc_id = '%d' \n\
                            and toc.doc_id = '%d' \n\
                            and tok.doc_id = '%d' \n\
                            %s \n\
	                  order by toc.para_no asc, toc.end_para_no asc "
	  ,did,did,did,did,current_ocr_filter
	  );

  if (debug) fprintf(stderr,"QUERY5=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }
  sql_res = mysql_store_result(conn);
  nn = mysql_num_rows(sql_res);

  if (debug) fprintf(stderr,"NUM_ROWS0=%d\n",nn);
  int ii = 0;
  if (nn == 0) {
    //fprintf(stderr,"Error: TOC_ITEM item not found in SQL\n");
  } else {
    while (ii < nn) {
      sql_row = mysql_fetch_row(sql_res);	
      // fprintf(stderr,"TOC:id=%d: did=%d: pid=%d: lev=%d: item_id=%d: enum=%d: section=%s: title=%s: header=%s:\n",atoi(sql_row[0]),atoi(sql_row[1]),atoi(sql_row[2]),atoi(sql_row[3]),atoi(sql_row[4]),atoi(sql_row[5]),sql_row[6],sql_row[7],sql_row[8]); 
      int lev;
      //int did = atoi(sql_row[1]);
      toc_array[ii].id = atoi(sql_row[0]);
      toc_array[ii].pid = atoi(sql_row[2]);
      toc_array[ii].lev = lev = atoi(sql_row[3]);
      toc_array[ii].item_id = atoi(sql_row[4]);
      toc_array[ii].my_enum = atoi(sql_row[5]);
      toc_array[ii].section = strdup(sql_row[6]);
      toc_array[ii].title = strdup(sql_row[7]);
      toc_array[ii].header = strdup(sql_row[8]);
      toc_array[ii].epid =  atoi(sql_row[9]);
      toc_array[ii].page_no = (sql_row[10]) ? atoi(sql_row[10]) : 0;
      toc_array[ii].line_no = (sql_row[11]) ? atoi(sql_row[11]) : 0;
      toc_array[ii].token_id = (sql_row[12]) ? atoi(sql_row[12]) : 0;
      toc_array[ii].special = atoi(sql_row[13]);
      toc_array[ii].seq_id = (sql_row[14]) ? atoi(sql_row[14]) : 0;
      toc_array[ii].grp_id = (sql_row[15]) ? atoi(sql_row[15]) : 0;                              
      if (0 && debug) fprintf(stderr,"TOC_NO=%d: id=%d:  lev=%d: pid=%d: plt=%d:%d:%d: tit=%s: sec=%s: hd=%s: seq=%d: grp=%d: spec=%d:\n"
			 ,ii,toc_array[ii].id, toc_array[ii].lev ,toc_array[ii].pid, toc_array[ii].page_no,toc_array[ii].line_no,toc_array[ii].token_id
			 , toc_array[ii].title, toc_array[ii].section, toc_array[ii].header, toc_array[ii].seq_id, toc_array[ii].grp_id, toc_array[ii].special);
      ii++;
      toc_no_array = ii;
      toc_no = ii;
      //max_toc_pid = atoi(sql_row[2]);
    }
  }
  return ii;
} // get_toc_item_from_sql1()


// should replace the one above after testing
int get_toc_item_from_sql_multiple_doc(MYSQL *conn, int did, int dd) {
  MYSQL_RES *sql_res;
  MYSQL_ROW sql_row;
  static char query[10000];
  int nn;
  //int toc_stack[50];

  //int max_toc_pid; // the max PID we ran into so far in the TOC
  sprintf(query,"select toc.id, toc.doc_id, toc.para_no, toc.level, toc.item_id, toc.enum, toc.section, toc.title, toc.header, toc.end_para_no, tok.page_no, tok.line_no, tok.sn, toc.special \n\
                          from deals_toc_item as toc \n\
                          left join deals_paragraphtoken as p2t on (toc.para_no = p2t.para_no) \n\
                          left join deals_token as tok on (tok.sn = p2t.token_id and tok.doc_id = '%d') \n\
                          where p2t.doc_id = '%d' \n\
                            and toc.doc_id = '%d' \n\
	                  order by toc.para_no asc, toc.end_para_no asc "
	  ,did,did,did
	  );

  if (debug) fprintf(stderr,"QUERY5=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }
  sql_res = mysql_store_result(conn);
  nn = mysql_num_rows(sql_res);

  if (debug) fprintf(stderr,"NUM_ROWS0=%d\n",nn);
  int ii = 0;
  if (nn == 0) {
    //fprintf(stderr,"Error: TOC_ITEM item not found in SQL\n");
  } else {
    while (ii < nn) {
      sql_row = mysql_fetch_row(sql_res);	
      // fprintf(stderr,"TOC:id=%d: did=%d: pid=%d: lev=%d: item_id=%d: enum=%d: section=%s: title=%s: header=%s:\n",atoi(sql_row[0]),atoi(sql_row[1]),atoi(sql_row[2]),atoi(sql_row[3]),atoi(sql_row[4]),atoi(sql_row[5]),sql_row[6],sql_row[7],sql_row[8]); 
      int lev;
      //int did = atoi(sql_row[1]);
      toc_array_dd[dd][ii].id = atoi(sql_row[0]);
      toc_array_dd[dd][ii].pid = atoi(sql_row[2]);
      toc_array_dd[dd][ii].lev = lev = atoi(sql_row[3]);
      toc_array_dd[dd][ii].item_id = atoi(sql_row[4]);
      toc_array_dd[dd][ii].my_enum = atoi(sql_row[5]);
      toc_array_dd[dd][ii].section = strdup(sql_row[6]);
      toc_array_dd[dd][ii].title = strdup(sql_row[7]);
      toc_array_dd[dd][ii].header = strdup(sql_row[8]);
      toc_array_dd[dd][ii].epid =  atoi(sql_row[9]);
      toc_array_dd[dd][ii].line_no = (sql_row[10]) ? atoi(sql_row[10]) : 0;
      toc_array_dd[dd][ii].page_no = (sql_row[11]) ? atoi(sql_row[11]) : 0;
      toc_array_dd[dd][ii].token_id = (sql_row[12]) ? atoi(sql_row[12]) : 0;
      toc_array_dd[dd][ii].special = atoi(sql_row[13]);                  
      if (debug) fprintf(stderr,"TOC_NO=%d: id=%d:  lev=%d: pid=%d: plt=%d:%d:%d: tit=%s: sec=%s: hd=%s: \n",ii,toc_array_dd[dd][ii].id, toc_array_dd[dd][ii].lev ,toc_array_dd[dd][ii].pid, toc_array_dd[dd][ii].page_no,toc_array_dd[dd][ii].line_no,toc_array_dd[dd][ii].token_id, toc_array_dd[dd][ii].title, toc_array_dd[dd][ii].section, toc_array_dd[dd][ii].header);
      ii++;
      toc_no_array_dd[dd] = ii;
      //max_toc_pid = atoi(sql_row[2]);
    }
  }
  return ii;
} // get_toc_item_from_sql_multiple_doc()

int get_toc_item_from_sql(MYSQL *conn, int did) {
  MYSQL_RES *sql_res;
  MYSQL_ROW sql_row;
  static char query[10000];
  int nn;

  sprintf(query,"select id, doc_id, para_no, level, item_id, enum, section, title, header, end_para_no \n\
                          from deals_toc_item dd \n\
                          where doc_id = '%d' "
	  ,did
	  );

  if (debug) fprintf(stderr,"QUERY52=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY52=%s\n",mysql_error(conn));
    exit(1);
  }
  sql_res = mysql_store_result(conn);
  nn = mysql_num_rows(sql_res);

  if (debug) fprintf(stderr,"NUM_ROWS0=%d\n",nn);
  int ii = 0;
  if (nn == 0) {
    //fprintf(stderr,"Error: TOC_ITEM item not found in SQL\n");
  } else {
    while (ii < nn) {
      sql_row = mysql_fetch_row(sql_res);	
      if (0) fprintf(stderr,"TOC:id=%d: did=%d: pid=%d: lev=%d: item_id=%d: enum=%d: section=%s: title=%s: header=%s:\n"
	      ,atoi(sql_row[0]),atoi(sql_row[1]),atoi(sql_row[2])
	      ,atoi(sql_row[3]),atoi(sql_row[4]),atoi(sql_row[5])
	      ,sql_row[6],sql_row[7],sql_row[8]); 

      //int did = atoi(sql_row[1]);
      toc_array[ii].id = (sql_row[0]) ? atoi(sql_row[0]) : 0;
      toc_array[ii].pid = (sql_row[2]) ? atoi(sql_row[2]) : 0;
      toc_array[ii].epid = (sql_row[9]) ? atoi(sql_row[9]) : 0;      
      toc_array[ii].lev = (sql_row[3]) ? atoi(sql_row[3]) : 0;
      toc_array[ii].item_id = (sql_row[4]) ? atoi(sql_row[4]) :0 ;
      toc_array[ii].my_enum = (sql_row[5]) ? atoi(sql_row[5]) : 0;
      toc_array[ii].section = (sql_row[6]) ? strdup(sql_row[6]) : "";
      toc_array[ii].title = (sql_row[7]) ? strdup(sql_row[7]) : "" ;
      toc_array[ii].header = (sql_row[8]) ? strdup(sql_row[8]) : "";
      //fprintf(stderr,"TOC_NO=%d: id=%d: pid=%d: hd=%s: sec=%s: lev=%d:\n",ii,toc_array[ii].id,toc_array[ii].pid,toc_array[ii].header,toc_array[ii].section,toc_array[ii].lev);
      ii++;
      toc_no_array = ii;
      toc_no = ii;      
    }
  }
  return ii;
} // get_toc_item_from_sql()

int print_toc_array() {
  int ii;
  if (debug) fprintf(stderr,"TOC:%d:\n",toc_no_array);
  for (ii = 0; ii < toc_no_array; ii++) {
    if (debug) fprintf(stderr,"II=%d: ID=%d: PID=%d-%d: LEV=%d: sec=%s: tit=%s: hdr=%s: special=%d:\n",
		       ii,
		       toc_array[ii].id,
		       toc_array[ii].pid,
		       toc_array[ii].epid,		       
		       toc_array[ii].lev,
		       toc_array[ii].section,
		       toc_array[ii].title,
		       toc_array[ii].header,
		       toc_array[ii].special		       
	    );
  }
  return 0;
}


char *rem_tag_only(char *text) { // remove tags 

  static char bext[5000];
  int ii, jj, in_tag;
  for (jj = 0,ii = 0,in_tag = 0; ii < strlen(text); ii++) {
    if (text[ii] == '<') {
      in_tag = 1;
    } else if (text[ii] == '>') {
      in_tag = 0;
    } else if (in_tag == 0) {
      if (text[ii] == '\n') {
	bext[jj++] = ' ';
      } else {
	bext[jj++] = text[ii];
      }
    }
  }
  bext[jj++] = '\0';
  return(bext);
} // rem_tag_only()

char *last_strstr(const char *haystack, const char *needle)  {
  if (*needle == '\0')
    return (char *) haystack;

  char *result = NULL;
  for (;;) {
    char *p = strstr(haystack, needle);
    if (p == NULL)
      break;
    result = p;
    haystack = p + 1;
   }

   return result;
} // last_strstr

int insert_instance_into_array_with_pid(char *my_text, char *label_name, int pid) {
  printf("%c<ZZ name=\"%s\">%s</ZZ>",my_text[0],label_name,my_text+1);

  instance_array[label_no].pid = pid;
  instance_array[label_no].text = strdup(rem_tag_only(my_text));
  instance_array[label_no].span_no = -1;
  instance_array[label_no].last_span_no = -1;
  instance_array[label_no].label_name = strdup(label_name);
  label_no++;
  return label_no;
}


char *under_score2camelBack(char *text) {
  int ii, jj;
  static char bext[200];
  int found_us = 0;
  for (ii = 0, jj = 0; ii < strlen(text); ii++) {
    if (text[ii] == '_') { // UZ removed after discussion
      found_us = 1;
    } else {
      bext[jj++] = (found_us == 1) ? toupper(text[ii]) : text[ii];
      found_us = 0;
    }
  }
  bext[jj++] = '\0';
  //fprintf(stderr,"MMMMMMMMMMMMM:%s:%s:\n",bext,text);
  return bext;
}

int insert_instance_into_array_with_atype(char *my_text, char *label_name, int span_no, int atype) {
  char *my_label_name = under_score2camelBack(label_name);
  printf("%c<ZZ name=\"%s\">%s</ZZ>",my_text[0],my_label_name,my_text+1);
  char *pp = last_strstr(my_text,"=sp_"); // the last span
  int last_span_no = span_no;
  if (pp) {
    last_span_no = atoi(pp+4); 
  }

  instance_array[label_no].atype = atype;
  if (atype == 0) { // INT
    instance_array[label_no].norm_int = identify_convert_txt_to_num(my_text, &(instance_array[label_no].no_int));
  }
  instance_array[label_no].text = strdup(rem_tag_only(my_text));
  instance_array[label_no].span_no = span_no;
  instance_array[label_no].pid = tok2para_array[span_no]; // this element is filled in only if convert_para2tok_to_tok2para() was called from main()
  instance_array[label_no].last_span_no = last_span_no;
  instance_array[label_no].label_name = strdup(my_label_name);

  label_no++;
  return label_no;
} // insert_instance_into_array_with_atype

int insert_instance_into_array_with_atype_and_norm_val(char *my_text, char *label_name, int span_no, int atype, char *val) {
  char *my_label_name = under_score2camelBack(label_name);
  printf("%c<ZZ name=\"%s\">%s</ZZ>",my_text[0],my_label_name,my_text+1);
  char *pp = last_strstr(my_text,"=sp_"); // the last span
  int last_span_no = span_no;
  if (pp) {
    last_span_no = atoi(pp+4);
  }

  instance_array[label_no].atype = atype;
  if (atype == 0) { // INT
    instance_array[label_no].norm_int = atoi(val);
  }
  if (atype == 1) { // TEXT
    instance_array[label_no].norm_text = val;
  }
  instance_array[label_no].text = strdup(rem_tag_only(my_text));
  instance_array[label_no].span_no = span_no;
  instance_array[label_no].pid = tok2para_array[span_no]; // this element is filled in only if convert_para2tok_to_tok2para() was called from main()
  instance_array[label_no].last_span_no = last_span_no;
  instance_array[label_no].label_name = strdup(my_label_name);

  label_no++;
  return label_no;
} // insert_instance_into_array_with_atype


int insert_double_instance_into_array_with_atype(char *my_text, char *label_name, int span_no, int atype, int last_span_no) {
  char *my_label_name = under_score2camelBack(label_name);
  printf("%c<ZZ name=\"%s\">%s</ZZ>",my_text[0],my_label_name,my_text+1);
  instance_array[label_no].atype = atype;
  if (atype == 0) { // INT
    instance_array[label_no].norm_int = identify_convert_txt_to_num(my_text, &(instance_array[label_no].no_int));
  }
  instance_array[label_no].text = strdup(rem_tag_only(my_text));
  instance_array[label_no].span_no = span_no;
  instance_array[label_no].pid = tok2para_array[span_no]; // this element is filled in only if convert_para2tok_to_tok2para() was called from main()
  instance_array[label_no].last_span_no = last_span_no;
  instance_array[label_no].label_name = strdup(my_label_name);

  label_no++;
  return label_no;
} // insert_double_instance_into_array_with_atype

int insert_instance_into_array(char *my_text, char *label_name, int span_no) {
  char *my_label_name = under_score2camelBack(label_name);
  printf("%c<ZZ name=\"%s\">%s</ZZ>",my_text[0],my_label_name,my_text+1);
  char *pp = last_strstr(my_text,"=sp_"); // the last span
  int last_span_no = span_no;
  if (pp) {
    last_span_no = atoi(pp+4); 
  }

  instance_array[label_no].text = strdup(rem_tag_only(my_text));
  instance_array[label_no].span_no = span_no;
  instance_array[label_no].pid = tok2para_array[span_no]; // this element is filled in only if convert_para2tok_to_tok2para() was called from main()
  //fprintf(stderr,"HHHH: pid=%d: label_no=%d: span_no=%d:\n",instance_array[label_no].pid, label_no, span_no);
  instance_array[label_no].last_span_no = last_span_no;
  instance_array[label_no].label_name = strdup(my_label_name);

  label_no++;
  return label_no;
} // insert_instance_into_array


int insert_instance_into_array_and_length(char *my_text, char *label_name, int span_no, int length) {
  char *my_label_name = under_score2camelBack(label_name);
  printf("%c<ZZ name=\"%s\">%s</ZZ>",my_text[0],my_label_name,my_text+1);

  int last_span_no = span_no + length-1;
  instance_array[label_no].text = strdup(rem_tag_only(my_text));
  instance_array[label_no].span_no = span_no;
  instance_array[label_no].pid = tok2para_array[span_no]; // this element is filled in only if convert_para2tok_to_tok2para() was called from main()
  //fprintf(stderr,"HHHH: pid=%d: label_no=%d: span_no=%d:\n",instance_array[label_no].pid, label_no, span_no);
  instance_array[label_no].last_span_no = last_span_no;
  instance_array[label_no].label_name = strdup(my_label_name);

  label_no++;
  return label_no;
} // insert_instance_into_array

int insert_double_instance_into_array(char *my_text, char *label_name, int first_span_no, int last_span_no) {
  char *my_label_name = under_score2camelBack(label_name);
  printf("%c<ZZ name=\"%s\">%s</ZZ>",my_text[0],my_label_name,my_text+1);

  instance_array[label_no].text = strdup(rem_tag_only(my_text));
  instance_array[label_no].span_no = first_span_no;
  instance_array[label_no].pid = tok2para_array[span_no]; // this element is filled in only if convert_para2tok_to_tok2para() was called from main()
  instance_array[label_no].last_span_no = last_span_no;
  instance_array[label_no].label_name = strdup(my_label_name);

  label_no++;
  return label_no;
} // insert_instance_into_array


int print_instance_array(int label_no) {
  int ii;
  if (debug) fprintf(stderr,"Printing Instance_Array:%d:\n",label_no);
  for (ii = 0; ii < label_no; ii++) {
    int my_pid = tok2para_array[instance_array[ii].span_no];
    if (debug) fprintf(stderr,"INSTANCE1:ii=%d: pid=%d:%d: sn=%d: li=%d: ln=%s: score=%d: fspan_no=%d: lspan_no=%d: text=%s:ty=%d: ni=%d: nf=%6.2f:\n",
	    ii,
	    my_pid ,
	    instance_array[ii].pid,
	    instance_array[ii].span_no,
	    instance_array[ii].label_id,
	    instance_array[ii].label_name,
	    instance_array[ii].score,
	    instance_array[ii].span_no,
	    instance_array[ii].last_span_no,
            instance_array[ii].text,
            instance_array[ii].atype,
	    instance_array[ii].norm_int,
	    instance_array[ii].norm_float
	    );
  }
  return label_no;
}

int no_of_ws(char *text) {
  int ret = 0;
  int jj;
  for (jj = 0; jj < strlen(text); jj++) {
    if (text[jj] == '<' && text[jj+1] == 's') {
      ret++;
    }
  }
  return(ret);
} //


int print_tok2para(int last_tok) {
  fprintf(stderr,"PRINTING T2PA:%d:\n",last_tok);
  int ii;
  for (ii = 0; ii < last_tok; ii++) {
    fprintf(stderr,"T2PA:%d:%d:\n",ii,tok2para_array[ii]);
  }
  return 0;
}

float rem_non_float(char *text) {
  int ii, jj;
  char bext[5000];
  for (ii = 0, jj = 0; ii < strlen(text); ii++) {
    if (isdigit(text[ii]) != 0 || text[ii] == '.') {
      bext[jj++] = text[ii];
    }
  }
  bext[jj++] = '\0';
  float num = atof(bext);
  return num;
}

#define FLOAT 2
int insert_instance_into_array_for_price(char *my_text, char *label_name, int span_no, int atype, char *source_prog) {
  char *my_label_name = label_name;
  char *pp = last_strstr(my_text,"=sp_"); // the last span
  int last_span_no = span_no;
  if (pp) {
    last_span_no = atoi(pp+4); 
  }

  instance_array[label_no].atype = atype;
  instance_array[label_no].norm_int = identify_convert_txt_to_num(my_text, &(instance_array[label_no].no_int));
  instance_array[label_no].norm_float = rem_non_float(my_text+2);
  instance_array[label_no].text = strdup(rem_tag_only(my_text));
  instance_array[label_no].norm_text = strdup(rem_tag_only(my_text+1));
  instance_array[label_no].span_no = span_no;
  instance_array[label_no].atype = FLOAT;  
  instance_array[label_no].pid = tok2para_array[span_no]; // this element is filled in only if convert_para2tok_to_tok2para() was called from main()
  instance_array[label_no].last_span_no = last_span_no;
  instance_array[label_no].label_name = strdup(my_label_name);
  //  instance_array[label_no].source_program = "my_price"; //source_prog;  

  printf("%c<ZZ name=\"%s\" norm=\"%.2f\" atype=\"2\" type=\"float\" text=\"%s\">%s</ZZ>", my_text[0], my_label_name, instance_array[label_no].norm_float, my_text+1, my_text+1);

  label_no++;

  return label_no;
} // insert_instance_into_array_for_price
