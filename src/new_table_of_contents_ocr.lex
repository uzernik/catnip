%x snew_para pp1
%{
/* 
** no decisions, just do counts to later decide if this page is a TOC or not based on it's stats
INPUT:  
OUTPUT: PAGE2SUMMARY
** STEP1: read summary stats -- so it won't be deleted
** STEP2: yylex -- collect toc stats
** STEP3: print summary plus toc stats: in 3134 the score for toc page is 375, the next page below is 10, in 3144 the last TOC page is 120

** CALL: index_file=iii; Lexicons=~/dealthing/lexicons;../bin/new_table_of_contents -d 129 -P localhost -N dealthing -U root -W imaof3 -D1 -I ../tmp/Index/index_file_129 -X ../../lexicons/Word_Counts/word_total_count -O aws < ../tmp/Xxx/aaa5 > /dev/null
*/
#include <string.h>
#include <time.h>
#include <math.h>  
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <mysql.h>
#include <stdlib.h>	/* for bsearch() */
#include "line_token_ocr.h"	/* for coords */
#include <sys/time.h>
#include <time.h>


#define MAX(a,b) (a>b)?a:b
#define MIN(a,b) (a<b)?a:b  
#define MAX_PID 10000
char *index_name;
FILE *index_file;
char *source_ocr="aby";

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

    int page_coords_OK;  // take into account in coords clustering
  }; // index_item
  #define MAX_TOC 5000
  struct Index_Item item_array[MAX_TOC];
  int index_no = 0;


long long tt1 = 0;
long long current_timestamp(int nn) {
  struct timeval te;
  gettimeofday(&te, NULL); // get current time
  long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
  if (nn == 0) tt1 = milliseconds;
  fprintf(stderr,"********** NN:%d: milliseconds: %lld\n", nn, milliseconds);
  if (nn == 1) fprintf(stderr,"TIME:%lld:\n", milliseconds - tt1);
  return milliseconds;
}


  /************** SQL ********************/
  //char *server = "localhost";
  char *db_IP; // "54.241.17.226"
  char *db_user_name; // "root";
  char *db_pwd; //"imaof3";
  char *db_name; // "dealthing";

MYSQL *conn;
MYSQL_RES *sql_res;
MYSQL_ROW sql_row;
char query[500000];

  /************** END SQL ****************/

 int cmp_flag = 0;
int new = 1;
int debug = 0;
char *prog;
int doc_id;
int gpid = 0;
int hr_page_no = 0; // HR page number -1
int gp_count = 0;
int para_no_on_page = 0; // we want to know when our para is first on the page so we won't count prev chars
int para_line_no = 0; // the line no at the beginning of a para.  We can't detect a new line if no para start, so missing some lines

char *dict_name;
FILE *english_file;

typedef struct Word_Item {
  char text[500];
  float freq;
  int sn;
} word_item;

word_item english_array[100000];
int english_no, dup_english_no;



char *word_to_lower_cln(char*text) {
  int ii;
  int jj;
  static char bext[500];
  for (jj = 0, ii = 0; ii < strlen(text); ii++) {
    if (isalnum(text[ii])) {
      bext[jj++] = tolower(text[ii]);
    }
  }
  bext[jj] = '\0';
  return strdup(bext);
}

int ppp_cmp(const void *a, const void *b) {
  char *x = ((word_item *)a)->text;
  char *y = ((word_item *)b)->text;
  if (cmp_flag==1) fprintf(stderr,"\nComparing <%s> <%s> <%d>\n",x,y,strcmp(x,y));
  //if (cmp_flag==2) fprintf(stderr,"\nEomparing <%s> <%s> <%d>\n",x,y,strcmp(x,y));
  return(strcmp(x,y));
}

int sort_dict_by_word(word_item male_array[], int male_no) {
  qsort((void *)&male_array[0],  male_no,  sizeof(word_item), ppp_cmp);
  return 0;
}

float find_fname(char *text, word_item male_array[], int male_no, int *sn, int english) {

  float ret = -1;
  struct Word_Item *p;
  struct Word_Item mmm;
  struct Word_Item *word_item1 = &mmm;

  strcpy(word_item1->text,word_to_lower_cln(text));

  p = (word_item *)bsearch((void *)word_item1,
				(void *)&male_array[0], male_no,
				sizeof(word_item), 
				ppp_cmp);

  if (p != NULL) {
    *sn =  ((p)->sn);
    ret = ((p)->freq);
  } else { // for english files return 1000000000 if not found (a very large number)
    if (english == 1) {
      *sn = 100000;
    } else {
      *sn = -1;
    }
    ret = -1;
  }
  return ret;
} // find_fname



 
struct Pid_Details {
  int len; // how many alnum chars in para
  int token_id; // token_id at start of para
  int line_no; // line at start of para  
  char *toc_item; // 2.1 at the beginning of the para
  int int_toc_item;
  char *toc_title; // "Table of Contents"
  char *page_ref; // "5", or "iii" at the end of the para
  int int_page_ref;
  int page_no; // from parsing
  int page_no1; // from token_id
  int rightmost_x;
  char *rightmost_word;  
} pid_stats_array[MAX_PID];


struct Page {
  int page_no;
  int toc_title_found; // found "table of contents" or "exhibits"
  int no_of_points; // all points, "Tenant's Name:"
  int toc_score; // totaling the scores of the paras
  int summary_score; // titles such as "Abstract Page", "Basic Provisions Page"
  int no_of_toc_paras; // toc paras
  
  int no_of_chars; // how many alunm chars in page
  int no_of_lines; // how many lines in page
  int no_of_centered_lines; // how many centered lines in page
  int no_of_diverse_lines; // how many lines that are off left and right indent
  int no_of_words; 
  int no_of_junk_words; // how many words with less than 2 alnum chars
  int no_of_dict_words; // how many words with more than 2 alnum chars  AND found in dictionary
  int no_of_first_cap_words; // how many centered lines in page  
  int no_of_title_page_words; // and, between, effective, dated, lease, agreement, inc, *xcompany*, *xname*, *xdate*
  int no_of_sig_block_words; // and, between, effective, dated, lease, agreement, inc, *xcompany*, *xname*, *xdate*  
  int no_of_LIs; // company, date2, person_name
  int no_of_VSs; // vertical spaces
  int title_page_score; //
  int sig_block_page_score; //   PAGE not BLOCK
  int sig_block_para; // where the sig_block starts  
  int new_title_page_score; //   is it a title page?
  int new_toc_score; // is is a TOC page
  int std_of_centered_lines_length; 
  int exhibit_on_first_lines;
  int exhibit_score;
  int right_indentation_cluster;
  int title_page_words;
  int sig_block_words;  

  int lease_title_para;
  int lease_title_score;
  int lease_title_line;
  int lease_title_no_of_lines;    

  int sig_page_follows_words;
  int WITNESS_words;
  int by_words;
  int name_title_date_words;  // words that appear of sig_block
  int PLAYER_words;
  int recitals_score;
  int recitals_start_para;   // where it starts
  int recitals_end_para;   // where it starts  
  char *recitals_words; // "WITNESSETH", "WHEREAS"
  
} page_array[MAX_PAGE];
int page_no;

int num_real_chars(char *text) {
  int ret = 0;
  int ii;
  for (ii = 0; ii < strlen(text); ii++) {
    if (isalnum(text[ii]) != 0) {
      ret++;
    }
  }
  return ret;
}
 
int read_index(MYSQL *conn, char *index_name, int doc_id) {
  index_file = fopen(index_name,"r");
  if (!index_file) {
    fprintf(stderr,"Info: can't (r) open index_file %s: no big deal!\n",index_name);
    return 0;
  }

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
    sscanf(line,"%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%[^\t\n]\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n"
	   ,&seqn,&my_enum,&ln,&gn,&pid,&lev_no,article,section,clean_header, &indent, &too_long, &center, &my_loc, &line_no, &page_no, &a1, &a2, &a3, &is_special
	   , &toc_page_no, &toc_page_no_type, &toc_page_no_coord);


    if (debug) fprintf(stderr,"MM ind=%d: too=%d: cen=%d:%d: lp=%d:%d: a=%d:%d:%d: is=:%d:\n", indent, too_long, center, my_loc, line_no, page_no, a1,a2,a3,is_special);



    if (debug) fprintf(stderr,"LL=%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t:%s:\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n"
		       ,seqn,my_enum,ln,gn,pid,lev_no,article,section,clean_header, indent, too_long, center, line_no, page_no, is_special
		       , toc_page_no, toc_page_no_type, toc_page_no_coord		       
		       );
    int same_as_before = ((in > 0)  // make sure identical lines are not inserted (a bug in get_preamble inserts some lines twice)
			  && (ln == item_array[in-1].line_no 
			      && my_enum == item_array[in-1].my_enum
			      && pid == item_array[in-1].pid));

    if (lev_no != 10 && same_as_before == 0) {
      item_array[in].indent = indent;
      item_array[in].too_long = too_long;      
      item_array[in].line_no = ln;
      item_array[in].my_enum = my_enum;
      item_array[in].grp_no = gn;
      item_array[in].seq_no = seqn;
      item_array[in].pid = pid;
      item_array[in].title = ((strcmp(article,"-")==0) ? "" : strdup(article));
      item_array[in].section = strdup(section);
      item_array[in].clean_header = strdup(clean_header);
      item_array[in].indent= indent;
      item_array[in].center = center;
      item_array[in].page_no = page_no;
      item_array[in].line_no = line_no;
      item_array[in].is_special = is_special;	                  

      item_array[in].toc_page_no = toc_page_no; // "23", "123", "xvii"
      item_array[in].toc_page_no_type = toc_page_no_type; // roman = 1, arab = 2, none = 0,3,4
      item_array[in].toc_page_no_coord = toc_page_no_coord; // 523123

      in++;
    }
  } // while
  return in;
} // read_index


int read_dict() {
  static char line[800];
  static char ttt[100];
  float total_words;

  int mm;
  int ii;

  /*******************  ENGLISH  *****************************/

  fprintf(stderr,"READING ENGLISH_WORDS:%s:\n",dict_name);
  english_file = fopen(dict_name,"r");
  if (!english_file) {
    fprintf(stderr,"Could not read open :%s:\n", dict_name);
    exit(0);
  }

  mm = 0;
  ii = 0;
  while (fgets(line,180,english_file))  {
    static char aa[200];
    int nn = sscanf(line,"%d\t%[0-9]\t%s", &english_array[mm].sn, ttt, aa);
    strcpy(english_array[mm].text,word_to_lower_cln(aa));
    english_array[mm].freq = atof(ttt);
    if (strcmp(aa,"__total_words__") == 0) {
      total_words = english_array[mm].freq;
    }
    english_array[mm].freq = english_array[mm].freq / total_words;
    if (ii++ < 10) {
      //fprintf(stderr,"MM:%d:%d:%d:%s:%f:\n",mm, nn, english_array[mm].sn, english_array[mm].text, english_array[mm].freq);
    }
    if (nn == 3) mm++;
    else fprintf(stderr,"Warning: incorrect line: %d:%s:\n",mm,line);
  }
  english_no = dup_english_no = mm;
  sort_dict_by_word(english_array, english_no);
  for (ii = 0; ii < english_no; ii++) {
    if (ii>0 && strcmp(english_array[ii].text,english_array[ii-1].text) == 0) {
      //fprintf(stderr,"Warning: duplicate:%d:-%s:%d:%f\n",ii,    english_array[ii].text,    english_array[ii].sn,    english_array[ii].freq);
      int jj = (english_array[ii-1].sn < english_array[ii].sn) ? ii-1 : ii;
      english_array[ii].sn = english_array[ii-1].sn = english_array[jj].sn;
      english_array[ii].freq = english_array[ii-1].freq = english_array[jj].freq;
    }
    //fprintf(stderr,"%d\t%d\t%s\t%f\n",ii,    english_array[ii].sn,    english_array[ii].text,    english_array[ii].freq);
  }
  fprintf(stderr,"TOTAL ENGLISH WORDS=%f EN=%d:\n",total_words, english_no);
  fprintf(stderr,"KKK\n");

  return 0;
} // read_dict()


