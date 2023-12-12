#define MAX_PARA 90000
int no_good_array[MAX_PARA]; // does the pid belong in the TOC

#define MAX_PREAM 100
struct Pream_Pair { // pairs of first and past para for the preambles in the file. Normally there's one, sometimes two docs strung together
    int first_pream;
    int last_pream;
    int first_pream_pid;
    int last_pream_pid;
    char *end_type; // TOC/REC/NULL
} edit_index_array[MAX_PREAM];


  struct Index_Item {
    int my_enum; // new, 4/30/13
    int line_no;
    int pid; // for MUP
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
    int center;
    int my_loc;
    int too_long; // headers too long and no Caps
    int page_no;
    int a1,a2,a3; // need to be flashed out
    int is_special;

    int toc_page_no;
    int toc_page_no_type;
    int toc_page_no_coord;				
    
    struct Index_Item *next;
    struct Index_Item *prev;
  };
  #define MAX_TOC 5000
  struct Index_Item index_item_array[MAX_TOC];
int index_array_no;

int sync_up_recitals_and_toc();
int scan_recitals_from_array(struct Pream_Pair edit_index_array[]); // output is first and last pid of preamble
int create_linked_list_index(int in);
int write_index(int in);
int modify_index(int edit_index_array_no);

int edit_index_array_no;
char *index_name;


int my_print;
int my_debug;

int largest_seq_no; //  the max seq_no encountered

