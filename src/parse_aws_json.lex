%x word_block word_block_text word_block_X word_block_Y word_block_page word_block_confidence word_block_id
%x line_block line_block_text line_block_X line_block_Y line_block_page line_block_confidence line_block_ids
%x page_block
%x s_find_token ending
%{
/*  
************        This program runs in two SEPARATE steps (-F0/1); Alma python in between **********************

1.  This program is called from DT_NEW_OCR
2.  It reads the AWS json
3.  it assigns "my_line" as it goes
4.  It assumes we have already ran ABBYY ocr into OCRTOKEN
5.  It populates the OCRTOKEN and OCRBLOCK table
6.  It standardizes coords (range is 0.00 to 1.00 multiplied by 10000 for we deal with int and not float) in OCRTOKEN and OCRBLOCK (for AWS and for ABBYY) 

The main difficulty is SYNCRONIZING THE TOKENs and the BLOCKs
For WW in WORD_array
   find an LL LINE it fits into (by overlap)
   1.  if it fits in current LINE then do nothing
   2.  else if it fits into the next LINE then advance LL by one
   2.  else if it fits into the one after next LINE then advance LL by two
   3.  else if it it above the currelnt LINE then advance LL by one

MOST IMPORTANT:  DON'T GET OFF THE RAILS!!!

## 33 pages
(../bin/parse_aws_json -d 71815 -Pudev1-from-prod-feb26.cu0erabygff3.us-west-2.rds.amazonaws.com -N dealthing -U root -W imaof333 -X 16.375 -Y 12.62 -D 1 -sAWS < ~/dealthing/dtcstuff/tmp/71815/71815_double_column_33_pages_pdf_1587597487.json -n1) >& ttt

OMER
(../bin/parse_aws_json -d 71840 -Pudev1-from-prod-feb26.cu0erabygff3.us-west-2.rds.amazonaws.com -N dealthing -U root -W imaof333 -X 16.375 -Y 12.62 -D 1 -sAWS < ../tmp/71840/71840_double_column_with_toc_indentation_omer_pdf_1588207568.json -n1> bbb) >& ttt_71840_parse

GROVER ENUM
(../bin/parse_aws_json -d 71851 -Pudev1-from-prod-feb26.cu0erabygff3.us-west-2.rds.amazonaws.com -N dealthing -U root -W imaof333 -X 16.375 -Y 12.62 -D 1 -sAWS < ../tmp/71851/71851_grover_enumerated_pdf_1589507168.json  -n1> bbb) >& ttt_71851_parse

Alma 11
(../bin/parse_aws_json -d 71852 -Pudev1-from-prod-feb26.cu0erabygff3.us-west-2.rds.amazonaws.com -N dealthing -U root -W imaof333 -X 16.375 -Y 12.62 -D 1 -sAWS < ../tmp/71852/11*.json  -n1> bbb) >& ttt

Alma 13
(../bin/parse_aws_json -d 71853 -Pudev1-from-prod-feb26.cu0erabygff3.us-west-2.rds.amazonaws.com -N dealthing -U root -W imaof333 -X 16.375 -Y 12.62 -D 1 -sAWS < ../tmp/71853/13*.json  -n1> bbb) >& ttt

CARR/PAIN MANAGEMENT
(../bin/parse_aws_json -d 58275 -Pudev1-from-prod-feb26.cu0erabygff3.us-west-2.rds.amazonaws.com -N dealthing -U root -W imaof333 -X 16.375 -Y 12.62 -x0.0616 -y0.079 -D 1 -sAWS -F1 < ../tmp/58275/2018-03-27\ _dvance\ _ain\ _anagement\ _nstitute\ _ease_pdf_1590967990.json  -n1> bbb) >& ttt

UDEV2:
CALLING: (../bin/parse_aws_json -d 97082 -Pudev2-from-prod-aug12-uribranch2.cu0erabygff3.us-west-2.rds.amazonaws.com -N dealthing -U root -W imaof333 -D 1 -sAWS -n1 -F0 -S../../..) >& www20 

UDEV3
(../bin/parse_aws_json -d 117564 -Pudev3-from-prod-jul27.cu0erabygff3.us-west-2.rds.amazonaws.com -N dealthing -U root -W imaof333 -D 1 -sAWS -n1 -F0 -S../../.. -L FRE) >& www64 // second round

*/

#include <stdlib.h>
#include <mysql.h>
#include <ctype.h>

#define MIN(a,b) ((a<b)?a:b)
#define MAX(a,b) ((a>b)?a:b)  

char *str_replace(char *haystack, size_t haystacksize,
		  const char *oldneedle, const char *newneedle);

  
MYSQL *conn;
MYSQL_RES *sql_res;
MYSQL_ROW sql_row;

int lex_return;
int find_token(char *text);

char *prog;
int debug;
char *db_IP, *db_name, *db_pwd, *db_user_name; // for DB config
int doc_id;
int first_pass = 1;
char *source_prog; 
int do_sql;
 char *my_language;

int corner_no = 0;
int line_ids_no = 0;
char obj_text[50000];
 
#define MAX_WORD 1000000
#define MAX_WORD_BATCH 10000
int in_word_block = 0;
int word_count = -1;


#define MAX_LINE 100000 
int in_line_block = 0;   
int line_count = -1;


float X_abbyy_factor = 16.375;
float Y_abbyy_factor = 12.62;
float aws_factor = 10000;
char *dt_path;

int has_sterling_character = 0;

#define X1  0
#define X2  2
 
#define Y1  0
#define Y2  3
 
#define MAX_VOTED 100000
struct Voted_OCR {
  int sn_in_doc;
  char *aws_text;
  float aws_confidence;
  char *gcs_text;
  float gcs_confidence;
  char *voted_text;
  float voted_confidence;
  int discrepancy;
  int split_token;
  int inserted_after_sn_in_doc;
  int order_after_sn_in_doc;

  int my_x[4]; // calculated
  int my_y[4];  

} voted_array[MAX_VOTED];
int voted_count = 0;

#define MAX_WORD_PER_LINE 50
struct Obj_Struct { // AWS original input array
  float in_x[4];  // x[0] is x1, x[2] is x2
  float in_y[4];  // y[0] is y1, y[3] is y2
  float confidence;
  int page;
  int corner_no;
  char *obj_text;

  int my_x[4]; // calculated
  int my_y[4];  

  int my_line_in_doc; // 
  int my_line_in_page; // 
  int line_in_doc;
  int line_in_page;   
  int sn_in_line; // only for words
  int sn_in_page; // only for words  
  int sn_in_doc; // only for words
  int old_sn_in_doc; // only for new_words, back-pointer
  int no_of_words; // only for lines
  int first_word, last_word; // only for lines
  int has_space_before;  // only for new_words: 0 space, 1 no space
  int order_after_sn_in_doc;
  int inserted_after_sn_in_doc;  
  int split_token;

  int token_no;  // for line only, the no of words included in this line
  char **token_array;  // for line only, the array of words included in this line
  char *ids_array[MAX_WORD_PER_LINE];  // for line only, the array of word_ids included in this line
  int ids_no; // for line only, the number of word_ids included in this line
  int last_ids_taken; // an index so we don't 
  char *id;  // for word only  
  int essential;
  struct Obj_Struct *next; // ptr to items added  by GCS not existing in AWS
}  word_array[MAX_WORD], new_word_array[MAX_WORD], line_array[MAX_LINE]
     , my_word_array[MAX_WORD] // for testing
     , my_line_array[MAX_LINE]; // for testing 
int new_word_count;
int my_word_count; 

int word_array_index[MAX_WORD]; // an index from sn_in_doc to ii

int is_word(char *text) { // 1 -- word only; 2 -- word plus punc; 3 -- word punc junk; 4 -- punc word punc; 5 -- mixed word plus punc
  int ret = 1;
  char before[400], after[400], word[400], junk[400];
  before[0] = after[0] = word[0] = '\0';
  int nn = sscanf(text,"%[A-Za-z]%[^a-zA-Z0-9]%[a-zA-Z])", word, after, junk);
  fprintf(stderr,"RRR (is_word): nn=%d:\n", nn);
  int mm = 0;
  if (nn == 1) {
    ret = 1; // word only
  } else if (nn == 2 && strlen(after) < 4) {
    ret = 2; // word").
  } else {
    mm = sscanf(text,"%[^a-zA-Z0-9]%[A-Za-z]%[^a-zA-Z0-9]%[A-Za-z])", before, word, after, junk);
    if (mm == 3 && strlen(after) < 4 && strlen(before) < 4) {
      ret = 3;
    } else if (mm == 2 && strlen(before) < 4) {
      ret = 4;
    } else {
      ret = 0;
    }
  }
  fprintf(stderr,"MMM (is_word): m=%d: ret=%d: b=%4s: a=%4s: w=%20s:  text=%s:\n", mm, ret, before, after, word, text);
  return ret;
} // is_word
 
char *un_escape(char *text) {
   int ii, jj;
   static char bext[5000];
   if (text) {
     for (ii = 0, jj = 0; ii < strlen(text); ii++) {
       if (text[ii] == '\\' && (text[ii+1] == '\'' || text[ii+1] == '\"' ) && text[ii] != '^') {
	 ;
       } else {
	 bext[jj++] = text[ii];
       }
     }
     bext[jj++] = '\0';
   } else {
     strcpy(bext,"-");
   }
   fprintf(stderr,"UNESCAPE:%s:%s:\n",text,bext);
   return strdup(bext);
}



 
int is_roman(char *text) { // (iix) or (aaa)
   int ret = 1;
   int ii;
   int len = strlen(text);
   for (ii = 0; ii < len; ii++) {
     if (isalpha(text[ii]) != 0 && text[ii] != 'i' && text[ii] != 'I' && text[ii] != 'V' && text[ii] != 'v' && text[ii] != 'x' && text[ii] != 'X' && text[ii] != 'L' && text[ii] != 'l') {
       ret = 0;
     }
   }
   if (ret == 0 &&
       ((len == 4 && text[1] == text[2])
	|| len == 5 && text[1] == text[2] && text[3] == text[2])) {
     ret = 1;
   }
   return ret;
 }

// new, 1 -- this is a new word peeled off from the stem,  old, 0 -- this is the original word
// front, 1 -- peeling is at the front,  back, -1 -- peeling is at the end,  0 - no peeling happened
 #define OLD 0
 #define NEW 1
 #define BACK -1
 #define FRONT 1
 #define NONE 0

int tokenize_and_copy_word_into_new_array(int lev, int ii, char *text, int *disp_in_line, int *disp_in_page, int *disp_in_doc, int old_new, int none_back_front, struct Obj_Struct *ptr) {
  int test_sterling = 1;
  if (text == NULL) text = "NAV";
  static int jj = 0;
  int len = strlen(text);
  int ret = is_word(text); //  1 -- word only; 2 -- word plus punc; 3 -- word punc junk; 4 -- punc word punc 
  ret = (find_token(text) == 1) ? -1 : ret; // trying flex
  int ll;
  int my_len = strlen(word_array[ii].obj_text);
  char txt1[100],txt2[200],txt3[100],txt4[100];
  
  for (ll = 0; ll < lev; ll++) {
    fprintf(stderr,"   ");
  }
  fprintf(stderr,"IIIi: ii=%d: page=%d: ret=%d: lev=%d: len=%d: t=%10s:%10s:\n", ii, word_array[ii].page, ret, lev, my_len, text, word_array[ii].obj_text);  

  if (strlen(text) > 3
      && (text[len-1] == ',' || text[len-1] == ';' || text[len-1] == '.' || text[len-1] == ':')) { // ends with a "," or; or. or:
    fprintf(stderr,"---UUU-1:text=%s:\n",text);
    char *text1 = strdup(text);
    text1[len-1] = '\0';
    tokenize_and_copy_word_into_new_array(lev+1, ii, text1, disp_in_line, disp_in_page, disp_in_doc, OLD, BACK, ptr);
    (*disp_in_line)++;    (*disp_in_page)++;    (*disp_in_doc)++;
    tokenize_and_copy_word_into_new_array(lev+1, ii, text+len-1/*","*/, disp_in_line, disp_in_page, disp_in_doc, NEW, BACK, ptr);    

  } else if (ret > -1 && len > 1 && text[len-1] == ',') { // ends with a ","
    fprintf(stderr,"---UUU0:text=%s:\n",text);
    char *text1 = strdup(text);
    text1[len-1] = '\0';
    tokenize_and_copy_word_into_new_array(lev+1, ii, text1, disp_in_line, disp_in_page, disp_in_doc, OLD, BACK, ptr);
    (*disp_in_line)++;    (*disp_in_page)++;    (*disp_in_doc)++;
    tokenize_and_copy_word_into_new_array(lev+1, ii, ",", disp_in_line, disp_in_page, disp_in_doc, NEW, BACK, ptr);    

  } else if (ret > -1 && len > 1 && text[len-1] == '\"') { // ends with a "\""
    fprintf(stderr,"---UUU1:text=%s:\n",text);    
    char *text1 = strdup(text);
    text1[len-1] = '\0';
    tokenize_and_copy_word_into_new_array(lev+1, ii, text1, disp_in_line, disp_in_page, disp_in_doc, OLD, BACK, ptr);
    (*disp_in_line)++;    (*disp_in_page)++;    (*disp_in_doc)++;
    tokenize_and_copy_word_into_new_array(lev+1, ii, "\"", disp_in_line, disp_in_page, disp_in_doc, NEW, BACK, ptr);    

  } else if (strcmp(my_language,"FRE") == 0
	     && (strstr(text,"\\u00e9") != NULL || strstr(text,"\\u00c9") != NULL

		 || strstr(text,"\\u00e8") != NULL || strstr(text,"\\u00c8") != NULL
		 || strstr(text,"\\u00ea") != NULL || strstr(text,"\\u00ca") != NULL

		 || strstr(text,"\\u00e0") != NULL	|| strstr(text,"\\u00c0") != NULL
		 || strstr(text,"\\u00e1") != NULL	|| strstr(text,"\\u00c1") != NULL
		 || strstr(text,"\\u00e2") != NULL	|| strstr(text,"\\u00c2") != NULL
		 || strstr(text,"\\u00c3") != NULL
		 || strstr(text,"\\u00ee") != NULL	|| strstr(text,"\\u00ce") != NULL		 
		 || strstr(text,"\\u00db") != NULL	|| strstr(text,"\\u00fb") != NULL
		 || strstr(text,"\\u00c7") != NULL	|| strstr(text,"\\u00e7") != NULL  // cedilla
		 || strstr(text,"\\u00b0") != NULL     // no
		 || strstr(text,"\\u00b2") != NULL     // sqr spurscript 2
		 || strstr(text,"\\u20ac") != NULL      || strstr(text,"\\u00a3") != NULL     // euro, sterling
		 || strstr(text,"\\u20a2") != NULL      || strstr(text,"\\u00a5") != NULL     // cent, yen		 
		 || strstr(text,"\\u00f4") != NULL	|| strstr(text,"\\u00d4") != NULL

		 || strstr(text,"\\u00eb") != NULL	|| strstr(text,"\\u00ef") != NULL  // umlaut
		 || strstr(text,"\\u00f6") != NULL	|| strstr(text,"\\u00e4") != NULL  // umlaut
		 || strstr(text,"\\u00fc") != NULL	|| strstr(text,"\\u00ff") != NULL  // umlaut
		 || strstr(text,"\\u00cf") != NULL     // umlaut 		 

		 )
	     ) { 

#define BUF 300
    char eee[2];
    char *final_text;
    eee[1] = '\0';
    int tt = 0;
    if (tt==0) {
      eee[0] = 234; char *retval2 = str_replace(text, BUF, "\\u00ea", eee); // ê   // URL: https://graphemica.com/ê
      eee[0] = 202; char *retval21 = str_replace(retval2, BUF, "\\u00ca", eee); // Ê

      eee[0] = 235; char *retval22 = str_replace(retval21, BUF, "\\u00eb", eee); // e umlaut, diaeresis
      eee[0] = 239; char *retval23 = str_replace(retval22, BUF, "\\u00ef", eee); // i umlaut
      eee[0] = 246; char *retval24 = str_replace(retval23, BUF, "\\u00f6", eee); // o umlaut
      eee[0] = 228; char *retval25 = str_replace(retval24, BUF, "\\u00e4", eee); // a umlaut      
      eee[0] = 252; char *retval26 = str_replace(retval25, BUF, "\\u00fc", eee); // u umlaut
      eee[0] = 255; char *retval27 = str_replace(retval26, BUF, "\\u00ff", eee); // y umlaut
      eee[0] = 239; char *retval28 = str_replace(retval27, BUF, "\\u00cf", eee); // i (again?) umlaut            
      
      eee[0] = 232; char *retval3 = str_replace(retval28, BUF, "\\u00e8", eee); // è
      eee[0] = 201; char *retval31 = str_replace(retval3, BUF, "\\u00c8", eee); // É // URL: https://graphemica.com/É
      eee[0] = 233; char *retval4 = str_replace(retval31, BUF, "\\u00e9", eee); // é // URL: https://graphemica.com/é
      eee[0] = 200; char *retval41 = str_replace(retval4, BUF, "\\u00c9", eee); // È    
      eee[0] = 224; char *retval5 = str_replace(retval41, BUF, "\\u00e0", eee); // à
      eee[0] = 192; char *retval51 = str_replace(retval5, BUF, "\\u00c0", eee); // À    
      eee[0] = 225; char *retval6 = str_replace(retval51, BUF, "\\u00e1", eee); // á
      eee[0] = 193; char *retval61 = str_replace(retval6, BUF, "\\u00c1", eee); // Á   
      eee[0] = 226; char *retval7 = str_replace(retval61, BUF, "\\u00e2", eee); // â
      eee[0] = 194; char *retval71 = str_replace(retval7, BUF, "\\u00c2", eee); // Â
      eee[0] = 212; char *retval72 = str_replace(retval71, BUF, "\\u00d4", eee); // O^ //circumflex
      eee[0] = 244; char *retval73 = str_replace(retval72, BUF, "\\u00f4", eee); // o^  //circumflex    
      eee[0] = 196; char *retval74 = str_replace(retval73, BUF, "\\u00c3", eee); // Ã
      eee[0] = 163; char *retval78 = str_replace(retval74, BUF, "\\u00a3", eee); // sterling
      eee[0] = 162; char *retval79 = str_replace(retval78, BUF, "\\u20a2", eee); // cent            
      eee[0] = 165; char *retval80 = str_replace(retval79, BUF, "\\u00a5", eee); // yen
      eee[0] = 128; char *retval81 = str_replace(retval80, BUF, "\\u20ac", eee); // euro            
      eee[0] = 238; char *retval82 = str_replace(retval81, BUF, "\\u00ee", eee); // i^
      eee[0] = 206; char *retval83 = str_replace(retval82, BUF, "\\u00ce", eee); // I^      
      eee[0] = 251; char *retval84 = str_replace(retval83, BUF, "\\u00fb", eee); // u^
      eee[0] = 198; char *retval85 = str_replace(retval84, BUF, "\\u00??", eee); // AE  "ligature"
      eee[0] = 199; char *retval86 = str_replace(retval85, BUF, "\\u00c7", eee); // C cedilla
      eee[0] = 231; char *retval87 = str_replace(retval86, BUF, "\\u00e7", eee); // c cedilla
      eee[0] = 230; char *retval88 = str_replace(retval87, BUF, "\\u00??", eee); // ae "ligature"
      eee[0] = 156; char *retval89 = str_replace(retval88, BUF, "\\u00??", eee); // oe "ligature"
      eee[0] = 50; char *retval90 = str_replace(retval89, BUF, "\\u00b2", eee); // 406,34 m^2 ??? PLACEHOLDER
      eee[0] = 50; char *retval91 = str_replace(retval90, BUF, "\\u00e8", eee); // 2? me  ??? PLACEHOLDER
      fprintf(stderr,"SSS0:%s:%s:\n",retval90,retval91);
      eee[0] = 248; char *retval92 = str_replace(retval91, BUF, "\\u00b0", eee); // No 39 // degree
      fprintf(stderr,"SSS1:%s:%s:\n",retval91,retval92);
      eee[0] = 219; final_text = str_replace(retval92, BUF, "\\u00db", eee); // Û 
      fprintf(stderr,"---UUU12:text=%s:   ot=:%s:\n",text, final_text);
    }
    if (tt==1) {
      eee[0] = 233; final_text = str_replace(text, BUF, "\\u00e9", eee); // é    // TESTING, take out
      fprintf(stderr,"---UUU13: 4=%s:   91=:%s:\n",text, final_text);
    }

    /* for more conversions see URL: http://accentcodes.com/  */
    fprintf(stderr,"TEXT=%s:  FINAL_TEXT=%s:\n", text, final_text);
    if (lev < 5) { // prevent infinite loop
      tokenize_and_copy_word_into_new_array(lev+1, ii, final_text, disp_in_line, disp_in_page, disp_in_doc, OLD, BACK, ptr);
    }
  } else if (2 == sscanf(text,"(\\u00a3%[0-9,.]%[)]",txt1,txt4)){ // (^34,500.00)
    fprintf(stderr,"---UUU4:text=%s:\n",text);
    fprintf(stderr,"VVV0:%s:  :%s:%s:\n",text,txt1,txt4);
    char hhh[4];
    if (test_sterling) {
      has_sterling_character = 1;
      strcpy(hhh,"£");
    } else {
      strcpy(hhh,"$");   // UZ TESTING
    }
    // STERLING
    //sprintf(txt2,"(%s%s)", hhh+1,txt1);
    sprintf(txt2,"(%s%s)", hhh,txt1);    

    tokenize_and_copy_word_into_new_array(lev+1, ii, txt2, disp_in_line, disp_in_page, disp_in_doc, NEW, FRONT, ptr);



  } else if (1 == sscanf(text,"\\u00a3%[0-9,.]",txt1)){ // ^47,600.00
    fprintf(stderr,"---UUU6:text=%s:\n",text);
    fprintf(stderr,"VVV6:%s:  :%s:\n",text,txt1);
    char hhh[4];
    if (test_sterling) {
      has_sterling_character = 1;
      strcpy(hhh,"£");
    } else {
      strcpy(hhh,"$");   // UZ TESTING
    }
    // STERLING
    //sprintf(txt2,"(%s%s)", hhh+1,txt1);
    sprintf(txt2,"%s%s", hhh,txt1);    

    tokenize_and_copy_word_into_new_array(lev+1, ii, txt2, disp_in_line, disp_in_page, disp_in_doc, NEW, FRONT, ptr);


    
  } else if (ret == 1 && len > 1 && text[len-1] == '\'') { // ends with a "\'", but not 3'' (inch)
    fprintf(stderr,"---UUU3:text=%s:\n",text);
    char *text1 = strdup(text);
    text1[len-1] = '\0';
    tokenize_and_copy_word_into_new_array(lev+1, ii, text1, disp_in_line, disp_in_page, disp_in_doc, OLD, BACK, ptr);
    (*disp_in_line)++;    (*disp_in_page)++;    (*disp_in_doc)++;    
    tokenize_and_copy_word_into_new_array(lev+1, ii, "\'", disp_in_line, disp_in_page, disp_in_doc, NEW, BACK, ptr);    

  } else if (ret > -1 && len > 2 && text[len-1] == '.'
	     &&  (text[len-2] == ')' || text[len-2] == ']')) { // ends with ")." or "]."
    fprintf(stderr,"---UUU4:text=%s:\n",text);
    char *text1 = strdup(text);
    text1[len-1] = '\0';
    tokenize_and_copy_word_into_new_array(lev+1, ii, text1, disp_in_line, disp_in_page, disp_in_doc, OLD, BACK, ptr);
    (*disp_in_line)++;    (*disp_in_page)++;    (*disp_in_doc)++;    
    tokenize_and_copy_word_into_new_array(lev+1, ii, ".", disp_in_line, disp_in_page, disp_in_doc, NEW, BACK, ptr);    


  } else if (ret > -1 && len > 3 // do 2020] or 2020] BUT don't touch 3) or 33) 
	     && (text[len-1] == ')' || text[len-1] == ']')) { // ends with "]" or ")"
    fprintf(stderr,"---UUU40:text=%s:\n",text);
    char *text1 = strdup(text);
    text1[len-1] = '\0';
    tokenize_and_copy_word_into_new_array(lev+1, ii, text1, disp_in_line, disp_in_page, disp_in_doc, OLD, BACK, ptr);
    (*disp_in_line)++;    (*disp_in_page)++;    (*disp_in_doc)++;    
    tokenize_and_copy_word_into_new_array(lev+1, ii, text+len-1, disp_in_line, disp_in_page, disp_in_doc, NEW, BACK, ptr);    



  } else if (ret > -1 && len > 2 && (text[len-1] == 's' || text[len-1] == 'S') && text[len-2] == '\'') { // ends with "'s"
    fprintf(stderr,"---UUU5:text=%s:\n",text);
    char *text2 = strdup(text);
    text2[len-2] = '\0';
    tokenize_and_copy_word_into_new_array(lev+1, ii, text2, disp_in_line, disp_in_page, disp_in_doc, OLD, BACK, ptr);
    char *l2 = text+len-2;
    (*disp_in_line)++;    (*disp_in_page)++;    (*disp_in_doc)++;    
    tokenize_and_copy_word_into_new_array(lev+1, ii, l2, disp_in_line, disp_in_page, disp_in_doc, NEW, BACK, ptr);    

  } else if (len > 1  // ("john BUT NOT 's  FRONTTTTT!!!  
	     && (ret == 3 || ret == 4)
	     && !(text[0] == '\'' && (text[1] == 's' || text[1] == 'S'))
	     && (text[0] == '\'' || text[0] == '\"' || text[0] == '(')) {
    fprintf(stderr,"---UUU6:text=%s:\n",text);
    char text0[2];

    sprintf(text0,"%c",text[0]);
    tokenize_and_copy_word_into_new_array(lev+1, ii, text0, disp_in_line, disp_in_page, disp_in_doc, NEW, FRONT, ptr);
    (*disp_in_line)++;    (*disp_in_page)++;    (*disp_in_doc)++;    
    tokenize_and_copy_word_into_new_array(lev+1, ii, text+1, disp_in_line, disp_in_page, disp_in_doc, OLD, FRONT, ptr);    

  } else if ((ret == 2 || ret == 3) // d)
	     && !(len < 6 && text[len-1] == ')' && text[0] == '(' && is_roman(text))) { // "word),"
    fprintf(stderr,"---UUU7:text=%s: ret=%d:\n",text,ret);
    char *text1 = strdup(text);
    text1[len-1] = '\0';
    tokenize_and_copy_word_into_new_array(lev+1, ii, text1, disp_in_line, disp_in_page, disp_in_doc, OLD, BACK, ptr);
    char *l1 = text+len-1;
    (*disp_in_line)++;    (*disp_in_page)++;    (*disp_in_doc)++;    
    tokenize_and_copy_word_into_new_array(lev+1, ii, l1, disp_in_line, disp_in_page, disp_in_doc, NEW, BACK, ptr);    
  } else if (1 == sscanf(text,"\\u00a3%[0-9,.]%s",txt1,txt4)){ // ^34,500.00
    fprintf(stderr,"---UUU8:text=%s:\n",text);
    fprintf(stderr,"PPP1:%s:  :%s:%s:\n",text,txt1,txt4);
    char hhh[40];
    has_sterling_character = 1;
    strcpy(hhh,"£");
    // STERLING
    //tokenize_and_copy_word_into_new_array(lev+1, ii, hhh+1, disp_in_line, disp_in_page, disp_in_doc, NEW, FRONT, ptr);
    tokenize_and_copy_word_into_new_array(lev+1, ii, hhh, disp_in_line, disp_in_page, disp_in_doc, NEW, FRONT, ptr);    
    (*disp_in_line)++;    (*disp_in_page)++;    (*disp_in_doc)++;    
    tokenize_and_copy_word_into_new_array(lev+1, ii, txt1, disp_in_line, disp_in_page, disp_in_doc, OLD, FRONT, ptr);    

  } else if (3 == sscanf(text,"%[a-zA-Z]%[-]%[a-zA-Z]%s",txt1,txt2,txt3,txt4) && strlen(txt1) > 1 && strlen(txt3) > 1){ // make sure it's twenty-seven and not abc-x1 or a-b
    fprintf(stderr,"---UUU9:text=%s:\n",text);    
    fprintf(stderr,"PPP0:%s:  :%s:%s:%s:%s:\n",text,txt1,txt2,txt3,txt4);
    tokenize_and_copy_word_into_new_array(lev+1, ii, txt1, disp_in_line, disp_in_page, disp_in_doc, NEW, FRONT, ptr);
    (*disp_in_line)++;    (*disp_in_page)++;    (*disp_in_doc)++;    
    tokenize_and_copy_word_into_new_array(lev+1, ii, txt2, disp_in_line, disp_in_page, disp_in_doc, OLD, FRONT, ptr);    
    (*disp_in_line)++;    (*disp_in_page)++;    (*disp_in_doc)++;    
    tokenize_and_copy_word_into_new_array(lev+1, ii, txt3, disp_in_line, disp_in_page, disp_in_doc, OLD, FRONT, ptr);    

  } else if (ret > -1 && len > 2 && text[len-1] == '.' && 2 != sscanf(text,"%[0-9]%[.]", txt1,txt4)) { // ends with "." and not a 12.
    fprintf(stderr,"---UUUA:text=%s:\n",text);
    char *text1 = strdup(text);
    text1[len-1] = '\0';
    tokenize_and_copy_word_into_new_array(lev+1, ii, text1, disp_in_line, disp_in_page, disp_in_doc, OLD, BACK, ptr);
    (*disp_in_line)++;    (*disp_in_page)++;    (*disp_in_doc)++;    
    tokenize_and_copy_word_into_new_array(lev+1, ii, ".", disp_in_line, disp_in_page, disp_in_doc, NEW, BACK, ptr);    

  } else {
    fprintf(stderr,"---UUUX:text=%s:\n",text);
    /***************** REAL COPY (end of recursion) *********************/

    new_word_array[jj].page = word_array[ii].page;
    new_word_array[jj].obj_text = strdup(text);
    new_word_array[jj].essential = 64;    

    int x1_coord = word_array[ii].my_x[0];
    int x2_coord = word_array[ii].my_x[2];
    int x3_coord = (int)x1_coord + (int)(float)((int)(x2_coord - x1_coord) * (int)(my_len -1)) / ((float)my_len);    // for back side
    int x4_coord = (int)x1_coord + (int)(float)((int)(x2_coord - x1_coord) * (int)(1)) / ((float)my_len);    // for front side

    fprintf(stderr,"   JJJi: x1=%d: x2=%d: x3=%d: x4=%d:\n", x1_coord, x2_coord, x3_coord, x4_coord );
    
    int kk;
    for (kk = 0; kk < 4; kk++) {
      if (none_back_front == 0) { // no action, just copy
	new_word_array[jj].my_x[kk] = (ptr == NULL) ? word_array[ii].my_x[kk] : ptr->my_x[kk];
      } else if (none_back_front == -1) { // back side
	if (old_new == 1) { // peel off a NEW piece at the BACK
	  if (kk == 1 || kk == 2) { // no change the BACK coords
	    new_word_array[jj].my_x[kk] = word_array[ii].my_x[kk];
	  } else { // (0 and 3)// do change on FRONT coords
	    new_word_array[jj].my_x[kk] = x3_coord;
	  }
	} else if (old_new == 0) { // chop off from the OLD stem at the BACK
	  if (kk == 1 || kk == 2) { // do change the back coords
	    new_word_array[jj].my_x[kk] = x3_coord;
	  } else { // (0 and 3)// no change on front coords
	    new_word_array[jj].my_x[kk] = word_array[ii].my_x[kk];	  
	  }
	} // back side
      } else if (none_back_front == 1) { // front side
	if (old_new == 1) { // peel off a NEW piece at the FRONT
	  if (kk == 1 || kk == 2) { // DO change the BACK coords
	    new_word_array[jj].my_x[kk] = x4_coord;
	  } else { // (0 and 3)// NO change on FRONT coords
	    new_word_array[jj].my_x[kk] = word_array[ii].my_x[kk];
	  }
	} else if (old_new == 0) { // chop off from the OLD stem at the FRONT
	  if (kk == 1 || kk == 2) { // NO change the back coords
	    new_word_array[jj].my_x[kk] = word_array[ii].my_x[kk];	  
	  } else { // (0 and 3)// DO change on front coords
	    new_word_array[jj].my_x[kk] = x4_coord;
	  }
	}
      } // front-side
      new_word_array[jj].my_y[kk] = (ptr == NULL) ? word_array[ii].my_y[kk] : ptr->my_y[kk];
    } // kk

    new_word_array[jj].my_line_in_page = word_array[ii].my_line_in_page;
    new_word_array[jj].my_line_in_doc = word_array[ii].my_line_in_doc;    
    new_word_array[jj].split_token = word_array[ii].split_token;    
    new_word_array[jj].line_in_page = word_array[ii].line_in_page;
    new_word_array[jj].line_in_doc = word_array[ii].line_in_doc;       
    new_word_array[jj].sn_in_line = word_array[ii].sn_in_line+*disp_in_line;
    new_word_array[jj].sn_in_page = word_array[ii].sn_in_page+*disp_in_page;  
    new_word_array[jj].sn_in_doc = word_array[ii].sn_in_doc+*disp_in_doc;
    new_word_array[jj].old_sn_in_doc = word_array[ii].sn_in_doc;
    new_word_array[jj].confidence = word_array[ii].confidence;
    //    new_word_array[jj].inserted_after_sn_in_doc = inserted;
    new_word_array[jj].inserted_after_sn_in_doc = (ptr==NULL) ? -31 : ptr->inserted_after_sn_in_doc;
    new_word_array[jj].order_after_sn_in_doc = (ptr==NULL) ? -31 : ptr->order_after_sn_in_doc;
    jj++;
    /***************** REAL COPY (end of recursion) *********************/    
  }
  for (ll = 0; ll < lev; ll++) {
    fprintf(stderr,"   ");
  }
  fprintf(stderr,"IIIo: ii=%d: t=%s: ret=%d: lev=%d: len=%d:\n", ii, text, ret, lev, my_len);  
  return jj;
} // tokenize_and_copy_word_into_new_array()
 
#define MAX_PAGE 1000
struct Page_Prop {
  int first_my_line_in_page;
} page_property_array[MAX_PAGE];

int remove_last_comma(char *text) {
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
    return 0;
}
  

char *escape_quote_aws(char *text) {
    static char bext[500];
    int ii, jj;
    for (ii = 0, jj = 0; ii < strlen(text); ii++) {
      if (text[ii] == '\\' && (text[ii+1] == '\"' || text[ii+1] == '\'')) { // since aws already escapes quotes for me SOMETIMES
	bext[jj++] = text[ii];
	bext[jj++] = text[++ii];
      } else if (text[ii] == '\'' || text[ii] == '\"') {
	bext[jj++] = '\\';
	bext[jj++] = text[ii];
      } else if (text[ii] == '\\') {
	bext[jj++] = '\\';
	bext[jj++] = text[ii];
      } else if (text[ii] == '\n') {
	//bext[jj++] = ' ';
      } else {
	bext[jj++] = text[ii];
      }
    }
    bext[jj++] = '\0';
    return bext;
}


int insert_words_into_sql(int word_count, struct Obj_Struct word_array[], int doc_id, int new_old_sql, char *source_program) {
  char query[6000000];
  char *sql_table = (new_old_sql == 1) ? "ocrtoken" : "orig_ocrtoken";

  /********************************************************************/
  sprintf(query,"delete from deals_%s \n\
           where doc_id = %d \n\
             and source_program = '%s' "
	  , sql_table, doc_id, source_program);
  if (debug) fprintf(stderr,"QUERY16=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY16=%s\n",mysql_error(conn));
    exit(1);
  }
  fprintf(stderr,"QUERY16 AWS Records deleted=%d:\n", (int)mysql_affected_rows(conn));    
  /********************************************************************/
  int kk;
  for (kk = 0; kk < (int)(word_count / MAX_WORD_BATCH) +1; kk++) { // split the query into several batches so not to exceed memory
    fprintf(stderr,"KKK=%d:%d:\n",kk,word_count);

    sprintf(query,"insert into deals_%s \n\
           (doc_id, text, sn_in_doc, my_x1, my_x2, my_y1, my_y2, block_in_doc, block_in_page, sn_in_block, sn_in_page, my_line_in_doc, my_line_in_page, page, confidence\n\
              , has_space_before, inserted_after_sn_in_doc, order_after_sn_in_doc, split_token, para, block, reorder_in_doc \n\
              , voted_text, gcs_text, gcs_confidence, discrepancy, source_program)\n\
           values \n", sql_table);

    char buff[5000];

    int ii;

    //for (ii = 0; ii < word_count; ii++) {
    for (ii = kk*MAX_WORD_BATCH; ii < word_count && ii < (kk+1)*MAX_WORD_BATCH; ii++) {    

      int page = word_array[ii].page;
      sprintf(buff,"(%d, '%s', %d,     %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %4.2f, %d, %d, %d, %d, -1, 0, 0, '_', '_', 0.0, 0,  '%s'), \n"
	      , doc_id, escape_quote_aws(word_array[ii].obj_text)
	      , word_array[ii].sn_in_doc

	      , word_array[ii].my_x[0]
	      , word_array[ii].my_x[2]  
	      , word_array[ii].my_y[0]
	      , word_array[ii].my_y[3]

	      , word_array[ii].line_in_doc
	      , word_array[ii].line_in_page 

	      , word_array[ii].sn_in_line
	      , word_array[ii].sn_in_page	    

	      , word_array[ii].my_line_in_doc
	      //, word_array[ii].my_line_in_doc - page_property_array[page].first_my_line_in_page
	      , word_array[ii].my_line_in_page
	      , page
	      , word_array[ii].confidence
	      , word_array[ii].has_space_before	    
	      , word_array[ii].inserted_after_sn_in_doc
	      , word_array[ii].order_after_sn_in_doc	    
	      , word_array[ii].split_token 
	      , source_program
	      );
      strcat(query,buff);


      struct Obj_Struct *ptr = word_array[ii].next;
      while (ptr != NULL) {
	sprintf(buff,"(%d, '%s', %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %4.2f, %d, %d, %d, -1, 0, 0, '_', '_', 0.0, 0, '%s'), \n"
		, doc_id, escape_quote_aws(ptr->obj_text)
		, ptr->my_x[0]
		, ptr->my_x[2]  
		, ptr->my_y[0]
		, ptr->my_y[3]

		, ptr->line_in_doc
		, ptr->line_in_page 

		, ptr->sn_in_line
		, ptr->sn_in_page	    
		, ptr->sn_in_doc

		, ptr->my_line_in_doc
		//, ptr->my_line_in_doc - page_property_array[page].first_my_line_in_page
		, ptr->my_line_in_page
		, page
		, ptr->confidence
		, ptr->has_space_before	    
		, ptr->inserted_after_sn_in_doc
		, ptr->order_after_sn_in_doc	    
		, source_program
		);
	strcat(query,buff);
	fprintf(stderr, "                          BEXT: insert=%d: order=%d: t=%s: \n", ptr->inserted_after_sn_in_doc, ptr->order_after_sn_in_doc, ptr->obj_text);
	ptr = ptr->next;
      }



    }

    remove_last_comma(query);

    if (1 && debug) fprintf(stderr,"QUERY15=%s\n",query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"QUERY15=%s\n",mysql_error(conn));
      exit(1);
    }
    fprintf(stderr,"QUERY15 AWS Records inserted=%d:\n", (int)mysql_affected_rows(conn));      
  }

  sprintf(query,"select id, text\n\
                 from deals_%s\n\
                 where doc_id=%d\n\
                       and text like '%c%s%c' "
	  , sql_table,doc_id, '\%', "£", '\%' );
  if (debug) fprintf(stderr,"QUERY215=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY215=%s\n",mysql_error(conn));
    exit(1);
  }
  sql_res = mysql_store_result(conn);
  int nn = mysql_num_rows(sql_res);
  sql_row = mysql_fetch_row(sql_res); 
  if (sql_row) fprintf(stderr,"ZZZZZZZ:%s:%s:\n",sql_row[0],sql_row[1]);
    
  /********************************************************************/  
  return 0;
} 