// to what extent page_ref is lined up
float calc_page_ref_aligned_score_per_page(int page_no) { 
  int ii;
  float std = 100;
  float total_coord = 0;
  float avg = 0;
  int total_no = 0;
  for (ii = 0; ii < index_no; ii++) {
    if (item_array[ii].page_no == page_no && item_array[ii].toc_page_no_type == 2) {

      int score_toc_page_no = (ii > 0 && item_array[ii].toc_page_no >= item_array[ii-1].toc_page_no ? 1 : -1) * 10
	+ (ii > 1 && item_array[ii].toc_page_no >= item_array[ii-2].toc_page_no) * 10
	+ (ii < index_no-1 && item_array[ii].toc_page_no <= item_array[ii+1].toc_page_no) * 10
	+ (ii < index_no-2 && item_array[ii].toc_page_no <= item_array[ii+2].toc_page_no) * 10;	
      int score_toc_enum = (ii > 0 && item_array[ii].my_enum >= item_array[ii-1].my_enum) * 10
	+ (ii > 1 && item_array[ii].my_enum >= item_array[ii-2].my_enum) * 10
	+ (ii < index_no-1 && item_array[ii].my_enum <= item_array[ii+1].my_enum) * 10
	+ (ii < index_no-2 && item_array[ii].my_enum <= item_array[ii+2].my_enum) * 10;	
      int bad_score_coord = (ii > 0 && ((item_array[ii].toc_page_no_coord - item_array[ii-1].toc_page_no_coord) * 25 > item_array[ii].toc_page_no_coord)) * 10
	+ (ii > 1 && ((item_array[ii].toc_page_no_coord - item_array[ii-2].toc_page_no_coord) * 25 > item_array[ii].toc_page_no_coord)) * 10
	+ (ii < index_no - 1 && ((item_array[ii].toc_page_no_coord - item_array[ii+1].toc_page_no_coord) * 25 > item_array[ii].toc_page_no_coord)) * 10
	+ (ii > index_no - 2 && ((item_array[ii].toc_page_no_coord - item_array[ii+2].toc_page_no_coord) * 25 > item_array[ii].toc_page_no_coord)) * 10		
	;
      (ii > 0 && ((item_array[ii].toc_page_no_coord - item_array[ii-1].toc_page_no_coord) * 25 < item_array[ii].toc_page_no_coord)) * 10;       
      if (0) fprintf(stderr,"LLLL:%2d:%2d:%2d:  :%s:%s:  :%d:%d: ss=%d:%d:%d:\n"
	      , ii, item_array[ii].my_enum, item_array[ii].toc_page_no
	      , item_array[ii].section,item_array[ii].clean_header
	      , item_array[ii].toc_page_no_type, item_array[ii].toc_page_no_coord
	      , 40-score_toc_enum, 40-score_toc_page_no, bad_score_coord
	      );
      total_coord += item_array[ii].toc_page_no_coord;
      total_no++;
      item_array[ii].page_coords_OK = (40-score_toc_enum + 40-score_toc_page_no + bad_score_coord < 40);
    }
  }
  avg = total_no > 3 ? (float) total_coord / (float)total_no : 0;    
  float total_sigma = 0;
  for (ii = 0; ii < index_no; ii++) {
    if (item_array[ii].page_coords_OK == 1) {
      total_sigma += pow((avg - item_array[ii].toc_page_no_coord),2.0);
    }
  }
  std = total_no > 3 ? sqrt (total_sigma ) / total_no : 100;
  float norm_std = (avg > 100000.0) ? std / avg : 100;
  if (debug) fprintf(stderr,"STD= norm=%2.4f: std=%2.4f: avg=%2.4f: no=%d\n", norm_std, std, avg, total_no);
  return norm_std;
} // calc_page_ref_aligned_score_per_page() { 

int calc_pid_stats(int gpid) {
  int ii;
  if (debug) fprintf(stderr,"\nCALC PID STATS:%d:\n",gpid);
  for (ii = 0; ii <= gpid; ii++) {  // go over paras
    int toc_score = 0;
    int token_id;
    int line_id;
    int page_id;
    toc_score -= (pid_stats_array[ii].len > 30) ? 5 : 0;    // take out 5 points for long para
    toc_score -= (pid_stats_array[ii].len > 100) ? 10 : 0;  // take out 10 points for longer para
    toc_score -= (pid_stats_array[ii].len > 200) ? 15 : 0;  // take out 15 points for longer para
    toc_score += (pid_stats_array[ii].toc_item) ? 5 : 0;
    toc_score += (pid_stats_array[ii].page_ref) ? 5 : 0;
    toc_score += (pid_stats_array[ii].toc_title) ? 300 : 0;
    toc_score += (pid_stats_array[ii].int_page_ref != 0 // if it's bigger than 0 and [0,1] difference
	      && (pid_stats_array[ii].int_page_ref - pid_stats_array[ii-1].int_page_ref == 0
		  || pid_stats_array[ii].int_page_ref - pid_stats_array[ii-1].int_page_ref == 1)
	      ) ? 10 : 0;
    toc_score += (pid_stats_array[ii].int_toc_item != 0 // if it's bigger than 0 and [0,1] difference
	      && (pid_stats_array[ii].int_toc_item - pid_stats_array[ii-1].int_toc_item == 0
		  || pid_stats_array[ii].int_toc_item - pid_stats_array[ii-1].int_toc_item == 1)
	      ) ? 10 : 0;

    pid_stats_array[ii].token_id = token_id = Paragraph2Token_array[ii];
    pid_stats_array[ii].line_no = line_id = token_array[token_id].line_no;
    pid_stats_array[ii].page_no1 = page_id = token_array[token_id].page_no;    
    pid_stats_array[ii].rightmost_x = page_line_properties_array[page_id][line_id].right_X;
    pid_stats_array[ii].rightmost_word = page_line_properties_array[page_id][line_id].line_right_word;
    //fprintf(stderr,"MMMMM:pid=%d: tid=%d: line=%d: page=%d: word=%s:\n",ii, token_id, line_id, page_id, page_line_properties_array[page_id][line_id].line_right_word);
    static int old_page_no = 0;
    if (pid_stats_array[ii].len > 0) {
      if (pid_stats_array[ii].page_no == 1) {
	if (0) fprintf(stderr,"                GGG:%d:%d:--:toc_item=%s: toc_ref=%s: toc_title=%s:\n",pid_stats_array[ii].page_no,ii
		,pid_stats_array[ii].toc_item, pid_stats_array[ii].page_ref,    pid_stats_array[ii].toc_title);
      }
      static int no_of_words = 0;
      static int no_of_first_cap_words = 0;
      if (ii > 0 && pid_stats_array[ii].page_no >= old_page_no +1) { // NEW PAGE
	//  THE TWO LINES I RECENTLY FIXED 102617
	toc_score += 4 * page_array[pid_stats_array[ii].page_no+1].right_indentation_cluster; // add the number of clustered right indent page numbers
	toc_score -= (page_array[pid_stats_array[ii].page_no+1].no_of_words > page_array[pid_stats_array[ii].page_no+1].no_of_lines * 8) ? 40 : 0; // the ratio per line s/b around 5 for TOC, around 10 for normal line

	float norm_std_page_no = 0;
	if (index_file != NULL) norm_std_page_no = calc_page_ref_aligned_score_per_page(pid_stats_array[ii].page_no);
	no_of_words = page_array[pid_stats_array[ii].page_no+1].no_of_words;
	no_of_first_cap_words = page_array[pid_stats_array[ii].page_no+1].no_of_first_cap_words;      
	page_array[pid_stats_array[ii].page_no].toc_score -= (no_of_words * 3 > no_of_first_cap_words * 4) ? 40 : 0; // take points if First Cap Ratio is too small
	page_array[pid_stats_array[ii].page_no].toc_score -= (no_of_words > 250) ? 20 : 0; // take points if more than 250 words
	page_array[pid_stats_array[ii].page_no].toc_score -= (norm_std_page_no > 0.02) ? 20 :0; 

	if (debug) fprintf(stderr,"-----------NP:%d: words:%d: Words:%d: OP:%d:%d: RATIO=%d: SCORE=%d: std=%2.4f:\n"
			   , pid_stats_array[ii].page_no, no_of_words,  no_of_first_cap_words
			   , page_array[pid_stats_array[ii].page_no].no_of_words
			   , page_array[pid_stats_array[ii].page_no].no_of_lines
			   , (page_array[pid_stats_array[ii].page_no].no_of_words > page_array[pid_stats_array[ii].page_no].no_of_lines * 8) ? 10 : 0
			   , page_array[pid_stats_array[ii].page_no].toc_score
			   , norm_std_page_no
			   );
      }
      old_page_no = pid_stats_array[ii].page_no;
      if (debug) fprintf(stderr,"PPPID:page=%d:\tpid=%d:\tscore=%d:\tpara_len=%d:\ttoc_item=%s:%d:\ttitle=%s:\tref=%s:%d:\tpn=%d:%d:\tline=%d:%d:%s:\t%s:%s:%s: score=%d:%d:\n"
	      , pid_stats_array[ii].page_no
	      , ii
	      , toc_score
	      , pid_stats_array[ii].len
	      , (pid_stats_array[ii].toc_item) ? pid_stats_array[ii].toc_item : ""
	      , pid_stats_array[ii].int_toc_item
	      , (pid_stats_array[ii].toc_title) ? pid_stats_array[ii].toc_title : ""
	      , (pid_stats_array[ii].page_ref) ? pid_stats_array[ii].page_ref : ""
	      , pid_stats_array[ii].int_page_ref
			 , pid_stats_array[ii].page_no, pid_stats_array[ii].page_no1
			 , pid_stats_array[ii].line_no, pid_stats_array[ii].rightmost_x, pid_stats_array[ii].rightmost_word
	      , pid_stats_array[ii].toc_item, pid_stats_array[ii].page_ref
			 ,    pid_stats_array[ii].toc_title
			 , page_array[pid_stats_array[ii].page_no].toc_score
			 , toc_score
			 );
      page_array[pid_stats_array[ii].page_no].toc_score += toc_score;
    }
  } // for gpid ii
} // calc_pid_stats


 
char *my_copy(char *text) { // stop at "<"or " "
  static char bext[20];
  int ii;
  for (ii = 0; ii < strlen(text); ii++) {
    if (text[ii] == '<' /*|| text[ii] == ' ' */ || text[ii] == '\n') {
      break;
    } else {
      bext[ii] = text[ii];
    }
  }
  bext[ii] = '\0';
  return bext;
} // my_copy()

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




 
#define X_THRESH 600
 /*
   we have 6 centers (lines). for each line find which of the other 5 centers fall within THERSH.  take the maximal
  */
int cluster_lines_by_center_x(int page_id) { 
  if (0) fprintf(stderr,"LLL:%d:\n",page_id);
  int ii, jj;
  int no_of_lines = page_array[page_id].no_of_lines;
  for (jj = 0; jj < no_of_lines; jj++) { // found how many neighbors each line center has
    int center_jj = page_line_properties_array[page_id][jj].center;
    int found_neighbors_jj = 0;
    for (ii = 0; ii < no_of_lines; ii++) {
      int center_ii = page_line_properties_array[page_id][ii].center;  
      if (abs(center_jj - center_ii) < X_THRESH) {
	found_neighbors_jj++;
      }
    }
    if (0) fprintf(stderr,"       LLL50:%d: :%d: :%d:\n",center_jj, jj, found_neighbors_jj);        
    page_line_properties_array[page_id][jj].found_center_neighbors = found_neighbors_jj;
  }
  int max_neighbors = -1;
  int max_ii = -1;  
  for (jj = 0; jj < no_of_lines; jj++) { // find the max
    if (page_line_properties_array[page_id][jj].found_center_neighbors > max_neighbors) {
      max_neighbors = page_line_properties_array[page_id][jj].found_center_neighbors;
      max_ii = jj;      
    }
  }
  if (0) fprintf(stderr,"   LLL0:%d:%d:%d:\n",page_id,max_ii,max_neighbors);
  // calculate std of line length
  // first calculate mean
  int total_length = 0;
  int nn = 0;
  for (jj = 0; jj < no_of_lines; jj++) { // find the mean of neibors of max_ii
    int center_jj = page_line_properties_array[page_id][jj].center;
    int center_ii = page_line_properties_array[page_id][max_ii].center;
    int diff = abs(center_jj - center_ii);
    if (0) fprintf(stderr,"       LLL5:%d:%d:%d:%d: :%d:\n",diff, X_THRESH, center_jj, center_ii, max_ii);    
    if (diff < X_THRESH) {
      int length_jj = page_line_properties_array[page_id][jj].right_X - page_line_properties_array[page_id][jj].left_X ;
      total_length += length_jj;
      nn++;
      if (0) fprintf(stderr,"            LLL6:%d:%d:%d:\n",nn,length_jj,total_length);
    }
  }
  if (0) fprintf(stderr,"   LLL1:%d:%d:%d:\n",max_ii,total_length,nn);  
  int mean = (int)((double)total_length / (double)nn);
  int total_deviation = 0;
  for (jj = 0; jj < no_of_lines; jj++) { // find the deviation
    int center_jj = page_line_properties_array[page_id][jj].center;
    int center_ii = page_line_properties_array[page_id][max_ii].center;        
    if (abs(center_jj - center_ii) < X_THRESH) {
      int length_jj = page_line_properties_array[page_id][jj].right_X - page_line_properties_array[page_id][jj].left_X ;      
      int dev = (length_jj - mean) * (length_jj - mean);
      total_deviation += dev / 10;
    }
  }
  int std = (int)sqrt((double)((double)total_deviation / (double)nn));
  if (0) fprintf(stderr,"   LLL2:%d:mean=%d: total_dev=%d:  std=%d:\n",max_ii,mean,total_deviation,std);    
  page_array[page_id].no_of_centered_lines = max_neighbors +1; // count also myself...
  page_array[page_id].std_of_centered_lines_length = std;  
} // cluster_lines_by_center_x()

 

