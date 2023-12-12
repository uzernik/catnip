#include <mysql.h>
char *prog;
int debug;
int doc_id;
char *db_IP, *db_name, *db_pwd, *db_user_name; // for DB config
int doc_id;

MYSQL *conn;
MYSQL_RES *sql_res;
MYSQL_ROW sql_row;
char query[500000];

char *normalize(char *text, int keep_case);  
int find_first_good_pair();
int create_toc_pairs();
int align_by_toc_toc();
int normalize_headers();
int add_top_seq_to_item_array();
int check_toc_toc_group();

int wrap_LD(char *str1, char *str2); // from levenshtein_functions.c

#define MAX_TOC 2000
struct Pair_Struct {
  int ii;
  int jj;
  int same_page; // how many TOC items on this page
} pair_array[MAX_TOC];
int pair_no;

// the list of aligned pairs of headers between.  These include many one char headers that are junk
// the ones with score bigger than 11 are taken as real headers 
struct NYU_Struct {
  char *nyu_total_merged_header; // the header merged with consecutive clumps
  int nyu_merged; // is it merged already (then it cannot be a good header)
  int nyu_merger_no; // how many little headers in this one
  int nyu_good_header;  // this is TAKEN
  int nyu_len; // length of header (minus front padding)
  int nyu_score; // score of header in isolation (not including merged)
  int nyu_dist; // gap between CURRENT item and NEXT
  int nyu_ptr1; // the position on toc_toc_file
  int nyu_ptr2;  // the position on rest file
  char *nyu_buff;
  int nyu_rest_pid; // the pid of the header in the rest file (aaa5 file)
  int nyu_toc_item_no; 
} nyu_results_array[MAX_TOC];
int nyu_results_no;
int nyu_good_results_no;

#define MAX_PARA 10000
int rest_line2position_array[MAX_PARA]; // for each para find its position
#define MAX_POSITION 200000
int rest_position2line_array[MAX_POSITION]; // for each position find its para, 0s in in-between cells

int toc_toc_line2position_array[MAX_PARA]; // for each para find its position
int toc_toc_position2line_array[MAX_POSITION]; // for each position find its para, 0s in in-between cells


#define MAX_PARA 10000
int rest_line2para_no_array[MAX_PARA]; // for each rest line ("$"), what's it's para_no
int toc_toc_line2item_no_array[MAX_PARA]; // for each rest line ("$"), what's it's item_no

#define TOC_TOC_PATH "toc_toc_name"
#define REST_PATH "rest_name"
#define RESULTS_PATH "results_name"
char toc_toc_name[500];
char rest_name[500];
char results_name[500];

struct Insert_Item_Struct {
  int my_item;
  int my_line;
  int in_my_loc;
  int my_group;
  char *my_text;
  char *title;
  char *header;
  int title_pid;
  int pid;
  int is_special;
}  insert_item_array[MAX_TOC];
int insert_item_no;
