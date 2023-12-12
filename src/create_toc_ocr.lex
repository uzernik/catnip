%x shead state_exhibit state_2n state_3p state_4p state_4p1 state_ignore state_5p state_2nA  YYYY1 YYYY2 YYYY3 YYYYcopy YYYY6 YYYY61

%{ 

  /*
CALLING: (../bin/create_toc -D1 -d 4798 -f Document -t ../tmp/Toc/toc.html -u URL -i ../../../dealthing/dtcstuff/tmp/Index/index_file_4798 -n DEAL -r 1 -P localhost -N dealthing -U root -W imaof3 -C ../../../dealthing/dtcstuff/tmp/Xxx/XXX_CCC  -A ../../../dealthing/webapp -X1 -Y1 < ../tmp/Xxx/aaa5 > bbb) >& ttt

CALLING: (../bin/create_toc -D1 -d 4798 -f Document -t ../tmp/Toc/toc.html -u URL -i iii4798 -n DEAL -r 1 -P localhost -N dealthing -U root -W imaof3 -C ../../../dealthing/dtcstuff/tmp/Xxx/XXX_CCC  -A ../../../dealthing/webapp -X1 -Y1 < ../tmp/Xxx/aaa5 > bbb) >& ttt

CALLING: (../bin/create_toc -D1 -d 4807 -f Document -t ../tmp/Toc/toc.html -u URL -i iii_4807 -n DEAL -r 1 -P localhost -N dealthing -U root -W imaof3 -C ../../../dealthing/dtcstuff/tmp/Xxx/XXX_CCC_4807  -A ../../../dealthing/webapp -X1 -Y1 -F1 < aaa5_4807 > bbb_4807) >& ttt_4807

CALLING: (../bin/create_toc -D1 -d 4804 -f Document -t ../tmp/Toc/toc.html -u URL -i iii -n DEAL -r 1 -P localhost -N dealthing -U root -W imaof3 -C ../../../dealthing/dtcstuff/tmp/Xxx/XXX_CCC_4804  -A ../../../dealthing/webapp -X1 -Y1 -F1 -m1 < aaa5_4804 > bbb) >& ttt

CALLING: (../bin/create_toc -D1 -d 4838 -f Document -t ../tmp/Toc/toc.html -u URL -i iii -n DEAL -r 1 -P localhost -N dealthing -U root -W imaof3 -C ../../../dealthing/dtcstuff/tmp/Xxx/XXX_CCC_4838  -A ../../../dealthing/webapp -X1 -Y0 -F1 -m1  -o aaa62 -GUSA < aaa5_4838) >& ttt

CALLING ON APP:
(../bin/create_toc -D1 -d 42774 -f Document -t ../tmp/Toc/toc.html -u URL -i iii -n DEAL -r 1 -P pm1pl5vtc8wkbly.cu0erabygff3\
.us-west-2.rds.amazonaws.com -N dealthing -U root -W imaof333 -C ../../../dealthing/dtcstuff/tmp/Xxx/XXX_CCC_0  -A ../../../dealthing/webapp -X1 -Y0 -F1 -m1 -o aaa62 -GGBR < aaa5_0) >& ttt

UDEV1 NEW OCR AWS:
(../bin/create_toc -D1 -d 71815 -f Document -t ../tmp/Toc/toc.html -u URL -i iii -n DEAL -r 1 -Pudev1-from-prod-feb26.cu0erabygff3.us-west-2.rds.amazonaws.com -N dealthing -U root -W imaof333 -C ../../../dealthing/dtcstuff/tmp/Xxx/XXX_CCC_71815  -A ../../../dealthing/webapp -X1 -Y0 -F1 -m1 -o aaa62 -GGBR < ./my_htmlfile) >& ttt

real coords_file in:  ../webapp/tmp/letters_coords_4804.tab

** 1.  scanning the text and inserting TOC items into INSERT_ARRAY (used to go directly to ITEM_ARRAY)
** 2.  now, from INSERT_ARRAY we insert into ITEM_ARRAY
** 3.  now, at the same time also GROUP_ITEM_ARRAY is populated
** 4.  identify TOC_TOC (the first duplicate items in the array of items)
** 5.  align TOC_TOC vs REST
** 6.  update the INSERT_ARRAY based on findings -- this is to add items that were found in TOC_TOC but missed on REST
** 7.  again, from INSERT_ARRAY we insert into ITEM_ARRAY
** 8.  again, at the same time also GROUP_ITEM_ARRAY is populated
** 9.  old_align: identify sequences, take into account 12/13
** 10. organize_levels

  */
  
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include <math.h>  
#include <ctype.h>
#include <unistd.h>
#include "create_toc_ocr.h"
#include "entity_functions.h"
#include "my_import_functions.h"  
#include "align_toc_functions.h"  
#define MAX(a,b) (a>b)?a:b
#define MIN(a,b) (a<b)?a:b  
#define FORTY_SIX 46
  
  //MYSQL *conn;
  //MYSQL_RES *sql_res;
  //MYSQL_ROW sql_row;
  

#define  YY_USER_ACTION my_loc += yyleng;   //strcat(MY_TOTAL_file,yytext); //fprintf(stderr,"PPPPPPP0:%d: 2n=%d: my_loc=%d:\n",YYSTATE,state_2n,my_loc);
int in_fundamentals = 0; // "LEASE FACE PAGE" // page-based
int in_fundamentals_section = 0;  // "Section I.  Fundamental Lease Terms" // toc-based
int fundamentals_no = 0;  // the counter
int required_fundamental_no  = 0; // do we need to dispense a number? not in "1. Tenant"
// this is for the fundamental points
// normally we count fundamental points as 1., 2. 3.
// should be Point 1., Point 2., etc
#define FUNDAMENTAL_POINT_GROUP_NO 17
// however, if the title itself is : "3. Basic Lease Provisions" then we use a., b., c.
#define ALTERNATIVE_FUNDAMENTAL_POINT_GROUP_NO 17
#define EXHIBIT_SPECIAL 10
#define LEASE_SPECIAL 1
#define FUNDAMENTALS_SPECIAL 2
#define FUNDAMENTAL_POINT_SPECIAL 3


int do_pass = 1;
int do_toc_toc_align = 1;
int do_fundamentals = 1;
int do_max_align = 0;
 
char *source_ocr="AWS";
 
int my_loc = -1 * FORTY_SIX;
 
int hr_pn = 0; // the page no by <HR

#define BAD_LEVEL 10

  int NO_TOC = 0;
  int NO_IDX = 0;


  char *prog;
  char *db_IP, *db_name, *db_pwd, *db_user_name; // for DB config
  char *url_name;
  char *indexed_name;
  char *toc_name;
  int file_no = 0; // relic of the past
  int doc_id;
  char *out_root; // aaa62

  int debug = 0;
  int dres = 1; // debug resolve contending
  int dseq = 0; // debug_seq
  int restart_toc = 0; // keep accumulating TOC = 0 / restart = 1
  FILE *coords_file;
  char *coords_name;

  int old_ln; // to keep the line_no at the beginning of the section title
  int old_loc;
  int pid = 0;
  int title_pid = -1; // in case we have "<P 21>ARTICLE I<P 22>INTRODUCTION<P>" we want to capture the initial ID, 21. IT is set when we get into state <title4p> it is reset at the out of insert_item()
  int in_td = 0; // keep track of when we are in a TR, so we do DSA only if TD_ID=0

  int d_group = 0; // to indicate it's the number 2. that came from "Section 2." to distinguish from simply "2."


char toc_query[100000]; // to insert into toc items into sql

//  char *title_array[50] = {"Part", "PART", "Paragraph", "PARAGRAPH", "Section", "SECTION", "Article", "ARTICLE", "Exhibit", "EXHIBIT", "Attachment", "ATTACHMENT", "Appendix", "APPENDIX", "Schedule", "SCHEDULE", "LEASE SUMMARY SHEET", "Lease Summary Sheet", "document cover Sheet", "RENT RIDER", "Rent Rider", "LEASE RIDER", "Lease Rider"};


char basic_phrase_array[20][200] = { // not in use
   "basic lease information"
  ,"basic lease provisions"
  ,"fundamental lease provisions"
  ,"basic lease terms"
  ,"lease summary sheet"
  ,"document cover sheet"
  ,"lease cover sheet"
  ,"contract cover sheet"
  ,"agreement cover sheet"              
  ,"summary page"
  ,"cover sheet"
  ,"lease face page"
  ,"lease data"
  ,"basic data"

  
};
  

int find_period_before_word(char *text, int first_not_capped, char *prev);

  char vtext[5000];
  int line_no = 0;
  int g_group_no = 0; // must be a global since we move to a new state and want to keep the group info
  int prev_g_group_no = 0; // the trailer for fundamentals_point
  int item_no;
  char vtitle[200];
  char vpp[100000];
  char *t_title = "_"; // pass the title of each item using global var into insert
  char *fundamentals_header = "_"; // the header of the page: e.g., "FACE PAGE", "FUNDAMENTAL PROVISIONS", etc
  char *t_text = NULL; // pass yytext of each item using global var into insert
  int cpp = 0;
  int g_allowed_length_fundamental_point = 0; // when we have a fundamental_point we need to limit the header by the given HSPACE
  char *gprev = NULL;


  /* this contains the entire file in memory ??? */
  #define MAX_FILE_LEN  2000000 
  int buff_len;
  char out_buff[MAX_FILE_LEN];
  int zero0 = 1; // indicates do headers the new way

int normcmp(char text1[], char text2[]) {
    // normalize string and cmp
    int ii, jj;
    static char nt1[5000], nt2[5000];
    for (jj = 0, ii = 0; ii < strlen(text1); ii++) {
      if (isalnum(text1[ii]) != 0) {
	nt1[jj++] = text1[ii];
      }
    }
    nt1[jj++] = '\0';
    for (jj = 0, ii = 0; ii < strlen(text2); ii++) {
      if (isalnum(text2[ii]) != 0) {
	nt2[jj++] = text2[ii];
      }
    }
    nt2[jj++] = '\0';
    return strcmp(nt1,nt2);
}

char *rem_endline(char text1[]) {
    // rem '/n'
    int ii, jj;
    static char nt1[5000];
    for (jj = 0, ii = 0; ii < strlen(text1); ii++) {
      if (text1[ii] == '\n') {
	nt1[jj++] = ' ';
      } else {
	nt1[jj++] = text1[ii];
      }
    }
    nt1[jj++] = '\0';
    return nt1;
}

/* also remove <ZZ> */
char *clean_leading_spaces(char *header) {
  int ii;
  int jj = 0;
  static char bext[5000];
  int first = 1;
  int in_bra = 0;
  for (ii = 0; ii < strlen(header); ii++) {
    if (first == 0) {
      if (in_bra == 0) {
	if (header[ii] == '<' && ii < strlen(header)-1 && header[ii+1] == 'Z') {
	  in_bra = 1;
	} else {
	  bext[jj++] = header[ii];
	}
      } else {
	if (header[ii] == '>') {
	  in_bra = 0;
	}
      }
    } else if (first == 1) {
      if (header[ii] != ' ' && header[ii] != '\n') {
	bext[jj++] = header[ii];
	first = 0;
      } else {
	;  // it is a space, do nothing
      }
    } 
  }
  bext[jj] = '\0';
  //fprintf(stderr,"LLL:%s:\n:%s:\n",bext,header);
  return strdup(bext);
} // clean_leading_spaces()

int do_THREE(int nn, char *q) {
  if (strcmp(q,"ONE") == 0)  nn += 1;     
      else if (strcmp(q,"TWO") == 0) nn += 2;
      else if (strcmp(q,"THREE") == 0) nn += 3;
      else if (strcmp(q,"FOUR") == 0) nn += 4;
      else if (strcmp(q,"FIVE") == 0) nn += 5;
      else if (strcmp(q,"SIX") == 0) nn += 6;
      else if (strcmp(q,"SEVEN") == 0) nn += 7;
      else if (strcmp(q,"EIGHT") == 0) nn += 8;
      else if (strcmp(q,"NINE") == 0) nn += 9;
      else if (strcmp(q,"TEN") == 0) nn += 10;
      else if (strcmp(q,"ELEVEN") == 0) nn += 11;
      else if (strcmp(q,"TWELVE") == 0) nn += 12;
      else if (strcmp(q,"THIRTEEN") == 0) nn += 13;
      else if (strcmp(q,"FOURTEEN") == 0) nn += 14;
      else if (strcmp(q,"FIFTEEN") == 0) nn += 15;
      else if (strcmp(q,"SIXTEEN") == 0) nn += 16;
      else if (strcmp(q,"SEVENTEEN") == 0) nn += 17;
      else if (strcmp(q,"EIGHTEEN") == 0) nn += 18;
      else if (strcmp(q,"NINETEEN") == 0) nn += 19;
      else if (strcmp(q,"TWENTY") == 0) nn += 20;
      else if (strcmp(q,"THIRTY") == 0) nn += 30;
      else if (strcmp(q,"FORTY") == 0) nn += 40;
      else if (strcmp(q,"FIFTY") == 0) nn += 50;
      else if (strcmp(q,"SIXTY") == 0) nn += 60;
      else if (strcmp(q,"SEVENTY") == 0) nn += 70;
      else if (strcmp(q,"EIGHTTY") == 0) nn += 80;
      else if (strcmp(q,"NINETY") == 0) nn += 90;
      return nn;
} // do_THREE()

 int do_THREE_first(int nn, char *q) {
   if (strcmp(q,"ONE") == 0) nn = 1;
   else if (strcmp(q,"TWO") == 0) nn = 2;
   else if (strcmp(q,"THREE") == 0) nn = 3;
   else if (strcmp(q,"FOUR") == 0) nn = 4;
   else if (strcmp(q,"FIVE") == 0) nn = 5;
   else if (strcmp(q,"SIX") == 0) nn = 6;
   else if (strcmp(q,"SEVEN") == 0) nn = 7;
   else if (strcmp(q,"EIGHT") == 0) nn = 8;
   else if (strcmp(q,"NINE") == 0) nn = 9;
   else if (strcmp(q,"TEN") == 0) nn = 10;
   else if (strcmp(q,"ELEVEN") == 0) nn = 11;
   else if (strcmp(q,"TWELVE") == 0) nn = 12;
   else if (strcmp(q,"THIRTEEN") == 0) nn = 13;
   else if (strcmp(q,"FOURTEEN") == 0) nn = 14;
   else if (strcmp(q,"FIFTEEN") == 0) nn = 15;
   else if (strcmp(q,"SIXTEEN") == 0) nn = 16;
   else if (strcmp(q,"SEVENTEEN") == 0) nn = 17;
   else if (strcmp(q,"EIGHTEEN") == 0) nn = 18;
   else if (strcmp(q,"NINETEEN") == 0) nn = 19;
   else if (strcmp(q,"TWENTY") == 0) nn = 20;
   else if (strcmp(q,"THIRTY") == 0) nn = 30;
   else if (strcmp(q,"FORTY") == 0) nn = 40;
   else if (strcmp(q,"FIFTY") == 0) nn = 50;
   else if (strcmp(q,"SIXTY") == 0) nn = 60;
   else if (strcmp(q,"SEVENTY") == 0) nn = 70;
   else if (strcmp(q,"EIGHTTY") == 0) nn = 80;
   else if (strcmp(q,"NINETY") == 0) nn = 90;

   else if (strncmp(q,"FIRST",5) == 0) nn = 1;
   else if (strncmp(q,"SECOND",6) == 0) nn = 2;
   else if (strncmp(q,"THIRD",5) == 0) nn = 3;
   else if (strncmp(q,"FOURTH",6) == 0) nn = 4;
   else if (strncmp(q,"FIFTH",5) == 0) nn = 5;
   else if (strncmp(q,"SIXTH",5) == 0) nn = 6;
   else if (strncmp(q,"SEVENTH",7) == 0) nn = 7;
   else if (strncmp(q,"EIGHTH",6) == 0) nn = 8;
   else if (strncmp(q,"NINTH",6) == 0) nn = 9;
   else if (strncmp(q,"TENTH",5) == 0) nn = 10;
   else if (strncmp(q,"ELEVENTH",8) == 0) nn = 11;
   else if (strncmp(q,"TWELFTH",7) == 0) nn = 12;
   else if (strcmp(q,"THIRTEENTH") == 0) nn = 13;
   else if (strcmp(q,"FOURTEENTH") == 0) nn = 14;
   else if (strcmp(q,"FIFTEENTH") == 0) nn = 15;
   else if (strcmp(q,"SIXTEENTH") == 0) nn = 16;
   else if (strcmp(q,"SEVENTEENTH") == 0) nn = 17;
   else if (strcmp(q,"EIGHTEENTH") == 0) nn = 18;
   else if (strcmp(q,"NINETEENTH") == 0) nn = 19;
   else if (strcmp(q,"TWENTIETH") == 0) nn = 20;


   return nn;
}

int loc_to_line(int in_my_loc) {
  int token_id = reverse_token_array[in_my_loc];
  return(token_array[token_id].line_no);  
}

 int loc_to_page(int in_my_loc) {
  int token_id = reverse_token_array[in_my_loc];
  return(token_array[token_id].page_no);  
}

int loc_to_x1(int in_my_loc) {
  int token_id = reverse_token_array[in_my_loc-1];
  return token_array[token_id].x1_1000;
}

int loc_to_y2(int in_my_loc) {
  int token_id = reverse_token_array[in_my_loc-1];
  return token_array[token_id].y2_1000;
}
 
int loc_to_indent(int in_my_loc) {
  int token_id = reverse_token_array[in_my_loc-1];
  int line = token_array[token_id].line_no;
  int page = token_array[token_id].page_no;  
  int indent = page_line_properties_array[page][line].left_X;
  fprintf(stderr,"     LOCO: loc=%d: tid=%d: line=%d: page=%d: indent=%d:\n",in_my_loc-1, token_id, line, page, indent);
  return indent;
}

 int loc_to_center(int in_my_loc) {
  int token_id = reverse_token_array[in_my_loc-1];
  int line = token_array[token_id].line_no;
  int page = token_array[token_id].page_no;  
  int center = page_line_properties_array[page][line].center;
  fprintf(stderr,"     LOCU: loc=%d: tid=%d: line=%d: page=%d: center=%d:\n",in_my_loc-1, token_id, line, page, center);
  return center;
}

#define MAX_HS 50000
int token2horizontal_tab_array[MAX_TOKEN];
int pageline2horizontal_array[MAX_PAGE][MAX_LINE];

struct Horizontal_Tab_Struct {
  int id, line_no, token_no, page_no, space_size, bin_no;
} horizontal_tab_array[MAX_HS];
horizontal_tab_no = 0;

int loc_to_hspace_token(int in_my_loc) { // this function returns the token after the HSPACE.  -1 if no such space
  int token_id = reverse_token_array[in_my_loc];
  int line_no = token_array[token_id].line_no;
  int page_no = token_array[token_id].page_no-1;
  int hs = pageline2horizontal_array[page_no][line_no];
  token_no = (hs == 0) ? -1 : horizontal_tab_array[hs].token_no;
  // fprintf(stderr,"JJ/JJ: tid=%d: pl=%d:%d: hs=%d: tn=%d:\n",token_id, page_no,line_no, hs, token_no);
  return token_no;
}


int ocr_error_correction(char *orig_first_field, int my_group,int my_item, int my_P_enum) {
    /**************************/
    // the last param on insert_into_group is converted, either 0 or 1.  
    // 1 means it's a spelling error (e.g., 1 becomes I).  
    // 0 means it's a different interpretation (e.g., I can be H I J OR I can be I II III)
    /**************************/

    int modi = my_group % 10;

    // take care of I that comes on A, B, C.
    if (strcmp(orig_first_field,"I") == 0 && modi  == 8 ) { // A. B. C., group =18, 58, 68, etc
      insert_into_group_plus(my_item,my_group+2, 1, my_P_enum, 0); // I,II XI
      insert_into_group_plus(my_item,my_group-7, 1, my_P_enum, 1); // 1,2,3,   17
      insert_into_group_plus(my_item,my_group-1, 12, my_P_enum, 1); // l, j,k,l  gr = 17
      insert_into_group_plus(my_item,my_group-1, 9, my_P_enum, 1); // i, j,k,l  gr = 17
    }

    // take care of II that comes on I,II,III
    if (strcmp(orig_first_field,"II") == 0 && modi  == 0 ) { // I. II. III., group = 10,20,etc
      insert_into_group_plus(my_item,my_group-9, 11, my_P_enum, 1); // 1,2,3,   gr 1,11,etc
    }

    // take care of V that comes on A, B C.
    if (strcmp(orig_first_field,"V") == 0 && modi == 8 ) { // A. B. C. V-> 5 , gr = 18
      insert_into_group_plus(my_item,my_group+2, 5 ,my_P_enum, 0); // I,II XI, gr == 20
    }

    // take care of X that comes on A, B C.
    if (strcmp(orig_first_field,"X") == 0 && modi == 8 ) { // A. B. C. X -> 10
      insert_into_group_plus(my_item,my_group+2, 10, my_P_enum, 0); // I,II XI
    }

    // take care of i (a b c)
    if (strcmp(orig_first_field,"i") == 0 && modi  == 7 ) { // a. b. c., group =17, 57, 67, etc gr == 17
      insert_into_group_plus(my_item,my_group+2, 1, my_P_enum, 0); // i,ii xi gr == 19
      insert_into_group_plus(my_item,my_group-6, 1, my_P_enum, 1); // 1,2,3,   gr == 11
    }

    // take care of v that comes on a, b c.
    if (strcmp(orig_first_field,"v") == 0 && modi == 7 ) { // a. b. c. v-> 5 , gr = 17
      insert_into_group_plus(my_item,my_group+2, 5, my_P_enum,  0); // i,ii xi, gr == 19
    }

    // take care of x that comes on a, b c.
    if (strcmp(orig_first_field,"x") == 0 && modi == 7 ) { // a. b. c.  gr == 17
      insert_into_group_plus(my_item,my_group+2, 10, my_P_enum, 0); // i,ii xi, gr == 19
    }

    // take care of m that comes on III
    if (strcmp(orig_first_field,"m") == 0 && modi == 7 ) { // a. b. c.  gr == 17
      insert_into_group_plus(my_item,my_group+2, 3, my_P_enum, 1); // i,ii xi, gr == 19 m becomes a iii
      insert_into_group_plus(my_item,my_group+3, 3, my_P_enum, 1); // I,II XI, gr == 20 m becomes a III
    }
    // take care of H that comes on II
    if (strcmp(orig_first_field,"H") == 0 && modi == 8 ) { // A. B. C.  GR == 18
      insert_into_group_plus(my_item,my_group+2, 2, my_P_enum, 1); // I,II XI, gr == 20 H becomes a II
    }

    // take care of G that comes on 6
    if (strcmp(orig_first_field,"G") == 0 && modi == 8 ) { // A. B. C.  GR == 18
      insert_into_group_plus(my_item,my_group-7, 6, my_P_enum, 1); // 1,2,3, gr == 11 G becomes a 6
    }

    // take care of G that comes on 6
    if (0 && strcmp(orig_first_field,"B") == 0 && modi == 8 ) { // A. B. C.  GR == 18
      insert_into_group_plus(my_item,my_group-7, 8, my_P_enum, 1); // 1,2,3, gr == 11 G becomes a 8
    }

    // take care of Z that comes on 2
    if (strcmp(orig_first_field,"Z") == 0 && modi == 8 ) { // A. B. C.  GR == 18
      insert_into_group_plus(my_item,my_group-7, 2, my_P_enum, 1); // 1,2,3, gr == 11 Z becomes a 2
    }

    // take care of 1 that comes on 2,3,4
    if (strcmp(orig_first_field,"1") == 0 && modi == 1 ) { // gr = 11
      insert_into_group_plus(my_item,my_group+8, 12, my_P_enum, 1); // l,m,n,o,p, gr = 19
      insert_into_group_plus(my_item,my_group+7, 9, my_P_enum, 1); // I, J,K,L  gr = 18
      insert_into_group_plus(my_item,my_group+9, 1, my_P_enum, 1); // I, II,III,  gr = 20
      insert_into_group_plus(my_item,my_group+6, 12, my_P_enum, 1); // l, j,k,l  gr = 17
      insert_into_group_plus(my_item,my_group+6, 9, my_P_enum, 1); // i, j,k,l  gr = 17
      insert_into_group_plus(my_item,my_group+8, 1, my_P_enum, 1); // i, ii,iii  gr = 19
    }

    // take care of v that comes on i ii iii
    if (strcmp(orig_first_field,"v") == 0 && modi == 9 ) { // i, ii, iii, grp = 19
      insert_into_group_plus(my_item,my_group-2, 22, my_P_enum,  0); // a,b,c, gr == 17
    }

    // take care of x that comes on i, ii, iii
    if (strcmp(orig_first_field,"x") == 0 && modi == 9 ) { // i,ii xi, gr == 19
      insert_into_group_plus(my_item,my_group-2, 24, my_P_enum, 0); //a. b. c.  gr == 17
    }

    // take care of i that comes on i, ii, iii
    if (strcmp(orig_first_field,"i") == 0 && modi == 9 ) { // i,ii xi, gr == 19
      insert_into_group_plus(my_item,my_group-2, 9, my_P_enum, 0); //a. b. c.  gr == 17
    }

    // take care of l that comes on i, ii, iii
    if (strcmp(orig_first_field,"l") == 0 && modi == 9 ) { // i,ii xi, gr == 19
      insert_into_group_plus(my_item,my_group-2, 12, my_P_enum, 0); //a. b. c.  gr == 17
    }
    return 0;
}


 int do_twenty_two(char *my_text, int my_item) {
  static char first_text[30];
  static char second_text[30];
  strcpy(first_text,my_text);
  int nn = 0;

  char *q,*p = NULL;
  p = strchr(my_text,'-');
  if (!p) {
    p = strchr(my_text,' ');
  }
  if (p) {
    strcpy(second_text,p+1);
    p[0] = '\0';
  } else {
    strcpy(second_text,"");
  }
    
  nn = 0;
  if (strlen(second_text) > 0) {
    q = second_text;
  } else {
    q = first_text;
  }
  nn = do_THREE_first(nn, q);
  if (strlen(second_text) > 0) {
    q = second_text;
    nn = do_THREE(nn,q);
  }
  item_array[my_item].section_v[0] = nn;
  item_array[my_item].section_no = 1;
  item_array[my_item].my_enum = nn;
  item_array[my_item].lower_enum = -1;
  return 0;
}
 
int calculate_value_for_each_type(int ii, int my_group,int my_item, char *pch) {
  if (my_group%10 == 0 || my_group%10 == 9) { // roman
    item_array[my_item].section_v[ii] = calc_roman(pch);	  
    //fprintf(stderr,"UUU32:ii=%d:  me=%d:\n", my_item, item_array[my_item].section_v[ii]);
  } else if (is_number(pch)) {
    item_array[my_item].section_v[ii] = atoi(pch);
    //fprintf(stderr,"UUU33:ii=%d:  me=%d:\n", my_item, item_array[my_item].section_v[ii]);
  } else if (my_group%10 == 8) { // A, B, C,
    char xx = item_array[my_item].section[0]; 
    if (xx == '(') {
      xx = item_array[my_item].section[1]; 
    }
    item_array[my_item].section_v[ii] = xx - 'A'+1;
    //fprintf(stderr,"UUU34:ii=%d:  me=%d:\n", my_item, item_array[my_item].section_v[ii]);
  } else if (my_group%10 == 7) { // a, b, c,
    char xx = item_array[my_item].section[0]; 
    if (xx == '(') {
      xx = item_array[my_item].section[1]; 
    }
    item_array[my_item].section_v[ii] = xx - 'a'+1;
    //fprintf(stderr,"UUU35:ii=%d:  me=%d:\n", my_item, item_array[my_item].section_v[ii]);    
  }
  //fprintf(stderr,"UUU3:ii=%d:  me=%d:\n", my_item, item_array[my_item].section_v[ii]);
  return 0;
}

 int calc_no_of_words_in_header(char *text, int *no_of_Caps, int *no_of_CAPs) {
  static char bext[500];
  sprintf(bext,"%s ",text); // add a dummy space at the end so last word 'd be counted
  int ii;
  int no_of_words = 0;
  int in_word = 0;
  *no_of_Caps = 0;
  *no_of_CAPs = 0;   
  int found_alnum = 0;
  int is_CAP_word = 1;
  int is_Cap_word;
  for (ii = 0; ii < strlen(bext); ii++) {
    if (in_word == 0) {
      //if (bext[ii] != ' ' && bext[ii] != '\n' && bext[ii] != '\t') { // getting into fresh word
      if (isalnum(bext[ii]) != 0) { // getting into fresh word
	in_word = 1;
	is_CAP_word = 1;
	found_alnum = 0;	 
	is_Cap_word =  (isupper(bext[ii]) != 0) ? 1 : 0;
	if (isalnum(bext[ii]) != 0) found_alnum++;
	if (isalpha(bext[ii]) != 0 && isupper(bext[ii]) == 0) {
	  is_CAP_word = 0;
	}
      } else {
	; // staying out of word
      }
    } else if (in_word == 1) {
      // if (!(bext[ii] != ' ' && bext[ii] != '\n' && bext[ii] != '\t')) { // getting out of word
      if (isalnum(bext[ii]) == 0) { // getting out of word	
	in_word = 0;
	if (found_alnum == 0) {
	  ;
	} else {
	  if (is_CAP_word) *no_of_CAPs = *no_of_CAPs + 1;
	  if (is_Cap_word) *no_of_Caps = *no_of_Caps +1;
	  no_of_words++;
	}
      } else {  // staying inside word, keep counting
	if (isalnum(bext[ii]) != 0) found_alnum++;	 
	if (isalpha(bext[ii]) != 0 && isupper(bext[ii]) == 0) {
	  is_CAP_word = 0;
	}
      }
    }
  } // for ii
  return no_of_words;
} // calc_no_of_words_in_header()
 
 int find_page_type(char *buff) { // 0 -- none, 1 -- roman, 2 -- arabic, 3 -- I, 4 -- bad
  int my_type = 0;
  int ii;
  for (ii = 0; ii < strlen(buff); ii++) {
    if (my_type == 2) { // already a number
      if (isdigit(buff[ii]) != 0) {
	; // OK
      } else {
	my_type = 4;
      }
    } else if (my_type == 1) { // already a roman
      if (buff[ii] == 'i' || buff[ii] == 'v' || buff[ii] == 'x') {
	; // OK
      } else {
	my_type = 4;
      }
    }  else if (my_type == 3) { // already I or i
      if (isdigit(buff[ii]) != 0) {
	my_type = 2;
      } else if (buff[ii] == 'i' || buff[ii] == 'v' || buff[ii] == 'x') {
	my_type = 1;
      } else {
	my_type = 4;
      }
    }  else if (my_type == 0) { // already I or i
      if (isdigit(buff[ii]) != 0) {
	my_type = 2;
      } else if (buff[ii] == 'i' || buff[ii] == 'v' || buff[ii] == 'x') {
	my_type = 1;
      } else if (buff[ii] == 'i' || buff[ii] == 'I') {
	my_type = 3;
      } else {
	my_type = 4;
      }
    }
  } // for ii
  if (my_type == 3 && strlen(buff) == 1) { // take "I" as "1"
    my_type = 2;
  }
  return my_type;
 } // find_page_type()
 
 // identify a page number at the end of TOC title
 int calculate_page_no(char *header, int in_my_loc, int my_loc, int *my_type, char number_text[]) {
   if (debug) fprintf(stderr,"FIND TOC PAGE: :%50s: loc=%5d: || loc-1=%5d: cc=%c: coord=%7d " // || loc-2=%5d: cc=%d:%c: coord=%7d: "
	  , header, in_my_loc
	  , my_loc-1, coords_array[my_loc-1].cc,    coords_array[my_loc-1].x1000
	  //, my_loc-2, coords_array[my_loc-2].cc, coords_array[my_loc-2].cc, coords_array[my_loc-2].x1000
	  );	
  int yy = 0;
  int last = 10;
  if (debug) fprintf(stderr,"|:");
  for (yy = 1; yy < 6; yy++) {
    if (coords_array[my_loc-yy].cc > 0) fprintf(stderr,"%c",coords_array[my_loc-yy].cc);
    else {
      last = yy;
      break;
    }
  }
  if (debug) fprintf(stderr,"__");
  char buff[60];
  for (yy = 1; yy < 6; yy++) {
    if (coords_array[my_loc-yy].cc > 0) {
      if (debug) fprintf(stderr,"-:%d:%d:%d: :loc=%d: :%d:%c:",last-yy-1,yy,last,   my_loc,   my_loc-last+yy, coords_array[my_loc-last+yy].cc);
      buff[yy-1] = coords_array[my_loc-last+yy].cc;
    }
    else {
      break;
    }
  }
  buff[yy-1] = '\0';
  if (debug) fprintf(stderr,":|%s:\n", buff);  

  *my_type = find_page_type(buff);
  int ret = 0;
  if (*my_type == 2) {
    ret = atoi(buff);
    if (ret == 0) { // take I as 1
      ret = 1; 
    }
  } else if (*my_type == 1) {
    ret = calc_roman(buff);
  } else if (*my_type == 3) {
    ret = 1;
  }
  strcpy(number_text, buff);
  return ret;
} //  calculate_page_no()
    

 
 int print_insert_item_array(int nn, int insert_item_no)  {
  int ii;
  fprintf(stderr,"\n\nIIARRAY%d::%d:\n",nn, insert_item_no);
  for (ii = 0; ii < insert_item_no; ii++) {
    fprintf(stderr,"   IIA:%d: :%d:%d:%d:%d: %s:%s:%s: :%d:%d:%d:\n"
	    , ii

	    , insert_item_array[ii].my_item
	    , insert_item_array[ii].my_line
	    , insert_item_array[ii].in_my_loc
	    , insert_item_array[ii].my_group

	    , insert_item_array[ii].my_text
	    , insert_item_array[ii].title
	    , insert_item_array[ii].header

	    , insert_item_array[ii].is_special
	    , insert_item_array[ii].title_pid
	    , insert_item_array[ii].pid	    	    
	    );
  }
  return 0;
}

int print_one_insert_item_array(int nn, int mm) {
   int ii = mm;
   fprintf(stderr,"   IIB%d: ii=%d: :%d:%d:%d:%d: %s:%s:%s: :%d:%d:%d:\n"
	   , nn
	   , ii

	   , insert_item_array[ii].my_item
	   , insert_item_array[ii].my_line
	   , insert_item_array[ii].in_my_loc
	   , insert_item_array[ii].my_group

	   , insert_item_array[ii].my_text
	   , insert_item_array[ii].title
	   , insert_item_array[ii].header

	   , insert_item_array[ii].is_special
	   , insert_item_array[ii].title_pid
	   , insert_item_array[ii].pid	    	    
	   );

   return 0;
}


int insert_into_insert_item_array(int my_item, int my_line, int in_my_loc, int my_group, char *my_text, char *title, char *header, int is_special, int title_pid, int pid) {
    insert_item_array[insert_item_no].my_item = my_item;
    insert_item_array[insert_item_no].my_line = my_line;    
    insert_item_array[insert_item_no].in_my_loc = in_my_loc;
    insert_item_array[insert_item_no].my_group = my_group;
    insert_item_array[insert_item_no].my_text = (my_text) ? strdup(my_text) : my_text;
    insert_item_array[insert_item_no].title = (title) ? strdup(title) : title;
    insert_item_array[insert_item_no].header = (header) ? strdup(header) : header;
    insert_item_array[insert_item_no].is_special = is_special;          
    insert_item_array[insert_item_no].title_pid = title_pid;
    insert_item_array[insert_item_no].pid = pid;
    insert_item_no++;

   return 0;
} // insert_into_insert_item_array

#define MAX_PAGES 1000
struct Page2Summary_Points {
  int words;
  int junk_words;
  int dict_words;
  float junk_ratio; // words / junk_words
  float dict_ratio; // words / dict_words  
} page2summary_points[MAX_PAGES];

int detect_map_pages() {
  char query[200000];
  sprintf(query,"select page_no, no_of_words, no_of_junk_words, no_of_dict_words \n\
                     from deals_page2summary_points \n\
                     where doc_id = '%d' and source_program='%s' "
	  , doc_id, source_ocr);

  if (debug_entity) fprintf(stderr,"QUERY48=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY48:%s\n",mysql_error(conn));
  }
  sql_res = mysql_store_result(conn);
  while ((sql_row = mysql_fetch_row(sql_res))) {
    int page_no = atoi(sql_row[0]);
    int words = page2summary_points[page_no].words = atoi(sql_row[1]);
    int junk_words = page2summary_points[page_no].junk_words = atoi(sql_row[2]);
    int dict_words = page2summary_points[page_no].dict_words = atoi(sql_row[3]);
    page2summary_points[page_no].junk_ratio = (float)((float)words / (float)junk_words);
    page2summary_points[page_no].dict_ratio = (float)((float)words / (float)dict_words);      
  }
  int ii;
  for (ii = 0; ii <= page_no; ii++) {
    fprintf(stderr,"PAGE_MAP: %d: :%f:%f: :%d:%d:%d:\n",ii, page2summary_points[ii].junk_ratio, page2summary_points[ii].dict_ratio, page2summary_points[ii].words, page2summary_points[ii].junk_words, page2summary_points[ii].dict_words);
  }
  return 0;
} // detect_map_pages() 