int is_word_upper(char *text) {
  int ii;
  int ret = 1;
  for (ii = 0; ii < strlen(text); ii++) {
    if (isalnum(text[ii]) != 0 && isupper(text[ii]) == 0) {
      ret = 0;
    }
  }
  return ret;
}

// 1 -- 12345; 2 -- ii; 3 -- (iv)
int isnum(char *word) {
  int ret = -1;
  if (word != NULL) {
    int ii;
    int no_good = 0;
    static char aa[100],bb[100],cc[100];
    for (ii = 0; no_good == 0 && ii < strlen(word); ii++) {
      if (isdigit(word[ii]) == 0) no_good = 1;
    }
    if (no_good == 0) {
      ret = 1;
    } else if (sscanf(word,"%[ivx]",aa) == 1)  {
      ret = 2;
    } else if (sscanf(word,"%[(]%[ivx]%[)]",aa,bb,cc) == 3)  {
      ret = 3;
    } else {
      ret = 0;
    }
  }
  return ret;
}

struct Coords_Cluster {
  int val; 
  int no_of_friends;
} coords_cluster_array[MAX_LINE];
 int coords_cluster_no;

#define MAX_LABEL_TYPE 5000
struct Label_Type {
  int label_type_id;
  char *label_type_name;
} label_type_array[MAX_LABEL_TYPE];
int label_type_no;

int ppp1_cmp(const void *a, const void *b) { // label_type
  char *x1 = (((struct Label_Type *)a)->label_type_name);
  char *y1 = (((struct Label_Type *)b)->label_type_name);
  //fprintf(stderr,"\nComparing <%s> <%s> <%d>\n",x1,y1,strncmp(x1,y1,strlen(x1)));
  return(strcmp(x1,y1));
}
int ppp2_cmp(const void *a, const void *b) { // coords_cluster_item
  int x1 = (((struct Coords_Cluster *)a)->val);
  int y1 = (((struct Coords_Cluster *)b)->val);
  //fprintf(stderr,"\nComparing <%s> <%s> <%d>\n",x1,y1,strncmp(x1,y1,strlen(x1)));
  return(x1>y1);
}


int sort_array_by_word() {
  qsort((void *)&label_type_array[0],label_type_no,sizeof(struct Label_Type),ppp1_cmp);
  return 0;
}

int sort_coords_cluster_array_by_val() {
  qsort((void *)&coords_cluster_array[0],coords_cluster_no,sizeof(struct Coords_Cluster),ppp2_cmp);
  return 0;
}
 