int insert_lines_into_sql(int line_count, int doc_id, char *source_program) {
  char query[5000000];
  /********************************************************************/
  sprintf(query,"delete from deals_ocrblock \n\
           where doc_id = %d \n\
             and source_program = '%s' "
	  , doc_id, source_program);
  if (debug) fprintf(stderr,"QUERY16A=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY16A=%s\n",mysql_error(conn));
    exit(1);
  }
  fprintf(stderr,"QUERY16A AWS Lines deleted=%d:\n", (int)mysql_affected_rows(conn));    
  /********************************************************************/

  sprintf(query,"insert into deals_ocrblock \n\
           (doc_id, my_x1, my_x2, my_y1, my_y2, my_line_in_doc, my_line_in_page, block_in_doc, block_in_page, page, first_word, last_word, confidence, text \n\
               , blocktype, x1, x2, y1, y2, para, reorder_in_page, reorder_in_doc, source_program)\n\
           values \n");

  char buff[5000];

  int ii;
  for (ii = 0; ii < line_count; ii++) {
    sprintf(buff,"(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %4.2f, '%s', 'LINE', 0,0,0,0, -1, 0, 0, '%s'), \n"
	    , doc_id
	    , line_array[ii].my_x[0]
	    , line_array[ii].my_x[2]  
	    , line_array[ii].my_y[0]
	    , line_array[ii].my_y[3]

	    , line_array[ii].my_line_in_doc
	    , line_array[ii].my_line_in_page	    
	    
	    , line_array[ii].line_in_doc
	    , line_array[ii].line_in_page 

	    , line_array[ii].page

	    , line_array[ii].first_word
	    , line_array[ii].last_word	    
	    
	    , line_array[ii].confidence
	    , escape_quote_aws(line_array[ii].obj_text)
	    , source_program
	    );
    strcat(query,buff);
  }
  remove_last_comma(query);

  if (debug) fprintf(stderr,"QUERY15A=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY15A=%s\n",mysql_error(conn));
    exit(1);
  }
  fprintf(stderr,"QUERY15A AWS Lines inserted=%d:\n", (int)mysql_affected_rows(conn));      
  /********************************************************************/  
  return 0;
} 



