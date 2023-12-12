%{ 


/*
calculates fundamental points.  
Into SQL: SummaryPoints
Do we need it? nobody reads it?
** CALLING:  (../bin/detect_rental_form -d3971 -Plocalhost -Ndealthing -Uroot -Wimaof3 -D1 < ../tmp/Xxx/mmm2.2  > bbb) >& ttt

../bin/detect_rental_form -d 71815  -Pudev1-from-prod-feb26.cu0erabygff3.us-west-2.rds.amazonaws.com -N dealthing -U root -W imaof333 -D1  -Oaws< ./my_htmlfile > /dev/null

*/


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>

#define MAX_LABEL 50000 
#define MIN(a,b) (a<b)?a:b
#define MAX(a,b) (a>b)?a:b
  
char *source_program;
int debug = 0;
int doc_id;
char *prog;
char *db_IP, *db_name, *db_pwd, *db_user_name; // for DB config
 char *source_ocr = "aby";  

MYSQL *conn;
MYSQL_RES *sql_res;
MYSQL_ROW sql_row;

int line_no;
int sn_on_line;  // the sn of the word on this line
int line_still_ok = 0; // are we still in ALL CAPS at beginning of line
int left_indent = -1; // the indent on the leftmoset word
int page_no;
int last_sp; 
int first_token_id; // the first token of the current point
 
#define MAX_PID 15000
#define MAX_TOKEN 500000  // an index from token to instance 
struct Token_Struct {
  char *text;
  int id;
  char *x1, *x2, *y1, *y2;
  int sn;
  int page_no;
  int line_no;  
  int sn_on_line; // is it first (0) second, etc on the line
} token_array[MAX_TOKEN];
int token_no;

#define LEFT_INDENT_TO_BIN 1000
//#define LEFT_INDENT_TO_BIN 100 
#define MAX_LINE 71
#define MAX_PAGE 400

int read_tokens_into_array(MYSQL *conn, int doc_id) {
  static char query[5000];
  sprintf(query,"select sn, x1, x2, y1, y2, page_no, id, text, line_no \n\
                        from deals_token \n\
                        where doc_id = %d and source_program='%s' ",
	  doc_id, source_ocr);

  if (debug) fprintf(stderr,"QUERY79_2 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY79_2 (%s): query=%s:\n",prog,query);
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
}
  

struct Par_Tok { // the index that tells us what tokens reside in each para
  int tok_id;
  int tok_num;
  char *name;
} par_tok_array[MAX_PID];
int par_tok_no_array;

#define MAX_TOK 500000
int tok2par_array[MAX_TOK]; // OLD
int tok2par_no; // tok2par_no is the last tok in the array; the last toks are not here

int tok2para_array[MAX_TOK]; // NEW
int tok2para_no;


int get_par_tok_from_sql(MYSQL *conn, int did) {
  MYSQL_RES *sql_res;
  MYSQL_ROW sql_row;
  static char query[10000];
  int nn;

  sprintf(query,"select para_no, token_id, num_tokens, doc_id \n\
                          from deals_paragraphtoken as dd \n\
                          where doc_id = '%d' and source_program='%s' "
	  , did, source_ocr );
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
    if (debug) fprintf(stderr,"Error: PARALEN item not found in SQL\n");
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

#define MAX_LABEL_TYPE 100000
struct Label_Type {
    int label_type_id;
    char *label_type_name;
} label_type_array[MAX_LABEL_TYPE];
int label_type_no;

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


struct Page_Point {
  int no_of_words;
  int no_of_chars;
  char text[500];
  int line_no;
  int page_no;
  int bin_no;
  int token_id;
  int left_indent;
}  page_point_array[MAX_PAGE][MAX_LINE];
int page_point_no[MAX_PAGE];

struct Page_Prop {
  int no_of_lines;
  int no_of_points;
  int mean_length;
  int std_length;
  int good_words_in_points; // words such as DATE, LANDLORD, ADDRESS, SIZE, etc.
} page_prop_array[MAX_PAGE];

#define MAX_BIN 1000
int bin_array[MAX_PAGE][MAX_BIN];
int max_bin_content[MAX_PAGE];
int max_bin_index[MAX_PAGE];


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
} // my_escape()

 
// don't show if max bin is below 3 points
#define PAGE_THRESHOLD 3
// show points +/- around the max
#define BIN_TOLERANCE 10
char *tally_page_points() {
  static char query_line[200000];
  int ii;
  fprintf(stderr,"PAGES=%d:\n",page_no);  
  for (ii = 0; ii < page_no+1; ii++) {
    if (page_prop_array[ii].no_of_points > 0) {
      int jj;
      for (jj = 0; jj < page_prop_array[ii].no_of_points; jj++) { // get the points
	if (1) fprintf(stderr,"POINT:pg=%d: ln=%d: len=%d: lX=%d: :%s:\n",ii, page_point_array[ii][jj].line_no, page_point_array[ii][jj].no_of_words,  page_point_array[ii][jj].left_indent, page_point_array[ii][jj].text);
	int bin_index = page_point_array[ii][jj].left_indent / LEFT_INDENT_TO_BIN;
	bin_array[ii][bin_index]++;
	if (max_bin_content[ii] <= bin_array[ii][bin_index]) {
	  max_bin_content[ii] = bin_array[ii][bin_index]+1;
	  max_bin_index[ii] = bin_index;
	}
	if (1) fprintf(stderr,"            LLL: pp=%d: idx=%d: cont=%d:    lX=%d:    max=%d:%d: PT=%d:\n"
		       ,ii, bin_index,   bin_array[ii][bin_index],     page_point_array[ii][jj].left_indent,        max_bin_index[ii], max_bin_content[ii],     PAGE_THRESHOLD);
      }
      if (max_bin_content[ii] >= PAGE_THRESHOLD) {
	fprintf(stderr,"\n***** MEAN PAGE pp=%d: no=%d: lX=%d:\n",ii, max_bin_content[ii], max_bin_index[ii]);
	for (jj = 0; jj < page_prop_array[ii].no_of_points; jj++) { // get the points
	  if (abs(max_bin_index[ii] - (page_point_array[ii][jj].left_indent / LEFT_INDENT_TO_BIN)) < BIN_TOLERANCE) {
	    fprintf(stderr,"         MEAN POINT:pg=%d: ln=%d: len=%d: lX=%d: :%s:\n",ii, page_point_array[ii][jj].line_no, page_point_array[ii][jj].no_of_words,  page_point_array[ii][jj].left_indent, page_point_array[ii][jj].text);
	    static char buff[1000];
	    sprintf(buff,"(%d, %d, %d, %d, %d, %d, %d, %d, '%s', '%s') , \n"
		    , doc_id
		    , page_point_array[ii][jj].page_no
		    , page_point_array[ii][jj].line_no
		    , page_point_array[ii][jj].token_id		    
		    , page_point_array[ii][jj].no_of_words
		    , page_point_array[ii][jj].bin_no		    
		    , page_point_array[ii][jj].left_indent
		    , tok2para_array[page_point_array[ii][jj].token_id]
		    , my_escape(page_point_array[ii][jj].text)
		    , source_ocr);
	    strcat(query_line, buff);
	  }
	}
      }
    }
  }
  return query_line;
} // tally_page_points() 


#define MAX_POINTS_ON_PAGE 200
#define MAX_ALLOWED_DISTANCE 5

/********************************************************************************/
%}