#define COORD_THRESH 4000 
int cluster_right_coords(int prev_page_no) {
  int ii, jj;
  int nol = page_array[prev_page_no].no_of_lines;
  for (ii = 0, jj = 0; ii < nol && ii < MAX_LINE; ii++) { // initialize
    coords_cluster_array[ii].val = -1;
    coords_cluster_array[ii].no_of_friends = 0;
  }
  //fprintf(stderr,"NOL=%d:\n",nol);
  for (ii = 0, jj = 0; ii < nol; ii++) { // fill in initial values
    if (page_line_properties_array[prev_page_no][ii].line_right_is_number > 0) {
      //fprintf(stderr,"     NOL=%d:%d:%d:\n",ii,jj,page_line_properties_array[prev_page_no][ii].right_X);      
      coords_cluster_array[jj++].val = page_line_properties_array[prev_page_no][ii].right_X;
    }
  }
  coords_cluster_no = jj;
  int kk,ll;
  for (kk = 0; kk < coords_cluster_no; kk++) { // do the lists of friends
    for (ll = 0; ll < coords_cluster_no; ll++) {
      if (kk != ll && abs(coords_cluster_array[kk].val - coords_cluster_array[ll].val) < COORD_THRESH) {
	coords_cluster_array[kk].no_of_friends++;
      }
    }
  }
  int max_no_of_friends = -1;
  int mm;
  for (mm = 0; mm < coords_cluster_no; mm++) { // find the max cluster
    if (max_no_of_friends < coords_cluster_array[mm].no_of_friends) {
      max_no_of_friends = coords_cluster_array[mm].no_of_friends;
    }
  }
  if (0) {
    fprintf(stderr," CLUSTERS:%d:%d:\n", prev_page_no, coords_cluster_no);
    for (ii = 0; ii < coords_cluster_no; ii++) { // fill in initial values
      fprintf(stderr,"     CLUSTER:%d:%d:%d: :%d:\n", ii, coords_cluster_array[ii].val,coords_cluster_array[ii].no_of_friends,max_no_of_friends);
    }
  }
  page_array[prev_page_no].right_indentation_cluster = max_no_of_friends;
  return 0;
}
int alnum_len(char *word) {
  int ii;
  int len = 0;
  for (ii = 0; ii < strlen(word); ii++) {
    if (isalnum(word[ii]) != 0) len++;
  }
  return len;
}

 
int calc_page_stats(int token_no) {
  int ii;
  int words_per_page = 0;
  int Words_per_page = 0;
  int junk_words_per_page = 0;   // less than 2 chars in the word are alnum
  int dict_words_per_page = 0; // more than 2 chars and found in dictionary
  int title_page_words_per_page = 0;
  int sig_block_words_per_page = 0;    
  int lines_per_page = 0;  
  int prev_ii = -1;
  int prev_page_no = -1;
  int curr_page_no = -1;  
  int prev_line_no = -1;
  int page_left_most_x1 = 1000000;
  int page_right_most_x2 = -1;
  int line_center_x;
  int line_left_x;
  int line_right_x;
  char *line_left_word = NULL;
  char *line_right_word = NULL;
  int line_right_is_number = -1;
  fprintf(stderr,"TONEN NO=%d:\n",token_no);
  for (ii = 0; ii < token_no; ii++) {
    int curr_page_no = token_array[ii].page_no;
    if (debug) fprintf(stderr,"TOKEN:%d: pn=%d: ln=%d: text=%s:\n",ii,  curr_page_no,  token_array[ii].line_no,token_array[ii].text);
    // for EXHIBIT we check only the first word on the line!!

    // don't take chardoshkes
    int token_legit = 1; //!(strlen(token_array[ii].text) == 1 && isalnum(token_array[ii].text[0]) == 0); // it's not legit if it's one char that is not anlum 
    {
      if (curr_page_no != prev_page_no || ii == token_no-1) { // NEW PAGE
	page_array[prev_page_no].page_no = page_no = prev_page_no;
	page_array[prev_page_no].no_of_words = words_per_page;
	page_array[prev_page_no].no_of_first_cap_words = Words_per_page;
	page_array[prev_page_no].no_of_junk_words = junk_words_per_page;
	page_array[prev_page_no].no_of_dict_words = dict_words_per_page;		
	page_array[prev_page_no].no_of_lines = lines_per_page-1; // SOMEHOW THE TOTAL CAME OUT WRONG URI 06/19/20
	page_array[prev_page_no].no_of_title_page_words = title_page_words_per_page;
	page_array[prev_page_no].no_of_sig_block_words = sig_block_words_per_page;
	cluster_right_coords(prev_page_no);
	if (1) fprintf(stderr,"PERPAGE: page=%d: lines=%d: words=%d: Words=%d: Jwords=%d: Dwords=%d: lm=%d: rm=%d:\n"
		       , prev_page_no
		       , lines_per_page
		       , words_per_page
		       , Words_per_page
		       , junk_words_per_page
		       , dict_words_per_page				       		       
		       , page_left_most_x1
		       , page_right_most_x2
		       );
	title_page_words_per_page = 0;
	sig_block_words_per_page = 0;	
	words_per_page = 0;
	Words_per_page = 0;
	junk_words_per_page = 0;
	dict_words_per_page = 0;
	lines_per_page = 0;
	prev_page_no = token_array[ii].page_no;
	prev_line_no = -1;
	page_left_most_x1 = 1000000;
	page_right_most_x2 = -1;
      } // new page
      if (token_legit) {  // identify words that impact TITLE_PAGE
	words_per_page++;
	if (isupper(token_array[ii].text[0]) != 0) Words_per_page++;
	if (num_real_chars(token_array[ii].text) < 2) junk_words_per_page++;
	int sn = 0;
	find_fname(token_array[ii].text, english_array, english_no, &sn, 1);
	if (0 && prev_page_no == 2) {
	  fprintf(stderr,"WWWWW:OK=%d: ++=%3d: ii=%4d: t=%20s: rc=%d: sn=%7d: \n"
		  , (num_real_chars(token_array[ii].text) > 2 && sn < 100000), dict_words_per_page, ii, token_array[ii].text, num_real_chars(token_array[ii].text), sn);
	}
	if (num_real_chars(token_array[ii].text) > 2 && sn < 100000) dict_words_per_page++;
	{
	  if (strcasecmp(token_array[ii].text,"dated") == 0) title_page_words_per_page +=10;
	  if (strcasecmp(token_array[ii].text,"effective") == 0) title_page_words_per_page +=10;
	  if (strcasecmp(token_array[ii].text,"between") == 0) title_page_words_per_page +=10;
	  if (strncasecmp(token_array[ii].text,"inc",3) == 0) title_page_words_per_page +=10;
	  if (strcasecmp(token_array[ii].text,"shopping") == 0) title_page_words_per_page +=10;
	  if (strcasecmp(token_array[ii].text,"center") == 0) title_page_words_per_page +=10;
	  if (strcasecmp(token_array[ii].text,"lease") == 0) title_page_words_per_page +=10;
	  if (strcasecmp(token_array[ii].text,"agreement") == 0) title_page_words_per_page +=10;
	  if (strcasecmp(token_array[ii].text,"and") == 0) title_page_words_per_page +=2;
	  if (strncasecmp(token_array[ii].text,"llc",3) == 0) title_page_words_per_page +=2;	  	  
	}
	{
	  if (strcmp(token_array[ii].text,"DATE") == 0 || strcmp(token_array[ii].text,"Date") == 0) sig_block_words_per_page +=10;
	  if (strcmp(token_array[ii].text,"BY") == 0 || strcmp(token_array[ii].text,"By") == 0) sig_block_words_per_page +=10;
	  if (strcmp(token_array[ii].text,"NAME") == 0 || strcmp(token_array[ii].text,"Name") == 0) sig_block_words_per_page +=10;
	  if (strncmp(token_array[ii].text,"TITLE",3) == 0 || strncmp(token_array[ii].text,"Title",3) == 0) sig_block_words_per_page +=10;
	}
      } // legit 

      
      if (1) if (ii >=0) fprintf(stderr,"          TOKEN line:%d: token-1:%d:%d:%s: token_0:%d:%d:%s:\n"
	      , prev_line_no
	      , ii-1, token_array[ii-1].line_no, token_array[ii-1].text
	      , ii, token_array[ii].line_no, token_array[ii].text);			  

      if (token_array[ii].line_no != prev_line_no) { // NEW LINE
	// copy into page_line_properties_array
	if (ii > 0) {
	  line_center_x = (line_left_x + line_right_x) / 2;
	  if (1 || prev_line_no >=0 ) {
	    if (0) fprintf(stderr,"   PERLINE: ii=%d: page=%d: line=%d: lm=%d:%s: rm=%d:%s: center=%d:\n"
			   , ii
		  , prev_page_no
		  , prev_line_no
		  , line_left_x
		  , line_left_word
		  , line_right_x
		  , line_right_word		  
		  , line_center_x
		  );

	    int page_no_0 = token_array[ii].page_no;
	    int line_no_0 = token_array[ii].line_no;
	    int page_no_1 = token_array[ii-1].page_no;	    
	    int line_no_1 = token_array[ii-1].line_no;

	    page_line_properties_array[page_no_0][line_no_0].no_of_chars += strlen(token_array[ii].text);
	    page_line_properties_array[page_no_0][line_no_0].line_no = line_no_1;
	    page_line_properties_array[page_no_0][line_no_0].page_no = page_no_1;

	    page_line_properties_array[page_no_0][line_no_0].left_X = line_left_x;
	    page_line_properties_array[page_no_0][line_no_0].line_left_word = line_left_word;	    

	    page_line_properties_array[page_no_1][line_no_1].right_X = line_right_x;
	    page_line_properties_array[page_no_1][line_no_1].line_right_word = line_right_word;	    

	    page_line_properties_array[page_no_1][line_no_1].center = (page_line_properties_array[page_no_1][line_no_1].right_X + page_line_properties_array[page_no_1][line_no_1].left_X) / 2;



	    if (1) fprintf(stderr,"   PERLINE: ii=%d: pl=%d:%d: page=%d: line=%d: lm=%d:%s: rm=%d:%s:\n"
			   ,ii
			   , page_no_1, line_no_1
			   , page_line_properties_array[page_no_1][line_no_1].page_no
			   , page_line_properties_array[page_no_1][line_no_1].line_no
			   , page_line_properties_array[page_no_1][line_no_1].left_X			   
			   , page_line_properties_array[page_no_1][line_no_1].line_left_word
			   , page_line_properties_array[page_no_1][line_no_1].right_X			   
			   , page_line_properties_array[page_no_1][line_no_1].line_right_word
		  );

	  } // prev_line_no >= 0
	}     

	{ //check left noise
	  line_left_x = token_array[ii].x1_1000;
	  line_left_word = token_array[ii].text;

	  int line_ii = token_array[ii].line_no;
	  //fprintf(stderr,"       GGGG0:%d:%d:%s:\n",line_ii, line_left_x, line_left_word);
	  int found = 0;
	  int found_kk = -1;
	  int mm;
	  int kk;
	  //if (token_array[ii].page_no == 22)

	  for (mm = 0, kk = ii; mm++ < 20 && found == 0 && token_array[kk].line_no == line_ii; kk++) { // move on as long as chardoshkes
	    found = (token_array[kk].text && ((strlen(token_array[kk].text) > 1) || (isalnum(token_array[kk].text[0]) != 0)));
	    found_kk = kk;
	    //fprintf(stderr, "GGGGGGGGGGGGG: ii=%d: kk=%d: line=%d: word=%s:\n", ii,kk,line_ii,token_array[kk].text);
	  }

	  if (found == 1) {
	    line_left_x = token_array[found_kk].x1_1000;
	    line_left_word = token_array[found_kk].text;
	  }

	}

	{ // check right noise
	  line_right_x = token_array[ii-1].x2_1000;
	  line_right_word = token_array[ii-1].text;
	  int line_ii = token_array[ii-1].line_no;
	  if (0) fprintf(stderr,"       FFFF0:%d:%d:%s:\n",line_ii, line_right_x, line_right_word);	  
	  int kk;
	  int found = 0;
	  int found_kk = -1;
	  int mm = 0;

	  for (kk = ii-1; mm++ < 20 && kk >= 0 && found == 0 && token_array[kk].line_no == line_ii; kk--) { // move on as long as chardoshkes
	    found = (strlen(token_array[kk].text) > 1 || isalnum(token_array[kk].text[0]) != 0);
	    found_kk = kk;
	  }
	  if (found == 1) {
	    line_right_x = token_array[found_kk].x2_1000;
	    line_right_word = (token_array[found_kk].text);
	    line_right_is_number = isnum(token_array[found_kk].text);
	    page_line_properties_array[prev_page_no][line_ii].line_right_is_number = line_right_is_number;
	    page_line_properties_array[prev_page_no][line_ii].line_right_word = strdup(line_right_word);
	    if (0) fprintf(stderr, "FFFFFFFFFFF: ii=%d: kk=%d: fkk=%d: line=%d:%d: page=%d: word=%s: num=%d:\n"
			   , ii
		    , kk+1
		    , found_kk
		    , line_ii
		    , prev_line_no
		    , prev_page_no
		    , line_right_word
		    , line_right_is_number);
	  }
	}

	//fprintf(stderr,"     TOK ii=%d line=%d: text=%s:\n", ii, token_array[ii].line_no, token_array[ii].text);
	if (strncasecmp(token_array[ii].text,"exhibie",7) == 0
	    && strncasecmp(token_array[ii].text,"article",7) == 0
	    && strncasecmp(token_array[ii].text,"schedule",7) == 0	    
	    && strncasecmp(token_array[ii].text,"addendum",7) == 0
	    && token_array[ii].line_no < 3) 	page_array[curr_page_no].exhibit_on_first_lines = 1;
	lines_per_page++;  
      } // new line

      prev_line_no = token_array[ii].line_no;
      if (token_legit) {
	if (line_left_word
	    && (strcasecmp(line_left_word,"exhibit") == 0
		|| strcasecmp(line_left_word,"article") == 0
		|| strcasecmp(line_left_word,"addendum") == 0	    
		|| strcasecmp(line_left_word,"schedule") == 0)) {
	  page_line_properties_array[prev_page_no][prev_line_no].left_word_is_exhibit = 1;
	}
	
	page_left_most_x1 = MIN(page_left_most_x1,token_array[ii].x1_1000);
	page_right_most_x2 = MAX(page_right_most_x2,token_array[ii].x2_1000);      
	page_line_properties_array[prev_page_no][prev_line_no].no_of_words++;
	
	if (isupper(token_array[ii].text[0]) != 0 || alnum_len(token_array[ii].text) < 3) page_line_properties_array[prev_page_no][prev_line_no].no_of_first_cap_words++;
	if (is_word_upper(token_array[ii].text) == 1 || alnum_len(token_array[ii].text) < 3) page_line_properties_array[prev_page_no][prev_line_no].no_of_all_cap_words++;
	if (0) fprintf(stderr,"   AGEOO: page=%d: line=%d: cCap:%d:%d: t=%s: is_upper=%d: all_upper=%d:, len=%d:\n"
		, prev_page_no, prev_line_no
		, page_line_properties_array[prev_page_no][prev_line_no].no_of_first_cap_words
		, page_line_properties_array[prev_page_no][prev_line_no].no_of_all_cap_words
		       , token_array[ii].text, isupper(token_array[ii].text[0]), is_word_upper(token_array[ii].text), alnum_len(token_array[ii].text)
		);

	{ // do lease_title
	  if (0
	      || (strncmp(token_array[ii].text,"LEASE",4) == 0
		  && ((ii <= 0) || strcasecmp(token_array[ii-1].text,"OF") != 0) // "terms of lease", "conditions of lease" are all bad DOC titles
		  && strncmp(token_array[ii+1].text,"SUMMARY",5) != 0
		  && strncmp(token_array[ii+1].text,"ADMINISTRATOR",5) != 0		  		  		  
		  && strcmp(token_array[ii+1].text,"DATA") != 0
		  && strcmp(token_array[ii+1].text,"TERMS") != 0
		  && strncmp(token_array[ii+1].text,"PROVISIONS",6) != 0
		  && strncmp(token_array[ii+1].text,"RESTRICTIONS",6) != 0		  		  		  
		  && strcmp(token_array[ii+1].text,"FACE") != 0
		  && strcmp(token_array[ii+1].text,"YEAR") != 0		  		  		  
		  && strncmp(token_array[ii+1].text,"INFORMATION",6) != 0) 
	      || strncmp(token_array[ii].text,"AGREEMENT",7) == 0
	      || strncmp(token_array[ii].text,"JOINDER",7) == 0	      
	      || strncmp(token_array[ii].text,"ATTORNMENT",6) == 0
	      || strncmp(token_array[ii].text,"DISTURBANCE",6) == 0	      	      
	      || strncmp(token_array[ii].text,"SUBLEASE",6) == 0 
	      || strcmp(token_array[ii].text,"ACKNOWLEDGEMENT") == 0
	      || strncmp(token_array[ii].text,"AMENDMENT",5) == 0
	      || strncmp(token_array[ii].text,"RENEWAL",7) == 0
	      || strncmp(token_array[ii].text,"SUBORDINATION",6) == 0	      	      
	      || strcmp(token_array[ii].text,"CONSENT") == 0	      
	      || strncmp(token_array[ii].text,"ADDENDUM",6) == 0
	      || strncmp(token_array[ii].text,"GUARANTY",8) == 0
	      || strncmp(token_array[ii].text,"ASSIGNMENT",7) == 0
	      || strcmp(token_array[ii].text,"ASSUMPTION") == 0	      	      
	      || strncmp(token_array[ii].text,"COMMENCEMENT",6) == 0
	      || strncmp(token_array[ii].text,"MEMORANDUM",6) == 0
	      || strncmp(token_array[ii].text,"MODIFICATION",6) == 0	      
	      || strncmp(token_array[ii].text,"ESTOPPEL",5) == 0
	      || (strcmp(token_array[ii].text,"RULES") == 0 && strncmp(token_array[ii+2].text,"REGULATIONS",4) == 0)
	      || strncmp(token_array[ii].text,"CERTIFICATE",5) == 0	      	      
	      || strncmp(token_array[ii].text,"LETTER",4) == 0
	      || strncmp(token_array[ii].text,"INVOICE",4) == 0
	      || strncmp(token_array[ii].text,"COMMISSION",4) == 0	      	      
	      //|| (strcmp(token_array[ii].text,"RE") == 0 && strcmp(token_array[ii].text,":") == 0)
	      )  {
	      if (debug) fprintf(stderr,"FOUND LEASE_TITLE_WORD:pl=%d:%d: text=%s: total=%d: now=%d:\n"
		      ,prev_page_no,  prev_line_no,  token_array[ii].text
				 , page_line_properties_array[prev_page_no][prev_line_no].lease_title_no_of_words
				 , page_line_properties_array[prev_page_no][prev_line_no+1].no_of_words);
	      
	      page_line_properties_array[prev_page_no][prev_line_no].lease_title_no_of_words++;
	    }

	  if (0
	      || (strcmp(token_array[ii].text,"Lease") == 0
		  && ((ii <= 0) || strcasecmp(token_array[ii-1].text,"of") != 0) 
		  && strcmp(token_array[ii+1].text,"Summary") != 0
		  && strcmp(token_array[ii+1].text,"Administrator") != 0		  
		  && strcmp(token_array[ii+1].text,"Data") != 0
		  && strcmp(token_array[ii+1].text,"Terms") != 0
		  && strcmp(token_array[ii+1].text,"Provisions") != 0
		  && strcmp(token_array[ii+1].text,"Restrictions") != 0		  		  		  
		  && strcmp(token_array[ii+1].text,"Face") != 0
		  && strcmp(token_array[ii+1].text,"Year") != 0		  		  		  
		  && strcmp(token_array[ii+1].text,"Information") != 0) 
	      || strcmp(token_array[ii].text,"Agreement") == 0
	      || strncmp(token_array[ii].text,"Joinder",7) == 0	      	      
	      || strcmp(token_array[ii].text,"Sublease") == 0
	      || strcmp(token_array[ii].text,"Acknowledgement") == 0	      
	      || strcmp(token_array[ii].text,"Amendment") == 0
	      || strncmp(token_array[ii].text,"Renewal",7) == 0	      
	      || strcmp(token_array[ii].text,"Extension") == 0
	      || strcmp(token_array[ii].text,"Addendum") == 0
	      || strcmp(token_array[ii].text,"Guaranty") == 0
	      || strcmp(token_array[ii].text,"Assignment") == 0
	      || strcmp(token_array[ii].text,"Assumption") == 0	      
	      || strcmp(token_array[ii].text,"Commencement") == 0
	      || strcmp(token_array[ii].text,"Modification") == 0	      	      
	      || strcmp(token_array[ii].text,"Memorandum") == 0
	      || strcmp(token_array[ii].text,"Estoppel") == 0
	      || strcmp(token_array[ii].text,"Certificate") == 0	      	      
	      || strcmp(token_array[ii].text,"Letter") == 0)
	    {
	      page_line_properties_array[prev_page_no][prev_line_no].lease_title_no_of_words +=1;
	      if (debug) fprintf(stderr,"FOUND LEASE_TITLE_Cap_WORD:pl=%d:%d: text=%s: total=%d: now=%d:\n"
		      ,prev_page_no,  prev_line_no,  token_array[ii].text
				 , page_line_properties_array[prev_page_no][prev_line_no].lease_title_no_of_words
				 , page_line_properties_array[prev_page_no][prev_line_no+1].no_of_words);
	      
	    }

	}

	prev_ii = ii;
      }
    } // nothing

  } // FOR TOKEN II

  for (ii = 0; ii <= page_no; ii++) {
    cluster_lines_by_center_x(ii);
    if (debug) fprintf(stderr,"PERPAGEOO: page=%d: lines=%d: clines=%d: words=%d: Words=%d: TPW=%d: SBW=%d: LI=%d: VS=%d:\n"
	    , ii
	    , page_array[ii].no_of_lines
	    , page_array[ii].no_of_centered_lines
	    , page_array[ii].no_of_words
	    , page_array[ii].no_of_first_cap_words
	    , page_array[ii].no_of_title_page_words
	    , page_array[ii].no_of_sig_block_words		       
	    , page_array[ii].no_of_LIs
	    , page_array[ii].no_of_VSs	    
	    );
    int jj;
    for (jj = 0; jj < 4; jj++) {
      if (debug) fprintf(stderr,"  PAGE_LINE_PROPERTIES_ARRAY:jj=%d: nw=%d: nW=%d: NW=%d: cen=%d: exhib=%d:\n", jj, page_line_properties_array[ii][jj].no_of_words
	      , page_line_properties_array[ii][jj].no_of_first_cap_words, page_line_properties_array[ii][jj].no_of_all_cap_words
	      , page_line_properties_array[ii][jj].center, page_line_properties_array[ii][jj].left_word_is_exhibit);
    }
  }

  return 0;
} // calc_page_stats()


