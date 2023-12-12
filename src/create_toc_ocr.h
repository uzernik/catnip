#include <stdio.h> 
#include <string.h>
#include <ctype.h>
#include <mysql.h>

int echo_to_buff();
int do_item();
int sub_section_function(char my_text[]);
int echo_to_buff_for_exhibit();
int echo_to_buff_for_title();
int insert_item(int nn, int my_item, int my_line, int my_loc, int my_group, char my_text[], char* title, char *header, int is_special, int title_pid, int pid, int pass_no);
int found_char_in_vpp(char text[]);
int echo_to_buff_for_item(); 
int init_seqs();
int init_items_limited();
int chop_CAP_header(char *text, int item_no);
int chop_Cap_header(char *text);
int align_forced_group(int kk);
int old_align(int kk);
int find_period(char *header);
int check_seq();
int organize_levels();
int print_rank_list();
int print_out_buff();
int sql_output(int doc_id);
int remove_item_from_seq(int my_seq, int other_seq,int other_seq_item, int item_no);
int calculate_first_and_last_on_seq();
int mark_include_relations();
int rearrange_levels(int top_seq,int rec_no); // <---------  recursive

int level_adjust_double_digit();
int level_adjust_paren_after_non_paren();
int level_adjust_paren_early_include();
int print_children_array(int no_of_children);
int collect_toc_summary(int no_of_children);
int collect_level_tally();
int final_level_tally();
int combine_linked_groups(int ii, int jj);
int copy_list_into_array();
int fix_neighbors_badness(int other_seq, int other_seq_item);

  /***********************************************************************/
  /* stuff for mesa */
#define MATCH  2
#define MISMATCH  -1
#define SPECIAL_MATCH 0
#define MAX_STRING 1000
#define FILE_GROUP 70
#define FUNDAMENTALS_GROUP 71  
#define MAX_LEVEL 50 // in realithy there are no more than 10 legit levels

int SEQ_70; // = 0
int SEQ_71;  // = 0
#define DUMMY_MESA 10001
#define MAX_SEQ 10000 // how many sequences allowed
#define MAX_SEQ_NO 400 // how many items per sequence

#define MAX_COUNT 60 // i allow the max value in a toc element to be 50

#define MAX_INCLUDED 400 // seqs that are included in my seq
#define MAX_INCLUDING 100 // seqs that include my seq
#define MAX_OVER 100 // overlapping seqs
#define MAX_AWK 100 // overlapping seqs

#define MAX_CONT_SEQ 600 // how many contending interpretations we allow one item (I -> 1,I,I)

  
  #define MAX_FILE 50
  #define MAX_HEADER 1500
  #define MAX_HEAD_LEN 60000
int max_file_no;
  int max_item_no;


  /****************************************************************/
#define MAX_SECTION_N 5 // 4.5.6.7.8 no more subsubsubsubsubsections than that...
  struct Cont_Pair {
    int seq_item_no;
    int seq_no;
    int my_enum;  // the value of this item according to the GROUP of this seq
  };

  struct Toc_Summary { // to print the goodness of the entire doc toc
    int good_no_of_items; // how many non-10 items in the TOC?
    int good_seq_no; // how many good nested seqs in TOC?
    int top_level_no_of_items; // how many level-3 items? 
    int top_level_seq_no; // how many level-3 seqs?  It's not so great if you have 1,2,3 .. 1,2,3 ... 1,2,3 at the top level
    int file_level_no_of_items; // how many level-2 items?
    int total_gap; // how many gaps in good seqs?
    int total_diff; // how many diffs in good seqs?
    int first_item;
    int last_item;
    int last_para; // how many para in the file
  } toc_summary;
  #define MAX_TOC 50000

  struct My_Item {
    int page_no; // new addition
    int pid; // for MUP
    int grp_no; // what group does it belong to?
    char *section; //4.5.6
    char *title; // EXHIBIT
    int section_no; // how many items we have in the section (1.2.3 = 3)
    char *section_s[MAX_SECTION_N]; // the string "5"
    int section_v[MAX_SECTION_N]; // the value (D is 4)
    char section_m[MAX_SECTION_N]; // the marker ("0" for none)
    char *prefix; // as in (a) or $30.5
    char *header; // INTRODUCTION
    char *clean_header; // chopped and cleaned
    int periods_in_header; // how many periods in the header? indication of a TOC ..........

    int selected_seq; // of all the possible seqs, the selected seq
    int no_of_seqs; // how many contending seqs
    int my_enum; // in case of "13.4" it is "4"
    int lower_enum; // in case of "13.4" it is "13"
    struct Cont_Pair seq_array[MAX_CONT_SEQ]; // pointers to contending sequences where this item shows
    int no_of_leading_spaces; // the leading spaces before the header
    int my_loc; // the position of the character in the file, calculated by YY_USER_ACTION
    int center; // the X coords of the center of this line
    int left_X;
    int line_no; // the line number on the page
    int too_long; // the headers of the items of this seq are too long and not capitalized.  Don't show.
    int token_id; // the token_id of the first word in the TOC item

    int no_of_words;
    int no_of_all_CAPs_words;
    int no_of_first_Caps_words;
    int CAP_hdr_len; // the length of the ALL CAP header

    int is_special; // lease = 1; fundamental_title = 2; fundamental_point = 3; toc_toc = 13; toc_rest = 14

    int toc_page_no; // "23", "123", "xvii"
    int toc_page_no_type; // roman = 1, arab = 2, none = 0,3,4
    int toc_page_no_coord; // 523123

    /* used by align toc_toc */
    char *norm_header; // not populated
    char *case_norm_header; // not populated   
  }; // My_Item
