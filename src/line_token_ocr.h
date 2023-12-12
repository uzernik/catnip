


#define MAX_LINE 100
#define MAX_PAGE 400
#define MAX_PARA 50000
#define MAX_TOKEN 300000

struct Token {
  int line_no;
  int page_no;
  int x1_1000;
  int x2_1000;
  char *text;
} token_array[MAX_TOKEN];
 int token_no;

int  Paragraph2Token_array[MAX_PARA]; 
 
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
  int line_right_is_number;  // is the rightmost word a number (for TOC)
  char *line_right_word; // for debgging
  char *line_left_word; // 
  int page_ref;
  int toc_item;
  int no_of_chars;
  int lease_title_no_of_words; // how many words such as AMEND, LEASE, SUBLEASE, AGREEMENT, ADDENDUM, EXTENSION
} page_line_properties_array[MAX_PAGE][MAX_LINE];

int get_tokens(int doc_id, char *source_ocr);