int get_vertical_spaces_per_page(int doc_id) {
  char query[10000];
  sprintf(query,"select page_no, id \n\
                from deals_verticalspacetoken \n\
                where  \n\
                   doc_id = '%d' and source_program='%s'"
	  , doc_id, source_ocr);

    if (debug) fprintf(stderr,"QUERY8=%s\n",query);    
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY8=%s\n",query);    
    }

    sql_res = mysql_store_result(conn);
    while (sql_row = mysql_fetch_row(sql_res)) {
      int page_id = atoi(sql_row[0]);
      page_array[page_id+1].no_of_VSs++;
    }
    return 0;
}

int get_paragraphtoken(int doc_id) {
  char query[10000];
  sprintf(query,"select para_no, token_id \n\
                from deals_paragraphtoken \n\
                where  \n\
                   doc_id = '%d' and source_program='%s' "
	  , doc_id, source_ocr);

    if (debug) fprintf(stderr,"QUERY84=%s\n",query);    
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY84=%s\n",query);    
    }

    sql_res = mysql_store_result(conn);
    while (sql_row = mysql_fetch_row(sql_res)) {
      int para_no = atoi(sql_row[0]);
      int token_id;
      Paragraph2Token_array[para_no] = token_id = atoi(sql_row[1]);
      //fprintf(stderr,"  P2T:%d:%d:\n",para_no,token_id);
    }
    return 0;
}

 
int delete_entries_from_sql(int doc_id) {
  sprintf(query,"delete \n\
                     from deals_page2summary_points \n\
                     where \n\
                       doc_id = %d ",
	  doc_id
	  );

  if (debug) fprintf(stderr,"QUERY770 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY770 (%s): query=%s:\n",prog,query);
  }

  sprintf(query,"delete \n\
                     from deals_page_line_properties \n\
                     where \n\
                       doc_id = %d ",
	  doc_id
	  );

  if (debug) fprintf(stderr,"QUERY771 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY771 (%s): query=%s:\n",prog,query);
  }

  return 0;
}


int insert_page2summary_array_into_sql(int page_no) {
  if (debug) fprintf(stderr,"INSERTING SQL:%d:\n",page_no);
   static char buff[5000];
   static char query[60000];
   sprintf(query,"insert into deals_page2summary_points \n\
           (doc_id, page_no, no_of_points, no_of_chars, toc_score, new_toc_score, summary_score, title_page_score, new_title_page_score, exhibit_on_first_lines \n\
           , exhibit_score, no_of_lines, no_of_words, no_of_CWords, no_of_junk_words, no_of_dict_words, no_of_title_page_words, no_of_sig_block_words, no_of_LIs, no_of_VSs \n\
           , right_indentation_cluster \n\
           , sig_block_page_score, sig_block_para \n\
           , recitals_words \n\
           , recitals_score, recitals_start_para, recitals_end_para \n\
           , lease_title_score, lease_title_para, lease_title_line, lease_title_no_of_lines) \n\
           values \n "); 
   int ii;
   for (ii = 0; ii < page_no; ii++) {
     sprintf(buff,"(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,         %d, %d,    '%s',        %d, %d, %d,         %d, %d, %d, %d),\n "
	     , doc_id
	     , ii+1 // the page_no
	     , page_array[ii].no_of_points
	     , page_array[ii].no_of_chars
	     , page_array[ii].toc_score
	     , page_array[ii].new_toc_score	     
	     , page_array[ii].summary_score
	     , page_array[ii+1].title_page_score
	     , page_array[ii+1].new_title_page_score	     
	     , page_array[ii+1].exhibit_on_first_lines
	     , page_array[ii+1].exhibit_score	     
	     , page_array[ii+1].no_of_lines
	     , page_array[ii+1].no_of_words
	     , page_array[ii+1].no_of_first_cap_words
	     , page_array[ii+1].no_of_junk_words
	     , page_array[ii+1].no_of_dict_words	     	     
	     , page_array[ii+1].no_of_title_page_words
	     , page_array[ii+1].no_of_sig_block_words	     
	     , page_array[ii+1].no_of_LIs
	     , page_array[ii+1].no_of_VSs
	     , page_array[ii+1].right_indentation_cluster

	     , page_array[ii+1].sig_block_page_score
	     , page_array[ii].sig_block_para

	     , "BB" //page_array[ii].recitals_words	     

	     , page_array[ii].recitals_score
	     , page_array[ii].recitals_start_para
	     , page_array[ii].recitals_end_para
	     
	     , page_array[ii-1].lease_title_score
	     , page_array[ii-1].lease_title_para
	     , page_array[ii-1].lease_title_line 
	     , page_array[ii-1].lease_title_no_of_lines
	     );

	     
     strcat(query,buff);
   }

   remove_last_comma(query);
   if (debug) fprintf(stderr,"QUERY51=%s\n",query);
   if (mysql_query(conn, query)) {
     fprintf(stderr,"%s\n",mysql_error(conn));
     fprintf(stderr,"QUERY51=%s\n",query);
   }
  return 0;
}  //  insert_page2summary_array_into_sql(int page_no) {


int insert_page_line_properties_into_sql(int page_no) {
  if (debug) fprintf(stderr,"INSERTING PAGE_LINE SQL:%d:\n",page_no);
   static char buff[5000];
   static char query[600000];
   sprintf(query,"insert into deals_page_line_properties \n\
           (doc_id, page_no, line_no, center, no_of_words, no_of_first_cap_words, no_of_all_cap_words, left_X, right_X, line_right_is_number, lease_title_no_of_words, source_program) \n\
           values \n "); 
   int ii, jj;

   for (ii = 1; ii <= page_no; ii++) {
     if (debug) fprintf(stderr,"     INSERTING PAGE SQL:%d:%d:%d: 2/0:%d:%d:\n",ii,page_array[ii].no_of_lines, MAX_LINE,   page_line_properties_array[2][0].left_X, page_line_properties_array[2][0].right_X);
     if (page_array[ii].no_of_lines < MAX_LINE) {
       for (jj = 0; jj <= page_array[ii].no_of_lines && jj < MAX_LINE; jj++) {     
	 if (debug) fprintf(stderr,"              INSERTING LINE SQL:pg=%d:  ln=%d: M_L%d:\n",ii,jj, MAX_LINE);
	 sprintf(buff,"(%d, %d, %d, %d, %d, %d, %d,   %d, %d   , %d, %d, '%s'),\n "
		 , doc_id
		 , ii
		 , jj
		 // the values in the PERLINE etl are one behind and must be jumped one up
		 , /*(jj+1 == page_array[ii].no_of_lines) ? page_line_properties_array[ii+1][0].center : page_line_properties_array[ii][jj+1].center */ page_line_properties_array[ii][jj].center
		 , page_line_properties_array[ii][jj].no_of_words
		 , page_line_properties_array[ii][jj].no_of_first_cap_words
		 , page_line_properties_array[ii][jj].no_of_all_cap_words
		 , /* (jj+1 == page_array[ii].no_of_lines) ? page_line_properties_array[ii+1][0].left_X : page_line_properties_array[ii][jj+1].left_X */ page_line_properties_array[ii][jj].left_X
		 //, page_line_properties_array[ii+1][0].left_X , page_line_properties_array[ii][jj+1].left_X		 
		 , /* (jj+1 == page_array[ii].no_of_lines) ? page_line_properties_array[ii+1][0].right_X : page_line_properties_array[ii][jj+1].right_X */ page_line_properties_array[ii][jj].right_X
		 //, page_line_properties_array[ii+1][0].right_X , page_line_properties_array[ii][jj+1].right_X		 
		 , page_line_properties_array[ii][jj].line_right_is_number
		 , page_line_properties_array[ii][jj].lease_title_no_of_words
		 , "AWS"
		 );
	 strcat(query,buff);

       } // for jj line
     }
   } // for ii page
   remove_last_comma(query);
   if (debug) fprintf(stderr,"QUERY52=%s\n",query);
   if (mysql_query(conn, query)) {
     fprintf(stderr,"%s\n",mysql_error(conn));
     fprintf(stderr,"QUERY52=%s\n",query);
   }
  return 0;
}  //  insert_page_line_properties_into_sql(int page_no) 



#define MAX_CHAR_PER_PAGE 20000
char curr_page_buff[MAX_PAGE][MAX_CHAR_PER_PAGE];
int curr_page_ptr[MAX_PAGE];
 
int copy_into_curr_page_buff(char curr_page_buff[], int curr_page_ptr, char text[]) {
  strcpy(curr_page_buff+curr_page_ptr,text);  
  return curr_page_ptr+strlen(text);
}

int line2para_array[MAX_PAGE][MAX_LINE]; // all cells are all filled in