// used to be 40 but it split too many lines
// we need to calculate the page tilt or get it from anazon
// DETERMINE NEW LINE
#define MAX_VAR_IN_ONE_LINE 50

int combine_lines(int word_count, int line_count) { // INTO MY LINE
  int ll;
  int prev_page_no = -1;
  int last_line_y1 = -1;
  int last_line_y2 = -1;  
  int prev_x2 = -1;
  int line_no = -1;
  int page_no = -1;
  for (ll = 0; ll < line_count; ll++) {
    page_no = line_array[ll].page;
    int x1 = line_array[ll].my_x[0];
    int x2 = line_array[ll].my_x[2];    
    
    int y1 = line_array[ll].my_y[0];
    int y2 = line_array[ll].my_y[3];    
    
    if (prev_page_no < page_no) {
      page_property_array[page_no].first_my_line_in_page = line_no+1;
      if (1) fprintf(stderr,"HHH0: ll=%d:%d: pp=%d:%d: first=%d:\n",ll,line_no, page_no, prev_page_no, page_property_array[page_no].first_my_line_in_page);
      line_no++;

    } else if (((y1 + y2) - (last_line_y1 + last_line_y2) >  2 * MAX_VAR_IN_ONE_LINE)
	       || (x1 < line_array[ll-1].my_x[2])
	       ) {

      if (0) fprintf(stderr,"HHH1: ll=%d:%d: pp=%d:%d: yy=%d:%d: xx=%d:%d:\n"
	      ,ll,line_no, page_no, prev_page_no, (y1+y2), (last_line_y1 + last_line_y2),  x1, line_array[ll-1].my_x[2]);      
      line_no++;
    }  else {
      if (0) fprintf(stderr,"  HHH2: ll=%d:%d: pp=%d:%d: yy=%d:%d: xx=%d:%d:\n"
	      ,ll,line_no, page_no, prev_page_no, (y1+y2), (last_line_y1 + last_line_y2),  x1, line_array[ll-1].my_x[2]);      
    }      

    last_line_y1 = y1;
    last_line_y2 = y2;
    prev_x2 = x2;
    prev_page_no = page_no;
    line_array[ll].my_line_in_doc = line_no;
    line_array[ll].my_line_in_page = line_no - page_property_array[page_no].first_my_line_in_page;    
  }
  return 0;
} // combine_lines()



