#define MAX_WORD 1000000 
#define MAX_BLOCK 100000
#define MAX_PAGE 300
#define MAX_CLUSTERS 400
#define MAX_BLOCKS_IN_CLUSTER 500
#define MAX_BLOCK_PER_LINE 10
#define MAX_LINE_PER_PAGE 200
#define MAX_GAP_PER_PAGE 400

#define MIN(a,b) (a<b)?a:b
#define MAX(a,b) (a>b)?a:b


#define MAX_BLOCK_SN 50000
#define MAX_GAPS_PER_BLOCK  10
#define MIN_GAP_THRESHOLD 100
#define GAP_TOP_THRESHOLD 20000


#define RENTAL_DESIGNATION 5
#define ENUM_DESIGNATION 4
#define UNTOUCHED_DESIGNATION 3
#define HEADER_DESIGNATION 2
#define FOOTER_DESIGNATION 1

#define MAX_PARA 100000

// update pages into sql in groups of 5
#define DELTA_PP 5

// above this ratio, the page is considered TOC
#define MAX_TOC_RATIO 0.5

// below 0.7 DC GROUP is not considered full-page
#define MIN_DC_LINE_RATIO 0.7

int DELTA_VERT_FOR_PARA;


MYSQL *conn;
MYSQL_RES *sql_res;
MYSQL_ROW sql_row;

char *prog;
int debug;
char *db_IP, *db_name, *db_pwd, *db_user_name; // for DB config
int doc_id;
char *source_prog; 

int SHOW_PAGE;

struct Page_Stats {
  int total_above_20;
  int good_score;   // sb same as above
  double meanL;
  double meanR;
  double calculated_mean; // calculated by slope and intercept
  double stdL;
  double stdR;
  double calculated_std; // relative to calculated_mean
  double slope_m; // y = m*x + c
  double intercept_c; 
}
page_stats_array[MAX_PAGE];

struct Obj_Struct { // AWS original input array
  float confidence;
  int page;
  int para; 
  char *text;
  char *textf, *textl; // only in blocks
  char *source_program;
  int doc_id;

  int my_x1; 
  int my_x2; 
  int my_y1; 
  int my_y2; 

  int obj_id; // either block_id or word_id
  int block_in_doc;
  int renumbered; // only for blocks
  int block_in_page;   

  int sn_in_block; // only for words
  int sn_in_page; // only for words  
  int sn_in_doc; // only for words  
  int sn_in_line; // only_for_words, starts from 1 !!!

  int my_line_in_doc; // only in blocks
  int my_line_in_page; // only in blocks
  int my_line_in_page_renumbered; // double_column lines
  int first_word, last_word; // only in blocks
  int is_header_footer; // 0 - nothing / 1 - header / 2 - footer

  int has_space_before; // words only
  int loc; // words only

  int split_token; // words_only: this word should be concat with the next word and with the DASH in between and with the comma and other punctuations after
  int found_section; // if the block starts: Section 5, A., 1), etc (done by found_section())
  int essential;
}  word_array[MAX_WORD], block_array[MAX_BLOCK]
  , rf_word_array[MAX_WORD]; // after insertion of Rental Form
int word_count;

int line2block_index[MAX_PAGE*MAX_LINE_PER_PAGE][MAX_BLOCK_PER_LINE];
int line2block_no[MAX_PAGE*MAX_LINE_PER_PAGE]; // how many blocks per line
int line2block_l_of_gap[MAX_PAGE*MAX_LINE_PER_PAGE];
int line2block_r_of_gap[MAX_PAGE*MAX_LINE_PER_PAGE];

// gaps between tokens max of 1 PDF page
struct Gap_Struct {
  int left, right, size, block_l, block_r;
  int taken;  
  int block;  
  int line_in_doc;
  int page;
  int block_in_doc;
} gap_array[MAX_PAGE][MAX_GAP_PER_PAGE];
int gap_no[MAX_PAGE];


#define MAX_LINES_IN_DOC 50000
struct Bad_Line {
  int first_word;
} line_array[MAX_LINES_IN_DOC];

struct Page_Properties {
  int first_block_in_page;
  int last_block_in_page;
  int first_line_in_page;
  int last_line_in_page;
  int no_of_blocks_in_page;
  int no_toc_clusters; // how mnay toc clusters on this page
  int no_toc_items;  // how many toc items in the toc clusters
  int min_block_x1;
  int max_block_x2;  
  int no_of_bad_toc_lines; // lines that don't start or end with a number
} page_properties_array[MAX_PAGE];



struct Line_Property {
  int gap_no_in_cluster;
  int gap_id;
  int gap_size;
  int block_l;
  int block_r;
  int block_l_size;
  int block_r_size;
  int gap_my_line;
  int gap_distance;
  int gap_x1;
  int gap_x2;
  int gap_y;
  int total_indent_l;
  int total_indent_r;
  int page;
  int belongs_in_group;
  int last_block_in_line;
  int first_block_in_line;  
} line_property_array[MAX_LINE_PER_PAGE*MAX_PAGE];