int calc_lease_title_score(int no_of_pages) {
  int pp;
  char *text;
  for (pp = 0; pp <= no_of_pages+1; pp++) {
    int PP = pp+1;
    //fprintf(stderr,"PP0=%d:%d: nol=%d::\n",PP,pp, page_array[PP].no_of_lines);
    int no_of_lines = page_array[PP].no_of_lines;
    int total_score = 0;
    if (no_of_lines > 0) {
      //fprintf(stderr,"PP1=%d:%d: nol=%d::\n",PP,pp, page_array[PP].no_of_lines);      
      int no_of_centered_lines = 0;
      int no_of_cap_lines = 0;
      int no_of_long_lines = 0; // lines longer than 8
      //title_page_words  from above;

      int ll;
      int no_of_real_lines = 0;
      for (ll = 0; ll < no_of_lines && ll < 5; ll++) { // must be at the top of the page
	//fprintf(stderr,"  PP2 p=%d: l=%d: nol=%d::\n",PP,ll, page_array[PP].no_of_lines);	
	int ii;

	int done = 0;
	int LL = ll;
	
	for (ii = 0
	       ; (done == 0
		  && (token_array[ii].page_no <= PP
		      || (token_array[ii].page_no == PP && token_array[ii].line_no <= ll))
		  && ii < token_no)
	       ; ii++) {
	  if (token_array[ii].page_no == PP
	      && token_array[ii].line_no == ll) {
	    text = token_array[ii].text;
	    done = 1;
	  }
	}

	if (1 || page_line_properties_array[PP][LL].no_of_chars > 3) { // to eliminate noise lines, lines that include only junk chars
	  int score = 0;
	  if (page_line_properties_array[PP][LL].lease_title_no_of_words == 0 || page_line_properties_array[PP][LL].no_of_words > 7) {
	    score = 0;
	  } else {
	    score += 10 * page_line_properties_array[PP][LL].lease_title_no_of_words;
	    score += 10 * ((page_line_properties_array[PP][LL+1].center > 285000    // add points if center is good
			    && page_line_properties_array[PP][LL+1].center < 315000) ? 1 : 0);
	    score -= 10 * (page_line_properties_array[PP][LL+1].center < 140000 ? 1: 0); // take out points for really bad center
			    

	    score += 10 * ((page_line_properties_array[PP][LL+1].left_X > 100000) ? 1 : 0);
	    score += 10 * (((page_line_properties_array[PP][LL].no_of_words * 2 < page_line_properties_array[PP][LL].no_of_all_cap_words * 3) // most words are upper
			    || (page_line_properties_array[PP][LL].no_of_words * 2 < page_line_properties_array[PP][LL].no_of_first_cap_words * 3)) ? 1 : 0);
	  }
	  if (debug) fprintf(stderr,"TRYING LEASE_TITLE: t=%d: :%s: twn=%d: now=%d: cen=%d: lx=%d: cap=%d:%d:pl=%d:%d: now=%d: SCORE=%d:\n"
		  , ii , text
		  , page_line_properties_array[PP][LL].lease_title_no_of_words
		  , page_line_properties_array[PP][LL].no_of_words
		  , page_line_properties_array[PP][LL+1].center
		  , page_line_properties_array[PP][LL+1].left_X
		  , page_line_properties_array[PP][LL].no_of_all_cap_words
		  , page_line_properties_array[PP][LL].no_of_first_cap_words
		  , PP, LL
		  , page_line_properties_array[PP][LL].no_of_words
		  , score
		  );

	  if (page_line_properties_array[PP][LL].lease_title_no_of_words > 0 // it can be simply "Sublease"
	      && ((page_line_properties_array[PP][LL].no_of_words < 8
		   && page_line_properties_array[PP][LL+1].center > 285000
		   && page_line_properties_array[PP][LL+1].center < 315000 
		   && page_line_properties_array[PP][LL+1].left_X > 100000
		   
		   && ((page_line_properties_array[PP][LL].no_of_words * 2 < page_line_properties_array[PP][LL].no_of_all_cap_words * 3) // most words are upper
		       || (page_line_properties_array[PP][LL].no_of_words * 2 < page_line_properties_array[PP][LL].no_of_first_cap_words * 3)))
		  )
	      || score >= 22 ) { // SHIRAN_ONLY , now doing many items

	    int PPP = PP-2;
	    // in case the previous line is also a header then length is 2
	    if (page_array[PPP].lease_title_line != LL - 1 || page_array[PPP].lease_title_score < 30) {
	      page_array[PPP].lease_title_no_of_lines = 1 ;
	    } else {
	      page_array[PPP].lease_title_no_of_lines = 2;
	    }
	    // in case the previous line is also a header then keep it
	    if (page_array[PPP].lease_title_line != LL - 1 || page_array[PPP].lease_title_score < 30) page_array[PPP].lease_title_line = LL;
	    page_array[PPP].lease_title_score += score;
	    page_array[PPP].lease_title_para = line2para_array[PP][LL] - (page_array[PPP].lease_title_no_of_lines - 1);

	    if (debug) fprintf(stderr,"       SUCKING LEASE_TITLE: t=%s: score=%d: page=%d: line=%d: para1=%d: nol=%d: ltp=%d:\n"
			       , text
			       , page_array[PPP].lease_title_score
			       , PP, LL
			       , line2para_array[PP][LL]
			       , page_array[PPP].lease_title_no_of_lines
			       , page_array[PPP].lease_title_para
			       );
	  }
	} // if line is OK
      } // for lines LL
    }
  } // for pages pp
  return 0;
} // calc_lease_title_score()


int calc_sig_block_score(int no_of_pages) {
  int pp;
  for (pp = 0; pp <= no_of_pages; pp++) {
    int no_of_lines = page_array[pp].no_of_lines;
    int total_score = 0;
    if (no_of_lines > 0) {
      int no_of_centered_lines = 0;
      int no_of_cap_lines = 0;
      int no_of_long_lines = 0; // lines longer than 8
      //title_page_words  from above;

      int ll;
      int no_of_real_lines = 0;
      for (ll = 0; ll < no_of_lines; ll++) {
	if (page_line_properties_array[pp][ll].no_of_chars > 2) { // to eliminate noise lines, lines that include only junk chars
	  //if (page_line_properties_array[pp][ll].no_of_chars > 2) {
	  if (page_line_properties_array[pp][ll+1].center > 285000
	      && page_line_properties_array[pp][ll+1].center < 315000 
	      && page_line_properties_array[pp][ll+1].left_X > 100000) {
	    no_of_centered_lines++;
	  }
	  if (page_line_properties_array[pp][ll].no_of_words > 9) {
	    no_of_long_lines++;
	  }
	  if ((page_line_properties_array[pp][ll].no_of_words * 2 < page_line_properties_array[pp][ll].no_of_all_cap_words * 3)
	      || (page_line_properties_array[pp][ll].no_of_words * 2 < page_line_properties_array[pp][ll].no_of_first_cap_words * 3)) {
	    no_of_cap_lines++;
	  }
	  no_of_real_lines++;
	}
      } // for lines ll
      total_score = 0;
      //total_score -= ((no_of_real_lines < 15) ? 0 : (no_of_real_lines - 15)) * 5;  // can't be longer than 15 lines
      int mm;
      //total_score -= mm = ((no_of_centered_lines - no_of_real_lines) / (no_of_real_lines +2)) * 5 ;

      //total_score -= ((no_of_long_lines) / (no_of_real_lines +2)) * 10;     -- we don't know the no of line UNDER the SIG_BLOCK PARA -- what's the line_no of the para???
      total_score += page_array[pp-1].by_words * 10;
      total_score += ((page_array[pp-1].PLAYER_words <= 4) ? page_array[pp-1].PLAYER_words : 8 - page_array[pp-1].PLAYER_words) * 10;
      total_score += page_array[pp-1].WITNESS_words * 150;
      total_score += ((page_array[pp-1].name_title_date_words < 10) ? page_array[pp-1].name_title_date_words : (10 - page_array[pp-1].name_title_date_words)) * 5;
      total_score += page_array[pp-1].sig_page_follows_words * 100;                        
	
      if (debug) fprintf(stderr,"SIG_BLOCK_SCORE: pp=%d: total=%d: no_of_real_lines=%d:%d: player=%d:%d: wt=%d:%d: nm=%d:%d: sg=%d:%d: center:%d:%d:%d:\n"
			 , pp
			 , total_score  
			 , no_of_real_lines, ((no_of_real_lines < 15) ? 0 : (no_of_real_lines - 15))
			 , page_array[pp-1].PLAYER_words, ((page_array[pp-1].PLAYER_words <= 4) ? page_array[pp-1].PLAYER_words : 8 - page_array[pp-1].PLAYER_words) * 10
			 , page_array[pp-1].WITNESS_words, page_array[pp-1].WITNESS_words * 100
			 , page_array[pp-1].name_title_date_words, ((page_array[pp-1].name_title_date_words < 10) ? page_array[pp-1].name_title_date_words : (10 - page_array[pp-1].name_title_date_words)) * 5
                         , page_array[pp-1].sig_page_follows_words, page_array[pp-1].sig_page_follows_words * 100
			 , no_of_centered_lines, (((no_of_centered_lines - no_of_real_lines) * 100) / (no_of_real_lines +2)) * 5 , mm
			 );
    } else {
      total_score = -1000;
      if (debug) fprintf(stderr,"SIG_BLOCK_SCORE:%d:%d: \n"
	      , pp,total_score
	      );
    }
    page_array[pp].sig_block_page_score = total_score;
  } // for pages pp
  return 0;
} // calc_sig_block_score()



 
int new_calc_title_page_score(int no_of_pages) {
  int pp;
  for (pp = 0; pp <= no_of_pages; pp++) {
    int no_of_lines = page_array[pp].no_of_lines;
    int no_of_words = page_array[pp].no_of_words;
    int no_of_sig_block_words = page_array[pp].no_of_sig_block_words;
    int total_score = 0;
    if (no_of_lines > 0) {
      int no_of_centered_lines = 0;
      int no_of_cap_lines = 0;
      int no_of_long_lines = 0; // lines longer than 8
      //title_page_words  from above;

      int ll;
      int no_of_real_lines = 0;
      for (ll = 0; ll < no_of_lines; ll++) {
	if (page_line_properties_array[pp][ll].no_of_chars > 2) { // to eliminate noise lines, lines that include only junk chars
	  //if (page_line_properties_array[pp][ll].no_of_chars > 2) {
	  if ((page_line_properties_array[pp][ll].center > 285000 && page_line_properties_array[pp][ll].center < 315000 && page_line_properties_array[pp][ll].left_X > 100000)
	      || ((page_line_properties_array[pp][ll].center > 275000 && page_line_properties_array[pp][ll].center < 535000) // more relaxed conditions
		  && (page_line_properties_array[pp][ll].left_X > 120000 && page_line_properties_array[pp][ll].left_X < 600000)) 
	       ) {
	    no_of_centered_lines++;
	  }
	  if (page_line_properties_array[pp][ll].no_of_words > 9) {
	    no_of_long_lines++;
	  }
	  if ((page_line_properties_array[pp][ll].no_of_words * 2 < page_line_properties_array[pp][ll].no_of_all_cap_words * 3)
	      || (page_line_properties_array[pp][ll].no_of_words * 2 < page_line_properties_array[pp][ll].no_of_first_cap_words * 3)) {
	    no_of_cap_lines++;
	  }
	  no_of_real_lines++;
	}
      } // for lines ll
      int real_lines, centered_lines, long_lines, sig_block_words,cap_lines;
      total_score = 0;
      total_score += real_lines = (no_of_real_lines < 11) ? 0 : (11 - no_of_real_lines) * 5;
      total_score += 0; //(page_array[pp].title_page_words *1000 / (page_array[pp].no_of_words+1) ) * 10;
      total_score += centered_lines = ((5 *(no_of_centered_lines - no_of_real_lines)) / (no_of_real_lines +2)) ;
      total_score -= long_lines = ((10 *no_of_long_lines) / (no_of_real_lines +2)) ;
      total_score -= sig_block_words = ((10 * no_of_sig_block_words) / (no_of_words +2)) ;          
      total_score += cap_lines = ((5*(no_of_cap_lines - no_of_real_lines)) / (no_of_real_lines +2));

      if (debug) fprintf(stderr,"NEW_TITLE_PAGE_SCORE: pp=%d: total=%d: no_of_real_lines=%d:%d: long=%d:%d: words=%d:%d: center:%d:%d: cap=%d:%d: sbw=%d:%d:%d:\n"
			 , pp, total_score  
			 , no_of_real_lines, real_lines
			 , no_of_long_lines, long_lines
			 , 0 ,0 //page_array[pp].title_page_words *1000 / (page_array[pp].no_of_words+1),  (page_array[pp].title_page_words * 1000 / (page_array[pp].no_of_words+1)) 
			 , no_of_centered_lines, centered_lines
			 , no_of_cap_lines, cap_lines
			 , no_of_sig_block_words, no_of_words,  sig_block_words
			 );
    } else {
      total_score = -1000;
      if (debug) fprintf(stderr,"NEW_TITLE_PAGE_SCORE:%d:%d: \n"
	      , pp,total_score
	      );
    }
    page_array[pp].new_title_page_score = total_score + 10;
  } // for pages pp
  return 0;
} // new_calc_title_page_score(int no_of_pages) 