#define MAX_BUFF 500000
int insert_item(int nn, int my_item, int my_line, int in_my_loc, int my_group, char my_text[], char* title, char *in_header, int is_special, int title_pid, int pid, int pass_no) { // pass_no = 0 real time; 1
  char header[MAX_BUFF];
  int jj,ii;
  for (jj = 0,ii = 0; ii < strlen(in_header) && ii < MAX_BUFF-1; ii++) {
    if (in_header[ii] != '\n') { // remove NL in header
      header[jj++] = in_header[ii];
    }
  }
  header[jj++] = '\0';

  calc_header(header,my_item);
  my_group = (my_group != 11) ? my_group : ((item_array[my_item].CAP_hdr_len < 3) ? 61 : 11);
  if (strcmp(title,"LR") == 0 || strcmp(title,"RF") == 0) is_special = FUNDAMENTAL_POINT_SPECIAL; // so that LRs are tucked under "Particulars" // 
  if (debug) fprintf(stderr,"\n\nINSERTING BEFORE INSERT:NN=%d: %d: :%d:  :%s:%s: grp=%d: chl=%d:  pid=%d: is_spec=%d: pn=%d:%f:\n"
		     , nn, my_item, insert_item_no  ,title,header, my_group, item_array[my_item].CAP_hdr_len  ,pid,  is_special, hr_pn, page2summary_points[hr_pn].dict_ratio);
  if (do_pass == 1 && pass_no == 0
      && (page2summary_points[hr_pn].dict_ratio < 3.0 || strlen(title) > 0)) { // allow SCHEDULE, EXHIBIT in MAP PAGE
    if (debug) fprintf(stderr,"INSERTING INSERT:%d: :%d:  :%s:%s: grp=%d:  pid=%d: is_spec=%d:\n"
		       , my_item,insert_item_no  ,title,header, my_group  , pid,  is_special);
    insert_into_insert_item_array(my_item, my_line, in_my_loc, my_group, my_text, title, header, is_special, title_pid, pid);
    return 0;
  }

  
  int token_id = reverse_token_array[in_my_loc];
  int disp = 0;
  for (disp = 0; token_id == 0 && disp < 5; disp++) { // the token might come after several spaces
      token_id = reverse_token_array[in_my_loc+disp];
  }

  item_array[my_item].line_no = token_array[token_id].line_no;
  item_array[my_item].page_no = token_array[token_id].page_no;    
  item_array[my_item].my_loc = in_my_loc;
  item_array[my_item].left_X = loc_to_indent(in_my_loc);  
  item_array[my_item].center = loc_to_center(in_my_loc);  
  fprintf(stderr,"LLLOC in=%d: loc=%d: x=%d: center=%d: \n", my_item, in_my_loc, loc_to_indent(in_my_loc), loc_to_center(in_my_loc));  
  item_array[my_item].grp_no = my_group;
  item_array[my_item].token_id = token_id;
  item_array[my_item].is_special = is_special;

  int page_no_type = 0;
  static char number_text[50000]; // that's the page_number in text
  int page_no = calculate_page_no(header, in_my_loc, my_loc, &page_no_type, number_text);
  int l1 = strlen(header);
  int l2 = strlen(number_text);
  if (l1 > l2 && l2 > 0 && header) {
    header[l1 - l2] = '\0';
  }

  item_array[my_item].toc_page_no = page_no;
  item_array[my_item].toc_page_no_type = page_no_type;
  item_array[my_item].toc_page_no_coord = coords_array[my_loc-1].x1000;
  if (debug) fprintf(stderr,"TTTTTTTTTTTTT0:my_item=%d: grp=%d: pp=%d:%d: loci=:%d:%d: tid=%d:%d:%d: c=%d: title=:%s:%s:%s: XXX:%c:%d: ln=%d: sec=%d:%s: is_spec=%d: yyt=%s: pn=%d: pt=%d: pc=%d: pp=%s:\n"
		     , my_item
		     , my_group
		     , title_pid,  pid
		     , in_my_loc,  my_loc
		     , token_id,  token_array[token_id].page_no, token_array[token_id].line_no,  item_array[my_item].center
		     , title,  my_text, header
		     , coords_array[in_my_loc].cc, coords_array[in_my_loc].x1000, item_array[my_item].line_no
		     , item_array[my_item].section_v[0], item_array[my_item].section, item_array[my_item].is_special
		     , yytext, page_no, page_no_type
		     , coords_array[my_loc-1].x1000
		     , header
		     );

  item_array[my_item].pid = (title_pid >=0) ? title_pid : pid; // in case we have "<P 21>ARTICLE I<P 22>INTRODUCTION<P>" we want to capture the initial ID, 21.
  item_array[my_item].section = (strlen(my_text)) ? strdup(my_text) : "-";
  item_array[my_item].title = strdup(title);
  char *new_header = (strcmp(title,"Lease") == 0) ? "-" : clean_leading_spaces(header);
  item_array[my_item].header = (strlen(new_header) == 0) ? "-" : new_header;

  int no_of_Caps = 0;
  int no_of_CAPs = 0;
  int now;
  item_array[my_item].no_of_words = now = calc_no_of_words_in_header(item_array[my_item].header, &no_of_Caps, &no_of_CAPs);
  item_array[my_item].no_of_all_CAPs_words = no_of_CAPs;
  item_array[my_item].no_of_first_Caps_words = no_of_Caps;

  if (0 && debug) fprintf(stderr,"\nOOO:in=%d: text=%s:  CAPS:%d: grp=%d: title=%s: new_h=%s: orig_h=%s:\n"
		     , my_item
		     , my_text
		     , item_array[my_item].CAP_hdr_len
		     , my_group
		     , title
		     , item_array[my_item].header
		     , header);
  item_array[my_item].selected_seq = -1;
  if (my_group > 20 && my_group < 31) { // remove first "(" for PCH
    static char vvv[500];
    strcpy(vvv,my_text+1);
    my_text = vvv;
  }

  if (my_group%10 == 6) { // TWENTY TWO
    do_twenty_two(my_text, my_item);
  } else {

    static char orig_text[30];
    static char new_text[30];    
    strcpy(orig_text,my_text);
    strcpy(new_text,my_text);    

    { // get the value for each one of the fields of the section
      char *sep = ".,:-)";

      char *pch = strtok(new_text,sep);

      int ii = 0;

      while (pch != NULL && ii < MAX_SECTION_N) { 
	item_array[my_item].section_s[ii] = strdup(pch);
	item_array[my_item].section_m[ii] = orig_text[strlen(pch)];
	{ // how do you calculate the value for different types?
	  //if (debug) fprintf(stderr,"ROM:%s: item:%d: group=%d:\n",pch,my_item,my_group);
	  calculate_value_for_each_type(ii, my_group, my_item, pch);
	}
	pch = strtok(NULL,sep);
	ii++;
      } // while on fields

      item_array[my_item].section_no = ii;
      item_array[my_item].my_enum = item_array[my_item].section_v[ii - 1];
      item_array[my_item].lower_enum = (ii > 1) ? item_array[my_item].section_v[ii - 2] : -1;

    } //  get the value for each one of the fields of the section
  } // if not TWENTY TWP

  
  /****** GROUP **********************/
  int vvv = item_array[my_item].section_no-1;
  int my_P_enum =  (vvv > 0) ? item_array[my_item].section_v[vvv-1] :0; 
  //fprintf(stderr,"UUU0:ii=%d: gg=%d: me=%d:\n",my_item,my_group, item_array[my_item].section_v[vvv]);
  insert_into_group(my_item, my_group, item_array[my_item].section_v[vvv], my_P_enum, 0);
  char *orig_first_field = item_array[my_item].section_s[0];
  if (orig_first_field) { // OCR ERROR CORRECTION
    ocr_error_correction(orig_first_field,my_group,my_item, my_P_enum);
  }
  /***********************************/

  max_item_no = my_item+1;
  if (max_item_no >= MAX_TOC-1) {
    fprintf(stderr,"Error: max_item_no =%d >= MAX_TOC\n",max_item_no);
  }
  title = "_";

  if (debug) fprintf(stderr,"\nTTTTTTTTTTTTT7:in=%d: grp=%d: pp=%d:%d: loci=:%d:%d: tid=%d:%d: c=%d: title=:%s:%s: XXX:%c:%d: ln=%d: sec=%d:%s: vvv=%d:%d:\n" 
		     , my_item, item_array[my_item].grp_no
		     , title_pid,pid
		     , in_my_loc, my_loc
		     , token_id,token_array[token_id].line_no, item_array[my_item].center
		     , title,  header
		     , coords_array[in_my_loc].cc, coords_array[in_my_loc].x1000
		     , item_array[my_item].line_no
		     , item_array[my_item].section_v[0], item_array[my_item].section
		     , vvv, my_P_enum
		     );
 
  return 0;
} // insert_item()

int run_insert_item_from_array(int insert_item_no) {
  item_no = max_item_no = 0;
  init_groups();
   
   int ii;
   fprintf(stderr,"RUN_INSERT:%d:\n",insert_item_no);
   for (ii = 0; ii < insert_item_no; ii++) {
     int pass_no = 1;
     fprintf(stderr,"   INSERTING ITEM: ii=%d: my_item=%d: pid=%d: grp=%d: :%s:%s:%s:\n"
	     ,ii,insert_item_array[ii].my_item,insert_item_array[ii].pid, insert_item_array[ii].my_group,insert_item_array[ii].title,insert_item_array[ii].my_text,insert_item_array[ii].header);
     insert_item(
		 0
		 ,ii // insert_item_array[ii].my_item
		 , insert_item_array[ii].my_line
		 , insert_item_array[ii].in_my_loc
		 , insert_item_array[ii].my_group
		 , insert_item_array[ii].my_text
		 , insert_item_array[ii].title		 
		 , insert_item_array[ii].header
		 , insert_item_array[ii].is_special
		 , insert_item_array[ii].title_pid		 
		 , insert_item_array[ii].pid		 
		 , pass_no
		 );
   }

   return 0;
}

// add an seq item into the insert_item_array that was missing
 int add_one_extra_item_to_insert_item_array(char *merged_header, int para2, int pos2, int line2, int is_special, int toc_item1) {
   int ii;
   int found = 0;
   for (ii = 0; ii < insert_item_no; ii++) {
     if (para2 == insert_item_array[ii].pid) {
       found = 1;
       break;
     }
   }
   if (found == 0) {
     insert_item_array[insert_item_no].my_line = line2;
     //insert_item_array[insert_item_no].page_no  = -1;
     insert_item_array[insert_item_no].in_my_loc = pos2;
     //insert_item_array[insert_item_no].left_X = -1;
     //insert_item_array[insert_item_no].center = -1;
     insert_item_array[insert_item_no].my_group = determine_group_for_header(item_array[toc_item1].section, item_array[toc_item1].title);
     //insert_item_array[insert_item_no].section = "";
     insert_item_array[insert_item_no].my_text = item_array[toc_item1].section;
     insert_item_array[insert_item_no].title = "";     
     insert_item_array[insert_item_no].header =  item_array[toc_item1].header;
     insert_item_array[insert_item_no].is_special = is_special;
     insert_item_array[insert_item_no].title_pid = para2;
     insert_item_array[insert_item_no].pid = para2;
     /*
       int no_of_Caps = 0;
       int no_of_CAPs = 0;
       insert_item_array[insert_item_no].no_of_words = calc_no_of_words_in_header(insert_item_array[insert_item_no].header, &no_of_Caps, &no_of_CAPs);
       insert_item_array[insert_item_no].no_of_all_CAPs_words = no_of_CAPs;
       insert_item_array[insert_item_no].no_of_first_Caps_words = no_of_Caps;  
     */
     fprintf(stderr,"NOW INSERT_ITEM_NO:%d: gr=%d: :%s:%s:  :%d: :%d:%s:%s:\n",insert_item_no
	     , insert_item_array[insert_item_no].my_group
	     ,  insert_item_array[insert_item_no].my_text,  insert_item_array[insert_item_no].header,  insert_item_array[insert_item_no].pid, toc_item1, item_array[toc_item1].section, item_array[toc_item1].header);
     insert_item_no++;
   }
   return insert_item_no;
} // add_one_extra_item_to_insert_item_array()



 
int calculate_exhibit_header(int nn, char mlabel[], char mhd[], char t_title[], int old_loc, char *vtext) {
  int old_loc_1 = old_loc -1;
  fprintf(stderr,"UUUUUUUUUOOOO nn=%d: mlabel=%s: mhd=%s: t_title=%s: old_loc=%d: vtext=%s:\n",nn, mlabel, mhd, t_title, old_loc, vtext);
  int line_id = -1;
  int page_id = -1;
  int sn[5];
  int center[5];
  int left_X[5], right_X[5];
  int now[5];
  int noW[5];
  int ll;
  int CENTER_THRESH = 8000;
  if (nn < 2 // if no mhd found
      || (nn == 2 && strlen(mhd) < 4)) { // or if found but it is less than 4 chars
    sn[0] = reverse_token_array[old_loc_1];
    line_id = token_array[sn[0]].line_no;
    page_id = token_array[sn[0]].page_no;
    fprintf(stderr,"      OOO1:loc=%d: sn[0]=%d: ll= %d: pp=%d:\n",old_loc_1, sn[0], line_id, page_id);
    for (ll = 0; ll < 5; ll++) {
      center[ll] = page_line_properties_array[page_id][line_id+ll].center;
      left_X[ll] = page_line_properties_array[page_id][line_id+ll].left_X;
      right_X[ll] = page_line_properties_array[page_id][line_id+ll].right_X;            
      now[ll] = page_line_properties_array[page_id][line_id+ll].no_of_words;
      noW[ll] = page_line_properties_array[page_id][line_id+ll].no_of_first_cap_words;      
    }

    sn[1] = sn[0] + now[0];
    sn[2] = sn[1] + now[1];
    sn[3] = sn[2] + now[2];
    sn[4] = sn[3] + now[3];        

    if (debug) fprintf(stderr,"\n           CONCAT_LINEOR:pl=%d:%d: c=%d:%d:%d:%d:  l=%d:%d:%d:%d:  r=%d:%d:%d:%d:  now=%d:%d:%d:%d:  Now=%d:%d:%d:%d: mhd=%s:\n"
		       , page_id, line_id
		       , center[0], center[1], center[2], center[3]
		       , left_X[0], left_X[1], left_X[2], left_X[3]
		       , right_X[0], right_X[1], right_X[2], right_X[3]		       
		       , now[0], now[1], now[2], now[3],   noW[0], noW[1], noW[2], noW[3], mhd);


    if (abs(center[0] - center[1]) < CENTER_THRESH
	&& now[0] < 10
	&& now[1] < 10
	//&& noW[0] * 2 >= now[0]
	//&& noW[1] * 2 >= now[1]
	) {
      int ii;
      for (ii = 0; ii < now[1]; ii++) {
	strcat(mhd, token_array[sn[1]+ii].text);
	strcat(mhd, " ");
      }
      if (debug) fprintf(stderr,"           CONCAT_LINE0:pl=%d:%d: c=%d:%d:%d:%d:  now=%d:%d:%d:%d:  Now=%d:%d:%d:%d: mhd=%s:\n"
		       , page_id, line_id
		       , center[0], center[1], center[2], center[3],     now[0], now[1], now[2], now[3],noW[0], noW[1], noW[2], noW[3], mhd);

    } // if line1


    if (abs(center[0] - center[2]) < CENTER_THRESH
	&& now[0] < 10
	&& now[2] < 10
	&& noW[0] * 2 >= now[0]
	&& noW[2] * 2 >= now[2]
	) {
      int ii;
      for (ii = 0; ii < now[2]; ii++) {
	strcat(mhd, token_array[sn[2]+ii].text);
	strcat(mhd, " ");
      }
    } // if line2
              if (debug) fprintf(stderr,"           CONCAT_LINE1:pl=%d:%d: c=%d:%d:%d:%d:  now=%d:%d:%d:%d: Now=%d:%d:%d:%d:  mhd=%s:\n"
		       , page_id, line_id
		       , center[0], center[1], center[2], center[3],     now[0], now[1], now[2], now[3], noW[0], noW[1], noW[2], noW[3], mhd);


    if (abs(center[0] - center[3]) < CENTER_THRESH
	&& now[0] < 6
	&& now[3] < 10
	&& noW[0] * 2 >= now[0]
	&& noW[3] * 2 >= now[3]
	) {
      int ii;
      for (ii = 0; ii < now[3]; ii++) {
	strcat(mhd, token_array[sn[3]+ii].text);
	strcat(mhd, " ");
      }
    } // if line3

                 if (debug) fprintf(stderr,"           CONCAT_LINE2:pl=%d:%d: c=%d:%d:%d:%d:  now=%d:%d:%d:%d: Now=%d:%d:%d:%d:  mhd=%s:\n"
		       , page_id, line_id
		       , center[0], center[1], center[2], center[3],     now[0], now[1], now[2], now[3], noW[0], noW[1], noW[2], noW[3], mhd);


    if (abs(center[0] - center[4]) < CENTER_THRESH
	&& now[0] < 6
	&& now[4] < 10
	&& noW[0] * 2 >= now[0]
	&& noW[3] * 2 >= now[4]
	) {

      int ii;
      for (ii = 0; ii < now[4]; ii++) {
	strcat(mhd, token_array[sn[4]+ii].text);
	strcat(mhd, " ");
      }
    } // if line3

                  if (debug) fprintf(stderr,"           CONCAT_LINE3: pl=%d:%d:  c=%d:%d:%d:%d:%d:   now=%d:%d:%d:%d:%d:  Now=%d:%d:%d:%d:%d:   mhd=%s:\n"
			    , page_id,  line_id
			    , center[0], center[1], center[2], center[3], center[4],     now[0], now[1], now[2], now[3],now[4],    noW[0], noW[1], noW[2], noW[3], noW[4],    mhd);
    
  }

  if (debug) fprintf(stderr,"IIIII: nn=%d: now1=%d: vtext=%s: toc_item_no=%d: rev=%d: loci=%d: line=%d: page=%d: c01=%d:%d: ll=%s:  hd=%s:  tit=%s:\n"
	  , nn, now[1], vtext, item_no, old_loc_1
	  , reverse_token_array[old_loc_1], line_id, page_id, center[0], center[1], mlabel, mhd, t_title);

  return 0;
} // calculate_exhibit_header(