int update_coords_into_aws_arrays(int doc_id, int word_count, int line_count) {
  int ii;
  for (ii = 0; ii < word_count; ii++) {
    int jj;
    for (jj = 0; jj < 4; jj++) {  // BACK_TO_ABY_FACTOR b/c we are using the ABY spdf
      word_array[ii].my_x[jj] = (int)(word_array[ii].in_x[jj] * aws_factor /* * X_back_to_aby_factor*/);
      word_array[ii].my_y[jj] = (int)(word_array[ii].in_y[jj] * aws_factor /* * Y_back_to_aby_factor*/);
    }
  }

  for (ii = 0; ii < line_count; ii++) {
    int jj;
    for (jj = 0; jj < 4; jj++) {
      line_array[ii].my_x[jj] = (int)(line_array[ii].in_x[jj] * aws_factor /* * X_back_to_aby_factor*/);
      line_array[ii].my_y[jj] = (int)(line_array[ii].in_y[jj] * aws_factor /* * Y_back_to_aby_factor*/);
    }
  }
  return 0;
}

 
int first_line_on_page_array[MAX_LINE];

    //    : (word_array[ww].page == line_array[ll].page) ? ((delta < 2 * MAX_VAR_IN_ONE_LINE) ? 1 : 0)

int ll_ahead_of_ww(int ww, int ll) {
  int ret = (
      word_array[ww].page < line_array[ll].page) ? 1
    : (word_array[ww].page == line_array[ll].page && word_array[ww].my_y[Y1] <= line_array[ll].my_y[Y1]) ? 1 
    : 0;
  return ret;
}

int ww_ahead_of_ll(int ww, int ll) {
  int ret = (
      word_array[ww].page > line_array[ll].page) ? 1
    : (word_array[ww].page == line_array[ll].page && word_array[ww].my_y[Y1] >= line_array[ll].my_y[Y1]) ? 1 
    : 0;
  return ret;
}


 
float is_overlap(int ww, int ll, int pp) {
  float ret = 0;
  int LLY1 = line_array[ll].my_y[Y1];
  int LLY2 = line_array[ll].my_y[Y2];
  int WWy1 = word_array[ww].my_y[Y1];
  int WWy2 = word_array[ww].my_y[Y2];

  int LLX1 = line_array[ll].my_x[X1];
  int LLX2 = line_array[ll].my_x[X2];
  int WWx1 = word_array[ww].my_x[X1];
  int WWx2 = word_array[ww].my_x[X2];

  int word_area = (word_array[ww].my_x[X2] - word_array[ww].my_x[X1]) * (word_array[ww].my_y[Y2] - word_array[ww].my_y[Y1]);
  int y_edge =
    (LLY2 >= WWy2 && WWy1 >= LLY1) ? (WWy2 - WWy1) // total inclusion
    : (WWy2 >= LLY2 && LLY2 >= WWy1 && WWy1 >= LLY1) ? (LLY2 - WWy1) // portrusion on right
    : (WWy1 <= LLY1 && LLY1 <= WWy2 && WWy2 <= LLY2) ? (WWy2 - LLY1) // portrusion on left
    : (WWy2 >= LLY2 && LLY1 >= WWy1) ? (LLY2 - LLY1) // total inclusion rare
    : 0;
  int x_edge =
    (LLX2 >= WWx2 && WWx1 >= LLX1) ? (WWx2 - WWx1) // total inclusion
    : (WWx2 >= LLX2 && LLX2 >= WWx1 && WWx1 >= LLX1) ? (LLX2 - WWx1) // portrusion on right
    : (WWx1 <= LLX1 && LLX1 <= WWx2 && WWx2 <= LLX2) ? (WWx2 - LLX1) // portrusion on left    
    : (WWx2 >= LLX2 && LLX1 >= WWx1) ? (LLX2 - LLX1) // total inclusion rare    
    : 0;
  int overlap_area = x_edge * y_edge;
  ret = (float)overlap_area / (float)word_area;
  if (pp) fprintf(stderr,"    BOX:ret=%4.2f: wl=%d:%d:  woarea=%d:%d: xyedge=%d:%d:  x=%d:%d: X=%d:%d: y=%d:%d: Y=%d:%d:\n", ret, ww,ll,    word_area, overlap_area,   x_edge,y_edge, WWx1, WWx2, LLX1, LLX2, WWy1, WWy2, LLY1, LLY2);
  if (pp) fprintf(stderr,"    BOX1: ti=%d:%d:  pr=%d:%d:  pl=%d:%d: ri=%d:%d: \n"
		  , (LLX2 >= WWx2 && WWx1 >= LLX1),(WWx2 - WWx1) // total inclusion
		  ,(WWx2 >= LLX2 && LLX2 >= WWx1 && WWx1 >= LLX1) , (LLX2 - WWx1) // portrusion on right
		  ,(WWx1 <= LLX1 && LLX1 <= WWx2 && WWx2 <= LLX2) , (WWx2 - LLX1) // portrusion on left
		  , (WWx2 >= LLX2 && LLX1 >= WWx1) , (LLX2 - LLX1)); // total inclusion rare    		  
  if (pp) fprintf(stderr,"    BOY1: ti=%d:%d:  pr=%d:%d:  pl=%d:%d: ri=%d:%d: \n"
		  , (LLY2 >= WWy2 && WWy1 >= LLY1),(WWy2 - WWy1) // total inclusion
		  ,(WWy2 >= LLY2 && LLY2 >= WWy1 && WWy1 >= LLY1) , (LLY2 - WWy1) // portrusion on right
		  ,(WWy1 <= LLY1 && LLY1 <= WWy2 && WWy2 <= LLY2) , (WWy2 - LLY1) // portrusion on left
		  , (WWy2 >= LLY2 && LLY1 >= WWy1) , (LLY2 - LLY1)); // total inclusion rare    		  
  if (pp) fprintf(stderr,"    BOY2:\n");
  return ret;
} // is_overlap()