int new_calc_toc_score(int no_of_pages) {
  int pp;
  for (pp = 0; pp <= no_of_pages; pp++) {
    int no_of_lines = page_array[pp].no_of_lines;
    int no_of_page_ref_lines  = 0;
    int total_score = 0;
    int no_of_centered_lines = 0;
    int no_of_cap_lines = 0;
    int no_of_long_lines = 0; // lines longer than 8
    int no_of_toc_item_lines = 0;
      //title_page_words  from above;

    if (no_of_lines > 0) {
      int ll;
      for (ll = 0; ll < no_of_lines; ll++) {
	if (page_line_properties_array[pp][ll].center > 285000
	    && page_line_properties_array[pp][ll].center < 315000 
	    && page_line_properties_array[pp][ll].left_X > 100000) {
	  no_of_centered_lines++;
	}

	if (page_line_properties_array[pp][ll].no_of_words > 9) {
	  no_of_long_lines++;
	}


	no_of_toc_item_lines += page_line_properties_array[pp][ll].toc_item;
	no_of_page_ref_lines += page_line_properties_array[pp][ll].page_ref;
	
	if (0 //(page_line_properties_array[pp][ll].no_of_words  < page_line_properties_array[pp][ll].no_of_all_cap_words * 3)
	    || (page_line_properties_array[pp][ll].no_of_words * 2  < page_line_properties_array[pp][ll].no_of_first_cap_words * 3)) {
	  no_of_cap_lines++;
	}
	if (0) fprintf(stderr,"NEW_PPP_SCORE: pp=%d: line=%d: nocl=%d: now=%d: cCap=%d:%d:\n"
		, pp ,ll ,no_of_cap_lines
		, page_line_properties_array[pp][ll].no_of_words, page_line_properties_array[pp][ll].no_of_all_cap_words, page_line_properties_array[pp][ll].no_of_first_cap_words);
      } // for lines ll
      total_score = 0;
      total_score -= (no_of_centered_lines * 100 / (no_of_lines +2));
      total_score -= (no_of_long_lines * 100 / (no_of_lines +2));
      total_score += (no_of_cap_lines * 100 / (no_of_lines +2));
      total_score += (no_of_toc_item_lines * 200 / (no_of_lines + 2));
      total_score += (no_of_page_ref_lines * 100 / (no_of_lines + 2));
      total_score += page_array[pp-1].toc_title_found * 150;      
      
      if (debug) fprintf(stderr,"NEW_TOC_SCORE: pp=%d: total=%d: no_of_lines=%d: long=%d:%d: center:%d:%d: cap=%d:%d: toc_item=%d:%d:  page_ref=%d:%d: toc_title=%d:%d:\n\n"
	      , pp, total_score, no_of_lines
	      , no_of_long_lines, (no_of_long_lines * 100 / (no_of_lines + 2)) 
	      , no_of_centered_lines, (no_of_centered_lines * 100 / (no_of_lines +2)) 
	      , no_of_cap_lines,(no_of_cap_lines * 100 / (no_of_lines +2)) 
	      , no_of_toc_item_lines, (no_of_toc_item_lines * 100 / (no_of_lines + 2)) 
	      , no_of_page_ref_lines, (no_of_page_ref_lines * 100 / (no_of_lines + 2))
	      , page_array[pp-1].toc_title_found, page_array[pp-1].toc_title_found * 150
	      );
    } else {
      total_score = -1000;
      if (debug) fprintf(stderr,"NEW_TOC_SCORE:%d:%d: \n"
	      , pp,total_score
	      );
    }
    page_array[pp-1].new_toc_score = total_score - 100; //  to remove bias; now 0 is the sea level line
  } // for pages pp
  return 0;
} // new_calc_toc_score(int no_of_pages) 