espan "</span"[^\>]*\>
span "<span"[^\>]*\>
tag "<"[^\>]*\>
%%
"<HR" {
  ECHO;
  page_prop_array[page_no+1].no_of_points = -1;
}

{span} { // removing all the class junk from the span  
  char *pp = strstr(yytext," id=sp_");
  int nn = 0;
  if (pp) {
    nn = sscanf(pp," id=sp_%d",&last_sp);
  }
  sn_on_line = token_array[last_sp].sn_on_line;
  line_no = token_array[last_sp].line_no;
  page_no = token_array[last_sp].page_no;  
  if (sn_on_line == 0) {
    left_indent = (int)(atof(token_array[last_sp].x1) * 1000.0) / 10;
    first_token_id = last_sp;
    line_still_ok = 1;
  }
  ECHO;
}

[A-Z\'\"\.\ \-\,\&\(\)\/\#]+/{espan} {  // we allow "/" for dba
  ECHO;
  if (sn_on_line == 0) {
    fprintf(stderr,"\nNEW LINE::%d: line_ok=%d:--",line_no, line_still_ok);
    page_prop_array[page_no].no_of_points++;
  }
  if (line_still_ok == 1) {
    int point_no = page_prop_array[page_no].no_of_points;
    fprintf(stderr,"UU:%d: t=%s: ppn=%d:%d:--", sn_on_line, yytext, page_no, point_no);
    page_point_array[page_no][point_no].no_of_words++;
    page_point_array[page_no][point_no].no_of_chars += strlen(yytext);
    page_point_array[page_no][point_no].line_no = line_no;
    page_point_array[page_no][point_no].bin_no = left_indent / LEFT_INDENT_TO_BIN;    
    page_point_array[page_no][point_no].page_no = page_no;
    page_point_array[page_no][point_no].left_indent = left_indent;
    page_point_array[page_no][point_no].token_id = first_token_id;    
    page_point_no[page_no] = point_no+1;
    strcat(page_point_array[page_no][point_no].text,yytext);
  }
}