#define MAX_PAGE 1000
int page_array_first_word[MAX_PAGE];
int page_array_first_line[MAX_PAGE]; 

int update_arrays(int ww, int ll, int page, int *prev_page, int *prev_ll, int LLL2) {
      page = word_array[ww].page;
      word_array[ww].sn_in_line++;
      if (page > *prev_page) {
	page_array_first_word[page] = ww;
	page_array_first_line[page] = ll;      
      }
      *prev_page = page;
    
      if (ll > *prev_ll) { // NEW LL
	if (ll > 0) line_array[ll-1].last_word = ww-1;
	line_array[ll].first_word = ww;
      }
    
      word_array[ww].line_in_doc = ll;
      word_array[ww].sn_in_line = ww - line_array[ll].first_word;
      line_array[ll].no_of_words = word_array[ww].sn_in_line;
    
      line_array[ll].line_in_doc = ll;
      line_array[ll].line_in_page = ll - page_array_first_line[page];
      word_array[ww].line_in_page = line_array[ll].line_in_page;
    
      word_array[ww].sn_in_doc = ww;
      word_array[ww].sn_in_page = ww - page_array_first_word[page];
      word_array[ww].my_line_in_doc = line_array[ll].my_line_in_doc;
      word_array[ww].my_line_in_page = line_array[ll].my_line_in_page;      

      static int ii = 0;
      fprintf(stderr,"IIIL: ii=%d: w=%d:%s: l=%d:%d:%s: fw=%d: lw-1=%d: my_line=%d:%d:\n",ii++, ww, word_array[ww].obj_text,   ll, *prev_ll,   line_array[ll].obj_text,  line_array[ll].first_word, line_array[ll-1].last_word, word_array[ww].my_line_in_page, word_array[ww].my_line_in_doc);
      *prev_ll = ll;    
      return 0;
} // update_arrays()




int check_id_include(int ww,int ll) {
  int ii;
  int found = 0;
  fprintf(stderr,"     FINDING_ID: ll=%d: ids_no=%d: lt=%d:   t=%s:%s:\n"
	  , ll, line_array[ll].ids_no, line_array[ll].last_ids_taken
	  , word_array[ww].obj_text, line_array[ll].obj_text);
  for (ii = MAX(0,line_array[ll].last_ids_taken); ii < line_array[ll].ids_no; ii++) {
    fprintf(stderr,"        COMPARE_ID: ll=%d: ii=%d: id=%s: w=%s:%s:\n"
	    , ll, ii, word_array[ww].id, line_array[ll].token_array[ii], word_array[ww].obj_text);
    if (line_array[ll].ids_array[ii]
	&& word_array[ww].id
	&& (strcmp(line_array[ll].ids_array[ii], word_array[ww].id) == 0
	    || strcmp(line_array[ll].token_array[ii], word_array[ww].obj_text) == 0)
	) {
	  
      found = 1;
      fprintf(stderr,"FOUND_ID: ll=%d: ii=%d: id=%s:%s:\n", ll, ii, word_array[ww].id, word_array[ww].obj_text);
      line_array[ll].last_ids_taken = ii+1;
      break;
    }
  }
  return found;
}

char** split_into_tokens(const char* my_input, const char* delim, int* tokenCount) {
    // Allocate array of char pointers
    char* temp = strdup(my_input);
    char** tokens = malloc(50 * sizeof(char*));
    if (!tokens) {
        perror("Failed to allocate memory for tokens");
        return NULL;
    }

    // Temporary copy of input because strtok modifies the string
    if (!temp) {
        perror("Failed to duplicate string");
        return NULL;
    }

    // Count tokens
    int count = 0;
    //fprintf(stderr,"TTT0:%s:\n",temp);
    char *token = strtok(temp, delim);
    fprintf(stderr,"TTT1:%s:%d:\n",token, count);    
    tokens[count++] = strdup(token);
    while (token) {
      //fprintf(stderr,"TTT20:%s:%d:\n",token, count);    	
        token = strtok(NULL, delim);
	if (token) tokens[count++] = strdup(token);
	//fprintf(stderr,"TTT21:%s:%d:\n",token, count);    	
    }
    //fprintf(stderr,"TTT3:%d:\n", count);    	    
    free(temp);
    //fprintf(stderr,"TTT4:%d:\n", count);
    *tokenCount = count;
    return tokens;
}
 
int check_next_id_include(int ww,int in_ll) {
  int ll = in_ll +1;
  int ii;
  int found = 0;
  fprintf(stderr,"     FINDING_NID: ll=%d: ids_no=%d: lt=%d:   t=%s:%s:\n", ll, line_array[ll].ids_no, line_array[ll].last_ids_taken, word_array[ww].obj_text,  line_array[ll].obj_text);
  {
    ii = 0;
    fprintf(stderr,"        COMPARE_NID: ll=%d: ii=%d: id=%s: w=%s:%s:\n"
	    , ll, ii, word_array[ww].id, line_array[ll].token_array[ii], word_array[ww].obj_text);
    if (line_array[ll].ids_array[ii]
	&& word_array[ww].id
	&& (strcmp(line_array[ll].ids_array[ii], word_array[ww].id) == 0
	    || strcmp(line_array[ll].token_array[ii], word_array[ww].obj_text) == 0
	    )) {
      found = 1;
      fprintf(stderr,"FOUND_NID: ll=%d: ii=%d:%s:\n", ll, ii, word_array[ww].obj_text);
      line_array[ll].last_ids_taken = ii+1;
      // break;
    }
  }
  return found;
}
 

// used to be 0.6
#define OVERLAP_THRESHOLD 0.5
#define LOW_OVERLAP_THRESHOLD 0.1
int check_word_line_inclusion(int doc_id, int word_count, int line_count) {
  int ll;
  int prev_ll;
  int prev_page;

  fprintf(stderr, "*****CHECK_NEW_WORD_LINE_INCLUSION w=%d: l=%d:\n", word_count, line_count);

  int no_overlap = 0;
  int ww;
  ll = 0;

  char *lltext = "";
  char *mmtext = strdup(lltext);
  for (ww = 0; ww < word_count; ww++) {
    char *wwtext = word_array[ww].obj_text;

    float overlap = is_overlap(ww, ll, 1);
    float o1 = is_overlap(ww, ll+1, 0);
    float o2 = is_overlap(ww, ll+2, 0);

    int page = word_array[ww].page;
    if (prev_page < page) {
      fprintf(stderr,"\nOL NEW PAGE:%d:\n", page);
    }
    
    int i1 = ll_ahead_of_ww(ww+1,ll);
    char *prev_mm = (mmtext) ? strdup(mmtext) : NULL; // just for printing
    //if (debug) fprintf(stderr,"   MEW WORD: LL=%s:%s:\n", mmtext, wwtext);
    mmtext = (mmtext) ? strstr(mmtext+1,wwtext) : lltext; // in case not found, return the entire new block
    char *next_mm = (mmtext&&wwtext) ? mmtext + strlen(wwtext) : mmtext;
    int my_diff = ((mmtext&&next_mm&&wwtext) ? (next_mm-mmtext-strlen(wwtext)) : 1000);

    fprintf(stderr,"CII: %d:%d:\n",ww,ll);
    int id_include = check_id_include(ww, ll);
    //int id_include = 1;    
    //int next_id_include = check_next_id_include(ww, ll);    
    int next_id_include = (id_include == 0) ? check_next_id_include(ww, ll) : 0;    
    if (debug) fprintf(stderr,"\n\n   NEW WORD: WW=%d:%s: LL=%d:%s: mm=%s: overlap=%4.2f:  o1=%4.2f: o2=%4.2f: prev_mm=%s: next_mm=%s: DIFF=%d: pg=%d: inc=%d:%d:\n"
		       , ww, wwtext,   ll, lltext, mmtext,  overlap,o1,o2, prev_mm, next_mm, my_diff, page, id_include, next_id_include);




    // keep syncing with the current block as long as there is geometrical or linguistic overlap 
    if (id_include == 1
	/* || overlap > OVERLAP_THRESHOLD  // overlap bigger than threshold
	||( overlap > LOW_OVERLAP_THRESHOLD && overlap > o1) // current overlap bigger than overlap with next block
	|| my_diff < 3 */ ) { // the word itself matches (up to a space left or right
      ; // normal in-block progression
      if (debug) fprintf(stderr,"MMM0: ww=%d:%s: ll=%d:%s:%s:: mmtext=%s:\n",ww,word_array[ww].obj_text,ll,line_array[ll].obj_text,lltext, mmtext);
      //update_arrays(ww, ll, page, &prev_page, &prev_ll, 1);
    } else {
      
      if (o2 > OVERLAP_THRESHOLD && next_id_include == 0) { // move TWO blocks up, we got out of sync so get back on the rails
	ll++; ll++; //ww--;
	lltext = line_array[ll].obj_text;
	mmtext = strdup(lltext);
	if (debug) fprintf(stderr,"LLL3:%d:%s:\n",ww,word_array[ww].obj_text);
	//update_arrays(ww,ll,page, &prev_page, &prev_ll, 2);	
      } else if (next_id_include == 1
		 /* || o1 > OVERLAP_THRESHOLD
		    || (o1 > LOW_OVERLAP_THRESHOLD && o1 > o2) */) { // normal move to next block
	ll++; 
	lltext = line_array[ll].obj_text;		
	mmtext = strdup(lltext);
	if (debug) fprintf(stderr,"NNN0:%d:%s:\n",ww,word_array[ww].obj_text);
	//update_arrays(ww,  ll,  page, &prev_page, &prev_ll, 1);	
      } else if (i1 > 0) { // give up on the current word, move to next word IN SAME BLOCK
	if (debug) fprintf(stderr,"LLL1:%d:%s:\n",ww,word_array[ww].obj_text);
	//update_arrays(ww,ll,page, &prev_page, &prev_ll, 1);	
      } else if (i1 == 0) { // BAD word, give up on it!, see 71851 page 63, "ONO o."
	is_overlap(ww, ll+1, 0);
	ll++;
	lltext = line_array[ll].obj_text;
	mmtext = strdup(lltext);
	if (debug) fprintf(stderr,"LLL0: ll=%d: ww=%d:%s:   giving up on bad word!!!\n", ll, ww, word_array[ww].obj_text);
	//update_arrays(ww,ll,page, &prev_page, &prev_ll, 2);
      } else { // the coords of the block are incorrect
	float o3 = is_overlap(ww,ll+3, 0);
	if (debug) fprintf(stderr,"LLL4:%d:%s:\n",ww,word_array[ww].obj_text);	
      }
    }
    //fprintf(stderr,"  LLL221:%d:%d:%s:\n",ww,ll, word_array[ww].obj_text);    
    update_arrays(ww, ll, page, &prev_page, &prev_ll, 1);
    fprintf(stderr,"  RRR222:%d:%d:%s:\n",ww,ll, word_array[ww].obj_text);        
  } // for ww
  return 0;
} // check_word_line_inclusion()

char *remove_spash(char *text) {
   char *bext = text;
   if (text && strlen(text) > 1 && strchr(text,'\\')) {
     bext = text;
   }
   return bext;
 }

int update_coords_in_sql(int doc_id) {
  char query[5000000];
  /***************************  AWS WORD *****************************************/
  sprintf(query,"update deals_ocrtoken \n\
           set \n\
                  my_x1 = x1 * %f \n\
                , my_x2 = x2 * %f \n\
                , my_y1 = y1 * %f \n\
                , my_y2 = y2 * %f \n\
           where doc_id = %d \n\
             and source_program = 'AWS' "
	  , aws_factor, aws_factor,aws_factor, aws_factor, doc_id);
  if (debug) fprintf(stderr,"QUERY17=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY17=%s\n",mysql_error(conn));
    exit(1);
  }
  fprintf(stderr,"QUERY17 AWS Words updated=%d:\n", (int)mysql_affected_rows(conn));  
  /***************************  AWS LINE *****************************************/
  sprintf(query,"update deals_ocrblock \n\
           set \n\
                  my_x1 = x1 * %f \n\
                , my_x2 = x2 * %f \n\
                , my_y1 = y1 * %f \n\
                , my_y2 = y2 * %f \n\
           where doc_id = %d \n\
             and source_program = 'AWS' "
	  , aws_factor, aws_factor,aws_factor, aws_factor, doc_id);
  if (debug) fprintf(stderr,"QUERY18=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY18=%s\n",mysql_error(conn));
    exit(1);
  }
  fprintf(stderr,"QUERY18 AWS Lines updated=%d:\n", (int)mysql_affected_rows(conn));  
  /***************************** ABBYY ***************************************/
  sprintf(query,"update deals_ocrtoken \n\
           set \n\
                  my_x1 = x1 * %f \n\
                , my_x2 = x2 * %f \n\
                , my_y1 = y1 * %f \n\
                , my_y2 = y2 * %f \n\
           where doc_id = %d \n\
             and source_program = '' "
	  , X_abbyy_factor, X_abbyy_factor, Y_abbyy_factor, Y_abbyy_factor, doc_id);
  if (debug) fprintf(stderr,"QUERY19=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY19=%s\n",mysql_error(conn));
    exit(1);
  }

  fprintf(stderr,"QUERY19 ABBYY Records updated=%d:\n", (int)mysql_affected_rows(conn));
  /********************************************************************/  
  return 0;
} // update_coords_in_sql()