%}
pp "<"[Pp]*[^\>]*\>
tag \<[^\>]*\> 
ws [\ \n]*
ws1 [\ \n\-\:\,\.\']*

title_page_words lease|amendment|guaranty|invoice|agreement|addendum|between|l(\.)?l(\.)?c|d(\/)?b(\/)?a|tenant|landlord|lessor|lessee|guarantor|dated|as
Title_Page_Words Lease|Amendment|Guaranty|Agreement|Invoice|Addendum|Between|l(\.)?l(\.)?c|d(\/)?b(\/)?a|Tenant|Landlord|Lessor|Lessee|Guarantor|Dated|As
TITLE_PAGE_WORDS LEASE|AMENDMENT|GUARANTY|AGREEMENT|INVOICE|ADDENDUM|BETWEEN|L(\.)?L(\.)?C|D(\/)?B(\/)?A|TENANT|LANDLORD|LESSOR|LESSEE|GUARANTOR|DATED|AS

Sig_Block_Words Name|Date|Title|By
SIG_BLOCK_WORDS NAME|DATE|TITLE|BY
  

players (landlord|lessor|lessee|tenant|agent|broker|sublessor|sublessee|sublandlord|subtenant|mortgagee|morgagor|buyer|reseller|seller|licensor|licensee|employer|employee|owner|licensee|licensor|lender|borrower|guarantor|assignor|assignee|declarant|reinsurer|customer|representative|supplier|distributor|witness|attest|property|building)(e)?({ws}[\']*(s|"(s)"))?
PLAYERS (LANDLORD|LESSOR|LESSEE|TENANT|AGENT|BROKER|SUBLESSOR|SUBLESSEE|SUBLANDLORD|SUBTENANT|MORTGAGEE|MORGAGOR|BUYER|RESELLER|SELLER|LICENSOR|LICENSEE|EMPLOYER|EMPLOYEE|OWNER|LICENSEE|LICENSOR|LENDER|BORROWER|GUARANTOR|ASSIGNOR|ASSIGNEE|DECLARANT|REINSURER|CUSTOMER|REPRESENTATIVE|SUPPLIER|DISTRIBUTOR|WITNESS|ATTEST|PROPERTY|BUILDING)(E)?({ws}[\']*(S|"(S)"))?  
%%

"<p"[^\>]*\> {
  static int old_gpid = -1;
  char *pp = strstr(yytext,"id=\"");
  if (pp) {
    sscanf(pp, "id=\"%d",&gpid);
  }
  if (para_no_on_page > 0) { // we do not want to count chars from the old page
    page_array[hr_page_no].no_of_chars += gp_count;
  }
  int token_id = Paragraph2Token_array[gpid];
  
  static int old_para_line_no = 0;
  para_line_no = token_array[token_id].line_no; // the line_no of empty paras is 0 and s/b the same line as before
  if (para_line_no == 0 && gpid == old_gpid) {
    para_line_no = old_para_line_no;
  } else {
    old_para_line_no = old_para_line_no;
  }
  int ii;
  //fprintf(stderr,"        QQQQQQQQQQQQ: pn=%d: token=%d: line=%d: pid=%d:\n",hr_page_no+1, token_id, para_line_no, gpid);
  for (ii = para_line_no; ii < para_line_no+1; ii++) { // so that all cells are filled in, not just the first line in the para
    if (line2para_array[hr_page_no+1][ii] == 0)  line2para_array[hr_page_no+1][ii] = gpid;
    //fprintf(stderr,"                  QQQQQQQQQQQQ: pn=%d: line=%d: pid=%d:\n",hr_page_no+1, ii, line2para_array[hr_page_no+1][ii]);
  }
  int para_page_no = token_array[token_id].page_no;  

  para_no_on_page++;
  if (old_gpid >= 0) pid_stats_array[old_gpid].len = gp_count; // it belongs to the old para
  //fprintf(stderr,"MMMM p=%d: c=%d: pp=%.*s\n", old_gpid, gp_count, 100, pp);

  pid_stats_array[gpid].page_no = hr_page_no; // this is for current gpid
  old_gpid = gpid;
  BEGIN snew_para;
  gp_count = 0;
  curr_page_ptr[hr_page_no] = copy_into_curr_page_buff(curr_page_buff[hr_page_no], curr_page_ptr[hr_page_no], yytext);
  //fprintf(stderr,"                  MMMMMMMM: pn=%d: line=%d: pid=%d:\n",1, 3, line2para_array[1][3]);  
}

<snew_para>(Exhibit|Schedule|Section|Article|EXHIBIT|SCHEDULE|SECTION|ARTICLE|[0-9]+\.|[0-9]+(\.[0-9]+)+|[a-zA-Z]\.|(\([A-Za-z0-9]{1,2}\))) { 
  pid_stats_array[gpid].toc_item = strdup(yytext);
  pid_stats_array[gpid].int_toc_item = (yytext) ? atoi(yytext) : -1;
  page_line_properties_array[hr_page_no+1][para_line_no].toc_item = 1;
  BEGIN 0;
  yyless(0);
}

<snew_para>[ \n] {
  curr_page_ptr[hr_page_no] = copy_into_curr_page_buff(curr_page_buff[hr_page_no], curr_page_ptr[hr_page_no], yytext);  
}

<snew_para>. {
  BEGIN 0;
  yyless(0);
}

<snew_para>"</p>" {
  BEGIN 0;
  yyless(0);
}


      /*  page number (1.03) at start of line */
<snew_para>[\ \t\n]*[0-9LlIO]{1,2}[\.\,][0-9LlIO]{1,2}[^A-Za-z0-9] {
  char *tmp = my_copy(yytext);
  pid_stats_array[gpid].toc_item = strdup(tmp);
  pid_stats_array[gpid].int_toc_item = (yytext) ? atoi(yytext) : -1;  
  page_line_properties_array[hr_page_no+1][para_line_no].toc_item = 1;  
  //fprintf(stderr,"  GGG:pid=%d:  toc_int=%d: text=%s:\n",gpid,   pid_stats_array[gpid].int_toc_item, pid_stats_array[gpid].toc_item);
  BEGIN 0;
  yyless(0);
}

      /*  NEW PAGE */
<*>("<HR"[^\>]*\>|"<hr"[^\>]*\>) { 
  page_array[hr_page_no].no_of_chars += gp_count; // finish the count of the last para in the prev page
  char *ptr = strstr(yytext, "pn=");
  if (ptr) {
    sscanf(ptr, "pn=%d", &hr_page_no);
    hr_page_no--;  // since now HR starts counting from 1
  }
  //  fprintf(stderr,"HHHHR:%d:%s:\n",hr_page_no, ptr);
  page_array[hr_page_no].no_of_chars = 0;
  para_no_on_page = 0;
  curr_page_ptr[hr_page_no] = 0;
  curr_page_ptr[hr_page_no] = copy_into_curr_page_buff(curr_page_buff[hr_page_no], curr_page_ptr[hr_page_no], yytext);  
  //fprintf(stderr,"FOUND HR:%d:%d:\n",hr_page_no,pp);
  page_array[hr_page_no].title_page_words = 0;
  page_array[hr_page_no].sig_block_words = 0;  
}

      /*  page number at end of line */
[\ \n][0-9]{1,3}{ws}"</p>" {
  char *tmp = my_copy(yytext+1);
  pid_stats_array[gpid].page_ref = strdup(tmp);
  page_line_properties_array[hr_page_no+1][para_line_no].page_ref = 1;  

  pid_stats_array[gpid].int_page_ref = (tmp) ? atoi(tmp) : -1;
  gp_count += strlen(tmp);
  curr_page_ptr[hr_page_no] = copy_into_curr_page_buff(curr_page_buff[hr_page_no], curr_page_ptr[hr_page_no], yytext);  
}

      /*  page number (roman) at end of line */
{ws}[ivx]{1,4}{ws}"</p>"  {
  char *tmp = my_copy(yytext+1);
  pid_stats_array[gpid].page_ref = strdup(tmp);
  page_line_properties_array[hr_page_no+1][para_line_no].page_ref = 1;    
  gp_count += strlen(tmp);
  curr_page_ptr[hr_page_no] = copy_into_curr_page_buff(curr_page_buff[hr_page_no], curr_page_ptr[hr_page_no], yytext);  
}

<*>{tag}   curr_page_ptr[hr_page_no] = copy_into_curr_page_buff(curr_page_buff[hr_page_no], curr_page_ptr[hr_page_no], yytext);

<*>([\ ]|\n)   curr_page_ptr[hr_page_no] = copy_into_curr_page_buff(curr_page_buff[hr_page_no], curr_page_ptr[hr_page_no], yytext);

.  {
     gp_count++;
     curr_page_ptr[hr_page_no] = copy_into_curr_page_buff(curr_page_buff[hr_page_no], curr_page_ptr[hr_page_no], yytext);
}

          /* RECITALS */
(R(ECITALS|ecitals)|W(ITNESSETH|itnesseth)|BACKGROUND|B{ws}A{ws}C{ws}K{ws}G{ws}R{ws}O{ws}U{ws}N{ws}D|Background|W{ws}I{ws}T{ws}N{ws}E{ws}S{ws}S{ws}E{ws}T{ws}H|W(HEREAS|hereas)|N{ws}O{ws}W{ws1}T{ws}H{ws}E{ws}R{ws}E{ws}F{ws}O{ws}R{ws}E|N(OW|ow){ws1}T(HEREFORE|herefore)|(agree{ws}as{ws}follows)) {
  gp_count += yyleng;
  if (1 || para_no_on_page < 5) {
    char ttt[80];
    strcpy(ttt,"");
    if (page_array[hr_page_no].recitals_words == NULL) page_array[hr_page_no].recitals_words = ttt;
    if (strlen(ttt) > 0) strcat(ttt," ");
    strcat(ttt,yytext);
    curr_page_ptr[hr_page_no] = copy_into_curr_page_buff(curr_page_buff[hr_page_no], curr_page_ptr[hr_page_no], yytext);
    page_array[hr_page_no].recitals_score++;
    if (page_array[hr_page_no].recitals_start_para == 0) page_array[hr_page_no].recitals_start_para = gpid;
    page_array[hr_page_no].recitals_end_para = gpid;        
    if (debug) fprintf(stderr,"          --NEW_recitals: page=%d: score=%d: paraSE=%d:%d:\n", hr_page_no, page_array[hr_page_no].recitals_score, page_array[hr_page_no].recitals_start_para, gpid);
  }
}

      /*  TOC Title */
((Index|INDEX|Table|TABLE){ws}(OF|of|Of){ws}(CONTENTS|Contents|EXHIBITS|Exhibits|ATTACHMENTS|Attachments|Riders|RIDERS|LIST{ws}OF{ws})) {
  gp_count += yyleng;
  if (1 || para_no_on_page < 5) {
    pid_stats_array[gpid].toc_title = strdup(yytext);
    curr_page_ptr[hr_page_no] = copy_into_curr_page_buff(curr_page_buff[hr_page_no], curr_page_ptr[hr_page_no], yytext);
    page_array[hr_page_no].toc_title_found = 1;
    if (debug) fprintf(stderr,"          --NEW_contents: pp=%d:%d:\n", hr_page_no+1, page_array[hr_page_no].toc_title_found);
  }
}

CONTENTS/{ws}"</p>" {
  gp_count += yyleng;
  if (1 || para_no_on_page < 5) {
    pid_stats_array[gpid].toc_title = strdup(yytext);
    curr_page_ptr[hr_page_no] = copy_into_curr_page_buff(curr_page_buff[hr_page_no], curr_page_ptr[hr_page_no], yytext);
    page_array[hr_page_no].toc_title_found = 1;
    if (debug) fprintf(stderr,"          --NEW_contents: pp=%d:%d:\n", hr_page_no+1, page_array[hr_page_no].toc_title_found);
  }
}

((Exhibits|EXHIBITS|Index|INDEX)) { // only if on a short line, we don't want any mention of exhibits to add 150 points
  gp_count += yyleng;
  if (debug) fprintf(stderr,"          --ALMOST NEW_exhib_contents: pp=%d:%d:  now=%d:noCw=%d: w00=%d:\n", hr_page_no+1, para_line_no
	  , page_line_properties_array[hr_page_no+1][para_line_no].no_of_words , page_line_properties_array[hr_page_no+1][para_line_no].no_of_first_cap_words
	  , page_line_properties_array[1][1].no_of_words
	  );
  if (page_line_properties_array[hr_page_no+1][para_line_no].no_of_words < 4
      && page_line_properties_array[hr_page_no+1][para_line_no].no_of_words * 2 < page_line_properties_array[hr_page_no+1][para_line_no].no_of_first_cap_words * 3) {
    pid_stats_array[gpid].toc_title = strdup(yytext);
    curr_page_ptr[hr_page_no] = copy_into_curr_page_buff(curr_page_buff[hr_page_no], curr_page_ptr[hr_page_no], yytext);
    page_array[hr_page_no].toc_title_found = 1;
    if (debug) fprintf(stderr,"          --NEW_exhib_contents: pp=%d:%d:\n", hr_page_no+1, page_array[hr_page_no].toc_title_found);
  }
}
  

(Page|PAGE) {
  gp_count += yyleng;
  pid_stats_array[gpid].toc_title = strdup(yytext);
  curr_page_ptr[hr_page_no] = copy_into_curr_page_buff(curr_page_buff[hr_page_no], curr_page_ptr[hr_page_no], yytext);  
}

{PLAYERS}/[^a-zA-Z0-9] { // landlord, tenant, etc, suitable for sig_block but also title_page
  page_array[hr_page_no].PLAYER_words++;
  page_array[hr_page_no].title_page_words++;
  if (debug) fprintf(stderr,"FOUND PLAYER:%s: total=%d:\n",yytext,page_array[hr_page_no].PLAYER_words);
}

(WITNESS(ES)?|Witness(es)?)({ws1}(AS|As|as))?({ws1}(TO|To|to))?/{ws1}{PLAYERS}[^a-zA-Z0-9] { // 
  page_array[hr_page_no].WITNESS_words++;
  page_array[hr_page_no].sig_block_para = gpid;
  if (debug) fprintf(stderr,"FOUND WIT:%s:%d:%d:\n",yytext,hr_page_no,gpid);
  page_array[hr_page_no].PLAYER_words = 0;  
}


((IN{ws})?(WITNESS|Witness){ws}(WHEREOF|Whereof))/[^a-zA-Z0-9] { // 
  page_array[hr_page_no].WITNESS_words++;
  if (page_array[hr_page_no].sig_block_para == 0) page_array[hr_page_no].sig_block_para = gpid; // if there's a sig_block on this page then don't step on the first one
  if (debug) fprintf(stderr,"FOUND WIT:%s:%d:%d:\n",yytext,hr_page_no,gpid);
  page_array[hr_page_no].PLAYER_words = 0;  
}


(SIGNATURE{ws}PAGE{ws}FOLLOWS)/[^a-zA-Z0-9] { // 
  page_array[hr_page_no+1].sig_page_follows_words++; // send the score to the next page
  if (debug) fprintf(stderr,"FOUND FOLLOWS:%s:\n",yytext);  
}

{title_page_words}|{TITLE_PAGE_WORDS}|{Title_Page_Words}/[^a-zA-Z0-9] {
  page_array[hr_page_no].title_page_words++;
}

{SIG_BLOCK_WORDS}|{Sig_Block_Words}/[^a-zA-Z0-9] {
  page_array[hr_page_no].name_title_date_words++;
  page_array[hr_page_no].sig_block_words++;
}

%%

int print_page_line_properties_array(int nn) {
  int ii;
  for (ii = 0; ii < 8; ii++) {
    if (debug) fprintf(stderr,"HHHHHHHH: nn=%d:ii=%d:   c=%d: l=%d: now=%d: cap=%d:%d:\n"
	    , nn, ii, page_line_properties_array[2][ii].center, page_line_properties_array[2][ii].left_X, page_line_properties_array[2][ii].no_of_words
	    , page_line_properties_array[2][ii].no_of_first_cap_words, page_line_properties_array[2][ii].no_of_all_cap_words);	    
  }
  return 0;
}

int organize_line2para_array(int no_of_pages) { // for the lines that continue an old para assign the last para_id so they won't have 0
  //int last_para = 0;
  //int last_pp = 0;
  int pid;
  int last_ln = -1;
  int last_pid = 0;
  int max_pg = 0;
  int last_tt = -1;
  fprintf(stderr,"        WWWW9:%d:\n",gpid);
  for (pid = 0; pid <= gpid; pid++) {
    int tt = Paragraph2Token_array[pid];
    int pg = token_array[tt].page_no;
    int ln = token_array[tt].line_no;
    if ((pg == max_pg && ln < last_ln) // take care of invalid PIDs in SQL
	|| pg < max_pg
	|| tt < last_tt
	) {
      //fprintf(stderr,"LLLLLL: ln=%d:%d:  pg=%d:%d:   tt=%d:%d:\n",ln,last_ln,   pg, max_pg,  tt, last_tt);
      ;  // a missing PID at the beginnig on a page
    } else {
      line2para_array[pg][ln] = pid;
      //fprintf(stderr,"                       WWWW: tt=%d: page=%d: line=%d: para=%d:\n",tt, pg,ln,  line2para_array[pg][ln]);    
      // fprintf(stderr,"             WWWWX:%d:\n",line2para_array[1][0]);
    }

    int ll;

    if (1) for (ll = last_ln + 1; ll < ln; ll++) {
      line2para_array[pg][ll] = last_pid;
      //fprintf(stderr,"                       WWWW: page=%d: line=%d: para=%d:\n",pg,ll,  line2para_array[pg][ll]);          
    }
    //fprintf(stderr,"             WWWWY:%d:\n",line2para_array[1][0]);        
    last_ln = ln;
    last_pid = pid;
    max_pg = MAX(pg,max_pg);
    last_tt = tt;
  }
  int pg;
  if (1) for (pg = 1; pg <= no_of_pages; pg++) {
    fprintf(stderr,"        WWWWWW0 page=%d:%d:\n",pg, page_array[pg].no_of_lines);
    int ln;
    for (ln = 0; ln < 30; ln++) {
      fprintf(stderr,"          WWWWWW pp=%d: ll=%d: pid=%d:\n", pg,ln,line2para_array[pg][ln]);
    }
  }
  return 0;
}

int main(int argc, char **argv) { 
  int get_opt_index;
  int c_getopt;
  prog = argv[0];
  opterr = 0;
  debug = 0;
  while ((c_getopt = getopt (argc, argv, "d:P:N:U:W:D:I:X:O:")) != -1) {
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
    case 'I':
      index_name = optarg;
      break;
    case 'X':
      dict_name = strdup(optarg);
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
      if (optopt == 'I')
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

  fprintf (stderr,"%s took: doc_id = %d, db_IP =%s, db_name =%s, db_user_name =%s, db_pwd=%s: new=%d:\n",
	   argv[0], doc_id, db_IP, db_name, db_user_name, db_pwd, new);

  for (get_opt_index = optind; get_opt_index < argc; get_opt_index++) {
    printf ("Non-option argument %s\n", argv[get_opt_index]);
  }

  conn = mysql_init(NULL);
  
  /* now connect to database */
  if (!mysql_real_connect(conn,db_IP,db_user_name,db_pwd,db_name,0,NULL,0)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }

  read_dict();  
  current_timestamp(0);  
  token_no = get_tokens(doc_id, source_ocr);
  current_timestamp(1);
  get_paragraphtoken(doc_id);
  current_timestamp(2);
  delete_entries_from_sql(doc_id);
  current_timestamp(3);  
  //  yylex();
  current_timestamp(4);  
  //page_no = hr_page_no+2;
  /****************/
  current_timestamp(5);  
  get_vertical_spaces_per_page(doc_id);
  print_page_line_properties_array(0);
  current_timestamp(6);  
  index_no = read_index(conn, index_name, doc_id);

  if (debug) fprintf(stderr,"STARTING YYLEX\n");
  yylex();
  organize_line2para_array(hr_page_no+1);
  fprintf(stderr,"                  MMMMMMMM1: pn=%d: line=%d: pid=%d:\n",1, 3, line2para_array[1][3]);  
  /* at DSA time there are no LIs !!! */
  //  get_interesting_LIs_per_page(doc_id);
  current_timestamp(7);  
  calc_page_stats(token_no);
  fprintf(stderr,"KKK00:%d:%d:\n",page_line_properties_array[2][0].left_X, page_line_properties_array[2][0].right_X);  
  print_page_line_properties_array(1);  
  current_timestamp(8);  
  //calc_title_page_score(doc_id);
  /****************/  
  current_timestamp(9);
  calc_pid_stats(gpid);
  print_page_line_properties_array(2);  
  current_timestamp(10);  
  new_calc_title_page_score(page_no);
  print_page_line_properties_array(3);
  current_timestamp(11);  
  new_calc_toc_score(page_no);
  current_timestamp(12);  
  calc_sig_block_score(page_no);
  calc_lease_title_score(page_no);
  current_timestamp(13);  
  insert_page2summary_array_into_sql(page_no);
  current_timestamp(14);
  fprintf(stderr,"KKK99:%d:%d:\n",page_line_properties_array[2][0].left_X, page_line_properties_array[2][0].right_X);
  insert_page_line_properties_into_sql(page_no);
  //print_all_pages(hr_page_no);
  print_page_line_properties_array(10);  
  if (index_file) fclose(index_file);
  return 0;
} // main()

int print_all_pages(int pn) {
  int ii;
  for (ii = 0; ii < pn+1; ii++) {
    if (page_array[ii].toc_score > 80) {
      printf("%s", "\n<UTABLE>\n");
    }
    printf("%s", curr_page_buff[ii]);
    if (page_array[ii].toc_score > 80) {
      printf("%s", "\n</UTABLE>\n");
    }
  }
  return 0;
}