int find_bad_exhibit_pages() {
  int ii;
  int prev_line = -1;
  int prev_page = -1;
  int max_page_no = -1;
  if (debug) fprintf(stderr,"PAGE1:%d:\n",token_no);  
  for (ii = 0; ii < token_no; ii++) {

    int line_no = token_array[ii].line_no;
    int page_no = token_array[ii].page_no;    
    max_page_no = MAX(page_no+1, max_page_no);

    if (page_no != prev_page) {
        if (debug && 0) fprintf(stderr,"  PAGE222: tok=%d: pn=%d:%d: %s:\n",token_no, page_no, line_no, token_array[ii].text);
      prev_page = page_no;
    }

    if (line_no != prev_line) {
      if (debug) fprintf(stderr,"  PAGE223: ii=%d: tok=%d: pl=%d:%d: %s: x1=%s:%d:\n",ii, token_no, page_no, line_no, token_array[ii].text,  token_array[ii].x1, token_array[ii].x1_1000);      
      if (token_array[ii].text
	  && token_array[ii].x1_1000 < 700000  // frequently there's EXHIBIT in the doc but not 
	  && token_array[ii].line_no < 6  // frequently there's EXHIBIT later later in the doc	  
	  && (strncasecmp(token_array[ii].text,"schedule",5) == 0
	      || strncasecmp(token_array[ii].text,"exhibit",5) == 0)) {
	exhibit_page_line_array[page_no][line_no] = 1;

	//if (debug) fprintf(stderr,"          PAGE224: tok=%d: x1=%s:%d: pn=%d:%d: %s:%s:\n",token_no, token_array[ii].x1,token_array[ii].x1_1000, page_no, line_no, token_array[ii].text,exhibit_page_line_array[page_no][line_no], token_array[ii].text);

      }
      prev_line = line_no;

    }

  } // for
  int pp;
  int found_on_page = 0;
  if (debug) fprintf(stderr,"PAGE5:%d:\n",max_page_no);  
  int last_exhibit_line;
  for (pp = 0; pp <= max_page_no; pp++) {
    int ll;
    last_exhibit_line = -100;
    for (ll = 0; ll < MAX_LINE; ll++) {
      if (ll == 0) last_exhibit_line = -100;
      if (exhibit_page_line_array[pp][ll] == 1) {
	found_exhibit_page_array[pp]++;
	if (debug) fprintf(stderr,"MYPAGE page=%d: line=%d: total=%d: pl=%d:\n",pp,ll,found_exhibit_page_array[pp], exhibit_page_line_array[pp][ll]);	
	if (ll - last_exhibit_line < 3) {
	  exhibit_gap_less_than_3_page_array[pp] = 1;
	}
	last_exhibit_line = ll;
      }
      if (exhibit_page_line_array[pp][ll] == 1 && ll < 4) {
	exhibit_at_4_top_lines_page_array[pp] = 1;
      }


    }
    if (debug && 0) fprintf(stderr,"NPAGE:%d:%d:\n",pp,found_exhibit_page_array[pp]);
  }
  return 0;
} // find_bad_exhibit_pages() {

char *norm_fund_header(char *text) { // change from "LEASE FACE PAGE" to "Lease Face Page"
  int ll = strlen(text);
  int ii;
  int in_word = 0;
  for (ii = 0; ii < ll; ii++) {
    if (in_word == 0 && isalpha(text[ii]) != 0) {
      in_word = 1;
      text[ii] = toupper(text[ii]);
    } else if (in_word == 1 && text[ii] == ' ') {
      in_word = 0;
    } else if (in_word == 1 && isalpha(text[ii]) != 0) {
      text[ii] = tolower(text[ii]);
    }
  }
  if (debug) fprintf(stderr,"To Camel Back: text=%s:\n",text);
  return text;
}

char *toupper_word(char *text) { // change from "Lease Face Page" to "LEASE FACE PAGE"
  int ll = strlen(text);
  int ii;
  for (ii = 0; ii < ll; ii++) {
      text[ii] = toupper(text[ii]);
  }
  return text;
}
 
int copy_into_MY_TOTAL() {
  int ii;
  for (ii = 0; ii < yyleng; ii++) {
    MY_TOTAL_file[buff_ctr++] = yytext[ii];
  }
  return 0;
}

int nyu_ptr1;
int nyu_ptr2;
char nyu_buff[100000];

%}
single_section_number ONE|TWO|THREE|FOUR|FIVE|SIX|SEVEN|EIGHT|NINE
teen_section_number TEN|ELEVEN|TWELVE|THIRTEEN|FOURTEEN|FIFTEEN|SIXTEEN|SEVENTEEN|EIGHTEEN|NINETEEN
twenty_section_number TWENTY|THIRTY|FORTY|FIFTY|SIXTY|SEVENTY|EIGHTY|NINETY
section_number {single_section_number}|{teen_section_number}|({twenty_section_number}([\ \-]+{single_section_number})?)|{NUMBER}

ws [\ \t\n]*
ws1 [\ \t\n\-\:\.\,]*

NUMBER (FIRST|SECOND|THIRD|FOURTH|FIFTH|SIXTH|SEVENTH|EIGHTH|NINTH|TENTH|ELEVENTH|TWELFTH|THIRTEENTH|FOURTEENTH|FIFTEENTH|SIXTEENTH|SEVENTEENTH|EIGHTEENTH|NINETEENTH|tWENTIETH)
							     /* yes LEASE, no BASIC */
fun0 (L(ease|EASE){ws1}(I(nformation|NFORMATION)|S(ummary|UMMARY)|D(ata|ATA)|P(rovisions|ROVISIONS)|T(erms|ERMS)))
							     /* perhaps LEASE, no BASIC */
fun1 (((((B(asic|ASIC))|F(undamental|UNDAMENTAL)|S(alient|ALIENT)|(G(eneral|ENERAL))){ws1}))(L(ease|EASE){ws1})?(I(nformation|NFORMATION)|S(ummary|UMMARY)|D(ata|ATA)|P(rovisions|ROVISIONS)|T(erms|ERMS)))  
fun2 (L(ease|EASE){ws1}S(ummary|UMMARY){ws1}(S(heet|HEET)|P(age|AGE)))
fun3 (((S(ummary|UMMARY))|(D(ocument|OCUMENT))|(L(ease|EASE))|(C(ONTRACT|ontract))|(A(greement|GREEMENT))){ws1})(C(over|OVER)){ws1}(S(heet|HEET))
fun4 (Leasing{ws}Scan{ws}{ws1}S(heet|HEET))
fun5 ((L(ease|EASE){ws1})?(F(ace|ACE)|C(over|OVER)){ws1}(P(age|AGE)|S(heet|HEET)))
fun6 D(efinitions|EFINITIONS)
particulars (Particulars|PARTICULARS|((L(ease|EASE)|O(ther|THER)){ws1})?(P(articulars|ARTICULARS))|(Land{ws}Registry)|(LAND{ws}REGISTRY))
fundamentals    ({fun0}|{fun1}|{fun2}|{fun3}|{fun4}|{fun5}|{fun6}|{particulars})
																   /* PART is good for UK leases, LR is for UK e.g., LR2.2, Renral Form RF3 */
title1 (LR|RF|Paragraph|PARAGRAPH|Section|SECTION|Part|PART|Article|ARTICLE|ARTI[^\ \n]{2,6}|Exhibit|EXHIBIT|EXHI[^\ \n]{3,8}|Attachment|ATTACHMENT|Appendix|APPENDIX|Schedule|SCHEDULE|SCHED[^\ \n]{2,6}|Part|PART|Glossary|GLOSSARY|((LEASE|RENT)[ \n]+RIDER)|(([A-Z\-]+[ \n]*)*RIDER))
title (LR|RF|Paragraph|PARAGRAPH|Section|SECTION|Part|PART|Article|ARTICLE|ARTI[^\ \n]{2,6}|Exhibit|EXHIBIT|EXHI[^\ \n]{3,8}|Appendix|APPENDIX|Schedule|SCHEDULE|SCHED[^\ \n]{2,6}|Part|PART|Glossary|GLOSSARY|((LEASE|RENT)[ \n]+RIDER)|(([A-Z\-]+[ \n]*)*RIDER)|(THE{ws}SCHEDULE)|(THE{ws}{NUMBER}{ws}SCHEDULE))

lori_title "BASIC"{ws1}"LEASE"{ws1}"TERMS"
marker [\.\,]
marker1111111 [\.\,\)\-\:]
				   /* removed all the other options as ocr is more accurate */
marker1 [\.\)] 
marker3 [\.\)\-\:] 
no_post [^0-9A-Za-z_]