%}

ws [\ \n\t]
letter [A-Za-z0-9]

%%
          /*************************  SECOND PASS ************************************/
<s_find_token>\([0-9]\) { // (9)
  lex_return = 1;
}

<s_find_token>\([a-z]\) { // (a)
  lex_return = 1;
}

<s_find_token>[a-z]\. { // x.
  lex_return = 1;
}

<s_find_token>[A-Z]\. { // X.
  lex_return = 1;
}


<s_find_token>\([IVXL]{1,5}\) { // (VII)
  lex_return = 1;
}

<s_find_token>\([ivxl]{1,5}\) { // (vii)
  lex_return = 1;
}


<s_find_token>\([a-z]{2}\) { // (bb)
      if (yytext[2] == yytext[1]) {
	lex_return = 1;
      } else {
	lex_return = 0;
      }
}

<s_find_token>\([A-Z]{2}\) { // (BB)
      if (yytext[2] == yytext[1]) {
	lex_return = 1;
      } else {
	lex_return = 0;
      }
}

<s_find_token>\([a-z]{3}\) { // (bbb)
      if (yytext[2] == yytext[1] && yytext[2] == yytext[3]) {
	lex_return = 1;
      } else {
	lex_return = 0;
      }
}


<s_find_token>\([BBB]{3}\) { // (bbb)
  if (yytext[2] == yytext[1] && yytext[2] == yytext[3]) {
    lex_return = 1;
  } else {
    lex_return = 0;
  }
}




<s_find_token>[\'\"]+[A-Z]((\-)?[0-9])?[\'\"]+ { // 'D' or "E" as in EXHIBIT "E" or Exhibit "E-1"
  lex_return = 1;
}

<s_find_token>[NS][\ ]*[0-9]{1,3}[\'\"]{1,2}[\ ]?[0-9]{1,3}[\'\"]{1,2}[\ ]?[0-9]{1,3}[\'\"]{1,2}[\ ]*[EW]/[ \n\<] {  // N59"57'5''W 
  lex_return = 1;
}


<s_find_token>[NS][\ ]*[0-9]{1,3}[\'\"]{1,2}[\ ]?[0-9]{1,3}[\'\"]{1,2}[\ ]?[0-9]{1,3}[\'\"]{1,2}[\ ]*[EW]/[ \n\<] {  // N59"57'5''W 
  lex_return = 1;
}



<s_find_token>{letter}+\'(s|S|ll|LL|D|d|T|t)/[^a-zA-Z] {  // couldn't
  lex_return = 1;
}


{letter}+\'{letter}+/[^0-9a-zA-Z] { // O'tool
  lex_return = 1;
}

{letter}+\&{letter}+/[^0-9a-zA-Z;] { // O&tool , rule out aaa&lt;bbb
  lex_return = 1;
}


{letter}({letter})?\./[^a-zA-Z0-9] { // Mr. but not Mrs.
  lex_return = 1;
}

[hH][Tt][Tt][Pp]([Ss])?[:\;]['/']+[^\ \<]+ { // url, http://www.abc.com/fff/vv.cc
  lex_return = 1;
}

  
[^\ \n\@\<\>\/\:\n]+\@[^\ \n\@\<\>\:\n]+(\.(com|COM|gov|GOV|[a-zA-Z]{2,3})){1,2}  {  // abc@cde.com
  lex_return = 1;
}


[A-Za-z]+(\.{letter})+(\.)?/[^a-zA-Z0-9] { // A.en, U.S.
  lex_return = 1;
}

[0-9]{1,2}\([a-z]{1,2}\) { // 12(c)
  lex_return = 1;
}

[a-z]\) { // a)
  lex_return = 1;
}


          /***************************FIRST PASS **************************************/


<*>\{[\ \n\t]+\"BlockType\"\: { // all other blocks ignore
  BEGIN 0;
  in_word_block = 0;
  in_line_block = 0;    
}

      /****************************************  WORD ***************************************************/

<*>\{[\ \n\t]+\"BlockType\"\:[\ \t]+\"WORD\"\,  { // capture data only in word blocks
  BEGIN word_block;
  corner_no = 0;
  in_word_block = 1;
  word_count++;
}

<word_block>Confidence {
  BEGIN word_block_confidence;
}

<word_block_confidence>[0-9\.]+/\, {
  word_array[word_count].confidence = atof(yytext);
  BEGIN word_block;
}

<word_block>X {
  BEGIN word_block_X;
}

<word_block_X>[0-9\.]+/\, {
  word_array[word_count].in_x[corner_no] = atof(yytext);
  //fprintf(stderr,"XXX:%d:%d:  :%s:  :%f:%s:\n",word_count, corner_no, word_array[word_count].obj_text, word_array[word_count].in_x[corner_no], yytext);
  BEGIN word_block;
}

<word_block>Y {
  BEGIN word_block_Y;
}

<word_block_Y>[0-9\.]+/[\n\ \t]+\}[\ \n\t]*[\,\]] {
  word_array[word_count].in_y[corner_no] = atof(yytext);
  word_array[word_count].corner_no = corner_no;
  //fprintf(stderr,"YYY:%d:%d:  :%s:  :%f:   word=%s:\n",word_count, corner_no, word_array[word_count].obj_text, word_array[word_count].in_y[corner_no], word_array[word_count].obj_text);  
  corner_no++;
  BEGIN word_block;
}



<word_block>Id\"\: {
  BEGIN word_block_id;
}

<word_block_id>\"[0-9a-z\-]*/\" {
  fprintf(stderr,"WORD ID:%s:\n", yytext+1);
  word_array[word_count].id = strdup(yytext+1);  
  BEGIN word_block;
}


<word_block>Page {
  BEGIN word_block_page;
}

<word_block_page>[0-9]+/{ws}*[\,\}] {
  word_array[word_count].page = atoi(yytext);
  BEGIN word_block;
}

<word_block>Text\"\:[\ \t]+\" {
  BEGIN word_block_text;
  obj_text[0] = '\0';
}

<word_block_text>.  {
  remove_spash(yytext);
  strcat(obj_text,yytext);
}

<word_block_text>\",/\n  {
  word_array[word_count].obj_text = un_escape(obj_text);
  BEGIN word_block;  
}


      /****************************************  PAGE ***************************************************/

<*>\{[\ \n\t]+\"BlockType\"\:[\ \t]+\"PAGE\"\,  {
  if (debug) fprintf(stderr,"READING PAGE:%d:\n",  line_array[line_count].page);
  BEGIN page_block;
}


      /****************************************  LINE ***************************************************/

<*>\{[\ \n\t]+\"BlockType\"\:[\ \t]+\"LINE\"\,  { // capture data only in word blocks
  BEGIN line_block;
  corner_no = 0;
  in_line_block = 1;
  line_array[line_count].line_in_doc = line_count;
  line_count++;
  line_ids_no = 0;
}

<line_block>Confidence {
  BEGIN line_block_confidence;
}

<line_block_confidence>[0-9\.]+/\, {
  line_array[line_count].confidence = atof(yytext);
  BEGIN line_block;
}


<line_block>Ids {
  BEGIN line_block_ids;
}

<line_block_ids>\"[0-9a-z\-]*/\" {
  fprintf(stderr,"IDS: lc=%d: lin=%d: id=%s:\n",line_count, line_ids_no, yytext+1);
  if (line_ids_no < MAX_WORD_PER_LINE) { // don't overflow -- can we have more than 50 words per line?
    line_array[line_count].ids_array[line_ids_no++] = strdup(yytext+1); 
    line_array[line_count].ids_no = line_ids_no;
  }
}

<line_block_ids>\] {
  BEGIN line_block;
}

<line_block>X {
  BEGIN line_block_X;
}

<line_block_X>[0-9\.]+/\, {
  line_array[line_count].in_x[corner_no] = atof(yytext);
  //fprintf(stderr,"XXX:%d:%d:  :%s:  :%f:%s:\n",line_count, corner_no, line_array[line_count].obj_text, line_array[line_count].in_x[corner_no], yytext);
  BEGIN line_block;
}

<line_block>Y {
  BEGIN line_block_Y;
}

<line_block_Y>[0-9\.]+/[\n\ \t]+\}[\ \n\t]*[\,\]] {
  line_array[line_count].in_y[corner_no] = atof(yytext);
  line_array[line_count].corner_no = corner_no;
  //fprintf(stderr,"YYY:%d:%d:  :%s:  :%f:%s:\n",line_count, corner_no, line_array[line_count].obj_text, line_array[line_count].in_y[corner_no], yytext);  
  corner_no++;
  BEGIN line_block;
}

<line_block>Page {
  BEGIN line_block_page;
}

<line_block_page>[0-9]+/{ws}*[\,\}] {
  int page = atoi(yytext);
  line_array[line_count].page = page;
  //if (page > 15) return 0;
  BEGIN line_block;
}

<line_block>Text\"\:[\ \t]+\" {
  BEGIN line_block_text;
  obj_text[0] = '\0';
}

<line_block_text>.  {
  strcat(obj_text,yytext);
}

<line_block_text>\",/\n  {
  static int nn = 0;
  line_array[line_count].obj_text = un_escape(obj_text);

  line_array[line_count].token_array = split_into_tokens(line_array[line_count].obj_text, " \t\n", &nn);
  fprintf(stderr,"TTT5:%d:%s:\n", nn, line_array[line_count].token_array[0]);    	  
  line_array[line_count].token_no = nn;
  //fprintf(stderr,"TTT:%d:%s:\n", nn++, obj_text);
  BEGIN line_block;  
}



<*>(.|\n) ;
%%

int print_words(int word_count, struct Obj_Struct word_array[], int nn) {
  int ii;
  fprintf(stderr,"PRINTING WORDS%d: count=%d:\n", nn,word_count);  
  for (ii = 0; ii < word_count; ii++) {
    fprintf(stderr,"       XWORD%d: ww=%d:%20s: xy0=%4d:%4d:  xy1=%4d:%4d:  xy2=%4d:%4d:  xy3=%4d:%4d: in_line=%d: in_page=%d: in_doc=%d: old_in_doc=%d: l=%d:%d: myline=%d:%d: pg=%d: :%4.2f: sp=%d: split=%d:\n"
	    , nn
	    , ii
	    , word_array[ii].obj_text
	    , (word_array[ii].my_x[0])
	    , (word_array[ii].my_y[0])        
	    , (word_array[ii].my_x[1])
	    , (word_array[ii].my_y[1]) 	    
	    , (word_array[ii].my_x[2])
	    , (word_array[ii].my_y[2]) 	    
	    , (word_array[ii].my_x[3])
	    , (word_array[ii].my_y[3])

	    , word_array[ii].sn_in_line
	    , word_array[ii].sn_in_page
	    , word_array[ii].sn_in_doc
	    , word_array[ii].old_sn_in_doc	    
	    
	    , word_array[ii].line_in_page
	    , word_array[ii].line_in_doc

	    , word_array[ii].my_line_in_page
	    , word_array[ii].my_line_in_doc

	    , word_array[ii].page

	    , word_array[ii].confidence
	    , word_array[ii].has_space_before
	    , word_array[ii].split_token	    
	    );	    
    struct Obj_Struct *ptr = word_array[ii].next;
    while (ptr != NULL) {
      fprintf(stderr, "                          NEXT: insert=%d: order=%d: t=%s: \n", ptr->inserted_after_sn_in_doc, ptr->order_after_sn_in_doc, ptr->obj_text);
      ptr = ptr->next;
    }

  }
  return 0;
}

