#include <stdbool.h>
#define MAX_LABEL 50000
#define MIN(a,b) (a<b)?a:b
#define MAX(a,b) (a>b)?a:b
int mishka;
int debug;
int pid;
int span_no;
int doc_id; // our current doc_id
int tenant_id;

  char *prog;
  char *db_IP, *db_name, *db_pwd, *db_user_name; // for DB config
  int org_id[2];
  char *source_prog;
  
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

  #define MAX_PID 15000
  int pid_span_array[MAX_PID];
  int pid_last_span_array[MAX_PID]; // the last span in the para


#define MAX_LABEL 50000
  struct Instance {
    int span_no;
    int last_span_no;
    char *label_name;
    char *desig;
    char *field_array[8];
    char *value_array[8];
    int f_tix_array[8];
    int num_tokens_array[8];
    int field_no;
    int label_id;
    char *text;
    char *norm_text;
    int atype;
    char *annot;
    int norm_int;
    bool no_int;
    double norm_float;
    bool no_float;
    char *date_norm; // this is the normalized date, for the case the text is a date. The label_name will be "date2"
    int score; // score given by the extracting program
    char *prog;
    int pid;
    int index; // for split_first_last_name, the index of the parent ZZ
  } instance_array[MAX_LABEL];
  int label_no;

#define MAX_TOKEN 500000  // an index from token to instance
int token_2_instance_array[MAX_TOKEN];
int max_token;

#define MAX_LABEL_TYPE 1000000
struct Label_Type {
    int label_type_id;
    char *label_type_name;
} label_type_array[MAX_LABEL_TYPE];
int label_type_no;


// originally, attributes were associated with labels 
#define MAX_LABEL_ATTRIBUTE 500
struct Label_Attribute {
    int label_id;
    char *attribute_name;
} label_attribute_array[MAX_LABEL_ATTRIBUTE];
int label_attribute_no;

#define MAX_TEMPLATE_LINE 50000
struct Template_Line {
    int template_line_id;
    char *template_line_name;
} template_line_array[MAX_TEMPLATE_LINE];
int template_line_no;



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

#define MAX_TOC 5000
#define MAX_DOC_IN_FOLDER 50
struct Toc_Item { // index as to where is preamble and  where is recitals
  int id;
  int pid;
  int epid;
  int lev;
  int item_id;
  int my_enum;
  char *section;
  char *title;
  char *header;
  char *norm_header; // currently empty
  char *case_norm_header; // currently empty , keep case intact
  int line_no;
  int page_no;
  int token_id;
// for doc_title_type.c
  int folder_id; // for Adam LI/LV
  int doc_id;
  int orig_doc_id;  
  int para_no, level;
  char short_header[150];     // for create_lv_li
  char short_header_underline[150]; //  "lease_amendmnet"
  int special;
  int parent_id;
  int orig_parent_id;
  int end_para_no;
  int orig_level;
  int grp_id;
  int seq_id;
  int indent;
  int center;
  int too_long;
  int retro;
  int is_entire_document;
  char *source_program;
  int deleted;

  // for get_file_name_date_orderl
  char *date;
  int org_id;

} toc_array[MAX_TOC], toc_array_dd[MAX_DOC_IN_FOLDER][MAX_TOC];
int toc_no_array; // stores how many cells we have per each doc_id
int toc_no_array_dd[MAX_DOC_IN_FOLDER]; // stores how many cells we have per each doc_id
int toc_no; // same, more modern
int toc_no_dd[MAX_DOC_IN_FOLDER];

struct Token_Struct {
  char *text;
  int id;
  char *x1, *x2, *y1, *y2;
  int sn;
  int pid;
  int page_no;
  int line_no;
  int center; // center of line
  int left_X; // left margin of line
  int sn_on_line; // is it first (0) second, etc on the line
  /********* for create_toc ********/
  int x1_1000  , x2_1000;
  int y1_1000  , y2_1000;
  int loc;
} token_array[MAX_TOKEN];
int token_no;

#define MAX_LINE 71
#define MAX_PAGE 400



struct Line {
  int line_no;
  int page_no;
  int no_of_words;
  int no_of_first_cap_words;
  int no_of_all_cap_words;
  int left_X; // coordinate
  int right_X; // coordinate
  int center; // the center coordinate
  int left_word_is_exhibit; // does the word Exhibit appear first in this line
  int found_center_neighbors; // how many close neighbors this center has
} page_line_properties_array[MAX_PAGE][MAX_LINE];
int page_no;

struct Page {
  int no_of_lines;
  int pid;
  int no_of_points;  // "Tenant's Name:"
  int sig_block_points; // "by:", "its:", "name:", "date:", "witness", "title:"
} page_line_array[MAX_PAGE];




int delete_old_from_sql(MYSQL *conn, int doc_id, char *source_prog, char *log_file_name, int debug);
int delete_old_from_sql__only_by_source_program(MYSQL *conn, int doc_id, char *source_prog, char *log_file_name, int debug, int only_by_source_program);
int enter_into_sql(MYSQL *conn, int doc_id, int label_no, int generate_lv_for_labeling);
int stick_dont_delete(MYSQL *conn, int doc_id, char *source_prog);
int read_label_type_array();
int read_label_attribute_array();
int get_par_tok_from_sql(MYSQL *conn, int doc_id);
int read_tokens_into_array(MYSQL *conn, int doc_id);
int print_tokens(int token_no);
int get_toc_item_from_sql(MYSQL *conn, int doc_id);
int print_instance_array(int label_no);
int get_label_type_id(char *my_label);
char *my_escape(char *zbuff);
int sort_array_by_word();
char *rem_tag_only(char *my_text);
int insert_instance_into_array(char *my_text, char *label_name, int first_sp);
int print_instance_array(int label_no);
char *last_strstr(const char *haystack, const char *needle);
int no_of_ws(char *text);
int my_import_load(MYSQL *conn, int doc_id, int org_id[]); // calls the various load functions required by my_import
int get_token_array(int doc_id);
char *under_score2camelBack(char *text);
int get_or_insert_label_type_id(char *name, char *type, char *source_prog);
int enter_instance_into_sql(MYSQL *conn, int doc_id, int label_no, int generate_lv_for_labeling, char *type, char *source_prog);

int insert_double_instance_into_array_with_atype(char *my_text, char *label_name, int span_no, int atype, int last_span_no);
int insert_instance_into_array_with_atype_and_norm_val(char *my_text, char *label_name, int span_no, int atype, char *val);

int get_or_create_label_type_id(char *name, char *type, char *source_prog);
void remove_last_comma(char text[]);
char *get_doc_ocr_engine(int doc_id);
int get_toc_item_from_sql1(MYSQL *conn, int did);
void copy_little_foot(char *pp);
int find_fname1(char *text);
int print_toc_array();
int insert_instance_into_array_and_length(char *my_text, char *label_name, int span_no, int length);
int insert_instance_into_array_with_index(char *my_text, char *label_name, int span_no, int index);
int insert_instance_into_array_with_atype(char *my_text, char *label_name, int span_no, int atype);
