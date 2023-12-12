%x in_para in_para_start
%{
  /*
    1.  retrive preamble labels
    2.  sync_up (num_labels=%d)
    3.  scan_recitals and generate edit list, decide if legit preamble
    4.  create linked list so can add items in the middle
    5.  edit linked list
    CALL: (../bin/get_preamble 0 3968 ../tmp/Index/index_file_3968 localhost dealthing root imaof3 1 < ../tmp/Xxx/aaa7) >& ttt
    CALL:  (../../../dealthing/dtcstuff/bin/get_preamble 0 100 ../../../dealthing/dtcstuff/tmp/Index/index_file_100 localhost dealthing root imaof3 1 < ../../../dealthing/dtcstuff/tmp/Xxx/aaa7) >& ttt

  */

  #include <ctype.h>
  #include <mysql.h>
  #include "entity_functions.h"
  #include "recitals_functions.h"
  FILE *index_file;

  char *db_IP, *db_name, *db_pwd, *db_user_name; // for DB config

  /******************************/
  int debug;
  int do_print;
  int org_id[1];
  char *prog;

  #define MAX_FILE 50
  #define MAX_HEADER 1500
  int max_file_no = 0;
  int max_item_no[MAX_FILE];

  /****************************/

  /************** SQL ********************/
  //  char *server = "localhost";
  /*
  char *server = db_IP; // "54.241.17.226";
  char *user = db_name; // "root";
  char *password = db_pwd; // "imaof3";
  char *database = db_user_name; //"dealthing";
  */
  MYSQL *conn;
  MYSQL_RES *res;
  MYSQL_ROW row;
  /************** END SQL ****************/

  // in reality we do not exceed 1000 in the buffer
#define MAX_VPARA 500 
#define VPARA_SIZE 50000
#define VPARA_SIZE2 100001
  char vpara[VPARA_SIZE];
  char chunk[VPARA_SIZE2]; // store the escaped para
  int doc_id; // input paqram
  int deal_id, oid; // from sql_params

  int pream_no = 0; // how many preams
  int sigblock_no = 0; // how many preams
  int rider_no = 0; // how many preams
  int table_of_contents_no = 0; // how many preams
  int recitals_no = 0; // how many preams

  int pid = 0;
  char *make_norm_vpara(char *text);

  int sql_region(char vpara[], int doc_id, char *rtype, int score, char *recitals_feature) { // rtype 0 NA, 1 now_therefore (the beginning of the bus part of the doc
    return 0;
    #define INTEGER 5
    #define TEXT 3
    #define COMPLEX 2

    int complex_id_name[1];

    int complex_id = insert_complex_field(conn, doc_id, deal_id, org_id[0], pid, rtype, complex_id_name,score,2,rtype,1);

    char chunk[1000];
    mysql_real_escape_string(conn,chunk,vpara,strlen(vpara));

    static char rtype_field[60];
    sprintf(rtype_field,"%s_field",rtype);
    //insert_text_field(conn,doc_id, deal_id, org_id[0],pid, complex_id, complex_id_name[0], rtype_field, chunk, 1, TEXT, score);

    int fid = insert_any_field(conn,"text",doc_id, deal_id, org_id[0],pid, complex_id, complex_id_name[0], rtype_field, "NA", chunk, 1, score,"text");

    if (recitals_feature && strlen(recitals_feature)) {
      insert_recitals_feature(conn, recitals_feature, fid, doc_id, deal_id, org_id[0]);
    }

    return 0;
  } // sql_region()

  int print_vpara() {
    char *norm_vpara = make_norm_vpara(vpara);
    static char pream_words[2000];
    static char sigblock_words[2000];
    static char rider_words[2000];
    static char table_of_contents_words[2000];
    static char recitals_words[2000];

    int preamble_score = 0;
    int sigblock_score = 0;
    int rider_score = 0;
    int table_of_contents_score = 0;
    int recitals_score = 0;

    /*********************************  PREAMBLE ****************************************/
    strcpy(pream_words,"");
    if (strcasestr(norm_vpara,"made") ) {
      strcat(pream_words,"made, ");
      preamble_score++;
    }

    if (strcasestr(norm_vpara,"entered") ) {
      strcat(pream_words,"entered, ");
      preamble_score++;
    }

    if (strcasestr(norm_vpara,"entered into") ) {
      strcat(pream_words,"entered into, ");
      preamble_score += 2;
    }

    if (strcasestr(norm_vpara,"having an office") ) {
      strcat(pream_words,"having an office, ");
      preamble_score += 2;
    }

    if (strcasestr(norm_vpara,"with offices at") ) {
      strcat(pream_words,"with offices at, ");
      preamble_score += 2;
    }

    if (strcasestr(norm_vpara,"with principal offices at") ) {
      strcat(pream_words,"with principal offices at, ");
      preamble_score += 2;
    }

    if (strcasestr(norm_vpara,"principal place of business") ) {
      strcat(pream_words,"principal place of business, ");
      preamble_score += 2;
    }

    if (strcasestr(norm_vpara,"located at") ) {
      strcat(pream_words,"located at, ");
      preamble_score += 2;
    }

    if (strcasestr(norm_vpara,"all capitalized") ) {
      strcat(pream_words,"all capitalized, ");
      preamble_score += 2;
    }

    if (strcasestr(norm_vpara,"Amendment") ) {
      strcat(pream_words,"Amendment, ");
      preamble_score += 1;
    } 

    if (strcasestr(norm_vpara,"Addendum") ) {
      strcat(pream_words,"Addendum, ");
      preamble_score += 1;
    } 

    if (strcasestr(norm_vpara,"amends") ) {
      strcat(pream_words,"amends, ");
      preamble_score += 1;
    } 

    if (strstr(norm_vpara,"Effective Date") ) {
      strcat(pream_words,"Effective Date, ");
      preamble_score += 2;
    } else if (strcasestr(norm_vpara,"Effective Date") ) {
      strcat(pream_words,"Effective Date, ");
      preamble_score += 1;
    }

    if (strcasestr(norm_vpara,"incorporated under") ) {
      strcat(pream_words,"incorporated under, ");
      preamble_score += 1;
    }

    if (strcasestr(norm_vpara,"limited liability company") ) {
      strcat(pream_words,"limited liability company, ");
      preamble_score += 1;
    }

    if (strcasestr(norm_vpara,"LLC") ) {
      strcat(pream_words,"LLC, ");
      preamble_score += 1;
    }

    if (strstr(norm_vpara,"INC") ) {
      strcat(pream_words,"INC, ");
      preamble_score += 1;
    } else if (strstr(norm_vpara,"Inc") ) {
      strcat(pream_words,"Inc, ");
      preamble_score += 1;
    }


    if (strcasestr(norm_vpara,"collectively") ) {
      strcat(pream_words,"collectively, ");
      preamble_score += 1;
    }

    if (strcasestr(norm_vpara,"day of ") ) {
      strcat(pream_words,"day of, ");
      preamble_score += 1;
    }

    if (strcasestr(norm_vpara,"corporation") ) {
      strcat(pream_words,"corporation, ");
      preamble_score += 1;
    }

    if (strcasestr(norm_vpara,"hereinafter") ) {
      strcat(pream_words,"hereinafter, ");
      preamble_score += 1;
    }

    if (strcasestr(norm_vpara,"behalf") ) {
      strcat(pream_words,"behalf, ");
      preamble_score += 2;
    }

    if (strcasestr(norm_vpara,"subsidiaries") ) {
      strcat(pream_words,"subsidiaries, ");
      preamble_score += 2;
    }

    if (strcasestr(norm_vpara,"affiliates") ) {
      strcat(pream_words,"affiliates, ");
      preamble_score += 2;
    }

    if (strcasestr(norm_vpara,"agree as follows") ) {
      strcat(pream_words,"agree as follows, ");
      preamble_score += 2;
    }

    if (strcasestr(norm_vpara,"hereby agree as follows") ) {
      strcat(pream_words,"hereby agree as follows, ");
      preamble_score += 4;
    }

    if (strcasestr(norm_vpara,"between") ) {
      strcat(pream_words,"between, ");
      preamble_score++;
    }

    if (strcasestr(norm_vpara,"herein") ) {
      strcat(pream_words,"herein, ");
      preamble_score++;
    }

    if (strcasestr(norm_vpara,"dated") ) {
      strcat(pream_words,"dated, ");
      preamble_score++;
    }
    if (strcasestr(norm_vpara,"among") ) {
      strcat(pream_words,"among, ");
      preamble_score++;
    }
    if (strcasestr(norm_vpara,"agreement")/* ||strcasestr(norm_vpara,"lease") || strcasestr(norm_vpara,"rider") || strcasestr(norm_vpara,"indenture")  */) {
      strcat(pream_words,"agreement, ");
      preamble_score++;
    }
    if (strcasestr(norm_vpara,"amendment") || strcasestr(norm_vpara,"addendum") ) {
      strcat(pream_words,"amendment, ");
      preamble_score++;
    }

    if (strcasestr(norm_vpara," by and between") ) {
      strcat(pream_words,"by and between, ");
      preamble_score += 2;
    }

    if (strcasestr(norm_vpara,"entered into") ) {
      strcat(pream_words,"entered into, ");
      preamble_score += 2;
    }

    if (strcasestr(norm_vpara," as of ") ) {
      strcat(pream_words,"as of, ");
      preamble_score++;
    }

    if (strcasestr(norm_vpara," effective ") ) {
      strcat(pream_words,"effective, ");
      preamble_score++;
    }

    if (strcasestr(norm_vpara,"made") ) {
      strcat(pream_words,"made, ");
      preamble_score++;
    }

    if (strstr(norm_vpara,"This ") ) {
      strcat(pream_words,"This, ");
      preamble_score += 2;
    }

    if (strstr(norm_vpara,"THIS ") ) {
      strcat(pream_words,"THIS, ");
      preamble_score += 2;
    }

    if (strcasestr(norm_vpara,"letter of intent") ) {
      strcat(pream_words,"letter of intent, ");
      preamble_score++;
    }

    if (strcasestr(norm_vpara,"statement of work") ) {
      strcat(pream_words,"statement of work, ");
      preamble_score++;
    }

    if (strcasestr(norm_vpara,"statement of services") ) {
      strcat(pream_words,"statement of services, ");
      preamble_score++;
    }

    if (strcasestr(norm_vpara,"sow") ) {
      strcat(pream_words,"sow, ");
      preamble_score++;
    }

    if (strstr(norm_vpara,"In connection") ) {
      strcat(pream_words,"In connection, ");
      preamble_score += 2;
    }

    if (strstr(norm_vpara,"In consideration") ) {
      strcat(pream_words,"In consideration, ");
      preamble_score += 2;
    }

    /***********************************************/

    if (strcasestr(norm_vpara,"sos") ) {
      strcat(pream_words,"sos, ");
      preamble_score++;
    }

    if (strcasestr(norm_vpara,"Miscellaneous") ) {
      strcat(pream_words,"*miscellaneous*, ");
      preamble_score -=2;
    }

    if (strcasestr(norm_vpara,"Termination") ) {
      strcat(pream_words,"*termination*, ");
      preamble_score -=2;
    }

    if (strcasestr(norm_vpara,"MISCELLANEOUS") ) {
      strcat(pream_words,"*miscellaneous*, ");
      preamble_score -= 6;
    }

    if (strcasestr(norm_vpara,"governing law") ) {
      strcat(pream_words,"*governing law*, ");
      preamble_score -= 3;
    }

    if (strcasestr(norm_vpara,"definitive agreement") ) {
      strcat(pream_words,"*definitive agreement*, ");
      preamble_score -= 3;
    }

    if (strcasestr(norm_vpara,"jurisdiction") ) {
      strcat(pream_words,"*jurisdiction*, ");
      preamble_score -= 3;
    }

    if (strcasestr(norm_vpara,"arbitration") ) {
      strcat(pream_words,"*arbitration*, ");
      preamble_score -= 3;
    }

    if (strcasestr(norm_vpara,"construed") ) {
      strcat(pream_words,"*construed*, ");
      preamble_score -= 2;
    }

    if (strcasestr(norm_vpara,"shall mean") || strcasestr(norm_vpara,"means") ) {
      strcat(pream_words,"*shall mean*, ");
      preamble_score -= 2;
    }

    if (strcasestr(norm_vpara,"governed") ) {
      strcat(pream_words,"*governed*, ");
      preamble_score -= 2;
    }

    if (strcasestr(norm_vpara,"controlled") ) {
      strcat(pream_words,"*controlled*, ");
      preamble_score -= 2;
    }

    if (strcasestr(norm_vpara,"no agency") ) {
      strcat(pream_words,"*governed*, ");
      preamble_score -= 2;
    }

    if (strcasestr(norm_vpara,"entire agreement") ) {
      strcat(pream_words,"*entire agreement*, ");
      preamble_score -= 2;
    }

    if (strstr(norm_vpara,"WHEREAS")||strstr(norm_vpara,"Whereas") ) {
      preamble_score -= 10;
    }

    if (strstr(norm_vpara,"THEREFORE")) {
      preamble_score -= 10;
    }

    //if (debug) fprintf(stderr,"PREAM33: i=%d: p=%d: s=%d: l=%d: w=%s:\n  p=%s:BBB\n", pream_no,pid,preamble_score,strlen(norm_vpara),pream_words,vpara);
    if (preamble_score > 2 
	&& no_good_array[pid] == 0 // what is this??
	&& pream_no < 25) {  // don't give me too many preams
      if (debug) fprintf(stderr,"PREAM5: i=%d:  no_good=%d: p=%d: score=%d: l=%d: w=%s:\n  p=%s:BBB\n", pream_no,no_good_array[pid],pid,preamble_score,strlen(norm_vpara),pream_words,vpara);
      pream_no++;
      sql_region(vpara, doc_id, "Preamble", preamble_score,NULL);
    } else {
      if (debug) fprintf(stderr,"NO PREAM5: i=%d:  no_good=%d: p=%d: score=%d: l=%d: w=%s:\n  p=%s:BBB\n", pream_no,no_good_array[pid], pid,preamble_score,strlen(norm_vpara),pream_words,vpara);
    }
    strcpy(pream_words,"");

    /*********************************  SIGBLOCK ****************************************/
    if (strcasestr(norm_vpara,"in witness whereof") ) {
      strcat(sigblock_words,"in witness whereof, ");
      sigblock_score += 10;
    }

    if (strcasestr(norm_vpara,"signatures follow") || (strcasestr(norm_vpara,"signatures") && strcasestr(norm_vpara,"follow") && strcasestr(norm_vpara,"page"))) {
      sigblock_score += 10;
    }

    if (strcasestr(norm_vpara,"AGREED AND ACCEPTED") || (strcasestr(norm_vpara,"ACCEPTED") && strcasestr(norm_vpara,"AGREED"))) {
      strcat(sigblock_words,"agreed, ");
      sigblock_score += 10;
    }

    if (strstr(norm_vpara,"Whereof") || strstr(norm_vpara,"WHEREOF") ) {
      strcat(sigblock_words,"whereof, ");
      sigblock_score += 10;
    }

    if (strcasestr(norm_vpara,"Witnesseth") == NULL && (strstr(norm_vpara,"Witness") || strstr(norm_vpara,"WITNESS"))) {
      strcat(sigblock_words,"witness, ");
      sigblock_score += 10;
    }

    if (strcasestr(norm_vpara,"AGREED AND ACCEPTED") || (strcasestr(norm_vpara,"ACCEPTED") && strcasestr(norm_vpara,"AGREED"))) {
      strcat(sigblock_words,"agreed, ");
      sigblock_score += 10;
    }

    if (sigblock_score > 5 
	&& no_good_array[pid] == 0
	&& sigblock_no < 5) { 
      if (debug) fprintf(stderr,"SIGBLOCK:  i=%d: p=%d: c=%d: l=%d: w=%s:\n  p=%s\n", sigblock_no,pid,sigblock_score,strlen(norm_vpara),sigblock_words,vpara);
      sigblock_no++;
      mysql_real_escape_string(conn,chunk,vpara,strlen(vpara)); // we might be running it twice for the same para.  No big deal
      sql_region(vpara, doc_id, "Sigblock", sigblock_score,NULL);
    }

    /*********************************  RIDER ****************************************/
    if (strstr(norm_vpara,"RIDER")) {
      strcat(rider_words,"Rider, ");
      rider_score += 10;
    }

    if (rider_score > 5 
	&& no_good_array[pid] == 0
	&& rider_no < 5) { 
      if (debug) fprintf(stderr,"RIDER:  i=%d: p=%d: c=%d: l=%d: w=%s:\n  p=%s\n", rider_no,pid,rider_score,strlen(norm_vpara),rider_words,vpara);
      rider_no++;
      mysql_real_escape_string(conn,chunk,vpara,strlen(vpara)); // we might be running it twice for the same para.  No big deal
      sql_region(vpara, doc_id, "Rider", rider_score,NULL);
    }


    /*********************************  TABLE_OF_CONTENTS ****************************************/

    if ((strstr(norm_vpara,"TABLE") && strstr(norm_vpara,"CONTENTS")) ||  strcasestr(norm_vpara,"TABLE OF CONTENTS")) {
      strcat(table_of_contents_words,"table_of_contents, ");
      table_of_contents_score = 10;
    }

    if (strstr(norm_vpara,"INDEX")) {
      strcat(table_of_contents_words,"index, ");
      table_of_contents_score += 15;
    }

    if (strstr(norm_vpara,"EXHIBITS")) {
      strcat(table_of_contents_words,"EXHIBITS, ");
      table_of_contents_score += 15;
    }

    if (strcasestr(norm_vpara,"LIST OF EXHIBITS")) {
      strcat(table_of_contents_words,"list_of_exhibits, ");
      table_of_contents_score += 15;
    }
    if (table_of_contents_score > 5 
	&& no_good_array[pid] == 0
	&& table_of_contents_no < 5) { 
      if (debug) fprintf(stderr,"TABLE_OF_CONTENTS:  i=%d: p=%d: c=%d: l=%d: w=%s:\n  p=%s\n", table_of_contents_no,pid,table_of_contents_score,strlen(norm_vpara),table_of_contents_words,vpara);
      table_of_contents_no++;
      mysql_real_escape_string(conn,chunk,vpara,strlen(vpara)); // we might be running it twice for the same para.  No big deal
      sql_region(vpara, doc_id, "Table_Of_Contents", table_of_contents_score,NULL);
    }
    /*********************************  RECITALS ****************************************/
    char *recitals_feature = NULL;
    if (0 &&(strcasestr(norm_vpara,"WITNESSETH") || strcasestr(norm_vpara,"W I T N E S S E T H") )) { // dropped since witnesses alone causes problem in DOLLAR GEN LSE
      strcat(recitals_words,"WITNESSETH, ");
      recitals_score += 10;
      recitals_feature = "WITNESSETH";;
    }

    if (strcasestr(norm_vpara,"whereas") || strcasestr(norm_vpara,"w h e r e a s") ) {
      strcat(recitals_words,"whereas, ");
      recitals_score += 10;
      recitals_feature = "WHEREAS";;
    }


    if (strcasestr(norm_vpara,"FOR AND IN CONSIDERATION") || strcasestr(norm_vpara,"IN CONSIDERATION") ) {
      strcat(recitals_words,"consideration, ");
      recitals_score += 5;
      recitals_feature = "CONSIDERATION";;
    }

    if (strcasestr(norm_vpara,"the parties agree as follows") ) {
      strcat(recitals_words,"the parties agree as follows, ");
      recitals_score += 10;
      recitals_feature = "AGREE";
    } else if (strcasestr(norm_vpara,"hereby agree as follows") ) {
      strcat(recitals_words,"hereby agree as follows, ");
      recitals_score += 10;
      recitals_feature = "AGREE";
    } else if (strcasestr(norm_vpara," agree as follows") ) {
      strcat(recitals_words," agree as follows, ");
      recitals_score += 5;
      recitals_feature = "AGREE";
    }

    char *pp1 = strcasestr(norm_vpara,"NOW");
    char *pp2;
    if (pp1) {
      pp2 = strcasestr(norm_vpara,"THEREFORE");
    }
    if (pp1 && pp2) {
      strcat(recitals_words,"now therefore, ");
      recitals_score += 10;
      recitals_feature = "NOW_THEREFORE";
    }

    if (strcasestr(norm_vpara,"Recitals") || strcasestr(norm_vpara,"R e c i t a l s") ) {
      strcat(recitals_words,"recitals, ");
      recitals_score += 10;
      recitals_feature = "RECITALS";
    }

    if (strstr(norm_vpara,"BACKGROUND") || strstr(norm_vpara,"Background") ) {
      strcat(recitals_words,"background, ");
      recitals_score += 10;
      recitals_feature = "BACKGROUND";
    }

    if (recitals_score > 5 
	&& no_good_array[pid] == 0
	&& recitals_no < 100) { 
      if (debug) fprintf(stderr,"RECITALS: i=%d: p=%d: s=%d: l=%d: w=%s:\n  p=%s\n", recitals_no,pid,recitals_score,strlen(norm_vpara),recitals_words,vpara);
      recitals_no++;
      mysql_real_escape_string(conn,chunk,vpara,strlen(vpara));
      sql_region(vpara, doc_id, "Recitals", recitals_score,recitals_feature);
    }

    strcpy(recitals_words,"");
    /****************************************************************************/
    strcpy(vpara,"");
    return 0;
  } //print_vpara()
%}