int print_lines(int line_count) {
  int ii;
  fprintf(stderr,"PRINTING LINES:%d:\n",line_count);    
  for (ii = 0; ii < line_count; ii++) {
    fprintf(stderr,"        LINE:%d:%s: :%4.3f:%4.3f:  :%4.3f:%4.3f:  :%4.3f:%4.3f:  :%4.3f:%4.3f: now=%d: p=%d: conf=%4.2f: ml=%d:%d:\n"
	    , ii, line_array[ii].obj_text
	    , (line_array[ii].in_x[0])
	    , (line_array[ii].in_y[0])        
	    , (line_array[ii].in_x[1])
	    , (line_array[ii].in_y[1]) 	    
	    , (line_array[ii].in_x[2])
	    , (line_array[ii].in_y[2]) 	    
	    , (line_array[ii].in_x[3])
	    , (line_array[ii].in_y[3])
	    , line_array[ii].no_of_words
	    , line_array[ii].page
	    , line_array[ii].confidence
	    , line_array[ii].my_line_in_doc
	    , line_array[ii].my_line_in_page	    
	    );	    
  }
  return 0;
}


int get_params(int argc, char **argv) {

  int get_opt_index;
  int c_getopt;
  prog = argv[0];
  opterr = 0;
  debug = 0;
  while ((c_getopt = getopt (argc, argv, "d:P:N:U:W:D:s:X:Y:Z:n:F:S:L:")) != -1) {
    fprintf(stderr,"GO: %c:%s:\n",c_getopt, optarg);
    switch (c_getopt) {
    case 'd':
      doc_id = atoi(optarg);
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
    case 's':
      source_prog = strdup(optarg);
      break;
    case 'X': // the abbyy factor
      X_abbyy_factor = atof(optarg);
      break;
    case 'Y': // the abbyy factor
      Y_abbyy_factor = atof(optarg);
      break;
    case 'Z': // the abbyy factor
      aws_factor = atof(optarg);
      break;
    case 'n': // do sql
      do_sql = atoi(optarg);
      break;
    case 'L': // language
      my_language = (optarg) ? strdup(optarg) : "ENG";
      break;
    case 'F': // pass 1 (read the json into orig_ocrtoken) OR pass 2 (read alma voting and prepare ocrtoken)
      first_pass = atof(optarg);
      break;
    case 'S': // pass 2
      dt_path = strdup(optarg);
      break;
    } //while
  }

  fprintf (stderr,"%s took: doc_id = %d, debug=%d db_IP =%s, db_name =%s, db_user_name =%s, db_pwd=%s:\n",
	   argv[0], doc_id, debug, db_IP, db_name, db_user_name, db_pwd);

  for (get_opt_index = optind; get_opt_index < argc; get_opt_index++) {
    printf ("Non-option argument %s\n", argv[get_opt_index]);
  }

  if (do_sql) {
    conn = mysql_init(NULL);

    /* now connect to database */
    if (!mysql_real_connect(conn,db_IP,db_user_name,db_pwd,db_name,0,NULL,0)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      exit(1);
    }
  }
  return 0;
}


int copy_array_into_new_array(int word_count) {
  int ii,jj;
  int disp_in_line = 0, disp_in_page =  0, disp_in_doc = 0;
  for (ii = 0, jj = 0; ii < word_count /* && strstr(word_array[ii].obj_text,"More") == NULL*/; ii++) {
    char *text = strdup(word_array[ii].obj_text);
    if (word_array[ii].sn_in_page == 0) disp_in_page = 0;
    if (word_array[ii].sn_in_line == 0) disp_in_line = 0;    
    int new = 0, front = 0, lev = 0;
    jj = tokenize_and_copy_word_into_new_array(lev, ii, text,     &disp_in_line, &disp_in_page, &disp_in_doc, new, front, &(word_array[ii]) );
    
    struct Obj_Struct *ptr = word_array[ii].next; // split token into multiple tokens
    while (ptr != NULL) {
      char *next_text = ptr->obj_text;
      int order_after_sn_in_doc = ptr->order_after_sn_in_doc;
      fprintf(stderr,"RRRRR:%s:\n",next_text);
      disp_in_line++;    disp_in_page++;    disp_in_doc++;
      jj = tokenize_and_copy_word_into_new_array(lev+1, ii, next_text,     &disp_in_line, &disp_in_page, &disp_in_doc, new, front, ptr);

      ptr = ptr->next;
    }
  }
  return jj;
} // copy_array_into_new_array()



#define NO_SPACE 0
#define SPACE 1
int calc_has_space_before(int new_word_count) {
  int ii;
  int last_sn = -1;
  int last_ordered = -1;  
  fprintf(stderr,"NEW_WORD_COUNT=%d:\n",new_word_count);
  for (ii = 0; ii < new_word_count; ii++) {
    int sn = new_word_array[ii].old_sn_in_doc;
    int inserted = new_word_array[ii].inserted_after_sn_in_doc;
    int ordered = new_word_array[ii].order_after_sn_in_doc;
    new_word_array[ii].has_space_before = (sn != last_sn || (ordered > 0 && ordered != last_ordered)) ? SPACE : NO_SPACE; // if the word is an inserted GCS then let's assume there is a space unless it's an inserted split word 
    last_sn = sn;
    last_ordered = ordered;
  }
  return 0;
}


int read_voted_from_sql(int doc_id) {
  char query[5000];
  sprintf(query, "select \n\
                     sn_in_doc_AWS \n\
                     , text_aws, confidence_aws \n\
                     , text_gcs, confidence_gcs \n\
                     , my_voted_text, voted_confidence \n\
                     , discrepancy, split_token \n\
                     , inserted_after_sn_in_doc, order_after_sn_in_doc \n\
                     , my_x1_gcs, my_x2_gcs, my_y1_gcs, my_y2_gcs \n\
                     from deals_alma_aligned_token \n\
                     where (doc_id = %d \n\
                        and (voted_ocr = 'gcs' or split_token > 0)) \n\
                     order by sn_in_doc_AWS asc, inserted_after_sn_in_doc asc, order_after_sn_in_doc asc "
	  ,doc_id);

  if (debug) fprintf(stderr,"QUERY70 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY70 (%s): query=%s:\n",prog,query);
  }
  sql_res = mysql_store_result(conn);
  int nn = mysql_num_rows(sql_res);
  int ii = 0;
  int prev_after = -10;
  int after = -10;
  int prev_order = 0; // Alma's order starts from 1, not from 0

  int order = 0;
  int sn;
  while ( (sql_row = mysql_fetch_row(sql_res) )) {
    voted_array[ii].sn_in_doc = sn = (sql_row[0]) ? atoi(sql_row[0]) : -1.0;
    voted_array[ii].aws_text = (sql_row[1]) ? strdup(sql_row[1]) : NULL;
    voted_array[ii].aws_confidence = (sql_row[2]) ? atof(sql_row[2]) : -1;
    voted_array[ii].gcs_text = (sql_row[3]) ? strdup(sql_row[3]) : NULL;
    voted_array[ii].gcs_confidence = (sql_row[4]) ? atof(sql_row[4]) : -1.0;    
    voted_array[ii].voted_text = (sql_row[5]) ? strdup(sql_row[5]) : NULL;
    voted_array[ii].voted_confidence = atof(sql_row[6]);
    voted_array[ii].discrepancy = (sql_row[7]) ? atoi(sql_row[7]) : -1;
    voted_array[ii].split_token = (sql_row[8]) ? atoi(sql_row[8]) : -1;
    prev_after = after;
    voted_array[ii].inserted_after_sn_in_doc = after = (sql_row[9]) ? ((atoi(sql_row[9]) > 0) ? (atoi(sql_row[9])) : -1) : -1; // for now until Alma sets default to -1 we do not allow 0
    if (prev_after != after) {

      prev_order = 0;
    } else {
      prev_order = order;
    }

    voted_array[ii].order_after_sn_in_doc = order = (sql_row[10]) ? ((atoi(sql_row[10]) > 0) ? (atoi(sql_row[10])) : -1) : -1; // for now until Alma sets default to -1 we do not allow 0


    voted_array[ii].my_x[0] = (sql_row[11]) ? atoi(sql_row[11]) : -1;
    voted_array[ii].my_x[2] = (sql_row[12]) ? atoi(sql_row[12]) : -1;        
    voted_array[ii].my_y[0] = (sql_row[13]) ? atoi(sql_row[13]) : -1;
    voted_array[ii].my_y[3] = (sql_row[14]) ? atoi(sql_row[14]) : -1;        

    fprintf(stderr,"READ0  ii=:%d: text=%s:%s:%s: sn=%d: after=%d:%d: order=%d:%d: \n"
	    , ii
	    , voted_array[ii].gcs_text, voted_array[ii].aws_text, voted_array[ii].voted_text
	    , voted_array[ii].sn_in_doc
	    , prev_after, voted_array[ii].inserted_after_sn_in_doc
	    , prev_order, voted_array[ii].order_after_sn_in_doc
	    );

    if (strchr(voted_array[ii].voted_text,'\\') != NULL
	&& voted_array[ii].aws_text
	&& strlen(voted_array[ii].aws_text) > 0
	&& strchr(voted_array[ii].aws_text,'\\') == NULL) {
      if (strlen(voted_array[ii].voted_text) == 2 && voted_array[ii].voted_text[0] == '\\'  && voted_array[ii].voted_text[1] == '-' ) {
	voted_array[ii].voted_text = "-";
	fprintf(stderr,"READ2  ii=:%d: text=%s:%s:%s: sn=%d: after=%d:%d: order=%d:%d: \n"
		, ii
		, voted_array[ii].gcs_text, voted_array[ii].aws_text, voted_array[ii].voted_text
		, voted_array[ii].sn_in_doc
		, prev_after, voted_array[ii].inserted_after_sn_in_doc
		, prev_order, voted_array[ii].order_after_sn_in_doc
		);
      } else {
	voted_array[ii].voted_text = voted_array[ii].aws_text;
        fprintf(stderr,"READ1  ii=:%d: text=%s:%s:%s: sn=%d: after=%d:%d: order=%d:%d: vt=%c:%c:\n"
	      , ii
	      , voted_array[ii].gcs_text, voted_array[ii].aws_text, voted_array[ii].voted_text
	      , voted_array[ii].sn_in_doc
	      , prev_after, voted_array[ii].inserted_after_sn_in_doc
	      , prev_order, voted_array[ii].order_after_sn_in_doc
	      , voted_array[ii].voted_text[0], voted_array[ii].voted_text[1]	      
	      );
      }
    }
    if (sn == -1 && order != prev_order+1) {
      fprintf(stderr,"ERROR\n");

      prev_order = 0;
      prev_after = -10;
    } else {
      ii++;
    }
  }

  sprintf(query, "select my_voted_text \n\
                     from deals_alma_aligned_token \n\
                     WHERE (doc_id=%d \n\
                           and (voted_ocr='gcs' or split_token > 0)) "
	  ,doc_id);

  if (debug) fprintf(stderr,"QUERY60 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY60 (%s): query=%s:\n",prog,query);
  }
  sql_res = mysql_store_result(conn);
  int discrepancies = mysql_num_rows(sql_res);
  fprintf(stderr,"FOUND  discrepancies=%d: GCS voted values=%d:\n", discrepancies, ii);
  return ii;
} // read_voted_from_sql(int doc_id) 