FUNDAMENTAL_POINT   [A-Z\ \n\'\&\-\(\)]+\:
Fundamental_Point   [A-Z][A-Za-z\ \n\'\&\-\(\)]+\:


				   /****************** DONE PREVENT *************/
				   // prevent before SECTION, as in "in section 13"
in_section [\n\ \,\.\;](In|in|This|this|under|Under|of|Of|with|With|to|To|On|on|The|the|an|attached|any|all|for|thereto|See|see|and|or|as)
				   // prevent after SECTION, as in "section 13 herein "
herein (and|or[^a-z]|herein|above|below|hereto|thereto|hereof|attached|shall|is|are|of|SF|sf)
				   // for prevent
word [^\>\<\ \n]+
				   /*  ws ({tag}|[\ \t\n])+ */
tag \<[^p\>\/][^\>]*\>
ws_comma ({tag}|[\ \t\n\,])+
prevent_section (Paragraph|PARAGRAPH|Section|SECTION|Article|ARTICLE|Exhibit|EXHIBIT|Attachment|ATTACHMENT|Appendix|APPENDIX|Schedule|SCHEDULE|(RENT[\ \n]+RIDER))(s)?
				   /****************** DONE PREVENT *************/

%%
   /*************  copying aaa5 into buffer.  making sure trumps all <*> during this file ******/
				   
<YYYYcopy>\n {
  MY_TOTAL_file[buff_ctr++] = yytext[0]; 
}

<YYYYcopy>. {
  MY_TOTAL_file[buff_ctr++] = yytext[0]; 
}
   /* the 7 rules below intended to trump the <*> rules in the rest of the LEX */

<YYYYcopy>"<td sn="\"0\"[^\>]*\>        copy_into_MY_TOTAL();
<YYYYcopy>"<td sn="\"[0-9]+\"[^\>]*\>   copy_into_MY_TOTAL();
<YYYYcopy>"<td id="\"0\"[^\>]*\>        copy_into_MY_TOTAL();
<YYYYcopy>"<td id="\"[0-9]+\"[^\>]*\>   copy_into_MY_TOTAL();
<YYYYcopy>"</td"[^\>]*\>                copy_into_MY_TOTAL();
<YYYYcopy>"<"[Pp][ ]+(sn|id)=\"[0-9]+\"/[\ \>\n] copy_into_MY_TOTAL();
<YYYYcopy>"<HR"[^\>]*\> copy_into_MY_TOTAL();

  /************************* replace PP by $ in rest file **************************/
  /* remember: the first '$ is added to the rest_file artificially */
<YYYY1>\<p[^\>\<]*\> {
  static int line_no = 0;
  strcat(buff_ptr,"$ ");
  static int para_no = 0;
  char *pp = strstr(yytext,"id=\"");
  sscanf(pp,"id=\"%d\"",&para_no);
  rest_line2para_no_array[line_no] = para_no-1; // this is bc the first '$ was added to the rest_file artificially
  line_no++;
 }
<YYYY1>\<[^\>\<]*\> ;
<YYYY1>\$  ;
<YYYY1>[a-zA-Z0-9]  {  // case_normalize
  static char buff[5];
  if (isalnum(yytext[0]) != 0) {
    buff[0] = yytext[0];
    buff[1] = '\0';
    strcat(buff_ptr,buff);
  } 
}

<YYYY1>\(([0-9]+|[a-z]+|[A-Z]+)\)[\ ]?  {  // (a)
  static char buff[5];

  strcpy(buff,yytext);
  strcat(buff_ptr,buff);

}

<YYYY1>[0-9]+\.  {  // 2.1, keep the period
  static char buff[5];
  if (isalnum(yytext[0]) != 0) {
    strcpy(buff,yytext);
    strcat(buff_ptr,buff);
  } 
}

<YYYY1>[0-9]+(\.)[ ]  {  // 2.1, keep also the space
  static char buff[5];
  if (isalnum(yytext[0]) != 0) {
    strcpy(buff,yytext);
    strcat(buff_ptr,buff);
  } 
}

<YYYY1>[0-9]+((\.)[0-9])+(\.)?  {  // 2.1, keep the period
  static char buff[5];
  if (isalnum(yytext[0]) != 0) {
    strcpy(buff,yytext);
    strcat(buff_ptr,buff);
  } 
}

<YYYY1>[0-9]+((\.)[0-9])+(\.)?[ ]  {  // 2.1, keep also the space
  static char buff[5];
  if (isalnum(yytext[0]) != 0) {
    strcpy(buff,yytext);
    strcat(buff_ptr,buff);
  } 
}

   /********  chop off at 50 chars in rest file *****************************************/

<YYYY2>[^\$]+ {
  static char mext[500];  
  //printf("BB0:%s:\n",yytext);
  if (yyleng > 50) {
    strcpy(mext,yytext);
    mext[49] = '\n';
    mext[50] = '\0';
  } else {
    sprintf(mext,"%s\n",yytext);
  }
  strcat(buff_ptr,mext);  
}
<YYYY2>.|\n  {static char buff[2]; sprintf(buff,"%s",yytext); strcat(buff_ptr,buff); }

   /************** remove empty lines from rest file ***********************************/

<YYYY3>(\n([\ \<])*)+ strcat(buff_ptr,"\n");
<YYYY3>.|\n  {static char buff[2]; sprintf(buff,"%s",yytext); strcat(buff_ptr,buff); }

   /**************** parse results file *********************************/
   /* 
** this is for the case that align caught two items at once:
260	3843	
$ 3. TERMLANDLORDSANDTENANTSWORK
$ 3.1 LeaseTerm

** we separate them and generate a displacement for the second lines
    */
<YYYY6>[0-9]+\t[0-9]+\t/\n\$[^\$\n]+\n\$ { // this caches the numbers only
  if (nyu_results_no >= 0) nyu_results_array[nyu_results_no].nyu_buff = strdup(nyu_buff);
  nyu_results_no++;
  sscanf(yytext,"%d\t%d",&nyu_ptr1, &nyu_ptr2);
  nyu_results_array[nyu_results_no].nyu_ptr1 = nyu_ptr1;
  nyu_results_array[nyu_results_no].nyu_ptr2 = nyu_ptr2;
  strcpy(nyu_buff,"");
  BEGIN YYYY61;
}

<YYYY61>\n\$[^\$\n]+/\n\$ { // this catches the first line and generates the numbers for the second
  int ptr1 = nyu_results_array[nyu_results_no].nyu_ptr1 + yyleng;
  int ptr2 = nyu_results_array[nyu_results_no].nyu_ptr2 + yyleng;  
  nyu_results_array[nyu_results_no].nyu_buff = strdup(yytext);
  nyu_results_no++;
  nyu_results_array[nyu_results_no].nyu_ptr1 = ptr1;
  nyu_results_array[nyu_results_no].nyu_ptr2 = ptr2;
  strcpy(nyu_buff,""); 
  BEGIN YYYY6;
}

<YYYY6>[0-9]+\t[0-9]+\t {
  if (nyu_results_no >= 0) nyu_results_array[nyu_results_no].nyu_buff = strdup(nyu_buff);
  nyu_results_no++;
  sscanf(yytext,"%d\t%d",&nyu_ptr1, &nyu_ptr2);
  nyu_results_array[nyu_results_no].nyu_ptr1 = nyu_ptr1;
  nyu_results_array[nyu_results_no].nyu_ptr2 = nyu_ptr2;
  strcpy(nyu_buff,"");
}
<YYYY6>.  if (yytext) strcat(nyu_buff,yytext);
<YYYY6>\n  strcat(nyu_buff," ");

   /*************************************************/

"<head"[^\>]*\> if (NO_TOC == 1) BEGIN shead; else BEGIN 0;
<shead>. ;
<shead>"</head"[^\>]*\> BEGIN 0;
<shead>"<body"  my_loc-=yyleng; yyless(0); BEGIN 0;

"<ZZ>"  BEGIN state_ignore; echo_to_buff(); // don't find items inside ZZ
<state_ignore>"</ZZ>" BEGIN 0; echo_to_buff(); // used to be <*> but caught too much

<*>\n  {
  /* <state_4p>\n line_no++; strcat(vpp," "); */
  line_no++; 
  if (YYSTATE == state_4p || YYSTATE == state_5p) {
    strcat(vpp,yytext);
  } else {
    echo_to_buff();
  }
}

  /***** 071513 adding [^\>]*\> so it doesn't "lose" it ***************/
<*>"<td sn="\"0\"[^\>]*\>        in_td = 0; echo_to_buff();// do DSA only in TD=0 
<*>"<td sn="\"[0-9]+\"[^\>]*\>   in_td = 1; echo_to_buff(); // don't find items in other cells
<*>"<td id="\"0\"[^\>]*\>        in_td = 0; echo_to_buff();// do DSA only in TD=0 
<*>"<td id="\"[0-9]+\"[^\>]*\>   in_td = 1; echo_to_buff(); // don't find items in other cells
<*>"</td"[^\>]*\>                in_td = 0; echo_to_buff();


<*>"<"[Pp][ ]+(sn|id)=\"[0-9]+\"/[\ \>\n] {
  //fprintf(stderr,"HHHHHHHH:loc=%d: text=%s: cc=%c:\n",my_loc-yyleng,yytext,coords_array[my_loc].cc);
  char *pp = strstr(yytext,"sn=\"");
  if (pp) {
    pid = atoi(pp+4);
  } else {
    pp = strstr(yytext,"id=\"");
    if (pp) {
      pid = atoi(pp+4);
    }
  }
  echo_to_buff(); 
  if (YYSTATE == state_4p || YYSTATE == state_5p ) {  // if header is empty, keep looking for header in next para
    BEGIN state_4p1;
  } else if (YYSTATE != state_ignore) {  
    BEGIN state_3p;  
  } else { // stay in STATE_IGNORE even when new para
    ;
  }
}



<state_3p>\> BEGIN state_2n;  d_group = 0; t_title = "";  echo_to_buff(); /* end of "<p >" */



<state_2n>([A-HJ-Z]{marker1})/{no_post} { // C.  -- "I," is taken separately to prevent "I, David R. Risk" // s/b in PREVENT!!!
  in_fundamentals_section = 0;
  in_fundamentals = 0; 
  required_fundamental_no = 0;
  if (d_group == 0) { // we came from nothing
    g_group_no = 18;
  } else if (d_group == 1){
    g_group_no = 8; // we came from SECTION
  } else if (d_group == 3){
    g_group_no = 38; // we came from PART
  } else if (d_group == 4){
    g_group_no = 48; // we came from PARA
  } else if (d_group == 5){
    g_group_no = 58; // we came from ARTICLE
  } else {
    g_group_no = 68; // we came from OTHER
  }
  do_item_BEGIN_state_4p();
}




<state_2n>([A-HJ-Z]{marker1}?)/{no_post} { // C.  -- "I," is taken separately to prevent "I, David R. Risk" // s/b in PREVENT!!!
  // I is missing!! It comes as ROMAN!!!
  in_fundamentals_section = 0;
  in_fundamentals = 0; 
  required_fundamental_no = 0;
  if (d_group == 3) { // we came from nothing
    g_group_no = 38;
  }
  do_item_BEGIN_state_4p();
}





<state_2n>I{marker3}/{no_post} { // C.  -- "I," is taken separately to prevent "I, David R. Risk" // s/b in PREVENT!!!
  in_fundamentals_section = 0;
  in_fundamentals = 0;   
  required_fundamental_no = 0;
  if (d_group == 0) { // we came from nothing
    g_group_no = 18;
  } else if (d_group == 1){
    g_group_no = 8; // we came from SECTION
  } else if (d_group == 3){
    g_group_no = 38; // we came from PART
  } else if (d_group == 4){
    g_group_no = 48; // we came from PARA
  } else if (d_group == 5){
    g_group_no = 58; // we came from ARTICLE
  } else {
    g_group_no = 68; // we came from OTHER
  }

  do_item_BEGIN_state_4p();
}


<state_2n>[ivxl]+{marker1}/{no_post} { // iii.
  in_fundamentals_section = 0; in_fundamentals = 0;
  required_fundamental_no = 0;
  if (d_group == 0) { // we came from nothing
    g_group_no = 19;
  } else if (d_group == 1){
    g_group_no = 9; // we came from SECTION
  } else if (d_group == 3){
    g_group_no = 39; // we came from PART
  } else if (d_group == 4){
    g_group_no = 49; // we came from PARA
  } else if (d_group == 5){
    g_group_no = 59; // we came from ARTICLE
  } else {
    g_group_no = 69; // we came from OTHER
  }

  do_item_BEGIN_state_4p();
}

<state_2n>[a-z]{marker1}/{no_post} { // c.
  in_fundamentals_section = 0; in_fundamentals = 0;
  required_fundamental_no = 0;
  if (d_group == 0) { // we came from nothing
    g_group_no = 17;
  } else if (d_group == 1){
    g_group_no = 7; // we came from SECTION
  } else if (d_group == 3){
    g_group_no = 37; // we came from PART
  } else if (d_group == 4){
    g_group_no = 47; // we came from PARA
  } else if (d_group == 5){
    g_group_no = 57; // we came from ARTICLE
  } else {
    g_group_no = 67; // we came from OTHER
  }

  do_item_BEGIN_state_4p();
}

<state_2n>XXI{marker1}?[a-z][a-z]+ {
  in_fundamentals_section = 0; in_fundamentals = 0;
  required_fundamental_no = 0;
  echo_to_buff();
}


<state_2n>(([IVXL][IVXL]*{marker1}?)|(I[IVXL]+{marker1}?))/{no_post} { // III.
  in_fundamentals_section = 0; in_fundamentals = 0;
  required_fundamental_no = 0;
  if (d_group == 0) { // we came from nothing
    g_group_no = 20;
  } else if (d_group == 1){
    g_group_no = 10; // we came from SECTION
  } else if (d_group == 3){
    g_group_no = 40; // we came from PART
  } else if (d_group == 4){
    g_group_no = 50; // we came from PARA
  } else if (d_group == 5){
    g_group_no = 60; // we came from ARTICLE
  } else {
    g_group_no = FILE_GROUP; // we came from OTHER
  }

  do_item_BEGIN_state_4p();
}

<state_2n>((I{marker3}))/{no_post} { // III.
  in_fundamentals_section = 0; in_fundamentals = 0;
  required_fundamental_no = 0;
  if (d_group == 0) { // we came from nothing
    g_group_no = 20;
  } else if (d_group == 1){
    g_group_no = 10; // we came from SECTION
  } else if (d_group == 3){
    g_group_no = 40; // we came from PART
  } else if (d_group == 4){
    g_group_no = 50; // we came from PARA
  } else if (d_group == 5){
    g_group_no = 60; // we came from ARTICLE
  } else {
    g_group_no = FILE_GROUP; // we came from OTHER
  }

  do_item_BEGIN_state_4p();
}

<state_2n>{section_number}/{no_post} { // III.
  in_fundamentals_section = 0; in_fundamentals = 0;
  required_fundamental_no = 0;
  if (d_group == 0) { // we came from nothing
    g_group_no = 16;
  } else if (d_group == 1){
    g_group_no = 6; // we came from SECTION
  } else if (d_group == 3){
    g_group_no = 30; // we came from PART
  } else if (d_group == 4){
    g_group_no = 46; // we came from PARA
  } else if (d_group == 5){
    g_group_no = 56; // we came from ARTICLE
  } else {
    g_group_no = 66; // we came from OTHER
  }

  do_item_BEGIN_state_4p();
}

<state_2n>[1-9][I0-9]?{marker1}?/{no_post} { // Section 12.
  // need to give relative coord
  #define CENTER_FOOTER_X1 280000
  #define BOTTOM_FOOTER_Y2 750000  
    in_fundamentals_section = 0; in_fundamentals = 0;
    required_fundamental_no = 0;  
    //fprintf(stderr,"LLL0:%s: loc=%d:%d: xy=%d:%d:\n",yytext, my_loc,my_loc-yyleng,loc_to_x1(my_loc-yyleng),loc_to_y2(my_loc-yyleng));
    int my_ignore_begin = 0;
    if (d_group == 0) { // we came from nothing
      g_group_no = 11;
      int x1_1000 = loc_to_x1(my_loc-yyleng);
      int y2_1000 = loc_to_y2(my_loc-yyleng);      
      if ((x1_1000 > CENTER_FOOTER_X1 && y2_1000 > BOTTOM_FOOTER_Y2)|| (0 && strchr(yytext,'.') == NULL)) {  // take "Section 12".  or "12.". BUT do not take "12", do not allow 12 if it's too deep in the page
	//fprintf(stderr,"LLL1:%s:%d:\n",yytext,my_loc);	
	my_loc-=yyleng; yyless(0); BEGIN 0;
	my_ignore_begin = 1;
      }
    } else if (d_group == 1) {
      g_group_no = 1; // we came from SECTION
    } else if (d_group == 3) {
      g_group_no = 31; // we came from PART
    } else if (d_group == 4) {
      g_group_no = 41; // we came from PARA
    } else if (d_group == 5) {
      g_group_no = 51; // we came from ARTICLE
    } else {
      g_group_no = 61; // we came from OTHER
    }
    if (my_ignore_begin == 0) {
      do_item_BEGIN_state_4p();
    }
}



<state_2n>[1-9I][I0-9]?{marker}[01-9Ii][0-9iI]?{marker}?/{no_post} { // 1.1
  in_fundamentals_section = 0; in_fundamentals = 0;
  required_fundamental_no = 0;  
  sub_section_function(yytext); // includes do_item
  // echo_to_buff();
}

<state_2n>[1-9I][I0-9]?[-][01-9Ii][0-9iI]?{marker}?/{no_post} { // 3-5 when you have two numbers like this it might be 3.5
  in_fundamentals_section = 0; in_fundamentals = 0;
  required_fundamental_no = 0;  
  static char bext[20];
  strcpy(bext,yytext);
  char *p = strchr(bext,'.');
  if (p) {
    p[0] = '.';
  }
  sub_section_function(bext); // includes do_item
  // echo_to_buff()
}

<state_2n>[1-9][0-9]?{marker}[1-9iI][0-9Ii]?{marker}[1-9Ii][0-9Ii]?{marker}?/{no_post} { // 1.1.1
  in_fundamentals_section = 0; in_fundamentals = 0;
  required_fundamental_no = 0;  
  // t_title = "_";
  if (d_group == 0) { // we came from nothing
    g_group_no = 13;
  } else if (d_group == 1){
    g_group_no = 3; // we came from SECTION
  } else if (d_group == 3){
    g_group_no = 33; // we came from PART
  } else if (d_group == 4){
    g_group_no = 43; // we came from PARA
  } else if (d_group == 5){
    g_group_no = 53; // we came from ARTICLE
  } else {
    g_group_no = 63; // we came from OTHER
  }

  do_item_BEGIN_state_4p();
}

<state_2n>[1-9][0-9]?{marker}[1-9Ii][0-9Ii]?{marker}[1-9Ii][0-9Ii]?{marker}[1-9Ii][0-9Ii]?{marker}?/{no_post} { // 1.1.1.1
  in_fundamentals_section = 0; in_fundamentals = 0;
  required_fundamental_no = 0;  
  // t_title = "_";
  if (d_group == 0) { // we came from nothing
    g_group_no = 14;
  } else if (d_group == 1){
    g_group_no = 4; // we came from SECTION
  } else if (d_group == 3){
    g_group_no = 34; // we came from PART
  } else if (d_group == 4){
    g_group_no = 44; // we came from PARA
  } else if (d_group == 5){
    g_group_no = 54; // we came from ARTICLE
  } else {
    g_group_no = 64; // we came from OTHER
  }
  
  do_item_BEGIN_state_4p();
}

<state_2n>[1-9][0-9]?{marker}[1-9Ii][0-9Ii]?{marker}[1-9Ii][0-9Ii]?{marker}[1-9Ii][0-9Ii]?{marker}[1-9Ii][0-9Ii]?{marker}?/{no_post} { // 1.1.1.1.1
  in_fundamentals_section = 0; in_fundamentals = 0;
  required_fundamental_no = 0;  
  // t_title = "_";
  g_group_no = 15;
  do_item_BEGIN_state_4p();
}

<state_2n>\([a-z]\) {  // (a)
  g_group_no = 27;
  /**************************/
  t_title = "_";
  in_fundamentals_section = 0; in_fundamentals = 0;
  required_fundamental_no = 0;  
  
  do_item_BEGIN_state_4p();
}

<state_2n>\([A-Z]\) {  // (A)
  g_group_no = 28;
  /**************************/
  t_title = "_";
  in_fundamentals_section = 0; in_fundamentals = 0;
  required_fundamental_no = 0;  
  
  do_item_BEGIN_state_4p();
}

<state_2n>\([ivx]+\) { // (xxi)
  g_group_no = 29;
  /**************************/
  t_title = "_";
  do_item_BEGIN_state_4p();
}

<state_2n>\([IVX]+\) {  // (XXI)
  g_group_no = 30;
  /**************************/
  t_title = "_";
  do_item_BEGIN_state_4p();
}

<state_2n>\([1-9][0-9]?\) {  // (79)
  g_group_no = 21;
  /**************************/
  t_title = "_";
  do_item_BEGIN_state_4p();
}

<state_2n>{lori_title} { // lori_title is "basic lease terms" or such
  in_fundamentals_section = 0; in_fundamentals = 0;
  required_fundamental_no = 0;  
  title_pid = pid; //    fprintf(stderr,"TTT13:%d:%d:\n",title_pid,pid);
  t_title = strdup(yytext);
  BEGIN state_exhibit; 
  strcpy(vtext,"");
  echo_to_buff_for_exhibit();
 }

  /*  taking care of headers */
<state_2n>{title}/[^a-zA-Z] { // title is all of exhibit or section
  in_fundamentals_section = 0; in_fundamentals = 0;
  required_fundamental_no = 0;  
  old_loc = my_loc - yyleng; // for EXHIBIT
  int token_id = reverse_token_array[old_loc];
  int page_no = token_array[token_id].page_no;
  title_pid = pid;     
  t_title = strdup(yytext);
  int no_of_exhibits_on_page = found_exhibit_page_array[page_no];
  /* 
     46 is the number of chars added at the beginning of the dsafile in calc_page_params.c:
     printf("<HTML><BODY>\n\
     <DIV class=\"H1\" lev=\"1\" name=\"_\">\n");

   */

  if (debug) fprintf(stderr,"        TTT13: pp=%d:%d: old_loc=%d:%d: line=%d:%d: text=%s:%s: pn=%d: no_ex=%d: t_title=%s: item_no=%d:\n"
		     , title_pid, pid
		     , old_loc, my_loc
		     , loc_to_line(old_loc-1),loc_to_line(my_loc-1), yytext,t_title, page_no, no_of_exhibits_on_page, t_title,item_no);  
  if (0
      || strncasecmp(t_title,"EXHI",4) == 0 

      || strstr(t_title,"RIDER") != NULL || strstr(t_title,"Rider") != NULL

      || normcmp(t_title,"RENT RIDER") == 0 || normcmp(t_title,"Rent Rider") == 0

      || normcmp(t_title,"LEASE RIDER") == 0 || normcmp(t_title,"Lease Rider") == 0       

      || strcasecmp(t_title,"LEASE SUMMARY SHEET") == 0 
      || strcasecmp(t_title,"GLOSSARY") == 0
      || strncasecmp(t_title,"sched",5) == 0 
      //|| strcasecmp(t_title,"attachment") == 0
      || strcasecmp(t_title,"addendum") == 0      
      || strcasecmp(t_title,"appendix") == 0)
    {// special treatment for EXHIBIT since no good enumeration

      if (no_of_exhibits_on_page > 1) {
	BEGIN 0;
	echo_to_buff();
	
      } else if ((strcasecmp(t_title,"EXHIBIT") == 0 || strcasecmp(t_title,"schedule") == 0)
		 && (loc_to_line(my_loc) > 2 && strcmp(doc_country,"GBR") != 0)
		 )
	{ // don't take an exhibit that is more than 3 lines deep in the page
	BEGIN 0;
	echo_to_buff();
	if (debug) fprintf(stderr,"FOUND BAD EXHIB:%d:%d: 4/3:%d:%d: state=%d: country=%s:\n",page_no,no_of_exhibits_on_page, exhibit_at_4_top_lines_page_array[page_no], exhibit_gap_less_than_3_page_array[page_no],YYSTATE, doc_country);	
      } else {
	BEGIN state_exhibit;
      }
      strcpy(vtext,"");
      echo_to_buff_for_exhibit();
      if (debug) fprintf(stderr,"NO OF EXHIB:%d:%d: 4/3:%d:%d: state=%d:\n",page_no,no_of_exhibits_on_page, exhibit_at_4_top_lines_page_array[page_no], exhibit_gap_less_than_3_page_array[page_no],YYSTATE);
    } else if (strcasecmp(t_title,"SECTION") == 0) {
    BEGIN state_2n;
    d_group = 1;
    echo_to_buff_for_title();
  } else if (strcasecmp(t_title,"PART") == 0) {
    BEGIN state_2n;
    d_group = 3;
    echo_to_buff_for_title();
  } else if (strcasecmp(t_title,"PARAGRAPH") == 0 || strcasecmp(t_title,"LR") == 0 || strcasecmp(t_title,"RF") == 0) {
    BEGIN state_2n;
    d_group = 4;
    echo_to_buff_for_title();
  } else if (strcasecmp(t_title,"ARTICLE") == 0) { 
    BEGIN state_2n;
    d_group = 5;
    echo_to_buff_for_title();
  } else {
    BEGIN state_2n;
    d_group = 2;
    echo_to_buff_for_title();
  }
}  

  /*  taking care of BASIC TERMS */
<state_2n>{fundamentals} { // <p> Basic Lease Provisions
  old_loc = my_loc - yyleng; 
  strcpy(vtext,"");
  fundamentals_header = strdup(yytext);  
  echo_to_buff_for_exhibit();
  BEGIN state_exhibit;
  in_fundamentals = 1;  // we are in the fundamentals page
  fundamentals_no = 0;   // reset to 0 
}  


<state_exhibit>"</"[Pp] {
  char mhd[5000],mlabel[100];
  int nn = 0;
  int is_special = 0;
  if (in_fundamentals == 1 || in_fundamentals_section == 1) {
    is_special = FUNDAMENTALS_SPECIAL;
    g_group_no = FUNDAMENTALS_GROUP;
    strcpy(mhd,norm_fund_header(fundamentals_header));
    strcpy(mlabel,"--");
  } else { // it's an exhibit, rider, schedule ,etc
    is_special = EXHIBIT_SPECIAL;
    g_group_no = FILE_GROUP; 
    strcpy(mhd,"");
    nn = sscanf(vtext,"%s %[^.]",mlabel,mhd);
    if (nn < 1) {
      mlabel[0] = '\0';
      mhd[0] = '\0';
    } else if (nn == 1) {
      mhd[0] = '\0';
    } else {
      ;
    }
  }

  //fprintf(stderr,"UUUIII vt=%s:  ml=%s: mh=%s:\n",vtext, mlabel,mhd);
    
  calculate_exhibit_header(nn, mlabel, mhd, t_title, old_loc, vtext);
  int pass_no = 0;
  insert_item(1, item_no++,line_no,old_loc,g_group_no,mlabel,t_title, mhd, is_special, title_pid, pid, pass_no);
  t_title = "_";
  echo_to_buff();
}


   /* we get here from all the STATE_2N guys */
<state_4p>"\<" BEGIN state_4p1; echo_to_buff();


<state_4p>XXXXXXX(\(([a-zA-Z]|[1-9])\)|{title}|([0-9](\.)[\ \t\n])|([a-zA-Z](\.)[\ \t\n]))  {  // when we have Section 13\n(A)
    BEGIN state_2n;  my_loc-=yyleng; yyless(0);
    int is_special = 0;
    int pass_no = 0;
    insert_item(2, item_no++, old_ln, old_loc, g_group_no, t_text, t_title, vpp,     is_special, title_pid, pid, pass_no);
}

<state_4p>.  {
    //fprintf(stderr,"RRR/RRR: %d:%d:%d:\n",cpp,MAX_HEAD_LEN,g_allowed_length_fundamental_point);
  if (cpp++ < MAX_HEAD_LEN) {
    strcat(vpp,yytext); 
  }
  if (cpp >= g_allowed_length_fundamental_point) { // don't read past the last word of a header limited by HSPACE
    BEGIN 0;  my_loc-=yyleng; yyless(0);
    int is_special = (required_fundamental_no == 1) ? FUNDAMENTAL_POINT_SPECIAL : 0;
    int pass_no = 0;
    insert_item(3, item_no++, old_ln, old_loc, g_group_no, t_text, t_title, vpp, is_special, title_pid, pid, pass_no);
    //required_fundamental_no = 0;  
    
    t_title = "_";

  }

  if (!zero0) {
    echo_to_buff();
  }
}

<state_4p>{fundamentals}  { // Article I. Basic Lease Terms
  strcat(vpp,yytext);
  in_fundamentals_section = 1; // we are now in a FUND page
  fundamentals_no = 0;  // init the counter
}


<state_4p1>[^\>] echo_to_buff(); // state_4p1 is when we find a <p inside a header (or before the header started)
<state_4p1>[\>] ; BEGIN state_4p; echo_to_buff();  // 4p is for a header?

<state_4p>"</"[Pp]">"|"<ZZ>" { // one problem: we might be screwing on this situation: <p>SECTION 1</p><p>1.01 Teachers</p>: the first item had no header so it ate the second title
  if (found_char_in_vpp(vpp) == 1) {
    BEGIN 0;  my_loc-=yyleng; yyless(0);
    int is_special = (required_fundamental_no == 1) ? FUNDAMENTAL_POINT_SPECIAL : 0;
    int pass_no = 0;
    insert_item(4, item_no++, old_ln, old_loc, g_group_no, t_text, t_title, vpp, is_special, title_pid, pid, pass_no);

    required_fundamental_no = 0;  
    
    t_title = "_";
    if (zero0) {
      echo_to_buff_for_item(); // ECO3
    }
    // echo_to_buff(); no need yyless
  } else {
    //if (debug) fprintf(stderr,"GFF0:%d:%s:\n",found_char_in_vpp(vpp),vpp);
    strcat(vpp," ");
    cpp++;
    echo_to_buff();
  }
  // no need for echo_to_buff()
}


<state_4p>"<p"[^\>]*\>[\ \n]*("</"[Pp]">"|"<ZZ>")*({title}|([0-9]+\.[0-9])|\([A-Z]\)) { // one problem: we might be screwing on this situation: <p>SECTION 1</p><p>1.01 Teachers</p>: the first item had no header so it ate the second title
    BEGIN 0;  my_loc-=yyleng; yyless(0);
    int is_special = 0;
    int pass_no = 0;
    insert_item(5, item_no++, old_ln, old_loc, g_group_no, t_text, t_title, "_", is_special, title_pid, pid, pass_no);
    t_title = "_";
    if (zero0) {
      echo_to_buff_for_item(); // ECO3
    }
}


<state_exhibit>. strcat(vtext,yytext);  echo_to_buff();


<state_2n>(EXHIBITS|Exhibits) {
  BEGIN 0;  my_loc-=yyleng; yyless(0);
}

<state_2n>{FUNDAMENTAL_POINT} { // detect ALL CAPS ("TENANT'S ADDRESS:") title after "FACE PAGE"
  /* 
     If the POINT ends with HSPACE and not with ":" then it is very complex to find the header using LEX
     First, we use the HORIZONTAL table to decide there is a POINT on this para
     if (hs_token > -1  below
     Second, we use the diff between the LOCs of last and first tokens to decide when to stop the headers. 
     (g_allowed_length_fundamental_point = token_array[hs_token].loc - token_array[token_id].loc) below
  */
  int disp = 0;
  for (disp = 0; yytext[disp] == ' ' && disp < yyleng; disp++) { // the token might come after several spaces
    ;
  }
  static char my_text[500];
  strcpy(my_text,yytext);
  BEGIN 0;  my_loc -= yyleng; yyless(0);

  int token_id = reverse_token_array[my_loc+disp];
  if (debug) fprintf(stderr,"   BTTTTTTTT0: my_loc=%d: tid=%d: t=%s: disp=%d:\n",my_loc,token_id,my_text, disp);
  if ((in_fundamentals == 1 || in_fundamentals_section == 1)
      && !strcasestr(my_text,"recitals")
      && !strcasestr(my_text,"witnesseth"))
      /*&& token2point[token_id] > 0) */ { // generate a POINT only if we ran into a FACE PAGE header on this page
    if (debug) fprintf(stderr,"      BTTTTTTTT1: my_loc=%d: tid=%d: point=%d:\n",my_loc,token_id,token2point[token_id]);    
    if (do_fundamentals) {
      required_fundamental_no = 1; // no section_no is provided so we need to dispense a number
    }
    g_group_no = FUNDAMENTAL_POINT_GROUP_NO;
    t_title = "_";
    do_item_BEGIN_state_4p();
  }
}


<state_2n>{Fundamental_Point} { // detect First Caps title after "FACE PAGE"
  int disp = 0;
  for (disp = 0; yytext[disp] == ' ' && disp < yyleng; disp++) { // the token might come after several spaces
    ;
  }
  static char my_text[500];
  strcpy(my_text,yytext);
  BEGIN 0;  my_loc -= yyleng; yyless(0);
  int token_id = reverse_token_array[my_loc+disp];
  if (debug) fprintf(stderr,"   BTTTTTTTTA: my_loc=%d: tid=%d:  t=%s: disp=%d: IF=%d:%d: t2p=%d:\n",my_loc,token_id, my_text, disp, in_fundamentals,in_fundamentals_section, token2point[token_id]);
  if ((in_fundamentals == 1 || in_fundamentals_section == 1)
      && !strcasestr(my_text,"recitals")
      && !strcasestr(my_text,"witnesseth"))
    /*&& token2point[token_id] > 0) */ { // generate a POINT only if we ran into a FACE PAGE header on this page
    if (debug) fprintf(stderr,"      BTTTTTTTTB: my_loc=%d: tid=%d: point=%d:\n",my_loc,token_id,token2point[token_id]);    
    if (do_fundamentals) {
      required_fundamental_no = 1; // no section_no is provided so we need to dispense a number
    }
    g_group_no = FUNDAMENTAL_POINT_GROUP_NO;
    t_title = "_";
    do_item_BEGIN_state_4p();
  }
}


<state_2n>[^\ \n] {
  BEGIN 0; my_loc-=yyleng; yyless(0);
  int token_id = reverse_token_array[my_loc];
  int hs_token = loc_to_hspace_token(my_loc); // the token just before the space; -1 if no hspace

  if (hs_token > -1 && (in_fundamentals == 1 || in_fundamentals_section == 1)) {
    BEGIN state_2nA;
    g_allowed_length_fundamental_point = token_array[hs_token].loc - token_array[token_id].loc;
  } else {
    g_allowed_length_fundamental_point = 100000;
  }
  //fprintf(stderr,"PID/LINE/PAGE=%d: tok=%d:%d:  %d:%d: diff=%d:\n",pid,  token_id,token_array[token_id].loc, hs_token,token_array[hs_token].loc,g_allowed_length_fundamental_point);
}

<state_2nA>[^\ \n]+ {
  BEGIN 0; my_loc-=yyleng; yyless(0);
  if (do_fundamentals) {
    required_fundamental_no = 1; // no section_no is provided so we need to dispense a number
  }
  g_group_no = FUNDAMENTAL_POINT_GROUP_NO;
  t_title = "_";
  do_item_BEGIN_state_4p();
}

<*>"<HR"[^\>]*\> {
  sscanf(yytext,"<HR pn=%d",&hr_pn);
  echo_to_buff();
  in_fundamentals = 0; // for now until we extend using clustering
  //in_fundamentals_section = 0; // for now until we extend using clustering   // NOW CHANGES ON TOC
  if (YYSTATE == state_4p) BEGIN 0;
  fprintf(stderr,"HRRR=%d:\n",hr_pn);
}

   /******************************  PREVENT ***********************************************************/

<INITIAL>{in_section}{ws}*"</p>"{ws}*"<p"[^\>]*\> {  // prevent <p></p> in Section 3
  echo_to_buff();
  fprintf(stderr,"EEEE:%d: :%s:\n",YYSTATE, yytext);
}

<INITIAL>{in_section}{ws}{prevent_section}({ws}[A-Z0-9]+([\.,][I0-9]+)*)?{ws}*(\([\']?[A-Za-z0-9]{1,2}[\']?\){ws_comma})?{herein} { // in section A - 1  */
  echo_to_buff();
  fprintf(stderr,"GGGG:%d: :%s:\n",YYSTATE, yytext);
}

<INITIAL>{prevent_section}({ws}[A-Z0-9]+([\.,][I0-9]+)*)?{ws}*(\([\']?[A-Za-z0-9]{1,2}[\']?\){ws_comma})?{herein} { // in section A - 1 hereof  */
  echo_to_buff();
  fprintf(stderr,"HHHH:%d: :%s:\n",YYSTATE, yytext);
}

   /******************************  PREVENT ***********************************************************/
<*>. {
  echo_to_buff();
}


<*>\n
%%

int echo_to_buff() {
  int jj;
  for (jj = 0; jj < yyleng; jj++) {
    out_buff[buff_len++] = yytext[jj];
  }
  return jj;
}

int MY_ECO1() { // section
  static char ttt[10];
  sprintf(ttt,"%d",item_no);
  int jj;
  out_buff[buff_len++] = '<';
  out_buff[buff_len++] = 'A';
  out_buff[buff_len++] = '1';
  out_buff[buff_len++] = '=';
  for (jj = 0; jj < strlen(ttt); jj++) {
    out_buff[buff_len++] = ttt[jj];
  }
  out_buff[buff_len++] = ':';
  for (jj = 0; jj < yyleng; jj++) {
    out_buff[buff_len++] = yytext[jj];
  }
  out_buff[buff_len++] = '>';
  return jj;
}

int echo_to_buff_for_title() { // title
  static char ttt[10];
  sprintf(ttt,"%d",item_no);
  int jj;
  out_buff[buff_len++] = '<';
  out_buff[buff_len++] = 'A';
  out_buff[buff_len++] = '2';
  out_buff[buff_len++] = '=';
  for (jj = 0; jj < strlen(ttt); jj++) {
    out_buff[buff_len++] = ttt[jj];
  }
  out_buff[buff_len++] = ':';
  for (jj = 0; jj < yyleng; jj++) {
    out_buff[buff_len++] = yytext[jj];
  }
  out_buff[buff_len++] = '>';
  return jj;
}
int echo_to_buff_for_vpp(char *my_buff) { // title
  static char ttt[10];
  sprintf(ttt,"%d",item_no);
  int jj;
  out_buff[buff_len++] = '<';
  out_buff[buff_len++] = 'A';
  out_buff[buff_len++] = '2';
  out_buff[buff_len++] = '=';
  for (jj = 0; jj < strlen(ttt); jj++) {
    out_buff[buff_len++] = ttt[jj];
  }
  out_buff[buff_len++] = ':';
  for (jj = 0; jj < yyleng; jj++) {
    out_buff[buff_len++] = my_buff[jj];
  }
  out_buff[buff_len++] = '>';
  return jj;
}

int echo_to_buff_for_exhibit() { // title
  static char ttt[10];
  sprintf(ttt,"%d",item_no);
  int jj;
  out_buff[buff_len++] = '<';
  out_buff[buff_len++] = 'A';
  out_buff[buff_len++] = '4';
  out_buff[buff_len++] = '=';
  for (jj = 0; jj < strlen(ttt); jj++) {
    out_buff[buff_len++] = ttt[jj];
  }
  out_buff[buff_len++] = ':';
  for (jj = 0; jj < yyleng; jj++) {
    out_buff[buff_len++] = yytext[jj];
  }
  out_buff[buff_len++] = '>';
  return jj;
}

int echo_to_buff_for_item() { // vpp
  static char ttt[10];
  sprintf(ttt,"%d",item_no);
  int jj;
  out_buff[buff_len++] = '<';
  out_buff[buff_len++] = 'A';
  out_buff[buff_len++] = '3';
  out_buff[buff_len++] = '=';
  for (jj = 0; jj < strlen(ttt); jj++) {
    out_buff[buff_len++] = ttt[jj];
  }
  out_buff[buff_len++] = ':';

  int found = 0;
  int no_of_leading_spaces = 0;
  int starting = 1;
  if (1) { // TEMP!!!
  for (jj = 0; (jj < strlen(vpp)); jj++) {
    if ((vpp[jj] == ' '/* || vpp[jj] == '\n'*/) && starting == 1) {
      no_of_leading_spaces++;
    } else {
      item_array[item_no-1].no_of_leading_spaces = no_of_leading_spaces;
      starting = 0;
    }
    out_buff[buff_len++] = vpp[jj];
    if (jj == strlen(item_array[item_no-1].clean_header) + no_of_leading_spaces) {
      out_buff[buff_len++] = '>';
      found = 1;
    }
  }
  } // if (1)
  if (found == 0) {
    out_buff[buff_len++] = '>';
  }
  return jj;
}


int do_item_BEGIN_state_4p() {
  static char buff[10];
  // decide whether to show the original number or the fundamental_no
  if (required_fundamental_no == 1 // we did not come from "1.  Tenant" 
      && g_group_no == FUNDAMENTAL_POINT_GROUP_NO // we are in the right group
      && (in_fundamentals == 1 || in_fundamentals_section == 1) // we are still in the zone
      )  {
    //sprintf(buff,"%c.",'a' + fundamentals_no++);
    if (prev_g_group_no != 11) {
      sprintf(buff,"%d.", ++fundamentals_no);
    } else {
      sprintf(buff,"%c.", 'a' + fundamentals_no++);
      g_group_no = ALTERNATIVE_FUNDAMENTAL_POINT_GROUP_NO;
    }
    t_text = strdup(buff); // take care of seq counter
  } else {
    t_text = strdup(yytext);
  }
  if (0 && debug) fprintf(stderr,"        RRRRRRRRR:req=%d: in_f=%d:%d: t_text=%s: gn=%d:%d:\n",required_fundamental_no, in_fundamentals, in_fundamentals_section, t_text, prev_g_group_no, g_group_no);
  strcpy(vpp,"");
  old_ln = line_no;
  old_loc = my_loc-yyleng;
  //insert_item(6, item_no++,line_no,g_group_no,t_text,t_title);
  cpp = 0;
  BEGIN state_4p; // 4p is for a header?
  title_pid = pid;
  MY_ECO1();
  if (!(in_fundamentals == 1 || in_fundamentals_section == 1)) {
    prev_g_group_no = g_group_no; // we want to know what the originating grp was so we don't confuse
  }
  return 0;
}

int init_structures() {
  //init_groups();
  init_seqs();
  init_items_limited(); // init only derived attributes  (not original ones)
  return 0;
} // init_structures()

int init_groups() {
  int ii,jj;
  for (ii = 0; ii < MAX_GROUP; ii++) {
    for (jj = 0; jj < MAX_GROUP_ITEM; jj++) {
      group_item_array[ii][jj].item_no = 0;
      group_item_array[ii][jj].my_enum = 0;
      group_item_array[ii][jj].my_P_enum = 0;
      group_item_array[ii][jj].convert = 0;
      group_item_array[ii][jj].strong_inserted = 0;
    }
    group_header_array[ii] = NULL;
    group_no_array[ii] = 0;
  }
  return 0;
}

int init_items_limited() { // do not erase original attributes, only derived ones
  int ii;
  for (ii = 0; ii < max_item_no-1; ii++) {
    item_array[ii].selected_seq = -1;
    item_array[ii].no_of_seqs = 0;
    item_array[ii].my_enum = 0;
    item_array[ii].lower_enum = 0;
    item_array[ii].seq_array[0].seq_no = 0; // array can only have one item
    item_array[ii].seq_array[0].seq_item_no = 0; // array can only have one item
  }
  return 0;
}


int contains_text(char *cln_hdr) {
  int ret = 0;
  if (cln_hdr == NULL) ret = 0;
  else if (strlen(cln_hdr) == 0) ret = 0;
  else {
    int ii;
    for (ii = 0; ret == 0 && ii < strlen(cln_hdr); ii++) {
      if (isalnum(cln_hdr[ii]) != 0) ret = 1;
    }
  }
  return ret;
}
  


int is_number(char *text) {
  int ii;
  for (ii = 0; ii < strlen(text); ii++) {
    if (isdigit(text[ii]) == 0)
      return 0;
  }
  return 1;
}


int calc_roman(char pch[]) {
  int res = 0;
  int ii;
  int length = strlen(pch);
  int previous = 1000;

  if (strcmp(pch,"XL") == 0) strcpy(pch,"XI");
  if (strcmp(pch,"VL") == 0) strcpy(pch,"VI");

  for (ii = 0; ii < length; ii++) {
    switch(pch[ii]) {
    case 'M':
    case 'm':
      res += 1000;
      if (previous < 1000) {
	res -= 2 * previous;
      }
      previous = 1000;
      break;
    case 'D':
    case 'd':
      res += 500;
      if (previous < 500) {
	res -= 2 * previous;
      }
      previous = 500;
      break;
    case 'C':
    case 'c':
      res += 100;
      if (previous < 100) {
	res -= 2 * previous;
      }
      previous = 100;
      break;
    case 'L':
    case 'l':      
      res += 50;
      if (previous < 50) {
	res -= 2 * previous;
      }
      previous = 50;
      break;
    case 'X':
    case 'x':
      res += 10;
      if (previous < 10) {
	res -= 2 * previous;
      }
      previous = 10;
      break;
    case 'V':
    case 'v':
      res += 5;
      if (previous < 5) {
	res -= 2 * previous;
      }
      previous = 5;
      break;
    case 'I':
    case 'i':
      res += 1;
      if (previous < 1) {
	res -= 2 * previous;
      }
      previous = 1;
      break;
    }      // switch
  } // for ii
  //if (debug) fprintf(stderr,"ROMAN:in=%s: out=%d: ll=%d:\n",pch,res,length);
  return(res);
}

void insert_into_group_plus(int my_item, int my_group, int my_enum, int my_P_enum, int convert) {
  //fprintf(stderr,"UUU1:ii=%d: gg=%d: me=%d:\n",my_item, my_group, my_enum);  
  if (my_group < 61) insert_into_group(my_item, my_group, my_enum, my_P_enum, convert);
  return;
}

// convert is 1 when I was changed to L; convert is 0 when I (letter) is changed to I (roman)
// my_P_enum is 6 in 6.13 and 0 otherwise
void insert_into_group(int my_item, int my_group, int my_enum, int my_P_enum, int convert) {
  if (group_no_array[my_group] < MAX_GROUP_ITEM) {
    if (my_enum >=0) {
      int gi_no = group_no_array[my_group];
      group_item_array[my_group][gi_no].item_no = my_item;
      group_item_array[my_group][gi_no].my_enum = my_enum;
      group_item_array[my_group][gi_no].my_P_enum = my_P_enum;
      group_item_array[my_group][gi_no].convert = convert;
      group_no_array[my_group]++;
      if (group_header_array[my_group] == NULL || gi_no == 0) {
	group_header_array[my_group] = &group_item_array[my_group][0];
      } else {
	group_item_array[my_group][gi_no].prev = &group_item_array[my_group][gi_no-1];
	group_item_array[my_group][gi_no-1].next = &group_item_array[my_group][gi_no];
      }      
      if (0) fprintf(stderr,"UUU: inserted into group: in=%d: g=%d: gn=%d: enum=%d:\n", my_item, my_group, gi_no, my_enum);
    }
  } else {
    fprintf(stderr,"ERROR: TOO MANY ITEMS (more than %d) in group:%d:\n", MAX_GROUP_ITEM,my_group);
  }
  return;
}


char *rem_eol(char *text) {
  int ii, jj;
  static char bext[5000];
  for (ii = 0, jj = 0; ii < strlen(text); ii++) {
    if (text[ii] != '\n') bext[jj++] = text[ii];
    else bext[jj++] = ' ';
  }
  bext[jj++] = '\0';
  title_pid = -1;
  return(bext);
} // rem_eol()

char * rem_entity(char *text) {
  int in_entity = 0;
  int ii, jj;
  static char bext[5000];
  char *pp;
  for (ii = 0, jj = 0; ii < strlen(text); ii++) {
    pp = strchr(text+ii,';');
    if (text[ii] == '&' && pp
	&& (strlen(text+ii) - strlen(pp)) < 6) { // see that it's not a lone '&' but &abcd;
      in_entity = 1;
    } else if (in_entity == 1) {
      if (text[ii] == ';') {
	in_entity = 0;
      } else {
	;
      }
    } else {
      bext[jj++] = text[ii];
    }
  }
  bext[jj++] = '\0';
  return bext;
}

int count_first_CAP(char *header) {
  int ii;
  int ret = 0;
  int len = strlen(header);
  for (ii = 0; ii < len; ii++) {
    //fprintf(stderr,"MMMMMK:ii=%d:%d: ALP=%d: UP=%d: :%s:\n", ii,len, isalpha(header[ii]),isupper(header[ii]), header);
    if (isalpha(header[ii]) != 0) {
      if (isupper(header[ii]) != 0) {
	ret = ii;
      } else {
	break;
      }
    } else {
      ; // skip spaces and non-alpha
    }
  }
  return ret;
}

/* calc the header, caps  */
int calc_header(char *in_header, int jj) {
  int kk;
  int debug_hdr = 0;

  char *header = in_header;
  for (kk = 0; kk < strlen(header); kk++) {
    item_array[jj].periods_in_header += (header[kk] == '.' || header[kk] == ':'  || header[kk] == '-') ? 1 : 0;
  }

  int b_chop = 0;
  int CAP_hdr_len = 0;
  b_chop = chop_CAP_header(header,jj); // chop CAP headers
  CAP_hdr_len = count_first_CAP(header);
  if (debug) {
    fprintf(stderr,"RR0:jj=%d: CAP_len=%d:%d: section=%s: b_chop=%d: b_chopC=%d:%c:\thdr:%s: hdr=%s:\n"
	    , jj
	    , item_array[jj].CAP_hdr_len, CAP_hdr_len
	    , item_array[jj].section
	    , b_chop,item_array[jj].CAP_hdr_len, header[b_chop]
	    , item_array[jj].clean_header
	    , header
	    );
  }


  item_array[jj].CAP_hdr_len = CAP_hdr_len; //b_chop;
  if (b_chop == 0 || b_chop > 80) {

    b_chop = chop_Cap_header(header); // chop first cap headers
    if (b_chop == 0 || b_chop > 80) {
      b_chop = 80; // if no chopping then limit the header to 30
    }
  }

  static char buff[4000];
  int buff_len = strlen(header);
  sprintf(buff,"%.*s",b_chop+1,header);

  item_array[jj].clean_header = strdup(rem_entity(rem_eol(buff)));

  if (0 && debug) fprintf(stderr,"BBBBBBBB :jj=%d: bl=%d: sec=%s:  b_chop=%d:  cap_hdr_len=%d \thdr:%s: chop_hd:%c:  c_hd=%s:\n"
	  , jj
	  , buff_len
	  , item_array[jj].section
	  , b_chop
	  , item_array[jj].CAP_hdr_len

	  , header
	  , header[b_chop-1]
	  , item_array[jj].clean_header
	  );

  if (debug) {
    fprintf(stderr,"HH:jj=%d: CAP_len=%d: section=%s: b_chop=%d: b_chopC=%d:%c:\thdr:%s:\n"
	    , jj
	    , item_array[jj].CAP_hdr_len
	    , item_array[jj].section
	    , b_chop,item_array[jj].CAP_hdr_len, header[b_chop-1]
	    , item_array[jj].clean_header
	    );
  }
  return 0;
} // calc_header()

/*****************************************/



int print_level_2(int NN) {
  //  int xx = (child_triple_array[2].ptr) ? child_triple_array[2].ptr->level : -1;
  int xx = (child_triple_array[1].ptr) ? child_triple_array[1].ptr->level : -1;
  fprintf(stderr,"YYYY0:%d: %d:\n",NN,xx);
  return 0;
}

int print_included_2(int NN,int inc) {
  //int xx = seq_array[2].included;
  fprintf(stderr,"YYYYN:%d: %d:%d:\n",NN,0,0);
  return 0;
}


int init_seqs() {
  g_seq_no = 0; // how many sequences do we have in the document?
  int ii,jj;
  for (ii = 0; ii < MAX_SEQ; ii++) {
    for (jj = 0; jj < MAX_SEQ_NO; jj++) {
      seq_item_array[ii][jj].badness = 0;
      seq_item_array[ii][jj].my_enum = 0;
      seq_item_array[ii][jj].group_item_no = 0;
      seq_item_array[ii][jj].item_no = 0;
      seq_item_array[ii][jj].removed = 0;
    }
    seq_array[ii].no_of_items = 0;
    seq_array[ii].real_no_of_items = 0;
    seq_array[ii].range = 0;
    seq_array[ii].group_no = 0;
    seq_array[ii].total_all_cap = 0;
    seq_array[ii].total_sep_period = 0;
    for (jj = 0; jj < NO_INC; jj++) {
      seq_array[ii].incl_total[jj] = 0;
    }
    seq_array[ii].max_gap = 0;
    seq_array[ii].total_gap = 0;
    seq_array[ii].max_gap_item = 0;
    seq_array[ii].max_diff = 0;
    seq_array[ii].total_diff = 0;
    seq_array[ii].max_diff_item = 0;

    seq_array[ii].awkward = 0;
    seq_array[ii].include = 0;
    seq_array[ii].included = 0;
    for (jj = 0; jj < MAX_INCLUDED; jj++) {
      seq_array[ii].include_array[jj] = 0;
    }
    seq_array[ii].overlap = 0;
    for (jj = 0; jj < MAX_OVER; jj++) {
      seq_array[ii].over_array[jj] = 0;
    }
    for (jj = 0; jj < MAX_AWK; jj++) {
      seq_array[ii].awkward_array[jj] = 0;
    }
    seq_array[ii].rank = 0;
    seq_array[ii].level = 0;
    seq_array[ii].fn = 0;
    seq_array[ii].ln = 0;
    seq_array[ii].prev_item = 0;
    seq_array[ii].total_converts = 0;
    seq_array[ii].level_reason = -1;
  } // for ii
  return 0;
}


/****** badness *************************
sequential on both sides: 0
start of seq (not 1) +2
start of seq (yes 1) +1
end of seq +1
not the same ALL CAP +1
gsp +1 for each gap letter
************************************/

int ttot(int kk, int nn) {
  fprintf(stderr,"TTOT:%d:  kk=%d: noi=%d:%d:\n", kk, nn, seq_array[183].no_of_items, seq_array[183].real_no_of_items);
  return 0;
}

void create_seq() {
  int kk;
  fprintf(stderr,"CREATE_SEQ:\n");
  g_seq_no = -1; // how many seq's total
  for (kk = 0; kk < MAX_GROUP && kk < 71; kk++) { // LOOP over GROUPs

    if (kk == FILE_GROUP) { // EXHIBIT, force it in
      align_forced_group(kk);
    } else if (kk == FUNDAMENTALS_GROUP) { // EXHIBIT, force it in
      align_forced_group(kk);
    } else { // don't use alignment for tiny seqs OR for 6.13, they requires some extra treatment (check also the prev span)
      make_max_align(kk);
    } //else if kk != 11

  } // for kk (for each group build all the seqs

  new_seq("in create_seq",kk);
} // create_seq



/*************************************************************/

int determine_group_for_header(char *section, char *title) {
  int a,b,c,d,e,f;
  int ret = 0;
  int nn = sscanf(section,"%d.%d.%d.%d.%d.%d",&a,&b,&c,&d,&e,&f);
  
  if (nn >= 1 && nn <=5) ret = nn;
  else if (section[0] == 'x' || section[0] == 'v' || section[0] == 'i') { ret = 9; fprintf(stderr,"TTT0\n"); }
  else if (section[0] == 'X' || section[0] == 'V' || section[0] == 'I') {ret = 10; fprintf(stderr,"TTT1\n"); }
  else if (isalpha(section[0]) != 0 && isupper(section[0]) != 0) { ret = 8; fprintf(stderr,"TTT2\n"); }
  else if (isalpha(section[0]) != 0 && islower(section[0]) != 0) { ret = 7; fprintf(stderr,"TTT3\n"); }
  
  if (ret > 0) {
    if (strcasecmp(title,"section") == 0) ret += 0;
    else if (strcasecmp(title,"") == 0) ret += 10;
    else if (strcasecmp(title,"paragraph") == 0 || strcasecmp(title,"lr") == 0) ret += 40;
    else if (strcasecmp(title,"article") == 0) ret += 50;
  }

  if (ret == 0 && section[0] == '(') {
    char letter2;
    int num11;
    char text11[20];
    int nn = sscanf(section,"(%d%c", &num11, &letter2);
    int tt = sscanf(section,"(%[a-z]%c", text11, &letter2);
    int TT = sscanf(section,"(%[A-Z]%c", text11, &letter2);        
    fprintf(stderr,"FFF6:%d:%c:%s:\n",num11,letter2,text11);
    if (nn == 2 && letter2 == ')') {
      ret = 21; // (3)
    } else if (tt == 2) { // (abc)
      int ll;
      int tmp_ret = 29;
      for (ll = 1; ll < strlen(section) -1; ll++) {
	if (section[ll] != 'i' && section[ll] != 'v' && section[ll] != 'x' && section[ll] != 'l') tmp_ret = 0;
      }
      if (tmp_ret > 0) ret = tmp_ret;//  (vii)
      else ret = 27; // (a)
    } else if (TT == 2) {
      int ll;
      int tmp_ret = 30;
      for (ll = 1; ll < strlen(section) -1; ll++) {
	if (section[ll] != 'I' && section[ll] != 'V' && section[ll] != 'X' && section[ll] != 'L') tmp_ret = 0;
      }
      if (tmp_ret > 0) ret = tmp_ret;//  (vii)
      else ret = 28; // (a)
    }
  }
  // fprintf(stderr,"FFF:%d::%s:%s:\n",ret,section,title);
  return ret;
}


/* group_types 
   0 - file
   exhibit
   attachment
   schedule
   appendix
   article

   1 - section 2.
   2 - section 2.1
   3 - section 2.1.3
   4 - section 2.1.3.4
   5 - section 1.1.1.1.1
   6 - section twenty two
   7 - section a., b.,  
   8 - section A., B.,
   9 - section x, v, i
   10 - section X, V, I

   11 - 1. ABC ALL CAPS
   61 - 1. Abc Not All Caps
   12 - 1.1
   13 - 1.1.1
   14 - 1.1.1.1
   15 - 1.1.1.1.1
   16 - twenty two
   17 - a., b.,  a)
   18 - A., B.,
   19 - x, v, i
   20 - X, V, I 
  
   19 - A.1

   21 - (3) 
   27 - (a) 
   28 - (A) 
   29 - (iv)
   30 - (IV)
   
   31 - Part 2.
   32 - Part 2.1
   33 - Part 2.1.3
   34 - Part 2.1.3.4
   35 - Part 1.1.1.1.1
   37 - Part a., b.,  
   38 - Part A., B.,
   39 - Part x, v, i
   40 - Part X, V, I

   41 - Paragraph 2. or LR2 or RF3
   42 - Paragraph 2.1
   43 - Paragraph 2.1.3
   44 - Paragraph 2.1.3.4
   45 - Paragraph 1.1.1.1.1
   47 - Paragraph a., b.,  
   48 - Paragraph A., B.,
   49 - Paragraph x, v, i
   50 - Paragraph X, V, I

   51 - Article 2.
   52 - Article 2.1
   53 - Article 2.1.3
   54 - Article 2.1.3.4
   55 - Article 1.1.1.1.1
   57 - Article a., b.,  
   58 - Article A., B.,
   59 - Article x, v, i
   60 - Article X, V, I

   61 - 1. Abc Not All Caps

   70 - exhibit
*/

int calc_my_lev(int jj) {
  int selected_seq = item_array[jj].selected_seq;
  return (child_triple_array[selected_seq].ptr) ? child_triple_array[selected_seq].ptr->level : seq_array[selected_seq].level;
}

char *limit_no_of_letters(char *title, int num) {
  int ii;
  static char bext[5000];
  for (ii = 0; ii < num && ii < strlen(title); ii++) {
    bext[ii] = title[ii];
  }
  bext[ii++] = '\0';
  return bext;
}

int print_child_triple_array() {
  int ii;
  return 0;
  fprintf(stderr,"PRINTING CHILD\n");
  for (ii = 0; ii < 200; ii++) {
    fprintf(stderr,"44444444 %d:%s:\n",ii, (child_triple_array[ii].ptr) ? "NO_NULL" : "YES_NULL");
  }
  return 0;
}


int print_item_array_idx(FILE *index_file) {
  #define BAD1_LEVEL 3
    int jj;

    static char buff2[20000], buff3[20000];
    //if (debug) fprintf(stderr,"%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n\n","SEQ","ENUM","LINE","GRP","PID","LEV","ART","SEC","HDR");

    for (jj = 0; jj < max_item_no; jj++) {
      int selected_seq = item_array[jj].selected_seq;
      int new_level = (child_triple_array[selected_seq].ptr) ? child_triple_array[selected_seq].ptr->level : seq_array[selected_seq].level;
      if (jj == 0 || jj == max_item_no -1) { // UZ hack to make sure the TOP and BOTTOM are 2 and not 10
	new_level = 2;
      }  else if (item_array[jj].grp_no == FUNDAMENTALS_GROUP) { // grp 71 is FUNDAMENTAL hdr
	new_level = 3;
	seq_array[selected_seq].level = 3;
      } 
      if (0 && debug) fprintf(stderr," MY_ITEM=%d:  LLL0 NEW LEVEL:%d: null?:%s: in=%d: grp=%d: cp_lev=%d: sel_seq=%d: sel_lev=%d: jj=%d: max_item_no=%d:\n"
			 , jj
			 , new_level
			 , (child_triple_array[selected_seq].ptr==NULL) ? "NULL" : "NOT NULL"
			 , jj , item_array[jj].grp_no
			 , (child_triple_array[selected_seq].ptr) ? child_triple_array[selected_seq].ptr->level : -100
			 , selected_seq
			 , seq_array[selected_seq].level
			 , jj
			 , max_item_no);
      //insert_bad_level(jj,selected_seq,new_level);
      if (new_level > 12) {
	if (debug) fprintf(stderr,"Error: new_lev=%d:\n",new_level);
	new_level = BAD_LEVEL;
	if (debug) fprintf(stderr,"GOT BAD LEVEL30:%d:\n",selected_seq);      
	if (child_triple_array[selected_seq].ptr) child_triple_array[selected_seq].ptr->level = 70; //BAD_LEVEL;
      } else if (new_level == 0) {
	if (debug) fprintf(stderr,"Error: new_new_lev=%d:%d:\n",selected_seq,new_level);
	new_level = BAD_LEVEL;
	if (debug) fprintf(stderr,"GOT BAD LEVEL31:%d:\n",selected_seq);      
	if (child_triple_array[selected_seq].ptr) child_triple_array[selected_seq].ptr->level = 60; //BAD_LEVEL;
      } else {
	;
      }

      
      int selseq = item_array[jj].selected_seq;

      if (seq_array[selseq].real_no_of_items < 2
	  && (item_array[jj].grp_no != 71 // don't throw out FUNDAMENTAL HEADER
	      && seq_item_array[selseq][0].group_no != 12
	      && seq_item_array[selseq][0].group_no != 2)) { // DON'T TAKE SEQ SHORTER THAN 2 !!! // take in seqs with more than 1 item or 2.1???
	if (debug) fprintf(stderr,"Cancelled BAD4 seq:%d:%d:title=%s: header=%s:\n",item_array[jj].selected_seq,seq_item_array[jj][0].group_no, item_array[seq_item_array[jj][0].item_no].title,  item_array[seq_item_array[jj][0].item_no].clean_header);
	new_level = BAD_LEVEL;
	if (child_triple_array[selected_seq].ptr) child_triple_array[selected_seq].ptr->level = BAD_LEVEL;
	if (debug) fprintf(stderr,"GOT BAD LEVEL4:%d:\n",selseq);
      }



      if (strlen(item_array[jj].section) > 20) { // make sure "Section 5" is not exceedingly long
	item_array[jj].section[20] = '\0';
      }
      int loc = item_array[jj].my_loc;
      int indent = (loc > 0 && loc < MAX_COORDS) ? coords_array[loc].x1000 : -1;
      int center = item_array[jj].center;
      char *title = ((strlen(item_array[jj].title)== 0)?"-":(rem_endline(item_array[jj].title)));

      if (strcasecmp(title,"exhibit") != 0 || loc_to_line(item_array[jj].my_loc) < 5) { // DONT ALLOW EXHIBIT HEADERS DEEPER THAN 5 lines ib the PAGE
	int test = 1;
	static char bbb[50];
	if (test) {
	  sprintf(bbb," %d %d ", loc_to_line(item_array[jj].my_loc), item_array[jj].my_loc);
	} else {
	  sprintf(bbb,"");
	}
	char *cln_hdr = item_array[jj].clean_header;
	int pid = item_array[jj].pid;
	int page_no = paragraphtoken_array[pid].page_no;
	int line_no = paragraphtoken_array[pid].line_no;	
	//sprintf(buff2,"%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\t%d\t%d"
	sprintf(buff2,"%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d"				  
		, selected_seq
		, item_array[jj].my_enum
		, item_array[jj].line_no
		, item_array[jj].grp_no
		, item_array[jj].pid
		, new_level
		, ((strlen(title) == 0) ? "_" : title)
		, item_array[jj].section 
		, (contains_text(cln_hdr) == 1) ? cln_hdr : "_" 
		, loc_to_indent(item_array[jj].my_loc)
		, item_array[jj].too_long
		, center
		, item_array[jj].my_loc
		, line_no // item_array[jj].line_no
		, page_no // item_array[jj].page_no

		, item_array[jj].no_of_words
		, item_array[jj].no_of_all_CAPs_words
		, item_array[jj].no_of_first_Caps_words
		, item_array[jj].is_special // 0 is regular, 1 is Lease, 2 is summary_points
		//, title
		//, bbb

		, item_array[jj].toc_page_no
		, item_array[jj].toc_page_no_type
		, item_array[jj].toc_page_no_coord

		); 
	if (debug) fprintf(stderr,"OUTPUT ITEM: jj=%d PID=%d SEQ=%d: LEV=%d: :%s:%s: seq=%d: spec=%d:",jj, item_array[jj].pid, selected_seq, new_level, item_array[jj].section, limit_no_of_letters(item_array[jj].header,50), item_array[jj].is_special);	
	if (debug) fprintf(stderr,"    ln=%d: title=%s:\n", loc_to_line(item_array[jj].my_loc), title);

	fprintf(index_file,"%s\n",buff2);

      } // buff2


    } // for jj
    fprintf(index_file,"%s\n",buff3);    
    return(0);
} // print_item_array_idx()

// stop on last alpha
int strlen_alpha(char *text) {
  int ii;
  int len = 0;
  for (ii = strlen(text); ii >= 0; ii--) {
    if (isalpha(text[ii]) != 0) {
      len = ii;
      break;
    }
  }
  return len;
}

/* chop all CAPS header */
int chop_CAP_header(char *text, int item_no) {
  int ii;
  int found_break = 0;
  int in_caps = 1;
  int first_cap_count = 0; // how many caps at the beginning of the line?
  int after_first_small_count = 0; // how many letters after the first drop
  int later_cap_count = 0; // how many caps after the first drop
  int my_len = 0;
  //int break_point = my_len = strlen(text); // where is the first drop?
  int break_point = my_len = strlen_alpha(text); // where is the first drop?  
  for (ii = 0; ii < my_len; ii++) {
    if (in_caps) {
      if (isalpha(text[ii]) != 0) {
	if (isupper(text[ii]) != 0) {
	  first_cap_count++;
	} else { // found a lower case?
	  in_caps = 0;
	  later_cap_count = 0;
	  after_first_small_count = 1;
	  break_point = ii;
	} // upper
      } // alpha
    } else { // after caps
      if (isalpha(text[ii]) != 0) {
	after_first_small_count++;
	if (isupper(text[ii]) != 0) {
	  later_cap_count++;
	} 
      }
    }
  }

  if ((first_cap_count > 2) && (((later_cap_count) *3) <= after_first_small_count)) { // what's this stupid condition???
    if (text[break_point-2] == ' ') {
      break_point--;
    }
    found_break = break_point;
  } else { // UZ added 02/01/19 b/c otherwise it returns 0...
    //found_break = break_point;
  }
  if (0 && debug) {
    fprintf(stderr,"CAP_COUNT:text=%s: cap_count=%d: bp=%d: fb=%d: later_cap=%d: after_first_small=%d:__:%s:<BR>",text,first_cap_count,break_point,found_break, later_cap_count,after_first_small_count,item_array[item_no].header);
  }

  return found_break;
} // chop_CAP_header(char *text, int item_no) 

struct Word {
  char *word;
  int Capped;
};


#define MAX_WORDS_IN_HEADER 20
struct Word word_array[MAX_WORDS_IN_HEADER];

/* chop first cap header.  "Accrued Expenses: Those should be delayed" */
int chop_Cap_header(char *text) {
  //fprintf(stderr,"YYY:%s:\n",text);
  if (!(text && strlen(text))) return 0;
  static char my_text[50000];
  strcpy(my_text,text);

  char *sep = "., :()[];?!\"-/\n";
  char *pch = strtok(my_text,sep);
  //fprintf(stderr,"PCH0=%s:\n",pch);  
  int ii = 0;
  int first_not_capped = -1;
  while (pch != NULL && ii < MAX_WORDS_IN_HEADER-1) {
    //fprintf(stderr,"PCH:%s:\n",pch);
    if (!is_number(pch)) {
      if (strcmp(pch,"is") != 0 && strcmp(pch,"and") != 0 && strcmp(pch,"or") != 0 && strcmp(pch,"if") != 0 
	  && strcmp(pch,"for") != 0 && strcmp(pch,"of") != 0 && strcmp(pch,"in") != 0 && strcmp(pch,"no") !=0
	  && strcmp(pch,"by") != 0 && strcmp(pch,"be") != 0 && strcmp(pch,"without") != 0 
	  && strcmp(pch,"a") != 0 && strcmp(pch,"the") != 0 && strcmp(pch,"with") != 0 && strcmp(pch,"not") !=0) {
	word_array[ii].word = strdup(pch);
	word_array[ii].Capped = (isupper(pch[0]) == 0) ? 0 : 1;
	if (first_not_capped == -1 && word_array[ii].Capped == 0) first_not_capped = ii;
	ii++;
      }
    }
    pch = strtok(NULL,sep);
    //fprintf(stderr,"PCH=%s:\n",pch);
  } // while
  int jj;
  gprev = NULL;

  int found_period = find_period(text); // this one added UZ since headers: '2.4 Lease Year. The "First Lease Year shall be bla'. We skip the period since 3 words behind are Capped
  //fprintf(stderr,"PCB:%s:  :%d: ",text, found_period);
  //fprintf(stderr,"FP:%d:\n",found_period);
  //  found_period = find_period_before_word(text,first_not_capped,gprev);

  if (!found_period) found_period = find_period_before_word(text,first_not_capped-1,gprev);
  //fprintf(stderr,"PCB:%d: ",found_period);
  if (!found_period) found_period = find_period_before_word(text,first_not_capped-2,gprev);
  //fprintf(stderr,"PCB:%d: \n",found_period);
  //fprintf(stderr,"TEXT=%d:%d:%d:%d:\n", found_period, strlen(text), ii, first_not_capped);
  for (jj = 0; jj < ii; jj++) {
    //fprintf(stderr,"WORD:%d:%d:%d:%s:\n",jj,word_array[jj].Capped,strlen(word_array[jj].word),word_array[jj].word);
  }
  return(found_period);
} // chop_Cap_header(char *text) 


int find_period(char *header) {
  int jj = -1;
  int ret = 0;

  for (ret = 0, jj = 0; ret == 0 && jj < strlen(header); jj++) {
    if (header[jj] == '.' || header[jj] == ':' || header[jj] == '-') {
      if (header[jj+1] == ' ' || header[jj+1] == '\n') {
	ret = jj;
      }
    }
  }
  if (ret < 3) ret = strlen(header); // we don't want to catch "- Summary of Lease" as in "Section 3.01 - Summary of Lease", 
  return ret;
}
  
// find the ii of the period before the first non-cap or its prior one so you can chop off at the period.
// header is the entire header to be chopped
// first_not_capped is the SN of the forst not capped word
// rest is the pointer to the word not capped (or its prior)
// prev is the previous found word. not to be exceeded
int find_period_before_word(char *header, int first_not_capped, char *prev) {
  char *word_not_capped = (first_not_capped > 0) ? word_array[first_not_capped].word : "_X_";

  /* find the location of the "word_not_capped" or the word prio to it. The header ends TWO words before the word_not_capped:  "Expenses. Those incrued prior..." */
  char *rest =  strstr(header,word_not_capped);
  char *rest1 = "_X_";
  if (rest) { // Sometimes the word "expenses" appears twice. as in: "Expenses. Expenses incrued prior..." . we need to find the SECOND one, that's what we do with rest1
    rest1 = strstr(rest+1,word_not_capped);
  }
  if (rest1 && rest1 < gprev) { // 
    rest = rest1; // use the next occurrence
  }

  int jj = -1;
  int ret = 0;
  //fprintf(stderr,"\nEEE:%d:%s:%s:",rest-header,rest,header);
  if (rest && strcmp(word_not_capped,"_X_") != 0) {
    while (isalnum(rest[jj]) == 0) {
      if (rest[jj] == '.' || rest[jj] == ':' || header[jj] == '-') {
	ret = jj +(rest-header)+1;
      }
      //fprintf(stderr,":%d:%c:__",jj,rest[jj]);
      jj--;
    }
  }
  if (rest) {

    gprev = strdup(rest);
  }
  //fprintf(stderr,":BB\n");
  return ret;
}


int print_groups(int nn, char *text) {
  int ii;
  fprintf(stderr,"\n\nMYGROUP:%d:---%s:\n",nn, text);
  for (ii = 0; ii < MAX_GROUP; ii++) {
    int jj;
    if (group_no_array[ii] > 0) {
      fprintf(stderr,"\n  GROUP:%d:%d:\n",ii,group_no_array[ii]);
      for (jj = 0; jj < group_no_array[ii]; jj++) {
	fprintf(stderr,"         ITEM:%d:%d: pid=%d:  is=%d: enum=%d:%d: :%s:%s:\n"
		,jj
		,group_item_array[ii][jj].item_no
		,item_array[group_item_array[ii][jj].item_no].pid
		,item_array[group_item_array[ii][jj].item_no].is_special
		,group_item_array[ii][jj].my_enum
		,group_item_array[ii][jj].my_P_enum		
		,item_array[group_item_array[ii][jj].item_no].section
		,item_array[group_item_array[ii][jj].item_no].header); 		
      }
    } // ii
  }
  return 0;
}


int read_coords_file(char *coords_name) {
  if (strcmp(toc_name,"NNN") != 0) {
    NO_IDX = 0;
   if (debug) fprintf(stderr,"Info: %s going to read open coords_file file=%s\n",prog,coords_name);
    coords_file = fopen(coords_name,"r");
    if (!coords_file) {
      fprintf(stderr,"Error: can't (r) open file %s\n",coords_name);
      exit(0);
    }

    int ii = 0;
    char cc;
    char xx[10];
    char yy[10];
    int page_no;
  
    static char line[180];
    while (fgets(line,180,coords_file)) {
      int nn = sscanf(line,"%d\t%c\t%d\t%s %s %*s %*s",
		      &ii, &cc, &page_no, xx, yy);
      if (nn == 5) {
	coords_array[ii].cc = cc;
	coords_array[ii].x1000 = (int)(atof(xx) * 1000.0);
	coords_array[ii].y1000 = (int)(atof(yy) * 1000.0);
	coords_array[ii].page_no = page_no;      
      }
      coords_no = (ii > coords_no) ? ii : coords_no;
    }
    fclose(coords_file);
    if (debug) fprintf(stderr,"Info: %s (r) read coords file %s, %d chars\n",prog,coords_name, coords_no);
  }

  return coords_no;
}

int collect_toc_suggestions(int doc_id, int page_no) {
  int ii;
  int last_kk = 0;
  int dod = 0;
  for (ii = 0; ii < page_no; ii++) {
    int jj, mm;
    for (mm = 0, jj = 0; jj < page_line_array[ii].no_of_lines; jj++) {    
      if ((page_line_properties_array[ii][jj].center > 285000
	   && page_line_properties_array[ii][jj].center < 315000
	   && page_line_properties_array[ii][jj].left_X > 100000)
	  && page_line_properties_array[ii][jj].no_of_words < 12
	  && page_line_properties_array[ii][jj].no_of_words * 2 < page_line_properties_array[ii][jj].no_of_all_cap_words * 3) {
	//fprintf(stderr,"CAND: mm=%d: page=%2d: line=2%d: lkk=%d: cen=%d: lx=:%d: :%d: :%d: --"
	if (dod) fprintf(stderr,"CAND: mm=%d: page=%2d:%2d line=%3d: --"		
		, mm++
		, ii, page_line_array[ii].no_of_lines, jj
		); /*
		, last_kk
		, page_line_properties_array[ii][jj].center
		, page_line_properties_array[ii][jj].left_X
		, page_line_properties_array[ii][jj].no_of_words
		, page_line_properties_array[ii][jj].no_of_all_cap_words); */
	int kk;
	for (kk = last_kk; token_array[kk].page_no < ii; kk++) {
	  //fprintf(stderr,"SS:kk=%d: ii=%d:%d: jj=%d:%d: t=%s:\n",kk,ii,token_array[kk].page_no, jj, token_array[kk].line_no, token_array[kk].text);
	}
	while (token_array[kk].page_no < ii
	       || (token_array[kk].page_no == ii && token_array[kk].line_no <= jj)) {
	  if (token_array[kk].line_no == jj) {
	    if (dod) fprintf(stderr,"%s_", token_array[kk].text);
	  }
	  kk++;
	} // while

	last_kk = kk;
	if (dod) fprintf(stderr,"_+\n");	
      }
    }
  }
  return 0;
} // collect_toc_suggestions(int doc_id, int page_no) 

int fix_seq_61() {
  int ii;
  if (debug) fprintf(stderr, "FIXING SEQ_61:%d:\n", g_seq_no);
  for (ii = 0; ii < g_seq_no; ii++) {
    //fprintf(stderr, "    FIXING SEQ_61:%d: noi=%d: gr=%d:\n", ii,seq_array[ii].no_of_items, seq_array[ii].group_no);      
    if (seq_array[ii].no_of_items > 0 && seq_array[ii].group_no == 61) {
      //fprintf(stderr, "       BFIXING SEQ_61:%d:\n", ii);      
      int jj;
      int found = 0;
      for (jj = 0; jj <= seq_array[ii].no_of_items; jj++) {
	int item_no = seq_item_array[ii][jj].item_no;
	char *pp = strchr(item_array[item_no].section,'.');
	if (pp == NULL) pp = strchr(item_array[item_no].section,')');
	if (pp) {
	  found = 1;	  
	  break;
	}
      } // jj for items in seq
      if (found == 0) {
	seq_array[ii].bad_found_61 = 1;
	fprintf(stderr, "  FOUND BAD seq_61:%d:%d:\n", ii,seq_array[ii].no_of_items);
      }
    }
  } // for ii
  return 0;
} // fix_seq_61() 


int get_params(int argc, char **argv) {
  int get_opt_index;
  int c_getopt;
     
  opterr = 0;
  debug = 0;
  debug_entity = 1;
  while ((c_getopt = getopt (argc, argv, "d:f:t:u:i:n:r:P:N:U:W:D:C:A:X:Y:Z:F:m:o:G:")) != -1)
    switch (c_getopt) {
    case 'd':
      doc_id = atoi(optarg);
      break;
    case 'D':
      debug =  dseq = atoi(optarg);
      debug_entity =  atoi(optarg);
      break;
    case 'f':
      file_name_root = optarg;
      break;
    case 't':
      toc_name = optarg;
      break;
    case 'u':
      url_name = optarg;
      break;
    case 'i':
      indexed_name = optarg;
      break;
    case 'C':
      coords_name = optarg;
      break;
      fprintf(stderr,"COORDS_NAME=%s:\n");
    case 'n':
      deal_root_name = optarg;
      break;
    case 'r':
      restart_toc = atoi(optarg);
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
    case 'A':
      app_path = strdup(optarg); // ~/dealthing/webapp
      break;
    case 'X':
      do_pass = atoi(optarg); // (if pass_no == 1) first collect all the items and then insert them from the collected array
      break;
    case 'Y':
      do_toc_toc_align = atoi(optarg); // (if do_align == 1) align with TOC_TOC
      if (do_toc_toc_align == 0) do_pass = 0; // otherwise LEX would run on neutral
      break;
    case 'Z':      
      do_take_headers_from_toc_toc = atoi(optarg); // take the headers from the toc_toc since probably more accurate
      break;
    case 'F':      
      do_fundamentals = atoi(optarg); // create TOC entries for rental_form (aka fundamental facts)
      break;
    case 'm':      
      do_max_align = atoi(optarg); // use max
      break;
    case 'o':      
      out_root = strdup(optarg); 
      break;
    case 'G':      
      doc_country = toupper_word(strdup(optarg)); 
      break;

    case '?':
      if (optopt == 'd')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'D')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'f')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 't')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'u')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'i')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'n')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'r')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'P')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'N')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'U')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'W')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'A')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'X')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'Y')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'Z')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'F')
	fprintf (stderr, "Option -%c requires an argument.\n", optopt);
      if (optopt == 'm')
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

  if (debug) fprintf (stderr,"doc_id = %d, file_name_root = %s, toc_name = %s, url_name = %s, indexed_name = %s, deal_root_name =%s, restart_toc=%d, db_IP =%s, db_name =%s, db_user_name =%s, db_pwd=%s: app_path:%s:\n",
		      doc_id, file_name_root, toc_name, url_name, indexed_name, deal_root_name, restart_toc,db_IP, db_name, db_user_name, db_pwd, app_path);

  for (get_opt_index = optind; get_opt_index < argc; get_opt_index++) {
    fprintf (stderr,"Non-option argument %s\n", argv[get_opt_index]);
  }
  return 0;
} // get_params()