epp \<\/[Pp][^\>]*\>
pp \<[Pp][^\>]*\>
tag \<[^\>]+\>
elem \&[^\;]{1,7}\;
%%
<*>  ;

{pp} {
  BEGIN in_para_start; strcpy(vpara,"");
  
  char *pp = strstr(yytext,"sn=\"");
  if (pp) {
    pid = atoi(pp+4);
  } else {
    pp = strstr(yytext,"id=\"");
    if (pp) {
      pid = atoi(pp+4);
    }
  }
}


<in_para_start>. {
  BEGIN in_para;
  yyless(0);
 }

<in_para>. if (strlen(vpara) < MAX_VPARA) strcat(vpara,yytext);

<in_para>\n if (strlen(vpara) < MAX_VPARA) strcat(vpara," ");

<in_para>{pp}|{epp} { 
  BEGIN 0; yyless(0); print_vpara();
}

<in_para>{tag}  if (strlen(vpara) < MAX_VPARA) strcat(vpara," ");
<in_para>{elem} if (strlen(vpara) < MAX_VPARA) strcat(vpara," ");
<*>.|\n  ;
%%

char *make_norm_vpara(char text[]) {
  int ii;
  int jj;
  static char bext[20000];
  int first = 1; // first non_alnum
  for (jj = 0, ii = 0; ii < strlen(text); ii++) {
    if (isalnum(text[ii])) {
      bext[jj++] = text[ii];
      first = 1;
    } else {
      if (first == 1) {
	bext[jj++] = ' ';
	first = 0;
      } else {
	;
      }
    }
  } // for
  bext[jj++] = '\0';
  return bext;
}