/*  a group of lines that are a double-column block
the folding happens from the first to the last line
 */
#define MAX_GROUP_PER_PAGE 100
struct Line_Group {
  int first_line, last_line;
  int first_gap, last_gap;   // gap in cluster
  int page;
  int how_many_lines;
  float line_ratio; // less than .7 means it's not a full page DC
} line_group_array[MAX_PAGE][MAX_GROUP_PER_PAGE];
int line_group_no[MAX_PAGE];



#define MAX_PAGE_BLOCK 1000
#define MAX_SIM (MAX_PAGE_BLOCK*MAX_PAGE_BLOCK)
struct Sim_Triple {
  int ll, kk; // first word
  int ll_last, kk_last; // last word
  int ll_block, kk_block; // the block
  int sim;
} sim_block_list[MAX_SIM];
int sim_block_no[MAX_PAGE];



struct Para { // obsolete
  int para;
  int page;
  int line;
  int token;
  int num_tokens;
  int doc;
  char *source_program;
} para_array[MAX_PARA];
int curr_para;


struct Para_Len {
  int page;
  int line;
  int first_token;
  int num_tokens;
  int no_of_real_words; // words in dict, not numbers, not two-char
  int is_all_upper; // are all the words all upper
  int no_of_all_upper;  // how many words are all upper
  int is_all_first_upper; // are all the words first upper
  int no_of_all_first_upper; // how many words are first upper
  int is_first_upper; // is the first char of the para upper
  int doc;
  char *source_program;
} para_len_array[MAX_PARA];
int para_len_no;


#define MAX_SPACE 100000
struct Vert_Space {
  int space_size;
  int token_no;
  int page_no;
  int line_no;
} space_array[MAX_SPACE];
int space_no;

#define MAX_ENUM_PER_PAGE 200
struct Enum_Triple {
  int ll, kk; 
  int page; // the page
  int diff;
  int size1, size2; // for debugging only
  int found_period1, found_period2;  // distinguish between "44" and "44." -- the period means it's part of TOC and not enum
} enumerated_list[MAX_PAGE][MAX_ENUM_PER_PAGE];
int enumerated_no[MAX_PAGE];


#define MAX_RENTAL_PER_PAGE 100
struct Rental_Triple {
  int ll, kk; 
  int page; // the page
  int diff;
} rental_list[MAX_PAGE][MAX_RENTAL_PER_PAGE]; 
int rental_no[MAX_PAGE];


struct Rental_Report {
  int cluster_serial_no;
  int cluster_no_of_enums;
  int real_no_of_clusters;
  int score;
  int total_subsection; // how many subsections are found under each enum
  int total_long_lines_same_x1; // how many lines found at the same x1 of enums WHAT IS THIS?? OUT FOR NOW... 12/23/2020
} rental_report_array[MAX_PAGE];
int total_no_of_rental_pages;
int total_no_of_rentals;

int total_no_of_rentals_inserted;
int total_no_of_rental_pages_inserted[MAX_PAGE];


struct Enum_Report {
  int cluster_serial_no;
  int cluster_no_of_enums;
  int real_no_of_clusters;
  int score;
  int total_subsection; // how many subsections are found under each enum
  int total_long_lines_same_x1; // how many lines found at the same x1 of enums WHAT IS THIS?? OUT FOR NOW... 12/23/2020
} enum_report_array[MAX_PAGE];
int total_no_of_enum_pages;
int total_no_of_enums;


#define MAX_RENAL_POINTS 1000
int index2rental_point_array[MAX_PAGE][MAX_LINE_PER_PAGE];
struct Rental_Point {
  int given_section_no;
  int block_no;
  int word_no;
  int line_no;
  int page_no;
} rental_point_array[MAX_RENAL_POINTS];
int rental_point_no;

int summary_point_page_array[MAX_PAGE];

int create_footer_clusters_new(int last_page_no); // create footer_cluster_array and mark BLOCK_ARRAY[bb].IS_HEADER_FOOTER 
int create_header_clusters_new(int last_page_no) ;
int create_enumerated_clusters(int last_page_no); // detect enumerated lines
int create_DC_overlap_clusters(int last_page_no);
int establish_groups_in_overlap_clusters(int last_page_no);
int create_TOC_block_clusters(int last_page_no, int *essential);
int insert_new_paras(int last_block_no);
int update_word_para_sql_per_page(int last_page_no, int total_renumbered_words);
int update_word_para_sql_per_page_rf(int last_page_no, int rf_word_count);
int get_org_id(int doc_id);
int insert_paras_into_sql(int last_para, int space_no, int doc_id, int org_id);
void remove_last_comma(char *query);    
int insert_spaces(int last_block_no);  
char *strrstr(char haystack[], char needle[]);
int is_all_upper(int para);
int create_rental_forms(int last_page_no);
int detect_toc_section(int bk) ;
int get_summary_page();

#define PAGE_RIGHT_MARGIN 8000
#define PAGE_LEFT_MARGIN 1000
#define PAGE_CENTER 5000
#define ESSENTIAL 6400