int apply_voting(int voted_count, int word_count) {
  int ii, kk=0;
  for (ii = 0; ii < voted_count; ii++) {

    int sn = voted_array[ii].sn_in_doc;
    int sn_after = voted_array[ii].inserted_after_sn_in_doc;
    int sn_order = voted_array[ii].order_after_sn_in_doc;
    int split = voted_array[ii].split_token;    
    if (sn > 0 && split == 0) { // simple case -- replace AWS by GCS
      if (debug) fprintf(stderr,"  LLL01: voted_ii=%d: sn=%d: sn_idx=%d:  obj_t=%s: voted_t=%s: \n", ii,  sn, word_array_index[sn],       word_array[word_array_index[sn]].obj_text, voted_array[ii].voted_text);  
      word_array[word_array_index[sn]].obj_text = voted_array[ii].voted_text;
    } else if (split == 1) { // simple case -- replace AWS by GCS
      if (debug) fprintf(stderr,"  LLL02: voted_ii=%d: sn=%d: sn_idx=%d:  obj_t=%s: voted_t=%s: \n", ii,  sn, word_array_index[sn],       word_array[word_array_index[sn]].obj_text, voted_array[ii].voted_text);  
      word_array[word_array_index[sn]].obj_text = voted_array[ii].voted_text;
      word_array[word_array_index[sn]].split_token = split;
    } else if (split == 2) { // simple case -- replace AWS by GCS
      if (debug) fprintf(stderr,"  LLL03: voted_ii=%d: sn=%d: sn_idx=%d:  obj_t=%s: voted_t=%s: \n", ii,  sn, word_array_index[sn],       word_array[word_array_index[sn]].obj_text, voted_array[ii].voted_text);  
      word_array[word_array_index[sn]].obj_text = voted_array[ii].voted_text;
    } else { // no SN meaning it's inserted by GCS
      int deltaP1 = word_array[sn_after].my_y[0]-voted_array[ii].my_y[0];
      int deltaP2 = word_array[sn_after].my_y[3]-voted_array[ii].my_y[3];
      int deltaC1 = word_array[sn_after+1].my_y[0]-voted_array[ii].my_y[0];
      int deltaC2 = word_array[sn_after+1].my_y[3]-voted_array[ii].my_y[3];
      if (debug) fprintf(stderr,"  LLL04: voted_ii=%d: sn=%d: sn_idx=%d:  obj_t=%s: voted_t=%s: yP1=%d:%d: yP2=%d:%d: yC1=%d:%d: yC3=%d:%d: V13=%d:%d:\t\t\t%s:%s:\n"
			 , ii,  sn, word_array_index[sn],       word_array[word_array_index[sn]].obj_text, voted_array[ii].voted_text
			 , word_array[sn_after].my_y[0], word_array[sn_after].my_y[0]-voted_array[ii].my_y[0], word_array[sn_after].my_y[3],  word_array[sn_after].my_y[3]-voted_array[ii].my_y[3]
			 , word_array[sn_after+1].my_y[0], word_array[sn_after+1].my_y[0]-voted_array[ii].my_y[0], word_array[sn_after+1].my_y[3], word_array[sn_after+1].my_y[3]-voted_array[ii].my_y[3]
			 , voted_array[ii].my_y[0]
			 , voted_array[ii].my_y[3]
			 , ((abs(deltaP1) > 200 || abs(deltaP2) > 200) ? "P" : "-")
			 , ((abs(deltaC1) > 200 || abs(deltaC2) > 200) ? "C" : "-")
			 );   
      if (sn_after > 0 && sn_after != 1000000
	  && abs(deltaP1) < 200 && abs(deltaP2) < 200 	  && abs(deltaC1) < 200 && abs(deltaC2) < 200) { // get rid of added GCS words that are grossly mismatched by size with neighbors

	struct Obj_Struct *new_obj = (struct Obj_Struct *)malloc(sizeof(struct Obj_Struct));
	new_obj->next = NULL;
	struct Obj_Struct *ptr = word_array[word_array_index[sn_after]].next;
	struct Obj_Struct *prev_ptr = &(word_array[word_array_index[sn_after]]);

	while (ptr != NULL) {

	  prev_ptr = ptr;

	  ptr = ptr->next;

	  if (debug) fprintf(stderr,"             LLL05: voted_ii=%d:%d: sn=%d:%s:%s: ptr=%s:%s: \n", ii, word_array_index[sn_after],  sn_after, word_array[word_array_index[sn_after]].obj_text, voted_array[ii].voted_text, prev_ptr->obj_text, "a"/*ptr->obj_text*/); 

	}

	prev_ptr->next = new_obj;
	//word_array[word_array_index[sn_after]].next = new_obj;
	new_obj->inserted_after_sn_in_doc = voted_array[ii].inserted_after_sn_in_doc;
	new_obj->order_after_sn_in_doc = voted_array[ii].order_after_sn_in_doc;
	new_obj->split_token = voted_array[ii].split_token;      
	new_obj->obj_text = voted_array[ii].voted_text;
	int kk;

	for (kk = 0; kk < 4; kk++) {
	  new_obj->my_x[kk] = voted_array[ii].my_x[kk];
	  new_obj->my_y[kk] = voted_array[ii].my_y[kk];	
	}

	if (debug) fprintf(stderr,"  LLL06: voted_ii=%d: sn_after=%d: sn_order=%d: sn=%d:%s:%s: yyyy=%d:%d: :%d:%d: :%d:%d:\n"
			   ,  ii,  sn_after, sn_order, word_array_index[sn_after], word_array[word_array_index[sn_after]].obj_text, voted_array[ii].voted_text

			   , word_array[sn_after].my_y[0], word_array[sn_after].my_y[3], word_array[sn_after+1].my_y[0], word_array[sn_after+1].my_y[3], voted_array[ii].my_y[0], voted_array[ii].my_y[3]
			   );

	// NEED TO DETERMINE X Y coords of this new GCS token
      } // if
    } // else

  }

  return kk;
} // apply_voting(int voted_count, int word_count) 



int read_words_from_sql(struct Obj_Struct word_array[], int doc_id)  {
  char query[5000];
  sprintf(query, "select \n\
                     text \n\
                     , my_x1, my_x2, my_y1, my_y2 \n\
                     , sn_in_block, sn_in_page, sn_in_doc \n\
                     , block_in_doc, block_in_page \n\
                     , my_line_in_doc, my_line_in_page \n\
                     , page, confidence \n\
                    /*, inserted_after_sn_in_doc, order_after_sn_in_doc, split_token*/ \n \
                     from deals_orig_ocrtoken \n\
                     where doc_id = %d and source_program = '%s' and sn_in_doc >= 0"
	  ,doc_id, "AWS");

  if (debug) fprintf(stderr,"QUERY80 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY80 (%s): query=%s:\n",prog,query);
  }
  sql_res = mysql_store_result(conn);
  int ii = 0;

  while ( (sql_row = mysql_fetch_row(sql_res) )) {

    word_array[ii].obj_text = strdup(sql_row[0]);

    word_array[ii].my_x[0] = atoi(sql_row[1]);
    word_array[ii].my_x[2] = atoi(sql_row[2]);    
    word_array[ii].my_y[0] = atoi(sql_row[3]);
    word_array[ii].my_y[3] = atoi(sql_row[4]);    

    
    word_array[ii].sn_in_line = atoi(sql_row[5]);
    word_array[ii].sn_in_page = atoi(sql_row[6]);
    word_array[ii].sn_in_doc = atoi(sql_row[7]);
    
    word_array[ii].line_in_doc = atoi(sql_row[8]);
    word_array[ii].line_in_page = atoi(sql_row[9]);

    word_array[ii].my_line_in_doc = atoi(sql_row[10]);
    word_array[ii].my_line_in_page = atoi(sql_row[11]);        

    word_array[ii].page = atoi(sql_row[12]);
    word_array[ii].confidence = atof(sql_row[13]);

    /*
    word_array[ii].inserted_after_sn_in_doc = (sql_row[14]) ? atoi(sql_row[14]) : -1;
    word_array[ii].order_after_sn_in_doc = (sql_row[15]) ? atoi(sql_row[15]) : -1;
    word_array[ii].split_token = (sql_row[16]) ? atoi(sql_row[16]) : -1;
    */

    if (word_array[ii].sn_in_doc >= 0) {
      word_array_index[word_array[ii].sn_in_doc] = ii;
    }

    word_array[ii].next = NULL;
    ii++;
  }
  if (debug) fprintf(stderr,"QUERY80 returned :%d: tokens\n",ii);    
  return ii;
} // read_words_from_sql(struct Obj_Struct word_array[], int doc_id)  


int read_lines_from_sql(struct Obj_Struct line_array[], int doc_id)  {
  char query[5000];
  sprintf(query, "select \n\
                    my_x1, my_x2, my_y1, my_y2 \n\
                    , my_line_in_doc \n\
                    , block_in_doc, block_in_page \n\
                    , page, first_word, last_word \n\
                    , confidence, text \n\
                    , my_line_in_doc \n\
                  from deals_ocrblock \n\
                  where doc_id = %d and source_program = '%s' "
	  ,doc_id, "AWS");

  if (debug) fprintf(stderr,"QUERY90 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY90 (%s): query=%s:\n",prog,query);
  }
  sql_res = mysql_store_result(conn);
  int ii = 0;

  while ( (sql_row = mysql_fetch_row(sql_res) )) {

    line_array[ii].my_x[0] = atoi(sql_row[0]);
    line_array[ii].my_x[2] = atoi(sql_row[1]);
    line_array[ii].my_y[0] = atoi(sql_row[2]);
    line_array[ii].my_y[3] = atoi(sql_row[3]);

    line_array[ii].my_line_in_doc = atoi(sql_row[4]);
	    
    line_array[ii].line_in_doc = atoi(sql_row[5]);
    line_array[ii].line_in_page = atoi(sql_row[6]);

    line_array[ii].page = atoi(sql_row[7]);

    line_array[ii].first_word = atoi(sql_row[8]);
    line_array[ii].last_word = atoi(sql_row[9]);
	    
    line_array[ii].confidence = atof(sql_row[10]);
    line_array[ii].obj_text = strdup(sql_row[11]);
    line_array[ii].my_line_in_doc = atoi(sql_row[12]);    
    ii++;
  }
  if (debug) fprintf(stderr,"QUERY90 returned %d blocks\n",ii);    
  return ii;
} // read_lines_from_sql()

int find_token(char *text) {
  BEGIN s_find_token;
  lex_return = 0;
  yy_scan_string(text);
  if (debug) fprintf(stderr,"DOING yylex0:%s:\n",text);
  yylex(); // changes lex_return

  return lex_return;
}

int main(int ac, char **av) {
  fprintf(stderr,"HHH0\n");
  get_params(ac, av);
  fprintf(stderr,"HHH1\n");

#define BUF 300
  char *retval1, *retval2;
  char message[BUF] = "Your name is $USERNAME,  Your name is $USERNAME.";
  char username[] = "admin";
  char username_toolong[] = "System Administrator";
  fprintf(stderr,"HHH2\n");
  retval2 = str_replace(message, BUF, "$USERNAME", username);
  fprintf(stderr,"HHH3\n");
  fprintf(stderr,"HAYY:%s:%s:\n",retval2, message);

  /************************* READING JSON, WRITING ORIG_OCRTOKEN ***************************/
  if (first_pass == 1) {
    fprintf(stderr,"YYY0: PARSE JSON\n");
    yylex(); // read AWS json
    line_count++; word_count++;
    fprintf(stderr,"YYY1: done lex: lines=%d: words=%d:\n", line_count, word_count);
    update_coords_into_aws_arrays(doc_id, word_count, line_count);
    fprintf(stderr,"YYY2: done update: lines=%d: words=%d:\n", line_count, word_count);
    combine_lines(word_count, line_count);  // determine my_line
    print_lines(line_count);
    fprintf(stderr,"YYY3: sync blocks and tokens: lines=%d: words=%d:\n", line_count, word_count);
    check_word_line_inclusion(doc_id, word_count, line_count); // THE MAIN SEQUENCER
#define ALMA_SQL 0
    fprintf(stderr,"YYY4: insert words into ORIG_OCRTOKEN lines=%d: words=%d:\n", line_count, word_count);    
    insert_words_into_sql(word_count, word_array, doc_id, ALMA_SQL, source_prog); // insert original AWS tokens into ORIG_OCRTOKEN
    fprintf(stderr,"YYY5: insert lines into OCRBLOCK lines=%d:\n", line_count);    
    insert_lines_into_sql(line_count, doc_id, source_prog); 
  }
  /************************* READING ORIG_OCRTOKEN, VOTED, WRITING OCRTOKEN ***************************/
  if (first_pass == 0) {  
    int nn;
    int word_count = read_words_from_sql(word_array, doc_id);
    nn = 0;
    print_words(word_count, word_array, nn);

    int line_count = read_lines_from_sql(line_array, doc_id);
    int voted_count = read_voted_from_sql(doc_id);
    apply_voting(voted_count, word_count); // (1) replace incorrect text; (2) add missing text as *next
  
    int new_word_count = copy_array_into_new_array(word_count); // HERE WE COPY FROM WOR_ARRAY TO NEW WORD ARRAY
    nn = 1;
    print_words(new_word_count, new_word_array, nn);

    fprintf(stderr,"YYY34: sync blocks and tokens: lines=%d: words=%d:\n", line_count, word_count);    
    calc_has_space_before(new_word_count);
    fprintf(stderr,"YYY3: done check_inclusion: lines=%d: words=%d:\n", line_count, word_count);

    nn = 4;
    print_words(new_word_count, new_word_array, nn);

#define NEW_SQL 1
    fprintf(stderr,"YYY35: insert from tokenized and voted into OCRTOKEN lines=%d: words=%d:\n", line_count, word_count);    
    insert_words_into_sql(new_word_count, new_word_array, doc_id, NEW_SQL, source_prog); // insert tokenized AWS into OCRTOKEN

    print_lines(line_count);  
  }

#define COMMAND_LENGTH 1000
  if (has_sterling_character) {
    // Running this command doubles the execution time of the program,
    // so we only run it if there is a £ character to fix
    char command[COMMAND_LENGTH];
    snprintf(
      command,
      COMMAND_LENGTH,
      "cd %s/dealthing/webapp; ../../dealthing-pyenv-current/bin/python manage.py fix_sterling_character --document_id %d",  dt_path,  doc_id
    );
    system(command);
    fprintf(stderr,"HIRSCH COMMAND:%s:\n",command);
  }
  return 0;
}

