// how many cols per row
#define ALIGN_MAX_COL 30
int debug;

struct Align_Triple {
  int sn;
  char my_char;
  int ref_point; // necessary only for triple_line_array but no OO design with inheritance here
  int pair_no; // the number of the partner
};

int suppress_secondary; // 1 is suppress extraneous tokens on S2, 0 is S1 and S2 are symmetrical
int align_debug;

int print_table(int l1, int l2, struct Align_Triple triple1[], struct Align_Triple triple2[]);
float wrap_align(struct Align_Triple triple1[], struct Align_Triple triple2[]);
float align(struct Align_Triple triple1[], struct Align_Triple triple2[]);
float align_for_clustering(struct Align_Triple triple1[], struct Align_Triple triple2[], int *lll1, int *lll2, int debug);
int make_dp_table(int l1, int l2, struct Align_Triple triple1[], struct Align_Triple triple2[]);
int init_triples(struct Align_Triple triple1[], struct Align_Triple triple2[]);
int len_triple(struct Align_Triple triple[]);
int prepare_one_triple(struct Align_Triple triple1[], char buff[]);