int print_reverse_token_array() {
  int loc;
  for (loc = 0; loc < 1000000; loc++) {
    if (reverse_token_array[loc] > 0) {
      fprintf(stderr,"REV  loc=%d: tok=%d: t=%s: pl=%d:%d:\n", loc, reverse_token_array[loc], token_array[reverse_token_array[loc]].text, token_array[reverse_token_array[loc]].page_no, token_array[reverse_token_array[loc]].line_no);
    }
  }
  return 0;
}    



int main(int argc, char **argv) {
  /******************/
  prog = argv[0];
  do_take_headers_from_toc_toc = 1;
  get_params(argc, argv);

   /************** SQL ********************/
  //char *server = "localhost";
  char *server = db_IP; // "54.241.17.226"
  char *user = db_user_name; // "root";
  char *password = db_pwd; //"imaof3";
  char *database = db_name; // "dealthing";
  /************** END SQL ****************/

  
  conn = mysql_init(NULL);
  

  /* now connect to database */
  if (!mysql_real_connect(conn,server,user,password,database,0,NULL,0)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }

  token_no = read_tokens();
  fprintf(stderr,"JJJJJJJJJJJ0\n");
  int my_para_no = read_paragraphtoken();
  fprintf(stderr,"JJJJJJJJJJJ1:%d:\n", my_para_no);
  print_paragraphtoken_array(my_para_no);
  fprintf(stderr,"JJJJJJJJJJJ2\n");
  read_summarypoints();
  read_horizontalTabToken();  
  //calc_centers(token_no);
  
  /******************/

  line_no = 0;
  item_no = 0;

  fprintf(stderr,"STEP1: reading coords, toc_name=%s:\n",toc_name);
  if (strcmp(toc_name,"NNN") == 0) {
    if (debug) fprintf(stderr,"Info: no IDX for input:%s:\n",toc_name);
    NO_IDX = 1;
  } else {
    NO_IDX = 0;
    if (debug) fprintf(stderr,"Info: %s going to (w) open file %s\n",argv[0],indexed_name);
    index_file = fopen(indexed_name,"w");
    if (!index_file) {
      fprintf(stderr,"Error: can't (w) open file %s\n",indexed_name);
      exit(0);
    }
    if (debug) fprintf(stderr,"Info: %s (w) opened file %s\n",argv[0],indexed_name);
  }


  coords_no = 0;//read_coords_file(coords_name);

  if (strcmp(toc_name,"NNN") == 0) {
    if (debug) fprintf(stderr,"Info: no TOC for input:%s:\n",toc_name);
    NO_TOC = 1;
  } else {
    NO_TOC = 0;
    if (debug) fprintf(stderr,"Info: %s going to (w) open file %s\n",argv[0],toc_name);
    if (restart_toc == 1) {
      toc_file = fopen(toc_name,"w"); 
    } else {
      toc_file = fopen(toc_name,"a"); 
    }
    if (!toc_file) {
      fprintf(stderr,"Error: can't (a) open file %s\n",toc_name);
      exit(0);
    }
    if (debug) fprintf(stderr,"Info: %s opened (w) file %s\n",argv[0],toc_name);
  }

  {
    g_group_no = FILE_GROUP;
    int is_special = LEASE_SPECIAL;
    int pass_no = 0;
    insert_item(7, item_no++, line_no, my_loc, g_group_no, "", "Lease", file_name_root,     is_special, title_pid, pid, pass_no);
  }

  t_title = "_";
  in_td = 0;
  if (debug) fprintf(stderr,"Find bad exhibit\n");
  find_bad_exhibit_pages();

  if (debug) fprintf(stderr,"page_line_prop\n");  
  page_no = get_page_line_properties();
  if (1) print_reverse_token_array();

  if (debug && 0) fprintf(stderr,"print page_line_prop\n");    
  if (debug && 0) print_page_line_properties(page_no);
  fprintf(stderr,"STEP2: entering items into groups\n");
  detect_map_pages();
  yylex();
  {
    // insert the last item as FILE
    g_group_no = FILE_GROUP;
    int is_special = 0;
    int pass_no = 0;
    insert_item(8, item_no++, line_no, my_loc, g_group_no, "", "ELease", "end of file",    is_special, title_pid, pid, pass_no);
  }

  fprintf(stderr,"STEP3: creating seqs, so far found %d:%d: groups, items\n",g_group_no, item_no);      
  if (debug) print_groups(17,"NOW");
  create_seq(); // input is group_item_array; output is seq_array
  fix_seq_61(); // check if seq_61 has even one period
  if (debug) print_seq_array(0);
  fprintf(stderr,"\n\nSTEP4: checking seqs:%d:\n",g_seq_no);
  check_seq();
  //print_item_array_nothing();

  fprintf(stderr,"STEP5: organize_levels.  seqs:%d:\n",g_seq_no);  
  organize_levels();
  if (debug) print_seq_array(1);

  if (1 || debug) print_groups(3, "after organize levels??");
  print_item_array_nothing(3);
  if (debug) fprintf(stderr,"\nPRINTING INDEX_FILE:\n");  
  print_child_triple_array();
  print_item_array_idx(index_file);

  if (debug) fprintf(stderr,"PRINTING SEQ ARRAY1:%d:\n",g_seq_no);
  if (debug) print_seq_array(2);
  fclose(index_file);
  if (debug) fprintf(stderr,"PRINTED TOC:\n");  
  /* now print the output file */

  print_out_buff();
  if (debug) fprintf(stderr,"PRINTED OUTPUT:\n");  
  if (debug) fprintf(stderr,"YONI: SQL started: let's see if your DB is in sync...\n");
  collect_toc_summary(g_seq_no);
  if (debug) fprintf(stderr,"YONI: SQL finished OK\n");
  collect_toc_suggestions(doc_id, page_no);
  if (debug) fprintf(stderr,"DONE:\n");  

  return(0);
} // main()