struct My_Item item_array[MAX_TOC];
int item_no;


  /**************************/
  int max_rank_seq;  // the seq that has the max rank
  int max_rank; // the max rank of all seqs
  int max_awkward_seq;  // 
  int max_awkward; // how many larger seqs it fits awkwardly into
  int max_include_seq;  // 
  int max_include; // the max range of all seqs
  int max_overlap_seq;  // overlapping with other seqs
  int max_overlap; // 
  /**************************/

char MY_TOTAL_file[1000000];
char MY_TOC_file[100000];
char MY_REST_file[10000000];


char *buff_ptr;

char rest_buff12[10000000]; // after remove of PP
char rest_buff22[10000000]; // after chop to 50
char rest_buff32[10000000]; // removed multiple \n

char toc_toc_buff[10000]; // items straight from array
int buff_ctr;

int my_begin_yyyy1();
int my_begin_yyyy2();
int my_begin_yyyy3();
int my_begin_yyyy6();
char *my_fopen(char *name);



  /****************************************************************/
#define MAX_COORDS 1000000
struct Coords_Item {
    int x1000;
    int y1000;
    char cc;
    int page_no;
} coords_array[MAX_COORDS];
int coords_no;

struct Tree_Node {
    struct Tree_Node *siblings; // list of siblings
    struct Tree_Node *r_siblings; // back ptr
    struct Tree_Node *children; // list of children
    struct Tree_Node *parent; // back ptr
    int no_of_children; // how many siblings under my children's node
    int level;
    int seq_no;
    int anchored; // this node is anchored at the top by some fixed level (e.g., max_ramk_seq)
};


  
/*********************************************************************/
struct Child_Triple {
  int seq_no;
  struct Tree_Node *ptr; // ptr to this child
};
struct Tree_Node *doc_seq_tree;


struct Child_Triple child_triple_array[MAX_SEQ]; // a hash table: items in this array are indexed by seq_no

#define MAX_LINE 71
#define MAX_PAGE 400

int found_exhibit_page_array[MAX_PAGE];  // this contains the number of EXHIBIT findings on this page.  More than 1 is bad.
int exhibit_at_4_top_lines_page_array[MAX_PAGE]; // exhibit must be at top 4 lines
int exhibit_gap_less_than_3_page_array[MAX_PAGE]; // exhibits are two close to each other
// a 1 indicates there was an Exhibit or a Schedule in that cell (first token on the line)
int exhibit_page_line_array[MAX_PAGE][MAX_LINE];

 
/*********************************************************************/
#define MAX_GROUP 500
#define MAX_GROUP_ITEM 1000
struct Group_Item {
  int item_no;
  int convert; // converted from I to 1
  int my_enum; // the value relative to the group: V =5 or V = 22, I as in (1,2,3)= 1, I (as in a,b,c) =10, I (as in i,ii,iii) = 1
  int my_P_enum; // the value relative to the group of the Prev span (e.g, 6 in 6.12)
  int my_seq_no; // the seq_no assigned to this item
  int seq_item_no; // the seq_no assigned to this item  
  struct Group_Item *next,*prev;
  int strong_inserted; // when inserting strong items
}; 
struct Group_Item *group_header_array[MAX_GROUP];
int group_no_array[MAX_GROUP]; // the max no of the groups

struct Group_Item group_item_array[MAX_GROUP][MAX_GROUP_ITEM]; // the items in each group
struct Group_Item tmp_group_item_array[MAX_GROUP_ITEM]; // for tmp copying

int found_toc12_in_group[MAX_GROUP];
int found_toc13_in_group[MAX_GROUP];  