int doc_name_abstract() {
  MYSQL_RES *sql_res;
  MYSQL_ROW sql_row;

  static char query[500];
  char *doc_name = NULL;
  sprintf(query,"select name \n\
                     from deals_document \n	\
                     where \n\
                       id = '%d' ",
	  doc_id
	  );

  if (debug) fprintf(stderr,"QUERY00 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY00 (%s): query=%s:\n",prog,query);
  }
  
  sql_res = mysql_store_result(conn);
  int nn = mysql_num_rows(sql_res);
  if (nn > 0) {
    sql_row = mysql_fetch_row(sql_res);
    doc_name = sql_row[0];
  }
  int is_abstract = strcasestr(doc_name,"abstract") ? 1 : 0;
  int is_finance = (strstr(doc_name,"CCR")
		    || strcasestr(doc_name,"recon"))
		    ? 1 : 0;

  // S/B UPDATE instead of REPLACE/ AVI is setting docinfo earlier
  sprintf(query,"select * \n\
                     from deals_docinfo \n\
                     where \n\
                       doc_id = '%d' ",
	  doc_id
	  );

  if (debug) fprintf(stderr,"QUERY01 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY01 (%s): query=%s:\n",prog,query);
  }

  sql_res = mysql_store_result(conn);
  nn = mysql_num_rows(sql_res);
  if (0) {
  if (nn != 0) {
    sprintf(query,"replace \n\
                     into deals_docinfo \n\
                     set \n\
                       organization_id = '%d' \n\
                       , is_abstract = '%d' \n\
                       , is_finance = '%d' \n\
                       , doc_id = '%d' "
	  , org_id[0], is_abstract, is_finance, doc_id);

    if (debug) fprintf(stderr,"QUERY02 (%s): query=%s:\n",prog,query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY02 (%s): query=%s:\n",prog,query);
    }

    fprintf(stderr,"Error: DOCINFO not found for doc=%d: \n",doc_id);
  } else {
    sprintf(query,"update \n\
                     deals_docinfo \n\
                     set \n\
                       organization_id = '%d' \n\
                       , is_abstract = '%d' \n\
                       , is_finance = '%d' \n\
                       , last_update_date = now() \n\
	             where \n\
                       doc_id = '%d' "
	  , org_id[0], is_abstract, is_finance, doc_id);

    if (debug) fprintf(stderr,"QUERY03 (%s): query=%s:\n",prog,query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY03 (%s): query=%s:\n",prog,query);
    }
  }
  }

  return 0;
}