int print_out_buff() { // the out buff has markers: "<A2=5:Section> <A1=5:1.2> <A3=5:Introduction. xxx ddfd>"
  { // we turn it into: "<H5>Section<B2> 1.2<B1> Introduction. </H5> xxx ddfd"
    static char tttB_name[300];
    sprintf(tttB_name,"%s/../dtcstuff/tmp/Xxx/%s.%d",app_path,out_root, doc_id);
    FILE *tttB = fopen(tttB_name,"w");
    if (!tttB) {
      fprintf(stderr,"Error: can't find file:%s:\n",tttB_name);
      exit(-1);
    } else {
      fprintf(stderr,"Info: write opened for outout:%s:\n",tttB_name);
    }
    int kk_array[MAX_TOC];
    out_buff[buff_len++] = '\0';
    
    int ii;
    int kk = 1;
    char num = ' ';
    int lev = 0;
    int start3 = -1;

    for (ii = 0; ii < buff_len-1; ii++) {

      if (out_buff[ii] == '<' && out_buff[ii+1] == 'A' && (out_buff[ii+2] == '1' || out_buff[ii+2] == '2')) { // print the MARK start

	num = out_buff[ii+2]; // 1 -- "1.5.7"/ 2 -- "Section" / 3 --"  Introduction." / 4 -- "Exhibit" 
	lev = calc_my_lev(kk);
	if (lev > 0 && kk_array[kk] == 0) {
	  //fprintf(tttB,"<X%d class=\"toc_header\">",lev); // X for Chaim
	  fprintf(tttB,"<MARK class=\"toc_header\">"); // X for Chaim
	  kk_array[kk] = 1; 
	}

	while (out_buff[ii-1] != ':') {
	  ii++;
	}
	//fprintf(tttB,"_%d:%s:%d_",kk,item_array[kk].section,calc_my_lev(kk));

      } else if (out_buff[ii] == '<' && out_buff[ii+1] == 'A' && out_buff[ii+2] == '3') { // do nothing

	num = out_buff[ii+2]; // 1 -- "1.5.7"/ 2 -- "Section" / 3 --"  Introduction." / 4 -- "Exhibit" 
	lev = calc_my_lev(kk);
	//fprintf(tttB,"_%d:%d:%d_",kk,strlen(item_array[kk++].clean_header),calc_my_lev(kk));

	while (out_buff[ii-1] != ':') {
	  ii++;
	}
	start3 = ii;
	kk++;

      } else if (out_buff[ii] == '<' && out_buff[ii+1] == 'A' && out_buff[ii+2] == '4') { // print </MARK>

	num = out_buff[ii+2]; // 1 -- "1.5.7"/ 2 -- "Section" / 3 --"  Introduction." / 4 -- "Exhibit" 
	lev = calc_my_lev(kk);
	if (lev > 0 && kk_array[kk] == 0) {
	  //fprintf(tttB,"<X%d class=\"toc_header\">",lev);  // X for Chaim
	  fprintf(tttB,"<MARK class=\"toc_header\">");  // X for Chaim
	  kk_array[kk] = 1; 
	}
	//fprintf(tttB,"_%d:%d:%d_",kk,strlen(item_array[kk++].clean_header),calc_my_lev(kk));
	while (out_buff[ii-1] != ':') {
	  ii++;
	}
	kk++;

      }


      if (out_buff[ii] == '>' && num != ' ') {
	if (lev > 0) {
	  if (num == '4') {
	    //fprintf(tttB,"</X%d>",lev); // for end of 4 print </H4> // X for Chaim
	    fprintf(tttB,"</MARK>"); // for end of 4 print </H4> // X for Chaim
	  } else if (num == '1' || num == '2') {
	    fprintf(tttB,"<B%c>",num); // for end of 1 and 2 print the markers <B1> and <B2>
	    if (out_buff[ii+1] == '<' && out_buff[ii+2] == '/' && out_buff[ii+3] == 'p') {
	      fprintf(tttB,"</MARK>"); 
	      start3 = -1; 
	    }
	  } else { // num == 3, do nothing
	    //fprintf(tttB,"<X%d>",strlen(item_array[kk-1].clean_header)+1);
	  }
	} else {
	  //fprintf(tttB,"%c%c",out_buff[ii],num);
	}
	num = ' ';
      } else {
	fprintf(tttB,"%c",out_buff[ii]);
      }


      if (lev > 0 && kk > 1 && start3 > 0       
	  && kk < max_item_no && ii >= start3 + strlen(item_array[kk-1].clean_header)+item_array[kk-1].no_of_leading_spaces 
	  && (out_buff[ii] == ' ' 
	      || (out_buff[ii] == '<' && out_buff[ii+1] == '/' && out_buff[ii+1] == 'p') // URI CHANGE 11/23/15 to prevent cases when there's no title
	      || out_buff[ii] == '\n')) { // kk = 0 when FILE
	fprintf(tttB,"</MARK>");
	start3 = -1;  
      } else {
	;
      }

    } // for

  }
  /* END now print the output file */
  return 0;
}
int count_cap(char *text) {
  int ret = 0;
  int ii;
  for (ii = 0; ii < strlen(text); ii++) {
    if (isupper(text[ii])) ret++;
  }
  return ret;
}