int found_toc12_in_seq[MAX_SEQ];
int found_toc13_in_seq[MAX_SEQ];  
int found_toc12_in_group[MAX_GROUP];
int found_toc13_in_group[MAX_GROUP];  

char *app_path;

int do_take_headers_from_toc_toc;

#define NO_INC 11 // how many types of inclusions between two seqs?

struct SEQ_O { // the entire seq
  int no_of_items; // how many items on it? this number stays the same when items are removed and a -1 is stuck as a place holder in the array. IT IS SHORT BY ONE
  int real_no_of_items; // how many items on it after some are removed.  IT IS NOT SHORT BY ONE!!!

  int first_page;
  int last_page;
  int range; // the span of the sequence (last - first item) // not no_of_items, but the range
  int group_no; // what group did we come from?
  int total_all_cap; // how many items are all caps?
  int total_sep_period; // how many items have . as the 0th separator?
  int incl_total[NO_INC]; // shows the tally of include relations bewteen the sequences
  int max_gap; // the max gap on this seq
  int total_gap; // total the gaps in the seq
  int max_gap_item; //where this gap is
  int max_diff;
  int total_diff; // total the diff in values in the seq
  int max_diff_item;
  int awkward; // how many seqs we fit awkwardly INTO? 
  int include; // how many seqs fit NEATLY INTO this seq?
  int included; // how many this seq fit NEATLY into?
  int include_array[MAX_INCLUDED]; // list all sequences included in this one
  int over_array[MAX_OVER]; // list overlapping seqs
  int awk_no;
  int awkward_array[MAX_AWK]; // list overlapping seqs -- this is for real (inc=4), I don't know about the one above (inc=7 , 8)
  int included_array[MAX_INCLUDING]; //  list all sequences that include this one
  int parent; // new 110917, the direct include
  int overlap; // how many docs we overlap with? (up and down)
  int rank; // is this the top sequence?
  int level; // what level am I?
  int fn, ln; // the first item and last item on the sequence (chopping off removed items)
  int prev_item; // the prev item on the containing seq (eg, hte item II in front of seq 2.1, 2.2, 2.3)
  int total_converts; // how many items are converts
  int total_period_headers;// how many items have more than X periods in the header
  int level_reason; // -1; no level / 0 embed / 1 double-decker / 2 paren / 3 early-include
  int mean_left_X;
  int std_left_X;  
  int mean_center;
  int std_center;
  int centered; // is the center really "centered" (std OK and center around 300,000
  int bad_found_61; // to eliminate sequences such as "1 cddfdgfd, 2 fsffd, 3 xccxvxv" as opposed to "1. cxscsc, 2.cxvv 3.ccc"
  int header_words; // how many words such as INDEMNITY, RENT, SECURITY found in headers of seq
}; // SEQ_O

struct SEQ_O seq_array[MAX_SEQ]; // the array of sequences
  struct Tree_Node *initialize_doc_tree(int seq_no, int level);
  struct Tree_Node *my_new_node(int node_no,struct Tree_Node *parent);

struct Seq_Item { // one item on the seq
  int badness; // how does this item deviate from the count of the seq, how separate is it from it's neighbors
  /*  INCORRECT SUBJECTIVE!!! */
  int my_enum; // what's my enum value
  int group_item_no; // the sn on the group
  int group_no; // which group it belongs to
  /*  DONE INCORRECT SUBJECTIVE!!! */
  int item_no; // back reference to item_array
  int removed; // this item has been removed by contending seq
};

struct Seq_Item seq_item_array[MAX_SEQ][MAX_SEQ_NO]; // all the sequence items
int g_seq_no; // how many sequences do we have in the document?

int max_level[MAX_GROUP];
int max_level_total[MAX_GROUP];
int total_level[MAX_GROUP];

struct Tally_Pair {
    int total; // how many seqs per each level
    int total_rank; // total the rank of the seqs
};

struct Tally_Pair group_level_tally_array[MAX_GROUP][MAX_LEVEL]; // the histogram of the levels selected for each group by the anchor


#define MAX_PARA 50000
struct ParagraphToken {
  int token_id;
  int page_no;
  int line_no;
  int dc_line_no; // double col lines
} paragraphtoken_array[MAX_PARA];
int para_no;
 
int token_no;
#define MAX_LOC 1000000
int reverse_token_array[MAX_LOC]; // loc -> token_id
#define MAX_TOKEN 200000
int token2point[MAX_TOKEN]; // an array of points indexed by tokens

/*****************************************************/

char *file_name_root;
char *deal_root_name; // the path of the src file
FILE *index_file;
FILE *toc_file;
char index_name[400];

/*****************************************************/
char *doc_country; // GBR /USA / HKG