int main(int ac, char **av) {
  prog = av[0];
  do_print = 0;
  if (ac != 9) {
    fprintf(stderr,"Error: %s takes (8) print_out, doc_id, index_file, db_IP, db_name, db_user_name, db_pwd, debug (got %d params)\n",av[0],ac);
    exit(0);
  }
  fprintf(stderr,"Info: %s got po=%s: doc_id:%s: index_name=%s: IP=%s: name=%s: user_name=%s: pwd=%s: debug=%s:\n",av[0],av[1],av[2],av[3],av[4],av[5],av[6], av[7], av[8]);
  do_print= atoi(av[1]);
  doc_id = atoi(av[2]);
  index_name = av[3];
  db_IP = av[4];
  db_name = av[5];
  db_user_name = av[6]; // for DB config
  db_pwd = av[7];
  debug_entity = my_debug = debug = atoi(av[8]);
  conn = mysql_init(NULL);
  //debug_entity = my_debug = debug = 0; // for now
  /* now connect to database */
  if (!mysql_real_connect(conn,db_IP,db_user_name,db_pwd,db_name,0,NULL,0)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }

  deal_id = get_did_oid(conn,doc_id, org_id);

  doc_name_abstract(); // add doc properties based on doc name

  delete_recitals_feature(conn, doc_id); // delete recitals_feature which extends recitals

  delete_complex_field(conn,doc_id,"preamble"); // remove old inserts
  delete_complex_field(conn,doc_id,"sigblock"); // remove old inserts
  delete_complex_field(conn,doc_id,"rider"); // remove old inserts
  delete_complex_field(conn,doc_id,"table_of_contents"); // remove old inserts
  delete_complex_field(conn,doc_id,"recitals"); // remove old inserts

  index_file = fopen(index_name,"r"); 
  if (!index_file) {
    fprintf(stderr,"Error: can't (r) open index_file %s\n",index_name);
    exit(0);
  }
  if (debug) fprintf(stderr,"STEP 0: %s read index\n",av[0]);    
  index_array_no = read_index(index_file); // read the index into index_array
  if (my_debug) fprintf(stderr,"INDEX_ARRAY_NO=%d:\n",index_array_no);
  yylex();

  fprintf(stderr,"Info: %s done (%d preams, %d recitals):\n",av[0],pream_no,recitals_no);

  if (debug) fprintf(stderr,"STEP 1: %s retrieve pream_labels\n",av[0]);  

  int num_labels = get_recitals_from_sql(conn, doc_id); // STEP 1

  if (debug) fprintf(stderr,"STEP 2: %s sync_up (num_labels=%d)\n",av[0],num_labels);  

  sync_up_recitals_and_toc(); // STEP 2

  if (debug) fprintf(stderr,"\n\n\n\nSTEP 3: %s create linked list\n\n\n\n",av[0]);  
  create_linked_list_index(index_array_no); // create a linked list so can add items in the middle // STEP 4 //F07

  if (debug) fprintf(stderr,"\n\n\n\nSTEP 4: %s scan_recitals and generate edit list\n\n\n\n",av[0]);  

  edit_index_array_no = scan_recitals_from_array(edit_index_array); // STEP 3: decide if legit preamble
  if (debug) fprintf(stderr,"FOUND %d PREAMs\n",edit_index_array_no);
  if (debug) fprintf(stderr,"Opened (r) index_file %s\n",index_name);


  if (debug) fprintf(stderr,"\n\n\n\nSTEP 5: %s edit linked list\n\n\n\n",av[0]);  

  /* int new_in =*/ modify_index(edit_index_array_no); // add items for preamble // STEP 5: generate the new entry in the index // F08
  insert_parts_into_sql(conn,doc_id); // F09
  write_index(index_array_no); // write out the modified index // STEP 6: print out //F10

  return 0;
}