int tally_headers_caps_converts() {
  int ii;
  for (ii = 0; ii <= g_seq_no && ii < MAX_SEQ; ii++) { // seq ii
    seq_array[ii].real_no_of_items = seq_array[ii].no_of_items+1;

    // calculate header and sep situation
    if  (seq_array[ii].no_of_items >= 0) {
      int jj;
      // tally all_cap headers and period_seps and converts
      seq_array[ii].total_converts = 0;
      seq_array[ii].total_period_headers = 0;
      int total_len = 0;
      int total_cap = 0;      
      for (jj = 0; jj < seq_array[ii].no_of_items; jj++) {
	int group_no = seq_item_array[ii][jj].group_no;
	int group_item_no = seq_item_array[ii][jj].group_item_no;
	seq_array[ii].total_converts += group_item_array[group_no][group_item_no].convert;

	int item_no = seq_item_array[ii][jj].item_no;
	seq_array[ii].total_period_headers += (item_array[item_no].periods_in_header > 3) ? 1 : 0;

	if (item_array[seq_item_array[ii][jj].item_no].CAP_hdr_len > 0) {
	  seq_array[ii].total_all_cap++;
	}
	if (item_array[item_no].section_m[0] == '.' || item_array[item_no].section_m[0] == ',') {
	  seq_array[ii].total_sep_period++;
	}
	total_len += strlen(item_array[item_no].clean_header);
	total_cap += count_cap(item_array[item_no].clean_header);
	if (debug) fprintf(stderr,"            KK:  ii=%d: ll=%3d: pid=%d: is=%d: cc=%3d: :%s:%s:\n"  
			   , item_no
			   , count_cap(item_array[item_no].clean_header)
			   , item_array[item_no].pid
			   , item_array[item_no].is_special			   
			   , strlen(item_array[item_no].clean_header)
			   , item_array[item_no].section, item_array[item_no].clean_header);
      }
      int too_long = 0;
      if (total_cap * 10 < total_len && total_len > seq_array[ii].no_of_items * 50) {
	too_long = 1;
	for (jj = 0; jj <= seq_array[ii].no_of_items; jj++) {
	  int item_no = seq_item_array[ii][jj].item_no;
	  item_array[item_no].too_long = too_long;
	} // for all items jj
      } 
      if (seq_array[ii].no_of_items > 1) {
	if (debug) fprintf(stderr,"SSSSSSS:seq=%d: noi=%d: tacp=%d:%d: len=%d: too_long=%d:\n\n",ii,seq_array[ii].no_of_items, seq_array[ii].total_all_cap, total_cap, total_len, too_long);
      }
    }
  } // all seqs ii
  return 0;
} // tally_headers_caps_converts()

int print_include_relations() {
  int ii;
  fprintf(stderr,"PRINTING INCLUDE RELATIONS\n");
  for (ii = 0; ii <= g_seq_no && ii < MAX_SEQ; ii++) {
    if (seq_array[ii].include + seq_array[ii].included > 0) {
      fprintf(stderr,"SEQ:%2d: rank=%2d-- ",ii, seq_array[ii].rank, seq_array[ii].include,seq_array[ii].included);

      int jj;
      for (jj = 0; jj < seq_array[ii].include; jj++) {
	fprintf(stderr,"%d:",seq_array[ii].include_array[jj]);
      }
      fprintf(stderr,"----");	        

      for (jj = 0; jj < seq_array[ii].included; jj++) {
	int item = seq_array[ii].included_array[jj];
	fprintf(stderr,"%d(%d): ",item,seq_array[item].included);
      }
      fprintf(stderr,"---\n");
    }
  }
  return 0;
}


int calculate_first_and_last_on_seq() {
  int ii;
  for (ii = 0; ii <= g_seq_no && ii < MAX_SEQ; ii++) { // calculate first and last
    int item_no1 = seq_array[ii].no_of_items;
    int real_item_no1 = seq_array[ii].real_no_of_items;

    int fn1 = -1;
    int ln1 = -1;
    //fprintf(stderr,"      MMMM90:%d:\n",ii);
    if ((item_no1 >= 1 && real_item_no1 >= 1)) { // actually this means 2 items and more

      int removed = 1;
      int jj = 0;
      fn1 = seq_item_array[ii][0].item_no;      
      while (seq_item_array[ii][jj].removed == 1) {
	removed = seq_item_array[ii][jj].removed;
	fn1 = seq_item_array[ii][jj].item_no;
	//fprintf(stderr,"     FOUND FIRST:seq=%d: item=%d: rem=%d:\n",ii,  jj, removed);
	jj++;
      }
      removed = 1;
      jj = item_no1-1;
      ln1 = seq_item_array[ii][jj].item_no;            
      while (removed == 1) {
	removed = seq_item_array[ii][jj].removed;	
	ln1 = seq_item_array[ii][jj].item_no;
	//fprintf(stderr,"     FOUND LAST:seq=%d: item=%d: rem=%d:\n",ii,  jj, removed);
	jj--;
      }

    } else {
      if (debug) fprintf(stderr,"                                --Cancelled BAD3 seq:%d:%d:\n",ii,seq_item_array[ii][0].group_no);
    }
    //fprintf(stderr,"      MMMM91\n");    
    seq_array[ii].fn = fn1;
    seq_array[ii].ln = ln1;
    int first_token = item_array[fn1].token_id;
    int last_token = item_array[ln1].token_id;
    seq_array[ii].first_page = token_array[first_token].page_no;
    seq_array[ii].last_page = token_array[last_token].page_no;
  } // ii
  return 0;
} // calculate_first_and_last_on_seq()

#define ALLOWED_GAP 500
#define ALLOWED_STD 5000
int calculate_indent_and_center() { // mostly this function disallows low-level sequences such as (a), (b), (c) to go over large distances
  int ii;
  if (debug) fprintf(stderr,"CALCULATE_INDENT_AND_CENTER:%d:\n",g_seq_no);

  for (ii = 0; ii <= g_seq_no && ii < MAX_SEQ; ii++) { // calculate first and last
    if (debug) fprintf(stderr,"  SEQ INDENT_AND_CENTER:%d:\n",ii);
    int item_no1 = seq_array[ii].no_of_items+1;
    int real_item_no1 = seq_array[ii].real_no_of_items;
    int level = seq_array[ii].level;    
    if ((item_no1 > 1 && real_item_no1 > 1)) { // actually this means 2 items and more
      int jj;
      int total_indent = 0;
      int total_center = 0;      
      int max_diff = 0;
      int diffF = 0;
      int diffL = 0;
      int DindentF = 0;
      int DindentL = 0;      
      char *title; // is not defined at seq level (sb declared at item level not at seq level), but we need it for
      for (jj = 0; jj < item_no1; jj++) { // calculate individual item properties
	if (debug) fprintf(stderr,"        ITEM INDENT_AND_CENTER:%d:\n",jj);
	int in = seq_item_array[ii][jj].item_no;
	int inN = (jj + 1 < item_no1) ? seq_item_array[ii][jj+1].item_no : -1;
	int inP = (jj > 0) ? seq_item_array[ii][jj-1].item_no : -1;	
	int indent = item_array[in].left_X;
	int center = item_array[in].center;
	int removed = seq_item_array[ii][jj].removed;
	title = item_array[in].title;
	char *section = item_array[in].section;
	int grp_no = item_array[in].grp_no;
	int token_id = reverse_token_array[item_array[in].my_loc];
	int token_idP = (inP > -1) ? reverse_token_array[item_array[inP].my_loc] : token_id;
	int token_idN = (inN > -1) ? reverse_token_array[item_array[inN].my_loc] : token_id;
	int diffP = token_id - token_idP;
	int diffN = token_idN - token_id;
	max_diff = MAX(max_diff,diffN);
	if (jj == 0)  diffF = diffN;	
	if (jj == item_no1 - 1)  diffL = diffP;	
	// printed later
	if (0) fprintf(stderr,"   SEQ_ITEM:seq=%d:jj=%d: item=%d: rem=%d: tid=%d:%d:%d: cen=%d: left=%d: grp=%d: section=%s: title=%s: level=%d: diff:%d:%d:\n"
		, ii, jj, in, removed
		, token_id, token_idP, token_idN
		, center, indent, grp_no
		, section, title, level, diffP, diffN);
	total_indent += indent;
	total_center += center;	
      } // for jj item

      int mean_indent = total_indent / real_item_no1;
      int mean_center = total_center / real_item_no1;      
      seq_array[ii].mean_left_X = mean_indent;
      seq_array[ii].mean_center = mean_center;      
      double dstd_in = 0;
      double dstd_cn = 0;      
      for (jj = 0; jj < item_no1; jj++) { // calculate diff relative to mean
	if (debug) fprintf(stderr,"        ITEM1 INDENT_AND_CENTER1:%d:\n",jj);
	int in = seq_item_array[ii][jj].item_no;
	int indent = item_array[in].left_X;
	int center = item_array[in].center;	
	dstd_in += (indent  - mean_indent) * (indent  - mean_indent);
	dstd_cn += (center  - mean_center) * (center  - mean_center);	
	if (jj == 0) DindentF = abs(indent - mean_indent);	
	if (jj == item_no1 - 1) DindentL = abs(indent - mean_indent);	
      } // for jj item

      dstd_in = (real_item_no1 > 1) ? dstd_in / (real_item_no1 - 1) : 0;
      int std_in = (int)(sqrt (dstd_in));
      dstd_cn = (real_item_no1 > 1) ? dstd_cn / (real_item_no1 - 1) : 0;
      int std_cn = (int)(sqrt ((dstd_cn>0)?dstd_cn:1000000000000));
      seq_array[ii].std_left_X = std_in;
      seq_array[ii].std_center = std_cn;
      seq_array[ii].centered = (std_cn < 3500 && mean_center < 310000 && mean_center > 290000);            // assuming the center is around 300,000


      if (debug) fprintf(stderr,"  SEQ1 INDENT_AND_CENTER:%d:%s: std_in=%d:%d: max_diff=%d:%d:\n",ii,title,
	      max_diff, ALLOWED_GAP , std_in , ALLOWED_STD);
      if (strlen(title) <= 3 // print only suspect sequences, NOT DOC, Lease, Exhibit, etc
	  /* && (max_diff > ALLOWED_GAP || std_in > ALLOWED_STD)*/) {
	if (debug) fprintf(stderr,"     SEQ2 INDENT_AND_CENTER:%d:%s:\n",ii,title);
	if (debug) fprintf(stderr,"\nSUSPECT SEQ:%d:%d:%d: std_in=%d:%d:%d: diff=%d:%d:%d: lev=%d:\n", ii, item_no1, real_item_no1,std_in, DindentF, DindentL, max_diff, diffF, diffL, level);

	// find items where the distance is too big 
	for (jj = 0; jj < item_no1; jj++) { // print item now including diff from std_in, 

	  int in = seq_item_array[ii][jj].item_no;
	  int inN = (jj + 1 < item_no1) ? seq_item_array[ii][jj+1].item_no : -1;
	  int inP = (jj > 0) ? seq_item_array[ii][jj-1].item_no : -1;	
	  int indent = item_array[in].left_X;
	  int center = item_array[in].center;
	  int removed = seq_item_array[ii][jj].removed;
	  char *title = item_array[in].title;
	  char *section = item_array[in].section;
	  int grp_no = item_array[in].grp_no;
	  int token_id = reverse_token_array[item_array[in].my_loc];
	  if (debug) fprintf(stderr,"  ALLOWED_GAP0 token_id=%d: loc==%d: in=%d:\n",token_id, item_array[in].my_loc, in);
	  int token_idP = (inP > -1) ? reverse_token_array[item_array[inP].my_loc] : token_id;
	  int token_idN = (inN > -1) ? reverse_token_array[item_array[inN].my_loc] : token_id;
	  int diffP = token_id - token_idP;
	  int diffN = token_idN - token_id;
	  if (debug) fprintf(stderr,"  ALLOWED_GAP1 token_id   P=%d: C=%d: N=%d:\n",token_idP,token_id,  token_idN);
	  //max_diff = MAX(max_diff,diffN);
	  diffF = (jj == 0) ? diffN : 0;
	  diffL = (jj == item_no1 - 1) ? diffP : 0;	
	  if (debug) fprintf(stderr,"  ALLOWED_GAP2 grp_no=%d, seq(ii)=%d, item(jj)=%d, item_no1=%d: token=%d, diffF=%d: diffL=%d: allowed:%d: tok=%s:\n"
			     , grp_no, ii, jj, item_no1, token_id, diffF, diffL, ALLOWED_GAP, item_array[in].section);
	  if (1 // this is BAD!!! we do not account for chain reactions, if item #1 is removed, what about item #2, etc.?
	      && (grp_no >= 17 && grp_no <= 30)  // I, V, X are no 18 and we don't want them removed
	      //&& grp_no == 28
	      //&& item_no1 <=14 // silly ang group any length can have a dangling bad element, check if it's near the edges (item_no <= 2 or total_items - item_no <=2)
	      && ((diffF > ALLOWED_GAP && jj < item_no1-1) || (diffL > ALLOWED_GAP && jj > 0))
	      ) {
	    seq_item_array[ii][jj].removed = 1;
	    if (debug) fprintf(stderr,"REMOVING2 GAP:%d:%d:\n",ii,jj);
	    seq_array[ii].real_no_of_items--;
	    if (debug) fprintf(stderr,"REMOVED ITEM GAP grp_no=%d, seq(ii)=%d, item(jj)=%d, item_no1=%d: token=%d, diffF=%d: diffL=%d:\n", grp_no, ii, jj, item_no1, token_id, diffF, diffL);
	  }
	  if (debug)fprintf(stderr,"   SEQ_ITEM:seq=%d:jj=%d: item=%d: rem=%d: tid=%d:%d:%d: cen=%d: left=%d: grp=%d: section=%s: title=%s: level=%d: diff:PN=%d:%d:  a=%d: item_no=%d:%d: jj=%d:\n"
		  , ii, jj, in, removed
		  , token_id, (inP > -1) ?token_idP : -1, (inN > -1) ? token_idN : -1
		  , center, indent,   grp_no
			    , section, title, level, diffP, diffN
			    , ALLOWED_GAP, item_no,item_no1, jj);
	  //total_indent += indent;

	} // for jj item

      } // if for print
    }

  }  // for ii seq
  return 0;
} // calculate_indent_and_center()



print_lev(int vv) {
  if (debug) fprintf(stderr,"    STEP777 %d::%d:%d:\n", vv, item_array[vv].selected_seq, seq_array[item_array[vv].selected_seq].level);
  return 0;
}




int print_BBBB(int nn, int ii) {
  return 0;
  int in = seq_item_array[ii][0].item_no; // just get one item arbitrarily
  int spn1 = item_array[in-1].selected_seq; // the seq_no of the previous itemd
  struct Tree_Node *pi = child_triple_array[ii].ptr;
  struct Tree_Node *pn = child_triple_array[spn1].ptr;
  char *dd = (nn == 6) ? "   ": "";
  fprintf(stderr,"   %sBBBBBB%d: seq=%d:%d: item=%d:%d: gn=%d:%d: lev=%d:%d: tit=%s:%s: sec=%s:%s:\n"
	  ,dd, nn, ii,spn1
	  ,   in, in-1
	  ,   seq_array[ii].group_no, seq_array[spn1].group_no
	  ,   (pi) ? child_triple_array[ii].ptr->level :-1,  (pn) ? child_triple_array[spn1].ptr->level : -1
	  ,   item_array[in].title,  item_array[in-1].title
	  ,   item_array[in].section, item_array[in-1].section);

  return 0;
}


int strong_groups_array[MAX_GROUP]; // list the groups that are reliable and can be the framework for the rest of the seqs
int identify_strong_groups() {

  strong_groups_array[1] = 1; 
  strong_groups_array[2] = 2; 
  strong_groups_array[3] = 3; 
  strong_groups_array[4] = 4; 
  strong_groups_array[5] = 5; 
  strong_groups_array[6] = 6; 
  strong_groups_array[7] = 7; 
  strong_groups_array[8] = 8; 
  strong_groups_array[9] = 9; 
  strong_groups_array[10] = 10; 

  strong_groups_array[51] = 51; 
  strong_groups_array[52] = 52; 
  strong_groups_array[53] = 53; 
  strong_groups_array[54] = 54; 
  strong_groups_array[55] = 55; 
  strong_groups_array[56] = 56; 
  strong_groups_array[57] = 57; 
  strong_groups_array[58] = 58; 
  strong_groups_array[59] = 59; 
  strong_groups_array[60] = 60; 

  strong_groups_array[12] = 12; 
  strong_groups_array[13] = 13; 
  strong_groups_array[70] = 70; 
  return 0;
}

int add_items_to_other_groups() { // insert strong items JJ such as SECTION X as sequence breakers (-10) into weak groups II such as a., b., c., so it limits the seqs
  int ii, jj;
  for (ii = 0; ii < MAX_GROUP; ii++) { // the including group (LINKED) (e.g., a. b. c.)

    if (group_no_array[ii] > 1 && strong_groups_array[ii] == 0) { // do not insert into another strong group, do not insert into an empty group
      for (jj = 0; jj < MAX_GROUP; jj++) { // JJ the strong group (ARRAY)(e.g. 8.01, 9.13, etc), items of the strong group (ARRAY) are inserted into the including group (LINK) (a b c)
	if (group_no_array[jj] > 0 && strong_groups_array[jj] != 0) { 

	  combine_linked_groups(ii,jj);

	}
      }
    }
  }
  return 0;
}

int insert_link_item(int jj, int jjk, int ii) { // insert strong (JJ) ARRAY into LINKED (II)
  struct Group_Item *next = group_header_array[ii];
  int inserted = 0;
  int prev_strong = 0; // we do not want to insert after an inserted item
  int ii_strong = 0;
  while (next && inserted == 0) {
    int ii_in = next->item_no;
    prev_strong = ii_strong;
    ii_strong = next->strong_inserted; // we do not want to insert b/f an inserted item
    if (group_item_array[jj][jjk].item_no < ii_in && ii_strong == 0) { // strong (JJ) is smaller than week (II)
      if (prev_strong == 0) { // don't insert after a strong item, that's a duplication
	if (ii ==11) {
	  if (debug) fprintf(stderr,"INSERTING: jj=%d jjk=%d ii=%d in=%d:%d str=%d:\n",jj,jjk,ii,group_item_array[jj][jjk].item_no,ii_in,ii_strong);
	}
	insert_it( jj, jjk, ii ,next);
      } 

      inserted = 1;
    } else {
      next = next->next; // no need to insert after the last item of jj, it won't do anything good
    }
  }
  return 0;
}

int insert_it(int jj, int jjk, int ii ,struct Group_Item *ptr) { // insert item JJ,JJK inside the II group before PTR 
  if (!ptr) {
    fprintf(stderr,"Error: PTR is NULL\n");
    return(0);
  }
  struct Group_Item *new = (struct Group_Item *)malloc(sizeof(struct Group_Item));
  new->item_no = group_item_array[jj][jjk].item_no;
  new->my_enum = -10; // this is how we indicate a "barrier"
  new->convert = 0;
  new->my_P_enum = -1;
  new->strong_inserted = 1;
  if (ptr->prev == NULL) { 
    ; // no need to insert first element. It won't do any good
  } else {
    struct Group_Item *prev = ptr->prev;
    prev->next = new;
    new->prev = prev;
    new->next = ptr;
    ptr->prev = new;
  }
  return 0;
}

int combine_linked_groups(int ii, int jj) { // ii is the including (linked a b c), jj is the strong (array SECTION)
  int jjk;
  for (jjk = 0; jjk < group_no_array[jj]; jjk++) {  // strong, array

    insert_link_item(jj,jjk,ii); // insert an array item (strong) into the linked list

  }
  return 0;
}

int split_groups() {
  identify_strong_groups();
  add_items_to_other_groups();
  copy_list_into_array();
  print_groups(4, "after split_groups ");
  return 0;
}

int print_group_list() {
  int ii = 11;
  struct Group_Item *next = group_header_array[ii];
  int iik = 0;
  if (debug) fprintf(stderr,"\nMMM 11 -- ");
  while (next) {
    if (debug) fprintf(stderr,"GRP =%d:%d:%d--",iik,next->item_no,next->my_enum);
    next = next->next; // no need to insert after the last item of jj, it won't do anything good
    iik++;
  } // ii
  if (debug) fprintf(stderr,"MM\n");
  return 0;
}


int copy_list_into_array() {
  int ii;
  for (ii = 0; ii < MAX_GROUP; ii++) {
    struct Group_Item *next = group_header_array[ii];
    int iik = 0;
    while (next) {
      tmp_group_item_array[iik].item_no = next->item_no;
      tmp_group_item_array[iik].convert = next->convert;
      tmp_group_item_array[iik].my_enum = next->my_enum;
      tmp_group_item_array[iik].my_P_enum = next->my_P_enum;
      tmp_group_item_array[iik].strong_inserted = next->strong_inserted;

      next = next->next; // no need to insert after the last item of jj, it won't do anything good
      iik++;
    }
    group_no_array[ii] = iik;
    int mm;
    for (mm = 0; mm < iik; mm++) {
      group_item_array[ii][mm].item_no = tmp_group_item_array[mm].item_no;
      group_item_array[ii][mm].convert = tmp_group_item_array[mm].convert;
      group_item_array[ii][mm].my_enum = tmp_group_item_array[mm].my_enum;
      group_item_array[ii][mm].my_P_enum = tmp_group_item_array[mm].my_P_enum;
      group_item_array[ii][mm].strong_inserted = tmp_group_item_array[mm].strong_inserted;
    }
       
  } // ii
  return 0;
}


int collect_toc_summary(int no_of_children) {
  int ii;
  for (ii = 0; ii < MAX_SEQ; ii++) {
    struct Tree_Node *ptr = child_triple_array[ii].ptr;
    if (ptr != NULL) {
      toc_summary.good_no_of_items += seq_array[ptr->seq_no].real_no_of_items;
      toc_summary.good_seq_no++;
      toc_summary.total_gap += seq_array[ptr->seq_no].total_gap;
      toc_summary.total_diff += seq_array[ptr->seq_no].total_diff;
      if (ptr->level == 3) {
	toc_summary.top_level_seq_no ++;
	toc_summary.top_level_no_of_items += seq_array[ptr->seq_no].real_no_of_items;
      }
      if (ptr->level == 2) {
	toc_summary.file_level_no_of_items += seq_array[ptr->seq_no].real_no_of_items;
      }
    }
  }
  int jj, done;
  char query_buff[2000];
  
  for (done = 0, jj = 1; jj < max_item_no-1; jj++) {
    int selected_seq = item_array[jj].selected_seq;
    int level = (child_triple_array[selected_seq].ptr) ? child_triple_array[selected_seq].ptr->level : seq_array[selected_seq].level;

    if (level > 2 && level < 10) {
      int loc = item_array[jj].my_loc;
      int indent = (loc < MAX_COORDS && loc > -1) ? coords_array[loc].x1000 : -1;
      sprintf(query_buff,"(%d, %d, %d, %d, %d, %d, '0_:_%s1_:_', '0_:_%s1_:_', '0_:_%s1_:_', %d), ", // inserting "0_:_ and 1_:_ to be replaced by ' after sql_real_escape
	      doc_id, jj, item_array[jj].pid, level, item_array[jj].my_enum, item_array[jj].grp_no, item_array[jj].section, item_array[jj].clean_header, strlen(item_array[jj].title) > 2 ? item_array[jj].title : "", indent);
      strcat(toc_query,query_buff);
      if (done == 0) {
	toc_summary.first_item = item_array[jj].pid;
	done = 1;
      }
    } // if level != 10
  } //for ii
  sprintf(query_buff,"(%d, %d, %d, %d, %d, %d, 0_:_%s1_:_, 0_:_%s1_:_, 0_:_%s1_:_, %d) ",
	  doc_id, jj, 10000, 10, 10, 10, "10", "dummy", "", -1);
  strcat(toc_query,query_buff);

  //toc_summary.first_item = item_array[1].pid;
  if (max_item_no > 3) {
    toc_summary.last_item = item_array[max_item_no-3].pid;
  } 
  toc_summary.last_para = pid;
  if (debug) fprintf(stderr,"SUMMARY: good_tot=%d good_seq=%d top_lev_seq=%d top_lev_items=%d file_lev_seq=%d tot_gap=%d tot_diff=%d first=%d last=%d last_para=%d\n"
	  ,toc_summary.good_no_of_items
	  ,toc_summary.good_seq_no
	  ,toc_summary.top_level_seq_no
	  ,toc_summary.top_level_no_of_items
	  ,toc_summary.file_level_no_of_items
	  ,toc_summary.total_gap
	  ,toc_summary.total_diff
	  ,toc_summary.first_item
	  ,toc_summary.last_item
	  ,toc_summary.last_para
	  );
  return 0;
} // collect_toc_summary()


// removing items that have duplicate seq and are bad
int remove_item_from_seq(int my_seq, int other_seq,int other_seq_item, int item_no) {
  if (other_seq == 18) fprintf(stderr,"JJJJ:my_seq=%d: other_seq=%d: other_seq_item=%d: in=%d:\n",
			       my_seq, other_seq, other_seq_item, item_no);
  seq_array[other_seq].real_no_of_items--;
  seq_item_array[other_seq][other_seq_item].removed = 1;
  if (debug) fprintf(stderr,"REMOVING1:other_seq=%d: other_seq_item=%d: seq=%d: in=%d:\n",other_seq,other_seq_item, my_seq, item_no);
  fix_neighbors_badness(other_seq,other_seq_item);

  return 0;
}

int rank_array[MAX_INCLUDED]; // how many items are included in each seq
int rank_included_seqs(int my_seq) {
  int ii;
  for (ii = 0; ii < seq_array[my_seq].include; ii++) {
    int seq_no = seq_array[my_seq].include_array[ii];
    if (0) {
      if (debug) fprintf(stderr,"<BR>INC: ii=%d seqn=%d: gn=%d noi=%d:\n",ii, seq_no, seq_array[seq_no].no_of_items,seq_array[seq_no].group_no);
    }
    // place here only direct includes:
    // if XIII (1 field) is the same as 13.3 (2 fields deep)
    // if new seq is included only in 1 
    rank_array[ii] = seq_array[my_seq].include_array[ii];
  }
  return 0;
}

int fix_neighbors_badness(other_seq,other_seq_item) {
  // not yet implemented
  // when an item is removed, we need to update it's neighbors badness (it should be reduced)
  return 0;
}

int found_char_in_vpp(char text[]) {
  int len = strlen(text);
  int ii;
  for (ii = 0; ii < len; ii++) {
    if (isalnum(text[ii]) > 0) {
      return 1;
    }
  }
  return 0;
}

int sub_section_function(char my_text[]) {
  if (d_group == 0) { // we came from nothing
    g_group_no = 12;
  } else if (d_group == 1){
    g_group_no = 2; // we came from SECTION
  } else if (d_group == 3){
    g_group_no = 32; // we came from PART
  } else if (d_group == 4){
    g_group_no = 42; // we came from PARA
  } else if (d_group == 5){
    g_group_no = 52; // we came from ARTICLE
  } else {
    g_group_no = 62; // we came from OTHER
  }
  // t_title = "_";
  do_item_BEGIN_state_4p();
  return 0;
}
int substitute_underscore(char *text) {
  int ii;
  for (ii = 0; ii < strlen(text); ii++) {
    if (text[ii] == '_' && text[ii-1] == ':' && text[ii-2] == '_' && (text[ii-3] == '0' || text[ii-3] == '1')) {
      if (text[ii-3] == '0') {
	text[ii] = '\''; text[ii-1] = ' '; text[ii-2] = ' ';text[ii-3] = ' ';
      } else {
	text[ii] = ' '; text[ii-1] = ' '; ; text[ii-2] = ' ';text[ii-3] = '\'';
      }
    }
  }
  return 0;
}