[^\<]+/{espan} {
  ECHO;
  line_still_ok = 0;
}

{tag} {
  ECHO;
}
%%

int get_params(int argc, char **argv) {
  int get_opt_index;
  int c_getopt;

  prog = argv[0];
  opterr = 0;

  while ((c_getopt = getopt (argc, argv, "d:P:N:U:W:D:O:")) != -1) {
    switch (c_getopt) {
    case 'd':
      doc_id = atoi(optarg);
      fprintf(stderr,"DOC_ID=%s:\n",optarg);
      break;
    case 'D':
      debug = atoi(optarg);
      break;
    case 'P':
      db_IP = strdup(optarg);
      break;
    case 'N':
      db_name = strdup(optarg);
      break;
    case 'U':
      db_user_name = strdup(optarg);
      break;
    case 'W':
      db_pwd = strdup(optarg);
      break;      
    case 'O':
      source_ocr = strdup(optarg);
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

  fprintf (stderr,"%s took: doc_id = %d, db_IP =%s, db_name =%s, db_user_name =%s, db_pwd=%s: debug=%d:\n",
	   argv[0], doc_id, db_IP, db_name, db_user_name, db_pwd, debug);

  for (get_opt_index = optind; get_opt_index < argc; get_opt_index++) {
    printf ("Non-option argument %s\n", argv[get_opt_index]);
  }
  conn = mysql_init(NULL);
  /* now connect to database */
  if (!mysql_real_connect(conn,db_IP,db_user_name,db_pwd,db_name,0,NULL,0)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }
  return 0;
} // get_params(argc, argv);


int main(int argc, char**argv) {
  get_params(argc, argv);
  prog = argv[0];

  token_no = read_tokens_into_array(conn, doc_id);
  get_par_tok_from_sql(conn, doc_id);
  tok2para_no = convert_para2tok_to_tok2para();
  read_label_type_array();

  yylex();
  char *query_line = tally_page_points();
  insert_summarypoints(query_line);
  return 0;
} // main()

int insert_summarypoints(char query_line[]) {

  static char query[500000];
  sprintf(query,"delete from deals_summarypoints \n\
           where doc_id = %d and source_program='%s'"
	  , doc_id, source_ocr ); 
  if (debug) fprintf(stderr,"QUERY50=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY50=%s\n",query);
  }

  sprintf(query,"insert into deals_summarypoints \n\
           (doc_id, page_no, line_no, token_id, num_tokens, bin_no, left_indent, para_no, text, source_program) \n\
           values \n"); 
  strcat(query,query_line);
  remove_last_comma(query);
  if (debug) fprintf(stderr,"QUERY51=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY51=%s\n",query);
  }

  return mysql_insert_id(conn);
}