int sql_output(int doc_id) {
   /************** SQL ********************/
  //char *server = "localhost";
  char *server = db_IP; // "54.241.17.226"
  char *user = db_user_name; // "root";
  char *password = db_pwd; //"imaof3";
  char *database = db_name; // "dealthing";
  /************** END SQL ****************/

  
  MYSQL *conn = mysql_init(NULL);
  

  /* now connect to database */
  if (!mysql_real_connect(conn,server,user,password,database,0,NULL,0)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }


#define INTEGER 5
#define TEXT 3
#define COMPLEX 2

  int org_id[1];
  int complex_id_name[1];
  int deal_id = get_did_oid(conn,doc_id, org_id);
  delete_complex_field(conn,doc_id,"toc_params"); // remove old inserts
  int score = 100;
  int complex_id = insert_complex_field(conn,doc_id, deal_id, org_id[0], pid, "toc_params",complex_id_name,score,4,"TOC",1);
  static char ttt[400];
  sprintf(ttt,"%d",toc_summary.first_item);
  insert_any_field(conn, "integer",doc_id, deal_id, org_id[0], pid, complex_id, complex_id_name[0], "first", "NA", ttt, 1,score,"first");
  sprintf(ttt,"%d",toc_summary.last_item);
  insert_any_field(conn, "integer",doc_id, deal_id, org_id[0], pid, complex_id, complex_id_name[0], "last", "NA", ttt, 2,score,"last");
  sprintf(ttt,"%d",toc_summary.last_para);
  insert_any_field(conn, "integer",doc_id, deal_id, org_id[0], pid, complex_id, complex_id_name[0], "last para", "NA", ttt, 3,score,"last_p");
  sprintf(ttt,"%d",toc_summary.good_no_of_items);
  insert_any_field(conn, "integer",doc_id, deal_id, org_id[0], pid, complex_id, complex_id_name[0], "total no", "NA", ttt, 4,score,"tots");
  sprintf(ttt,"%d",toc_summary.good_seq_no);
  insert_any_field(conn, "integer",doc_id, deal_id, org_id[0], pid, complex_id, complex_id_name[0], "nested_seq_no", "NA", ttt, 5,score,"nest");
  sprintf(ttt,"%d",toc_summary.file_level_no_of_items);
  insert_any_field(conn, "integer",doc_id, deal_id, org_id[0], pid, complex_id, complex_id_name[0], "exhibit no", "NA", ttt, 6,score,"exh");
  sprintf(ttt,"%d",toc_summary.top_level_no_of_items);
  insert_any_field(conn, "integer",doc_id, deal_id, org_id[0], pid, complex_id, complex_id_name[0], "top lev no", "NA", ttt, 7,score,"tops");
  sprintf(ttt,"%d",toc_summary.total_gap);
  insert_any_field(conn, "integer",doc_id, deal_id, org_id[0], pid, complex_id, complex_id_name[0], "gap no", "NA", ttt, 8,score,"gap");
  sprintf(ttt,"%d",toc_summary.total_diff);
  insert_any_field(conn, "integer",doc_id, deal_id, org_id[0], pid, complex_id, complex_id_name[0], "diff no", "NA", ttt, 9,score,"diff");

  return 0;
} // sql_output()

int calc_centers(int token_no) { // in order to determine the center of a line with EXHIBIT
  int ii;
  int first_coord = 0;
  int first_i = 0;  
  for (ii = 0; ii < token_no; ii++) {
    if (ii > 0 && token_array[ii].x2_1000 > 0) {
      if (token_array[ii].line_no > token_array[ii-1].line_no
	  || token_array[ii].page_no > token_array[ii-1].page_no
	  ) {
	int last_coord = token_array[ii-1].x2_1000;
	int center = token_array[first_i].center = (last_coord + first_coord) / 2;
	if (0) fprintf(stderr,"      JHHHHHHHH0:ii=%d: xx=%d:%d: PL=%d:%d: c=%d: t=%s:\n"
		,first_i, first_coord,last_coord,token_array[first_i].page_no, token_array[first_i].line_no, center, token_array[first_i].text);
	first_coord = token_array[ii].x1_1000;
	first_i = ii;
	// first_word = strdup(token_array[ii].text);
      }
    }
  } // for
  return 0;
}

int read_tokens() { // in order to determine the center of a line with EXHIBIT
  char query[200000];
  sprintf(query,"select sn, x1, x2, page_no, line_no, text, loc, y1, y2 \n\
                     from deals_token \n\
                     where doc_id = '%d' and source_program='%s' "
	  , doc_id, source_ocr);

  if (debug_entity) fprintf(stderr,"QUERY41=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY41:%s\n",mysql_error(conn));
  }
  sql_res = mysql_store_result(conn);
  while ((sql_row = mysql_fetch_row(sql_res))) {
    token_no = atoi(sql_row[0]);
    if (token_no < 0 || token_no >= MAX_TOKEN) {
      fprintf(stderr,"Error: token_no:%d: exceeded MAX_TOKEN:%d:\n",token_no,MAX_TOKEN);
      exit(0);
    }
    token_array[token_no].sn = token_no;
    token_array[token_no].x1_1000 = (int)(atof(sql_row[1]) * 1000.0);
    token_array[token_no].x2_1000 = (int)(atof(sql_row[2]) * 1000.0);        
    token_array[token_no].page_no = atoi(sql_row[3]);
    token_array[token_no].line_no = atoi(sql_row[4]);
    token_array[token_no].text = strdup(sql_row[5]);
    token_array[token_no].loc = atoi(sql_row[6]);    
    int loc = atoi(sql_row[6]); 
    token_array[token_no].y1_1000 = (int)(atof(sql_row[7]) * 1000.0);
    token_array[token_no].y2_1000 = (int)(atof(sql_row[8]) * 1000.0);        
    //    token_array[token_no].loc = loc;
    if (loc < 0 || loc >= MAX_LOC) {
      fprintf(stderr,"Error: loc:%d: exceeded MAX_LOC:%d:\n",token_no,MAX_TOKEN);
      exit(0);
    }
    reverse_token_array[loc] = token_no;
    if (debug && 0) fprintf(stderr,"BRRRRR0 tn=%d: loc=%d: reverse=%d: x1=%d: pl=%d:%d: text=%s:\n"
		   ,token_no, loc, reverse_token_array[loc], token_array[token_no].x1_1000,token_array[token_no].page_no,token_array[token_no].line_no,token_array[token_no].text);
  }
  fprintf(stderr,"Returning array with :%d: TOKENs\n",token_no);
  return token_no+1;
}



int print_paragraphtoken_array(int max_para_no) {
  int ii;
  for (ii = 0; ii < max_para_no; ii++) {
    fprintf(stderr,"PARA_INFO: para=%d: line=%d: page=%d:\n", ii, paragraphtoken_array[ii].line_no , paragraphtoken_array[ii].page_no);
  }
  return 0;
} // print_paragraphtoken_array() 

	    
 
int read_horizontalTabToken() { // in order to determine the center of a line with EXHIBIT
  char query[200000];
  sprintf(query,"select token_no, id, line_no, page_no, space_size, bin_no \n\
                     from deals_horizontaltabtoken \n\
                     where doc_id = '%d' and source_program='%s' "
	  , doc_id, source_ocr);

  if (debug_entity) fprintf(stderr,"QUERY95=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY95:%s\n",mysql_error(conn));
  }
  sql_res = mysql_store_result(conn);
  int space_size = 0;
  while ((sql_row = mysql_fetch_row(sql_res))) {
    int token_no = atoi(sql_row[0]);
    int line_no = atoi(sql_row[2]);
    int page_no = atoi(sql_row[3]);        
    horizontal_tab_array[horizontal_tab_no].id = atoi(sql_row[1]);
    horizontal_tab_array[horizontal_tab_no].line_no = line_no;
    horizontal_tab_array[horizontal_tab_no].token_no = token_no;
    horizontal_tab_array[horizontal_tab_no].page_no = page_no;    
    horizontal_tab_array[horizontal_tab_no].space_size = space_size = atoi(sql_row[4]);
    horizontal_tab_array[horizontal_tab_no].bin_no = atoi(sql_row[5]);
    if (space_size > 0 && pageline2horizontal_array[page_no][line_no] == 0) pageline2horizontal_array[page_no][line_no] = horizontal_tab_no;
    //fprintf(stderr,"   FF/FF: pl=%d:%d: hs_no=%d: \n", page_no, line_no, pageline2horizontal_array[page_no][line_no]);
    token2horizontal_tab_array[token_no] = horizontal_tab_no;
    horizontal_tab_no++;
  }
  fprintf(stderr,"Returning array with :%d: HSs\n",horizontal_tab_no);
  int pp,ll;
  for (pp  = 0; pp < 4; pp++) {
    for (ll = 0; ll < 60; ll++) {
      if (ll == 0) fprintf(stderr,"\n\n");
      int hs_no = pageline2horizontal_array[pp][ll];
      if (0) fprintf(stderr,"   MM/MM: pl=%d:%d: hs_no=%d: p=%d: l=%d: t=%d: sp=%d:\n",pp,ll,  hs_no
	      , (hs_no == 0) ? -1 : horizontal_tab_array[hs_no].page_no  
	      , (hs_no == 0) ? -1 : horizontal_tab_array[hs_no].line_no
	      , (hs_no == 0) ? -1 : horizontal_tab_array[hs_no].token_no
	      , (hs_no == 0) ? -1 : horizontal_tab_array[hs_no].space_size);
    }
  }
  return horizontal_tab_no;
}



int read_summarypoints() { // in order to determine the center of a line with EXHIBIT
  char query[200000];
  sprintf(query,"select token_id, id, line_no, page_no, left_indent, text \n\
                     from deals_summarypoints \n\
                     where doc_id = '%d' and source_program='%s'"
	  , doc_id, source_ocr);

  if (debug_entity) fprintf(stderr,"QUERY39=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY39:%s\n",mysql_error(conn));
  }
  sql_res = mysql_store_result(conn);
  int mm = 0;
  while ((sql_row = mysql_fetch_row(sql_res))) {
    int token_id = atoi(sql_row[0]);
    token2point[token_id] = atoi(sql_row[1]);
    //if (debug) fprintf(stderr,"IDDDDD:tid=%d: id=%d: text=%s:\n",token_id,atoi(sql_row[1]), sql_row[5]);
    mm++;
  }
  fprintf(stderr,"Returning array with :%d: POINTs\n",mm);
  return mm;
}


int read_paragraphtoken() { 
  char query[200000];
  sprintf(query,"select para_no, token_id, page_no, line_no \n\
                     from deals_paragraphtoken \n\
                     where doc_id = '%d' and source_program='%s'"
	  , doc_id, source_ocr);

  if (debug) fprintf(stderr,"QUERY412=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY412:%s\n",mysql_error(conn));
  }
  sql_res = mysql_store_result(conn);
  int nn = mysql_num_rows(sql_res);
  while ((sql_row = mysql_fetch_row(sql_res))) {
    int para_no = atoi(sql_row[0]);    
    paragraphtoken_array[para_no].token_id = atoi(sql_row[1]); 
    paragraphtoken_array[para_no].page_no = atoi(sql_row[2]);
    paragraphtoken_array[para_no].line_no = atoi(sql_row[3]);
    //fprintf(stderr,"DDDD: para=%d: page=%d: line=%d:\n", para_no, atoi(sql_row[2]), atoi(sql_row[3]));
    //paragraphtoken_array[para_no].dc_line_no = atoi(sql_row[4]);           
  }
  fprintf(stderr,"Returning array with :%d:%d: PARATOKENs\n",nn, para_no);
  return para_no+1;
} // read_paragraphtoken


int get_page_line_properties() {
  char query[10000];
  sprintf(query,"select page_no, line_no, center, no_of_words, no_of_first_cap_words, no_of_all_cap_words, left_X \n\
                from deals_page_line_properties \n\
                where  \n\
                   doc_id = '%d' and source_program='%s' "
	  , doc_id, source_ocr);

    if (debug) fprintf(stderr,"QUERY8.1=%s\n",query);    
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY8.1=%s\n",query);    
    }

    sql_res = mysql_store_result(conn);
    int prev_page_id = -1;
    int line_id = -1;
    int page_id = -1;
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
      fprintf(stderr,"WWWWWWWWB:pp=%d: ln=%d: now=%d:  cnt=%d: lx=%d: cap=%d:%d: \n",page_id,line_id
	      , page_line_properties_array[page_id][line_id].no_of_words
	      , page_line_properties_array[page_id][line_id].center
	      , page_line_properties_array[page_id][line_id].left_X
	      , page_line_properties_array[page_id][line_id].no_of_first_cap_words
	      , page_line_properties_array[page_id][line_id].no_of_all_cap_words    
	      );
	      
    }
    return page_id+1;
} // get_page_line_properties(

int print_page_line_properties(int page_no) {
  int ii, jj;
  fprintf(stderr,"PRINTING PAGE_LINE_PROPS:%d:\n", page_no);
  for (ii = 0; ii <= page_no; ii++) {
    fprintf(stderr,"     PAGE:%d:%d:\n",ii, page_line_array[ii].no_of_lines);    
    for (jj = 0; jj < page_line_array[ii].no_of_lines; jj++) {
      fprintf(stderr,"       LINE:%d:%d:%d:\n",ii,jj, page_line_properties_array[ii][jj].center);
    }
  }
  return 0;
}

int print_item_array_nothing(int nn) {
  int jj;
  if (debug) fprintf(stderr,"\nMYITEM NOTHING%d:%d:\n", nn, max_item_no);
  for (jj = 0; jj < max_item_no-1; jj++) {
    int selected_seq = item_array[jj].selected_seq;
    if (debug) fprintf(stderr,"  VVV:jj=%d: my_item=%d: no_seqs%d: pid=%d: grp=%d: :%s:%s:  selected=:%d:--"
	    , jj, 0/*item_array[jj].my_item*/, item_array[jj].no_of_seqs, item_array[jj].pid, item_array[jj].grp_no
	    , item_array[jj].section,item_array[jj].header, selected_seq);

    int kk;
    for (kk = 0; kk < item_array[jj].no_of_seqs; kk++) {
      if (debug) fprintf(stderr,"kk=%d:%d:%d: ",kk, item_array[jj].seq_array[kk].seq_no, item_array[jj].seq_array[kk].seq_item_no);
    }
    fprintf(stderr,"\n");
  }
  return(0);
} // print_item_array_nothing()

int print_seq_55(int nn, int seq_no) {
  int item_no = seq_item_array[seq_no][0].item_no;
  int selected_seq = item_array[item_no].selected_seq;
  int new_level = (child_triple_array[selected_seq].ptr) ? child_triple_array[selected_seq].ptr->level : seq_array[selected_seq].level;
  fprintf(stderr, "\n       SEQ%d_55: seq_no=%d: lev=%d:%d: noi=%d:%d: fl=%d:%d: gn=%d: in=%d cn=%d: flpage=%d:%d: primary=%f: rank2=%d:\n"
	  , nn
	  , seq_no, new_level, seq_array[seq_no].level, seq_array[seq_no].real_no_of_items,seq_array[seq_no].no_of_items, seq_array[seq_no].fn, seq_array[seq_no].ln , seq_array[seq_no].group_no, seq_array[seq_no].mean_left_X, seq_array[seq_no].mean_center, seq_array[seq_no].first_page, seq_array[seq_no].last_page, log((float)(seq_array[seq_no].last_page - seq_array[seq_no].first_page +1)) * log((float)seq_array[seq_no].real_no_of_items+1), seq_array[seq_no].rank);
  return 0;
}

char *lim(char *text, int num) {
  if (strlen(text) > num) {
    text[num] = '\0';
  }
  return text;
}
    
int print_seq_array(int nn) {
  int ii;
  if (debug || 1) fprintf(stderr, "PRINTING MYSEQ ARRAY%d:%d:\n", nn,g_seq_no);
  for (ii = 0; ii <= g_seq_no; ii++) {
    //if ((1 || seq_array[ii].real_no_of_items > 1) && (seq_array[ii].group_no != 61 /*seq_array[ii].group_no < 61 || seq_array[ii].group_no > 69*/ || (seq_array[ii].bad_found_61 == 0 && seq_array[ii].real_no_of_items > 1))) { // seq_61 gets tough treatment
    int item_no = seq_item_array[ii][0].item_no;
    int selected_seq = item_array[item_no].selected_seq;
    int new_level = (child_triple_array[selected_seq].ptr) ? child_triple_array[selected_seq].ptr->level : seq_array[selected_seq].level;
    int no_of_items = seq_array[ii].no_of_items;
    if (no_of_items > 0) {
    fprintf(stderr, "       SEQ: seq_no=%d: lev=%d:%d: noi=%d:%d: fl=%d:%d: gn=%d: in=%d cn=%d: flpage=%d:%d: primary=%f: rank2=%d:\n"
	    , ii, new_level, seq_array[ii].level, seq_array[ii].real_no_of_items, seq_array[ii].no_of_items, seq_array[ii].fn, seq_array[ii].ln , seq_array[ii].group_no, seq_array[ii].mean_left_X, seq_array[ii].mean_center, seq_array[ii].first_page, seq_array[ii].last_page, log((float)(seq_array[ii].last_page - seq_array[ii].first_page +1)) * log((float)seq_array[ii].real_no_of_items+1), seq_array[ii].rank);

    int jj;
    for (jj = 0; jj < no_of_items; jj++) {
      int group_no = seq_item_array[ii][jj].group_no;
      int group_item_no = seq_item_array[ii][jj].group_item_no;
      seq_array[ii].total_converts += group_item_array[group_no][group_item_no].convert;

      int item_no = seq_item_array[ii][jj].item_no;
      seq_array[ii].total_period_headers += (item_array[item_no].periods_in_header > 3) ? 1 : 0;

      if (item_array[seq_item_array[ii][jj].item_no].CAP_hdr_len > 0) {
	seq_array[ii].total_all_cap++;
      }
      if (item_array[item_no].section_m[0] == '.' || item_array[item_no].section_m[0] == ',') {
	seq_array[ii].total_sep_period++;
      }
      int selected_seq = item_array[jj].selected_seq;
      int lev = seq_array[selected_seq].level;
      char *mext = lim(item_array[item_no].clean_header,100);
      if (debug || nn ==1) fprintf(stderr,"            KKM:jj=%d: in=%d: gin=%d: pid=%d: ss=%d: lev=%d: ll=%3d: cc=%3d: :%d:%s:%s: pid=%d: is=%d: gn=%d: sseq=%d: rem=%d:\n"
				   , jj, item_no, group_item_no
				   , item_array[item_no].pid
				   , count_cap(item_array[item_no].clean_header)
				   , selected_seq, lev
				   , strlen(item_array[item_no].clean_header)
				   , item_array[item_no].section_v[0], item_array[item_no].section, item_array[item_no].clean_header, item_array[item_no].pid
				   , item_array[item_no].is_special, item_array[item_no].grp_no, item_array[item_no].selected_seq, seq_item_array[ii][jj].removed);

    } // for jj
    }
  } // for ii
  if (debug || 1) fprintf(stderr, "PRINTED MYSEQ ARRAY%d:%d:\n", nn,g_seq_no);  
  return 0;
}

/*  K-MEANS:
4 centroids
either one of three left points or center
*/
#define MAX_CLUST_ITEM 200
struct Group_Cluster {
  int no_of_items;
  int item_array[MAX_CLUST_ITEM];
} group_cluster_array[4];

#define MAX_ITER 5
#define K_MEAN 5
int cluster_group_left_center(int group_no) {
  int ii;
  if (group_no != 20) return 0;
  if (group_no_array[group_no] > 0) {
  if (debug) fprintf(stderr,"CLUSTERING GROUP:%d: %d:\n", group_no, group_no_array[group_no]);  
  int mean_left[K_MEAN];
  mean_left[0] = 295000; // center0
  mean_left[1] = 30000000; // center1  

  mean_left[2] = 50000;
  mean_left[3] = 150000;
  mean_left[4] = 250000;


  int kk;
  int iter;
  int same_all_centroids = 0;
  int ttt = 0;

  for (iter = 0; same_all_centroids == 0 && iter < MAX_ITER; iter++) { // do 3 iterations, see if stabilizes
    if (ttt) fprintf(stderr,"   ITER:%d:\n",iter);
    int cent;
    int total_no_of_items = 0;
    for (cent = 0; cent < K_MEAN; cent++) { // init each centroid
      group_cluster_array[cent].no_of_items = 0;
    }

    for (kk = 0; kk < item_no; kk++) {
      if (item_array[kk].grp_no == group_no) {
	total_no_of_items++;
	int min_dist = 10000000;
	int cent;
	int the_cent = -1;  // the best centroid for this item
	for (cent = 0; cent < K_MEAN; cent++) { // // select best centroid out of K_MEAN (6)
	  if (cent < 2) { // special test for center
	    int diff = 2 * abs(item_array[kk].center - mean_left[cent]); // CENTER MULTIPLIED SINCE MANY NORMAL LINES GRAVITATE NEAR CENTER
	    //fprintf(stderr,"           TTT0: cent=:%d: the_cent=%d: xx=%d:%d: diff=%d: min_dist=%d:\n", cent, the_cent, item_array[kk].center, mean_left[cent], diff, min_dist); 
	    if (diff < min_dist) { // here we check vis the center of the item
	      min_dist = diff;
	      the_cent = cent;
	      //fprintf(stderr,"              TTT01: cent=:%d:  the_cent=%d: xx=%d:%d: diff=%d: min_dist=%d:\n", cent, the_cent, item_array[kk].center, mean_left[cent], diff, min_dist);	      	      
	    }
	  } else { // regular test for left_X
	    int diff = abs(item_array[kk].left_X - mean_left[cent]);
	    //fprintf(stderr,"           TTT3: cent=:%d: the_cent=%d: xx=%d:%d: diff=%d: min_dist=%d:\n", cent, the_cent, item_array[kk].left_X, mean_left[cent], diff, min_dist);	    
	    if (diff < min_dist) { // here we check vis the leftX of the item
	      min_dist = diff;
	      the_cent = cent;
	      //fprintf(stderr,"              TTT31: cent=:%d:  the_cent=%d: xx=%d:%d: diff=%d: min_dist=%d:\n", cent, the_cent, item_array[kk].left_X, mean_left[cent], diff, min_dist);	      
	    }
	  }
	} // select best centroid
	group_cluster_array[the_cent].item_array[group_cluster_array[the_cent].no_of_items] = kk;      
	group_cluster_array[the_cent].no_of_items++;
	//fprintf(stderr,"        TTT:%d:%d:\n",the_cent,group_cluster_array[the_cent].no_of_items);
      
      } // check groupiness
    } // go over each item

    if (ttt) fprintf(stderr,"CENTERS FOR GROUP:%d: %d:\n", group_no, total_no_of_items);
    same_all_centroids = 1;
    for (cent = 0; cent < K_MEAN; cent++) { // tally each centroid
      int ii;
      int total_center = 0;
      if (group_cluster_array[cent].no_of_items > 0) { // calc mean
	for (ii = 0; ii < group_cluster_array[cent].no_of_items; ii++) {
	  if (cent < 2) {
	    int in = group_cluster_array[cent].item_array[ii];
	    total_center += item_array[in].center;
	  } else {
	    int in = group_cluster_array[cent].item_array[ii];
	    total_center += item_array[in].left_X;	
	  }
	} // for ii all items      

	int old_center = mean_left[cent];
	int new_center = total_center / group_cluster_array[cent].no_of_items;
	if (old_center != new_center) same_all_centroids = 0;
	mean_left[cent] = new_center;

	if (ttt) fprintf(stderr,"    CENT:%d: %d: o/n=%d:%d:\n", cent, group_cluster_array[cent].no_of_items, old_center, new_center);
      } // if more than 0 items
    } // for each centroid
  } // iter

  
  if (debug) fprintf(stderr,"GROUP MEAN/STD:%d: %d:\n", group_no, group_no_array[group_no]);  
  int cent;
  for (cent = 0; cent < K_MEAN; cent++) { // tally each centroid
    if (group_cluster_array[cent].no_of_items > 0) {
      int total_std = 0;
      for (ii = 0; ii < group_cluster_array[cent].no_of_items; ii++) {
	if (cent < 2) {
	  int in = group_cluster_array[cent].item_array[ii];
	  total_std += pow((item_array[in].center - mean_left[cent])/100,2);
	} else {
	  int in = group_cluster_array[cent].item_array[ii];
	  total_std += pow((item_array[in].left_X - mean_left[cent])/100,2);
	}
      } // for ii all items
      int std = total_std * 100 / group_cluster_array[cent].no_of_items;
      if (debug) fprintf(stderr,"    CENT:%d: %d: MEAN=%d: STD=%d:\n", cent,  group_cluster_array[cent].no_of_items, mean_left[cent], std);
    }
  }
  if (debug) fprintf(stderr,"END CLUSTERING GROUP:%d: %d:\n\n", group_no, group_no_array[group_no]);
  }
  return 0;
} // cluster_group_left_center(ii);

int cluster_each_group() {
  int ii;
  for (ii = 0; ii < MAX_GROUP; ii++) {
    if (0 || ii == 20) {
      cluster_group_left_center(ii);
    }
  }
  return 0;
}

int my_begin_yyyy1() {
  BEGIN YYYY1;
  return 0;
}

int my_begin_yyyy2() {
  BEGIN YYYY2;
  return 0;
}

int my_begin_yyyy3() {
  BEGIN YYYY3;
  return 0;
}

int my_begin_yyyy6() {
  BEGIN YYYY6;
  return 0;
}

char *my_fopen(char *results_name) {
  static char buff[100000];
  FILE *results_file = fopen(results_name,"r");
  if (!results_file) {
    fprintf(stderr,"Error: Can't read open :%s:\n",results_name);
    exit(0);
  }
  fprintf(stderr,"Error: Read Opened results_name:%s:\n",results_name);  
  static char line[10000];
  while (fgets(line, 1000, results_file) != NULL) {
    strcat(buff,line);
  }
  fclose(results_file);
  return buff;
}

int my_rewind_and_BEGIN() {
  rewind(yyin);
  BEGIN YYYYcopy;
  return 0;
}

