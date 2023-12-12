#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include <math.h>  
#include <ctype.h>
#include <unistd.h>
#include "my_import_functions.h"  
#include "create_toc_ocr.h"
#include "entity_functions.h"
#include "align_toc_functions.h"  
#define MAX(a,b) (a>b)?a:b
#define MIN(a,b) (a<b)?a:b  

int lev_debug = 0;
int testing_removed = 1;  // the main_seq gets removed, for now see how to restore main_seq , UZ 062819
// create the tree of all the seqs, initially it's flat
// initialize the tree and the triple array
struct Tree_Node *initialize_tree_node(int seq_no, int level) {
  struct Tree_Node *ptr = (struct Tree_Node *)malloc(sizeof(struct Tree_Node));
  ptr->seq_no = seq_no;
  ptr->level = level;
  ptr->anchored = 1;
  ptr->no_of_children = 0;

  ptr->anchored = 0;
  ptr->siblings = NULL;
  ptr->children = NULL;
  ptr->r_siblings = NULL;
  ptr->parent = NULL;


  if (debug || lev_debug) fprintf(stderr,"    LLL5 seq=%d: INIT LEVEL = %d: max_rank_seq=%d:\n",seq_no, level, max_rank_seq);
  child_triple_array[seq_no].ptr = ptr;
  child_triple_array[seq_no].ptr->level = level;
  return ptr;
} // initialize_tree_node()

int print_seq_tree(int nn, struct Tree_Node *doc_seq_tree) {
  fprintf(stderr,"MYTREE:%d:\n",nn);
  struct Tree_Node *ptr = doc_seq_tree;
  while (ptr) {
    fprintf(stderr,"  PTR:%d:%d:%d:\n", ptr->seq_no, ptr->level, ptr->no_of_children);
    ptr = ptr->siblings;
  }
  return 0;
}

int level_adjust_double_digit() { // make sure 11.2 is indented one deeper than 11.
  int ii;
  for (ii = 0; ii <= g_seq_no; ii++) { // for II
    int in = seq_item_array[ii][0].item_no; // just get one item arbitrarily
    int sec_n = item_array[in].section_no; // how many fields in this item
    if (0) fprintf(stderr,"                    <MR>LLL7:%d:%d: nn=%d: no=%d: \n",ii,in      
	      ,     item_array[in].lower_enum//,item_array[prev_in].my_enum
	      ,     item_array[in].section_no//,item_array[prev_in].section_no
	      //	,     spn
	      );

    if (sec_n > 1) { // a double decker
      int kk;
      int prev_in = 1;
      int found = 0;

      for (kk = 1; found == 0 && prev_in > 0 && kk < 5; kk++) {
	prev_in = in-kk;
	int spn = item_array[prev_in].selected_seq; // the seq_no of the prev item
	if (item_array[in].lower_enum == item_array[prev_in].my_enum) {  // IV and 4.1
	  if (item_array[in].section_no != item_array[prev_in].section_no) { // don't allow 3.4 vs 4.1
	    found = 1;
	    if (child_triple_array[ii].ptr && spn >= 0 && child_triple_array[spn].ptr) {
	      child_triple_array[ii].ptr->level = child_triple_array[spn].ptr->level + 1;
	      if (debug || lev_debug) fprintf(stderr,"LLL2 seq=%d: INIT LEVEL = %d:\n",ii,child_triple_array[ii].ptr->level);
	      seq_array[ii].level_reason = 1;
	      rearrange_nested_seqs(ii, 0);	// bump up all the included and below
	    }
	  } else { // keep 3.5 and 5.1 at same level
	    found = 1;
	    if (child_triple_array[ii].ptr && spn >= 0 && child_triple_array[spn].ptr) {
	      child_triple_array[ii].ptr->level = child_triple_array[spn].ptr->level; // + 1;
	      if (debug || lev_debug) fprintf(stderr,"LLL3 seq=%d: INIT LEVEL = %d:\n",ii,child_triple_array[ii].ptr->level);
	      seq_array[ii].level_reason = 1;
	      rearrange_nested_seqs(ii, 0);	// bump up all the included and below
	    }

	  }
	}

      } // for kk
    }
  } // for ii
  return 0;
} // level_adjust_double_digit
// obtain the group_level and fix the unanchored levels

int adjust_unanchored_levels() {
  int ii;
  for (ii = 0; ii < g_seq_no; ii++) {
    if (child_triple_array[ii].ptr && (1 || child_triple_array[ii].ptr->anchored == 0)) {
      int gn = seq_array[ii].group_no;
      int group_level = max_level[gn];
      child_triple_array[ii].ptr->level = seq_array[ii].level = group_level;
      if (debug || lev_debug) fprintf(stderr,"LLL seq=%d: GROUP LEVEL:%d:\n",ii,group_level);
    }
  }
  return 0;
}



int final_level_tally() {
  int ii,jj;
  for (ii = 0; ii < MAX_GROUP; ii++) {
    max_level[ii] = 0;
    total_level[ii] = 0;
    max_level_total[ii] = 0;
    for (jj = 0; jj < MAX_LEVEL; jj++) {
      if (group_level_tally_array[ii][jj].total > max_level_total[ii]) {
	max_level_total[ii] = group_level_tally_array[ii][jj].total;
	max_level[ii] = jj;
      }
    }
  }
  return 0;
} //final_level_tally() {

int level_adjust_paren_after_non_paren() { // make sure (a) is deeper than Section 1.

  int ii;
  for (ii = 0; ii <= g_seq_no; ii++) { // for II
    int in = seq_item_array[ii][0].item_no; // just get one item arbitrarily
    int spn1 = item_array[in-1].selected_seq;

    if (child_triple_array[ii].ptr
	&& child_triple_array[spn1].ptr) {
      //print_BBBB(0, ii);
      
      if (seq_array[ii].group_no <=30 && seq_array[ii].group_no >=21) { // current seq is a paren group

	if ((seq_array[spn1].group_no > 30 || seq_array[spn1].group_no < 21) //  prev seq is NOT a paren group
	    && child_triple_array[ii].ptr->level == child_triple_array[spn1].ptr->level) { // and the two seqs are at the same level

	  child_triple_array[ii].ptr->level = child_triple_array[spn1].ptr->level + 1;	// push this one deeper by 1
	  if (debug || lev_debug) fprintf(stderr,"LLL1 seq=%d: INIT LEVEL = %d:\n",ii,child_triple_array[ii].ptr->level);
	  seq_array[ii].level_reason = 2;
	}
      }  // 30/21
    }
  } // for

  return 0;
} // level_adjust_double_digit


int level_adjust_paren_early_include() { // in "A.,B., 1.,2.,3., C.,1.,2." , make sure 1.,2. is the same as 1.,2.,3.
  int ii;
  for (ii = 0; ii <= g_seq_no; ii++) { // for II
    int in = seq_item_array[ii][0].item_no; // just get one item arbitrarily
    int spn1 = item_array[in-1].selected_seq;
    if (child_triple_array[ii].ptr
	&& child_triple_array[spn1].ptr
	&& child_triple_array[ii].ptr->level > 2) { // don't do it for top levels
      
      if (child_triple_array[ii].ptr->level == child_triple_array[spn1].ptr->level) { // the two seqs are at the same level
	int kk;
	int found = 0;
	for (kk = 0; found == 0 && kk < seq_array[spn1].include; kk++) {
	  int spi2 = seq_array[spn1].include_array[kk]; // the included seq
	  int ggn = seq_array[spi2].group_no; // the group of the included seq
	  if (ggn == seq_array[ii].group_no) { // curr group
	    child_triple_array[ii].ptr->level = child_triple_array[spn1].ptr->level + 1;	// push this one deeper by 1
	    if (debug || lev_debug) fprintf(stderr,"     LLL00 seq=%d: INIT LEVEL = %d:\n", ii, child_triple_array[ii].ptr->level);
	    seq_array[ii].level_reason = 3;
	    found = 1;
	  }
	} // for kk
      }
    }
  } // for
  return 0;
} // level_adjust_double_digit


int collect_level_tally() {
  int ii;
  // first collect the seqs
  for (ii = 0; ii < g_seq_no && ii < MAX_SEQ; ii++) {
    //if (debug || lev_debug) fprintf(stderr,"IN COLLECT: %d\n",ii);
    struct Tree_Node *ptr = child_triple_array[ii].ptr;
    if (ptr && ptr->anchored == 1) {
      int gn = seq_array[ii].group_no;
      group_level_tally_array[gn][ptr->level].total++;
      //if (debug || lev_debug) fprintf(stderr,"TALLY:gr=%d seq=%d lev=%d lev=%d anchor=%d<BR>\n",gn,ii,ptr->level,seq_array[ii].level,ptr->anchored);
    }
  }
  // now tally over each group
  final_level_tally();
  //if (debug || lev_debug) fprintf(stderr,"DONE COLLECT\n");
  return 0;
} //collect_level_tally() {


int traverse_tree(struct Tree_Node *parent) {
  if (debug || lev_debug) fprintf(stderr,"       BEGIN TRAVERSE\n");
  if (parent) {
    if (debug || lev_debug) fprintf(stderr,"            PARENT: seq=%d: lev=%d: no_of_children=%d:\n",parent->seq_no,parent->level, parent->no_of_children);
    if (parent->no_of_children == 0) {
      if (parent->children) {
	if (debug || lev_debug) fprintf(stderr,"                BAD: NOT SUPPOSED TO FIND CHILDREN\n");
      } else {
	if (debug || lev_debug) fprintf(stderr,"                GOOD: NOT SUPPOSED TO FIND CHILDREN\n");
      }
    } else { // no_of_children > 0
      if (parent->children) {
	int sib = 0;
	struct Tree_Node *ptr = parent->children;
	while (ptr) { 
	  if (debug || lev_debug) fprintf(stderr,"                                  CHILD: seq=%d: lev=%d anchor=%d no_of_children=%d:\n",ptr->seq_no, ptr->level, ptr->anchored, ptr->no_of_children);	  
	  ptr = ptr->siblings;
	  sib++;
	}

	if (sib != parent->no_of_children) {
	  if (debug || lev_debug) fprintf(stderr,"                                  BAD: INCOMPAT NO OF CHILDREN parent=%d sib=%d children=%d",parent->seq_no,sib,parent->no_of_children);
	} else {
	  if (debug || lev_debug) fprintf(stderr,"                                  FOUND: parent=%d sib=%d children=%d:",parent->seq_no,sib,parent->no_of_children);
	}
      } else {
	  if (debug || lev_debug) fprintf(stderr,"                                   BAD: SUPPOSED TO FIND A CHILD\n");
      }
    }
  } else {
    if (debug || lev_debug) fprintf(stderr,"BAD: NO PARENT NODE<BR>\n");
  }
  if (debug || lev_debug) fprintf(stderr,"         DONE TRAVERSE\n");
  return 0;
} // traverse_tree()

#define top_level 2
#define main_level 3

int organize_levels() {      
  if (debug || lev_debug) fprintf(stderr,"    STEP5.1  BEGIN ORGANIZE LEVELS\n");
  //print_seq_55(0,55);
  doc_seq_tree = initialize_tree_node(max_rank_seq,top_level); // create the tree structure and the hash table, now we have a single node
  int kk;
  for (kk = 0; kk <= g_seq_no; kk++) {
    if (kk != max_rank_seq) {
      initialize_tree_node(kk,0);
    }
  }
  seq_array[SEQ_70].level = top_level+1; // EXHIBIT_LEVEL  (3)  

  rearrange_nested_seqs(max_rank_seq, 0); // THIS IS DONE RELATIVE TO THE ANCHOR (document) 

  level_adjust_double_digit(); // this one is recursive    

  level_adjust_paren_after_non_paren();                  

  //level_adjust_paren_early_include(;
  //level_adjust_paren_early_include();         

  if (debug || lev_debug) fprintf(stderr,"    STEP5.5 print_children_array:%d: %d:%d:\n",g_seq_no, item_array[36].selected_seq, seq_array[item_array[36].selected_seq].level);                       
  print_children_array(g_seq_no);   
  //  collect_toc_summary(g_seq_no); 
  collect_level_tally();   
  //adjust_unanchored_levels(); 
  traverse_tree(doc_seq_tree);

  correct_main_seq();

  print_seq_tree(1, doc_seq_tree);
  print_doc_seq_tree(doc_seq_tree);
  return 0;
} //organize_levels() 

int print_children_array(int no_of_children) {
    fprintf(toc_file,"PRINTING CHILDREN_ARRAY<BR>\n");
    int ii;
    for (ii = 0; ii < MAX_SEQ; ii++) {
      struct Tree_Node *ptr = child_triple_array[ii].ptr;
      if (ptr != NULL) {
	fprintf(toc_file,"CHILD: ii=%d: lev=%d: anc=%d sn=%d: real_tot=%d:<BR>\n",ii,ptr->level,ptr->anchored, ptr->seq_no, seq_array[ptr->seq_no].real_no_of_items);
      }
    }
    return 0;
}

// the children seq of a top_seq, ranked by significance (e.g., I(1) II II IV V(5) VI VII VIII IX X(10) more significant than (I(10) V(22) X(24))
struct Rank_Pair {
  int rank;
  int seq_no;
  int taken;
  struct Rank_Pair *next;
};
 

struct Rank_Pair *rank_list;

// add a node to the list by descending RANK order
int add_to_rank_list(int seq_no, int rank) {
  seq_array[seq_no].rank = rank;
  struct Rank_Pair *new_seq = (struct Rank_Pair *)malloc(sizeof(struct Rank_Pair));
  new_seq->seq_no = seq_no;
  new_seq->rank = rank;

  if (rank_list == NULL) {
    rank_list = new_seq;
  } else {
    struct Rank_Pair *prev_ptr = rank_list;
    struct Rank_Pair *ptr = rank_list;
    if (lev_debug || debug) fprintf(stderr,"                              WHILE AGAIN ADDING: rank1=%d:\n",rank); 
    while (ptr && rank < ptr->rank) {
      if (lev_debug || debug) fprintf(stderr,"                            NXT: rank=%d sn=%d ptr_rank=%d psn=%d pt=%d \n",rank ,seq_no,ptr->rank, ptr->seq_no, ptr->taken);
      prev_ptr = ptr;
      if (ptr == ptr->next) break;
      ptr = ptr->next;
    }
    new_seq->next = ptr;
    prev_ptr->next = new_seq;
    if (debug || lev_debug) fprintf(stderr,"                              DOD: r=%d sn=%d t=%d rnoi=%d:\n", new_seq->rank, new_seq->seq_no, new_seq->taken, seq_array[new_seq->seq_no].real_no_of_items );
  }
  return 0;
} //add_to_rank_list


// FIX THE LEVEL OF DIRECT CHILDREN and call TRAVERSE for each
int rearrange_nested_seqs(int top_seq, int rec_no) { // <---------  recursive
  if (1 || debug || lev_debug) fprintf(stderr,"BEGIN REARRANGE_NESTED_SEQS top_seq=%d: rec_no=%d:\n",top_seq,rec_no);
  //print_BBBB(4,top_seq);
  if (rec_no > 100) {
    fprintf(stderr,"ERROR: infinite recursion!\n"); 
    return 0;
  }
  int ii;
  init_rank_list();
  fprintf(stderr,"GOING OVER INCLUDE OF SEQ:%d how many:%d:\n",top_seq, seq_array[top_seq].include);
  for (ii = 0; ii < seq_array[top_seq].include; ii++) { // GO OVER ALL INCLUDED SEQS AND ADD TO RANK_LIST
    int seq_no = seq_array[top_seq].include_array[ii];
    if (debug || lev_debug) fprintf(stderr,"INNNC ii=%d seq=%d top=%d: gn=%d:\n",ii, seq_no, top_seq, seq_array[seq_no].group_no);
    if (seq_array[seq_no].real_no_of_items > 1
	|| seq_item_array[seq_no][0].group_no == 2
	|| seq_item_array[seq_no][0].group_no == 12
	) { // take in seqs with more than 1 item or 2.1???
      if (debug || lev_debug) fprintf(stderr,"            STEP10.1 add_to_rank_list:top_sewq=%d: seq_no=%d: rank=%d:\n", top_seq, seq_no, seq_array[seq_no].rank);        
      add_to_rank_list(seq_no, seq_array[seq_no].rank);
    } else {
      if (debug || lev_debug) fprintf(stderr,"Cancelled BAD1 seq:%d: grp_no=%d:\n",seq_no,seq_item_array[seq_no][0].group_no);
    }
  }
  print_rank_list(1); 
  scan_rank_list(top_seq,rec_no); // <------ RECURSION IN HERE: A CALL TO REARRANGE!!!!
  //if (debug || lev_debug) fprintf(stderr,"DONE REARRANGE_TREE<BR>\n");
  //if (top_seq == 198) print_BBBB(5,top_seq);
  return 0;
} // rearrange_nested_seqs

// CALL REARRANGE for each one of nodes ranked by significance
int scan_rank_list(int top_seq,int rec_no) {
  struct Tree_Node *top_ptr = child_triple_array[top_seq].ptr;
  struct Rank_Pair *ptr = rank_list;
  while (ptr && ptr->rank > -100000000) {
    if (1) fprintf(stderr,"\n       TRAV:%d:%d: top_seq=%d: ",ptr->seq_no,ptr->rank, top_seq);    
    int seq_no = ptr->seq_no;
    int found;
    { // the insert
      int prev_item = 0;
      if (seq_no >=0) { //UZK 1025
	prev_item = seq_array[seq_no].prev_item;
      } else {
	seq_no = 0;
      }
      found = 0;
      int jj;
      for (jj = 0; found == 0 && jj < seq_array[top_seq].no_of_items; jj++) {
	if (1) fprintf(stderr,"TOP_SEQ=%d NOI=%d SEQ_NO=%d IN=%d PREV=%d: REM=%d:\n",top_seq,seq_array[top_seq].no_of_items,seq_no,seq_item_array[top_seq][jj].item_no, prev_item, seq_item_array[top_seq][jj].removed);
	if ((testing_removed == 1 || seq_item_array[top_seq][jj].removed == 0) && seq_item_array[top_seq][jj].item_no == prev_item) {
	  found = 1;
	}
      }
      if (found != 1) { 
	;
      } else { // it is a direct-include

	  if (top_ptr) { // sanity check
	    if (debug || lev_debug) fprintf(stderr,"       CREATING NEW NODE :%d: under=%d:lev=%d:\n",seq_no,top_seq, top_ptr->level);
	    struct Tree_Node *ptr = my_new_node(seq_no,top_ptr);
	    child_triple_array[seq_no].ptr = ptr;
	    rearrange_nested_seqs(seq_no, rec_no+1); // <-----------------------------------------------------------the recursion!!!!!!
	  } // top_ptr exists

      } // else
    } // the insert
    if (ptr == ptr->next) break;
    ptr = ptr->next;
  } // while
  return 0;
} // scan_rank_list(int top_seq) 

int init_rank_list () {
  fprintf(stderr,"INIT_RANK_LIST SEQ_70=%d:\n",SEQ_70);
  seq_array[SEQ_70].total_diff = 0; // <-------- the FILE SEQ (the seq of group 70) obtains diff of 0 since exhibits are not really numbered
  rank_list = (struct Rank_Pair *)malloc(sizeof(struct Rank_Pair));
  rank_list->seq_no = -100;
  rank_list->rank = 100000000;
  rank_list->next = NULL;
  return 0;
}

int print_rank_list(int nn) {
  fprintf(stderr,"PRINTING RANK LIST:%d:\n",nn);
  struct Rank_Pair *ptr = rank_list;
  while (ptr && ptr->rank > -10000) {
    int seq_no = ptr->seq_no;
    if (seq_no > 0) {
      if (debug) fprintf(stderr,"RRR:sn=%d: rank=%d: gn=%d gap=%d: diff=%d rnoi=%d: DIR=<b>%s</b>  FILE:<b>%s</b>:\n",seq_no,ptr->rank,(seq_no>0)?seq_item_array[seq_no][0].group_no:-100, seq_array[seq_no].total_diff,seq_array[seq_no].total_gap,seq_array[seq_no].real_no_of_items,deal_root_name,file_name_root);
    } else {
      ;
    }
    if (ptr == ptr->next) break;
    ptr = ptr->next;
  }
  fprintf(stderr,"DONE PRINTING RANK LIST:%d:\n",nn);
  return 0;
}

struct Tree_Node *my_new_node(int node_no, struct Tree_Node *parent) {
  if (0 && debug) fprintf(stderr,"   *******NEW_NODE: %d null=%d\n",node_no,(parent==NULL) ? 1 :0);
  struct Tree_Node *ptr = (struct Tree_Node *)malloc(sizeof(struct Tree_Node));
  ptr->seq_no = node_no;
  ptr->no_of_children = 0;
  if (parent) { // s never be NULL
    ptr->level = parent->level+1;
    ptr->anchored = parent->anchored;
    ptr->siblings = NULL;
    ptr->children = NULL;
    ptr->r_siblings = parent->children;
    ptr->parent = parent;
    parent->children = ptr;
  } else {
    if (debug || lev_debug) fprintf(stderr,"<font color=red>Error: NULL parent (child_id=%d)!</font><BR>\n",node_no);
    //exit(0);
    return 0;
  }
  if (debug || lev_debug) fprintf(stderr,"   *******NEW_NODE: seq=%d:%d: lev=%d:%d: gn=%d:%d: null=%d\n",ptr->seq_no, parent->seq_no, ptr->level, parent->level, seq_array[node_no].group_no, seq_array[parent->seq_no].group_no, (parent==NULL) ? 1 :0);  
  return ptr;
} // my_new_node

void print_doc_seq_tree(struct Tree_Node *ptr) { // RECURSIVE
  fprintf(stderr,"DST: seqw_no=%d: leverl=%d: noc=%d:\n",ptr->seq_no, ptr->level, ptr->no_of_children);
  return 0;
}

// ii is included in jj
int make_included(int ii,int jj, int fn1, int fn2, int ln1,int ln2, int incl, int *no_included) {
  int mmm = 1; // (ii == 183 && jj == 166) ? 1 : 0;
  // if (mmm == 1) fprintf(stderr,"        IN_INCL=%d: ij=%d:%d: inc=%d:%d:\n",incl, ii,jj,  seq_array[ii].included,seq_array[ii].include);
  int xx;
  if (0 && mmm == 1) {
    seq_array[ii].include_array[(*no_included)++] = jj;
    incl = 1;
  } else {
  for (xx = 0; xx < seq_array[ii].no_of_items; xx++) {
    //if (mmm == 1) fprintf(stderr,"LLLL0: ii=%d: %d:%d: rem=%d:\n",ii,xx,seq_array[ii].no_of_items,seq_item_array[ii][xx].removed);
    if (testing_removed == 1 || mmm == 1 || seq_item_array[ii][xx].removed == 0) {
      int my_itema = seq_item_array[ii][xx].item_no; // itema is the last not-removed before the removed
      int removed = 1;
      int my_itemb = my_itema;
      int uu = 1;
      while (removed == 1) { // go to the end of all the removed items at the beginning of the sequence
	my_itemb = seq_item_array[ii][xx+uu].item_no;
	removed = seq_item_array[ii][xx+uu].removed;
	uu++;
      } // itemb is the last removed, 
      //fprintf(stderr,"     MAKE_INLUDED:  xu=%d:%d: ia=%d:%s: ib=%d:%s: f12=%d:%d: l12=%d:%d:\n",xx,uu, my_itema, item_array[my_itema].section,  my_itemb, item_array[my_itemb].section, fn1,fn2, ln1,ln2);
      // if (mmm ==1) fprintf(stderr,"     BBBBBBBB0: fl2=%d:%d: iab=%d:%d: \n",fn2,ln2,my_itema,my_itemb);      
      if ((fn2 > my_itema && fn2 < my_itemb)) {
	if (1 || (ln2 > my_itema && ln2 < my_itemb)) { // UZ 01162018 -- perhaps take out the parent and not the son?  doc_id 4124 HUGE BUG!!!
	  if (seq_array[jj].prev_item == 0 || seq_array[jj].prev_item < my_itema) { // in case we have II a b c i ii d, we want "c" and not II to be the prev of "i" 
	    seq_array[jj].prev_item = my_itema;
	  }

	  incl = 1; // it fits perfectly as an insert
	  seq_array[ii].include	= *no_included+1;
	  seq_array[ii].include_array[(*no_included)++] = jj;
	  // if (mmm == 1) fprintf(stderr,"     BBBBBBBB: ii=%d jj=%d: fn12=%d:%d: ln12=%d:%d: incl=%d: ",ii,jj,fn1,fn2,incl,*no_included);
		  
	} else {
	  incl = 2;
	}
      } else {
	// not included
      }
    } // if not removed
  } // for xx
  }
  //fprintf(stderr,"        RETURN_INCL=%d:\n",incl);
  return incl;
} // make_included()

int make_including(int ii,int jj, int fn1, int fn2, int ln1,int ln2, int incl, int *no_including) {
  int xx;
  int mmm = 1; // ((ii == 94 && jj == 117) || (jj == 94 && ii == 117)) ? 1 : 0;
  for (xx = 0; xx < seq_array[jj].no_of_items; xx++) {
    if (testing_removed == 1 ||  mmm == 1 || seq_item_array[jj][xx].removed == 0) {
      int my_itema = seq_item_array[jj][xx].item_no;

      int removed = 1;
      int my_itemb;
      int uu = 1;
      while (removed == 1) {
	my_itemb = seq_item_array[jj][xx+uu].item_no;
	removed = seq_item_array[jj][xx+uu].removed;
	uu++;
      }
      if (fn1 > my_itema && fn1 < my_itemb) { // I. 1.2.3.4 II.,      1 > I, 1 < II
	if (ln1 > my_itema && ln1 < my_itemb) { //  I. 1.2.3.4 II.   4 > I, 4 < II
	  int no_included = seq_array[ii].included;
	  seq_array[ii].included_array[no_included] = jj;
	  seq_array[ii].included++;
	  //seq_array[ii].included_array[(*no_including)++] = jj;
	  incl = 3; // the other is inserted into this one neatly
	} else {
	  int awk_no = seq_array[ii].awk_no;
	  seq_array[ii].awkward_array[awk_no] = jj;
	  seq_array[ii].awk_no++;
	  //if (debug) fprintf(stderr,"HHH ii=%d jj=%d: awk_no=%d: mia=%d, mib=%d fn1=%d, ln1=%d,  fn2=%d, ln2=%d\n",ii,jj,seq_array[ii].awk_no, my_itema, my_itemb, fn1, ln1,fn2,ln2 );
	  incl = 4; // a bit overlap
	  { // remove Exhibit items in the middle of ARTICLE sequence
	    int yy;
	    for (yy = 0; yy < seq_array[ii].no_of_items; yy++) {
	      if (seq_item_array[ii][yy].item_no > my_itema && seq_item_array[ii][yy].item_no < my_itemb) { // find the items on jj surrounding my_itemb
		// if (debug) fprintf(stderr,"YYY:yy=%d in0=%d, in1=%d: s70=%d ee0=%d ee1=%d:\n",yy,seq_item_array[ii][yy].item_no,seq_item_array[ii][yy+1].item_no,SEQ_70	  ,seq_item_array[ii][yy].my_enum,seq_item_array[ii][yy+1].my_enum);
		// URI CHANGE 081417
		if ((SEQ_70 == jj && seq_item_array[ii][yy].my_enum + 1 == seq_item_array[ii][yy+1].my_enum)) { // remove the item only if it's an Exhibit in the middle of a good seq
		  int xx1;
		  for (xx1 = 0; xx1 < seq_array[jj].no_of_items; xx1++) {
		    int in = seq_item_array[jj][xx1].item_no;
		    int pid = item_array[in].pid;
		    int tid = paragraphtoken_array[pid].token_id;
		    int page_no = paragraphtoken_array[pid].page_no;		    
		    int line_no = token_array[tid].line_no;
		    int center = page_line_properties_array[page_no][line_no].center;
		    int next_center = page_line_properties_array[page_no][line_no+1].center;		    
		    int now = page_line_properties_array[page_no][line_no].no_of_words;
		    int noW = page_line_properties_array[page_no][line_no].no_of_first_cap_words;		    		    
		    int next_now = page_line_properties_array[page_no][line_no+1].no_of_words;
		    int next_noW = page_line_properties_array[page_no][line_no+1].no_of_first_cap_words;		    		    

		    int in1 = seq_item_array[jj][xx1+1].item_no;
		    int pid1 = item_array[in1].pid;
		    int tid1 = paragraphtoken_array[pid1].token_id;
		    int page_no1 = paragraphtoken_array[pid1].page_no;		    
		    int line_no1 = token_array[tid1].line_no;
		    int center1 = page_line_properties_array[page_no1][line_no1].center;

		    int in0 = -1;
		    int pid0 = -1;
		    int tid0 = -1;
		    int page_no0 = -1;
		    int line_no0 = -1;
		    int center0 = -1;

		    if (xx1 > 0) {
		      in0 = seq_item_array[jj][xx1-1].item_no;
		      pid0 = item_array[in0].pid;
		      tid0 = paragraphtoken_array[pid0].token_id;
		      page_no0 = paragraphtoken_array[pid0].page_no;		    
		      line_no0 = token_array[tid0].line_no;
		      center0 = page_line_properties_array[page_no0][line_no0].center;
		    }

		    int same_page0 = page_no == page_no0; // if two exhibits on same page then bad
		    int same_page1 = page_no == page_no1; // if two exhibits on same page then bad
		    int top_of_page = line_no;  // if not top of page then bad
		    int centered_with_next = abs(center-next_center) < 4000; // if not centered with next then bad
		    int bad_now = now > 5; // don't like more than 5 words in "EXHIBIT A"
		    int bad_noW = now > noW *2;  // likes words are first cap
		    int bad_next_now = next_now > 10; // don't like more than 5 words in "TENANT BILL OF RIGHTS AND GOVERNANCE"
		    int bad_next_noW = next_now > next_noW * 2; // likes words are first cap
		    int cond_BAD = ((same_page0 +same_page1 + top_of_page + centered_with_next
				     + bad_now + bad_noW + bad_next_now + bad_next_noW) < 4) ? 0 : 1;
		    
		    if ((strcmp(doc_country,"GBR") != 0 ) // in GBR allow crazy SCHEDULE
			&& cond_BAD // remove exhibits if they are in the same page or two many words or not first Caps
			&& seq_item_array[jj][xx1].item_no > seq_item_array[ii][yy].item_no
			&& seq_item_array[jj][xx1].item_no < seq_item_array[ii][yy+1].item_no) {
		      seq_item_array[jj][xx1].removed = 1;
		      if (debug) fprintf(stderr,"REMOVING3:%d:%d:\n",jj,xx1);		      
		      int in= seq_item_array[jj][xx1].item_no;
		      item_array[in].selected_seq = -1;
		    }
		    if (0 && debug) fprintf(stderr,"           ---JJJ BAD=%d: xx1=%d jj=%d in0=%d in1=%d in2=%d  pid=%d: token=%d: center1n2=%d:%d:%d: line123=%d: p123=%d:%d:%d:\n"
				       , cond_BAD
				       , xx1,jj,seq_item_array[ii][yy].item_no
				       , seq_item_array[jj][xx1].item_no
				       , seq_item_array[ii][yy+1].item_no
				       , pid
				       , tid
				       , center
				       , next_center
				       , center1				       
				       , line_no
				       , page_no 
				       , page_no0 
				       , page_no1 
				       );
		  }
		  //seq_item_array[jj][xx+uu-1].removed = 1;
		}
	      }
	    }
	  }
	}
      }
    } // if not removed
  } // for xx
  return incl;
} // make_including()

int mark_include_relations() {
  seq_array[SEQ_70].total_diff = 0; // <-------- the FILE SEQ (the seq of group 70) obtains diff of 0 since exhibits are not really numbered
  //init_rank_list(); // create the top node in the linked list (seq70);  we are adding items to ranks_array at the bottom of the function
  int ii,jj;
  // a separate loop for finding overlaps and other relations between seqs, now that extraneous items were removed
  if (debug) fprintf(stderr,"MARKING SEQ RELATIONS:%d %d:\n",g_seq_no, MAX_SEQ);
  for (ii = 0; ii <= g_seq_no && ii < MAX_SEQ; ii++) { // SEQ II
    for (jj = 0; jj < NO_INC; jj++) { // initialize
      seq_array[ii].incl_total[jj] = 0; // initialize the incl_array
    }
    // determine the seq relations
    int item_no1 = seq_array[ii].no_of_items+1;
    int real_item_no1 = seq_array[ii].real_no_of_items;

    if (debug) fprintf(stderr,"   MARK SEQ RELATIONS: seq1=%d tin=%d:%d: fln=%d:%d:\n",ii,item_no1,real_item_no1, seq_array[ii].fn, seq_array[ii].ln);
    if (item_no1 > 1 && (real_item_no1 > 1 || ii == SEQ_70) && seq_array[ii].fn > -1 && seq_array[ii].ln > -1) { // actually this means 2 items and more

      int incl = 0;
      int fn1 = seq_array[ii].fn;
      int ln1 = seq_array[ii].ln;
      int no_included = 0;
      int no_including = 0;
      int no_over = 0;
      for (jj = 0; jj <= g_seq_no; jj++) { // vs SEQ JJ
	if (seq_array[jj].no_of_items >= 0 && seq_array[jj].fn > -1 && seq_array[jj].ln > -1) {
	  if (debug) fprintf(stderr,"              MARK SEQ RELATIONS5: seq1=%d seq2=%d:\n",ii,jj);
	  int item_no2 = seq_array[jj].no_of_items+1;
	  int fn2 = seq_array[jj].fn;
	  int ln2 = seq_array[jj].ln;

	  int gn1 = seq_array[ii].group_no;
	  int gn2 = seq_array[jj].group_no;

	  int cn1 = seq_array[ii].mean_center;
	  int cn2 = seq_array[jj].mean_center;	  
	  int scn1 = seq_array[ii].std_center;
	  int scn2 = seq_array[jj].std_center;	  


	  int id1 = seq_array[ii].mean_left_X;
	  int id2 = seq_array[jj].mean_left_X;	  
	  int sid1 = seq_array[ii].std_left_X;
	  int sid2 = seq_array[jj].std_left_X;	  

	  int consider_ARTICLE = 0; // used to be 0, now trying 1, UZ 082017
	  /*************************************************/
	  int real_item_no2 = seq_array[jj].real_no_of_items;

	  char *t1 = item_array[seq_array[ii].fn].title;
	  char *t2 = item_array[seq_array[jj].fn].title;	  

	  char *s1 = item_array[seq_array[ii].fn].section;
	  char *s2 = item_array[seq_array[jj].fn].section;

	  int sv1 = item_array[seq_array[ii].fn].section_v[0];	  

	  if (0) fprintf(stderr,"   IIIIIIII55:i=%d: j=%d: fl1=%d:%d:  fl2=%d:%d: tt=%s:%s:%s:%s\n"  ,ii,jj, fn1,ln1, fn2,ln2, s1,s2, t1, t2);
	  if (fn1 < fn2 && ln1 > ln2) { // 2 IS INCLUDED IN 1
	    incl = make_included(ii,jj,fn1,fn2,ln1,ln2,incl,&no_included);
	    if (debug) fprintf(stderr,"       INCLUDED:ij=%d:%d: incl=%d: no_incl=%d:%d: fn=%d:%d: ln=%d:%d: gn=%d:%d:  cn=%d:%d: %d:%d:  id=%d:%d: %d:%d: tit=%s:%s: sec=%s:%s:%d:\n" ,ii,jj, incl, no_included, seq_array[ii].include, fn1,fn2,  ln1,ln2, gn1,gn2, cn1,cn2, scn1,scn2, id1,id2, sid1,sid2, t1,t2, s1,s2,sv1);	    

	  } else if (fn1 > fn2 && ln1 < ln2) { // 1 IS INCLUDED IN 2  Articles are always "included" in Exhibit
	    incl = make_including(ii,jj,fn1,fn2,ln1,ln2,incl,&no_including);
	    if (debug) fprintf(stderr,"         INCLUDING:ij=%d:%d: incl=%d: fn=%d:%d: ln=%d:%d: gn=%d:%d:  cn=%d:%d: %d:%d:  id=%d:%d: %d:%d: tit=%s:%s: sec=%s:%s:\n" 
			       ,ii,jj, incl,   fn1,fn2,  ln1,ln2, gn1,gn2, cn1, cn2, scn1,scn2, id1,id2, sid1,sid2, t1,t2, s1,s2);	    
	  } else if (ln1 < fn2) { // 1 IS ABOVE 2
	    //fprintf(stderr,"IIIIIIII5:ij=%d:%d: fn=%d:%d: ln=%d:%d: gn=%d:%d:  cn=%d:%d: %d:%d:  id=%d:%d: %d:%d: tit=%s:%s: sec=%s:%s:\n" ,ii,jj,   fn1,fn2,  ln1,ln2, gn1,gn2, cn1, cn2, scn1,scn2, id1,id2, sid1,sid2, t1,t2, s1,s2);	    
	    incl = 5;

	  } else if (ln2 < fn1) { // 2 IS ABOVE 1
	    //fprintf(stderr,"IIIIIIII6:ij=%d:%d: fn=%d:%d: ln=%d:%d: gn=%d:%d:  cn=%d:%d: %d:%d:  id=%d:%d: %d:%d: tit=%s:%s: sec=%s:%s:\n" ,ii,jj,   fn1,fn2,  ln1,ln2, gn1,gn2, cn1, cn2, scn1,scn2, id1,id2, sid1,sid2, t1,t2, s1,s2);	    	    
	    incl = 6;

	  } else if (fn2 > fn1 && ln2 > ln1) { // 2 OVERLAPS ABOVE 1
	    if (consider_ARTICLE && (seq_array[ii].group_no == 51 || seq_array[ii].group_no == 2)) { // 1 is ARTICLE
	      incl = make_including(ii,jj,fn1,fn2,ln1,ln2,incl,&no_including);
	      if (debug) fprintf(stderr,"OVERLAP4, inc=%d: %d:%d:\n",incl,seq_array[ii].group_no,seq_array[jj].group_no);

	    } else if (consider_ARTICLE && (seq_array[jj].group_no == 51 || seq_array[ii].group_no == 2)) { // 2 is ARTICLE
	      incl = make_included(ii,jj,fn1,fn2,ln1,ln2,incl,&no_included);
	      if (debug) fprintf(stderr,"OVERLAP6, inc=%d: %d:%d:\n",incl,seq_array[ii].group_no,seq_array[jj].group_no);

	    } else if (strlen(t1) > 2 && strlen(t2) < 2) { // title1 vs no_title2
	      incl = make_including(ii,jj,fn1,fn2,ln1,ln2,incl,&no_including);
	      fprintf(stderr,"OVERLAP4, inc=%d: %d:%d:\n",incl,seq_array[ii].group_no,seq_array[jj].group_no);
	      if (debug) fprintf(stderr,"              OVERLAP4:ij=%d:%d: incl=%d: fn=%d:%d: ln=%d:%d: gn=%d:%d:  cn=%d:%d: %d:%d:  id=%d:%d: %d:%d: tit=%s:%s: sec=%s:%s:\n" ,ii,jj, incl,   fn1,fn2,  ln1,ln2, gn1,gn2, cn1, cn2, scn1,scn2, id1,id2, sid1,sid2, t1,t2, s1,s2);	    	      	      	      

	    } else {
	      incl = 7;
	      //fprintf(stderr,"OVERLAP1, inc=%d: %d:%d:\n",incl,seq_array[ii].group_no,seq_array[jj].group_no);
	      seq_array[ii].over_array[no_over++] = jj;
	      if (debug) fprintf(stderr,"            OVERLAP:ij=%d:%d: incl=%d: fn=%d:%d: ln=%d:%d: gn=%d:%d:  cn=%d:%d: %d:%d:  id=%d:%d: %d:%d: tit=%s:%s: sec=%s:%s:\n" ,ii,jj, incl,   fn1,fn2,  ln1,ln2, gn1,gn2, cn1, cn2, scn1,scn2, id1,id2, sid1,sid2, t1,t2, s1,s2);	    	      
	    }
	  } else if (fn1 > fn2 && ln1 > ln2) { //1 OVERLAPS ABOVE 2
	    if (consider_ARTICLE && (seq_array[ii].group_no == 51 || seq_array[ii].group_no == 2)) { // ARTICLE or section 2.1
	      incl = make_including(ii,jj,fn1,fn2,ln1,ln2,incl,&no_including);
	      if (debug) fprintf(stderr,"              OVERLAP3, inc=%d: %d:%d:\n",incl,seq_array[ii].group_no,seq_array[jj].group_no);

	    } else if (consider_ARTICLE && (seq_array[jj].group_no == 51  || seq_array[ii].group_no == 2)) { // ARTICLE
	      incl = make_included(ii,jj,fn1,fn2,ln1,ln2,incl,&no_included);
	      if (debug) fprintf(stderr,"OVERLAP7, inc=%d: %d:%d:\n",incl,seq_array[ii].group_no,seq_array[jj].group_no);

	    } else if (strlen(t1) < 2 && strlen(t2) > 2) { // no_title1 vs title2
	      incl = 8;
	      seq_array[ii].over_array[no_over++] = jj;	      
	      if (debug) fprintf(stderr,"OVERLAP71, inc=%d: %d:%d:\n",incl,seq_array[ii].group_no,seq_array[jj].group_no);
	      if (debug) fprintf(stderr,"IIIIIIII7:ij=%d:%d: incl=%d: fn=%d:%d: ln=%d:%d: gn=%d:%d:  cn=%d:%d: %d:%d:  id=%d:%d: %d:%d: tit=%s:%s: sec=%s:%s:\n" ,ii,jj, incl,   fn1,fn2,  ln1,ln2, gn1,gn2, cn1, cn2, scn1,scn2, id1,id2, sid1,sid2, t1,t2, s1,s2);	    	      	      

	    } else {
	      incl = 8;
	      //fprintf(stderr,"OVERLAP2, inc=%d: %d:%d:\n",incl,seq_array[ii].group_no,seq_array[jj].group_no);
	      seq_array[ii].over_array[no_over++] = jj;
	      if (debug) fprintf(stderr,"                    IIIIIIII8:ij=%d:%d: incl=%d: fn=%d:%d: ln=%d:%d: gn=%d:%d:  cn=%d:%d: %d:%d:  id=%d:%d: %d:%d: tit=%s:%s: sec=%s:%s:\n" ,ii,jj, incl,   fn1,fn2,  ln1,ln2, gn1,gn2, cn1, cn2, scn1,scn2, id1,id2, sid1,sid2, t1,t2, s1,s2);	    	      	      
	    }

	  } else if (fn1 == fn2 && ln1 == ln2) { // 2 IS IDENTICAL TO 1

	    //fprintf(stderr,"IIIIIIII9:ij=%d:%d: fn=%d:%d: ln=%d:%d: gn=%d:%d:  cn=%d:%d: %d:%d:  id=%d:%d: %d:%d: tit=%s:%s: sec=%s:%s:\n" ,ii,jj,   fn1,fn2,  ln1,ln2, gn1,gn2, cn1, cn2, scn1,scn2, id1,id2, sid1,sid2, t1,t2, s1,s2);	    	      	    
	    incl = 9;

	  } else {
	    incl = 10;
	    if (0 && debug) fprintf(stderr,"IIIIIIIIA:ij=%d:%d: fn=%d:%d: ln=%d:%d: gn=%d:%d:  cn=%d:%d: %d:%d:  id=%d:%d: %d:%d: tit=%s:%s: sec=%s:%s:\n" ,ii,jj,   fn1,fn2, incl,  ln1,ln2, gn1,gn2, cn1, cn2, scn1,scn2, id1,id2, sid1,sid2, t1,t2, s1,s2);
	  }
	  seq_array[ii].incl_total[incl]++;
	} // if jj
      } // for jj
      // is it included awkwardly in some seqs
      int curr_awkward = seq_array[ii].incl_total[4];
      if (max_awkward < curr_awkward) {
	max_awkward = curr_awkward;
	max_awkward_seq = ii;
      }

      // how many seqs are included OK in this one?
      int curr_include = seq_array[ii].incl_total[1];
      if (max_include < curr_include) {
	max_include = curr_include;
	max_include_seq = ii;
      }

      // how many seqs overlap with this one
      int curr_overlap = seq_array[ii].incl_total[7] + seq_array[ii].incl_total[8];
      if (max_overlap < curr_overlap) {
	max_overlap = curr_overlap;
	max_overlap_seq = ii;
      }

      seq_array[ii].awkward = seq_array[ii].incl_total[4];
      //seq_array[ii].include = seq_array[ii].incl_total[1];
      //seq_array[ii].included = seq_array[ii].incl_total[3];
      seq_array[ii].overlap = seq_array[ii].incl_total[7] + seq_array[ii].incl_total[8];
    } else {  // if ii
      //fprintf(stderr,"Cancelled BAD2 seq:%d:\n",ii);
    }
    
    // calculate the range -- not used anywhere
    seq_array[ii].range = seq_item_array[ii][seq_array[ii].no_of_items].item_no - seq_item_array[ii][0].item_no;
    // calculate ranking
    // NOI is downgaded /5 since we don't want a useless long seq to take over
    // this formula attempts to place our top seq first and any bad overlapping seqs towards the end

    int curr_rank = seq_array[ii].rank = seq_array[ii].include - seq_array[ii].overlap * 3 - ((seq_array[ii].no_of_items == 0) ? 0 : (float)(seq_array[ii].awkward  + seq_array[ii].total_diff + seq_array[ii].total_gap ) * 5 / (float)seq_array[ii].no_of_items);  // <------------------------------------------ THE RANKING FORMULA!!!
    if (debug) fprintf(stderr,"RANKK: ii=%d: rank=%d: (include=%d: - overlap=%d: * 3  - %2.2f=(awkward=%d:  + total_diff=%d: + total_gap=%d:) * 5 / noi=%d:)"
	    , ii, curr_rank
	    , seq_array[ii].include, seq_array[ii].overlap
	    ,(float)(seq_array[ii].awkward  + seq_array[ii].total_diff + seq_array[ii].total_gap ) * 5 / (float)seq_array[ii].no_of_items
	    , seq_array[ii].awkward, seq_array[ii].total_diff, seq_array[ii].total_gap, seq_array[ii].no_of_items);
    if (ii == SEQ_70) {
      //curr_rank = seq_array[ii].rank = 10000; // <------ special treatment of GROUP 70
    }
    //add_to_rank_list(ii,curr_rank);
    if (max_rank < curr_rank) {
      max_rank = curr_rank;
      max_rank_seq = ii;
    }
  } // for ii
  for (ii = 0; ii <= g_seq_no && ii < MAX_INCLUDED; ii++) { // SEQ II
    if (debug) fprintf(stderr, "    INCLUDE IN: ii=%d:%d:--", ii, seq_array[ii].include);
    for (jj = 0; jj < seq_array[ii].include; jj++) { // SEQ II
      if (debug) fprintf(stderr, ":%d:%d:",jj,seq_array[ii].include_array[jj]);
    }
    if (debug) fprintf(stderr, "    INCIN\n");
  }
  return 0;
} // mark_include_relations()


#define MAX_HEADER_WORD_NO 100
char header_word[MAX_HEADER_WORD_NO][40] = {"lease", "rent", "rental", "compliance", "parking", "brokerage", "forcemajeure", "estoppel", "waiver", "warranty", "improvements", "guaranty", "guarantee", "alterations", "utilities", "miscellaneous", "term", "basic", "terms", "premises", "improvements", "realestate","taxes", "cam", "common", "insurance", "use", "maintenance", "hazardous", "material", "signange", "indemnification", "indemnity", "destruction", "eminent", "default", "notices", "subordination", "entire", "assignment", "subletting", "mortgaging", "security", "construction", "provisions"};

char *to_lower_and_chop_line(char *text, int len) {
   int ii;
   char static bext[5000];
   int rlen = strlen(text);
   for (ii = 0; ii < rlen && ii < len; ii++) {
     bext[ii] = tolower(text[ii]);
     if (text[ii] == '\n') {
       bext[ii] = ' ';
     }
   }
   bext[ii] = '\0';
   return bext;
} // to_lower_and_chop_line(char *text, int len) 


  /* features:
  ** 1.  span / number of sections/ number of pages
  ** 2.  sections: roman / number
  ** 3.  children 1.2, etc
  ** 4.  seq quality
  ** 5.  header style
  ** 6.  header names (lease / rent / miscellaneous / term / basic terms / premises / improvements / taxes /cam / insurance /use / maintenance /hazardous material / signange / indemnification / destruction / eminent / default /notices / subordination / entire)
  */

int correct_main_seq() {
  int ii;
  int kk;
  float max_total;
  int main_seq;

  for (ii = 0; ii < g_seq_no;  ii++) {
    int total_header_words = 0;
    int total_no_of_words = 0;
    int total_no_of_WORDS = 0;
    int total_no_of_Words = 0;        
    int noi = seq_array[ii].no_of_items;
    int rnoi = seq_array[ii].real_no_of_items;    
    int jj;
    int first_page = -1;
    int last_page = -1;
    int min_page = 10000;
    int max_page = -1;
    int has_title = 0;
    int good_group = (seq_array[ii].group_no == 1
			|| seq_array[ii].group_no == 8 
			|| seq_array[ii].group_no == 10
			|| seq_array[ii].group_no == 11
			|| seq_array[ii].group_no == 20
			|| seq_array[ii].group_no == 41
			|| seq_array[ii].group_no == 48
			|| seq_array[ii].group_no == 50
			|| seq_array[ii].group_no == 51
			|| seq_array[ii].group_no == 58
			|| seq_array[ii].group_no == 60 
			|| seq_array[ii].group_no == 61) ? 1 : 0;
    for (jj = 0; jj < noi; jj++) {
      int kk;
      int len_kk = 1;
      int in = seq_item_array[ii][jj].item_no;
      total_no_of_words += item_array[in].no_of_words;
      total_no_of_Words += item_array[in].no_of_first_Caps_words;
      total_no_of_WORDS += item_array[in].no_of_all_CAPs_words;

      has_title += (item_array[in].title && strlen(item_array[in].title) > 1) ? 1 : 0;

      if (jj == 0) first_page = item_array[in].page_no;
      if (jj == noi-1) last_page = item_array[in].page_no;
      max_page = MAX(item_array[in].page_no, max_page);
      min_page = MIN(item_array[in].page_no, min_page);      
      char *header = item_array[in].header;
      char *header_jj = to_lower_and_chop_line(header,100);
      for (kk = 0; len_kk > 0 && kk < MAX_HEADER_WORD_NO; kk++) {
	len_kk = strlen(header_word[kk]);
	if (header_jj && header_word[kk] && len_kk > 0) {
	  char *same = strstr(header_jj, header_word[kk]);
	  if (same) {
	    char *pp_after = same+strlen(header_word[kk]);
	    int mm_aft = (isalpha(pp_after[0]) > 0) ? 1 : 0; // make sure it's not "amusement" for "use"
	    int mm_bef = (isalpha(same[-1]) > 0) ? 1 : 0; // make sure it's not "abuse" for "use"
	    if (mm_aft == 0 && mm_bef == 0) total_header_words++;
	    //fprintf(stderr,"                         HHH4 ii=%d:%d: jj=%d: kk=%d: hd=%s:------:%s:  :%10s:---:%s:%d: bef=%d:\n",  ii,  total_ii, jj,kk,    header_word[kk],    header_jj, same, pp_after,mm_aft,mm_bef);
	  }
	}
      }
    } // jj
    if (noi > 2 && total_header_words > 0 && ii != SEQ_70) {
      float grand_total = (0
			   + 10 * log(noi+1) - ((noi <= 3) ? 20 : 0) // the length
			   + 10 * (log(total_header_words+1) - log(noi+1)) // header words such as Lease, Term, Rent, Security, CAM, Indemnity 
			   + (log(seq_array[ii].include+1)) // no of seqs included in this one
			   - (log(seq_array[ii].included +1)) // no of seqs including this one
			   + (log(total_no_of_Words+1) - log(total_no_of_words+1)) // no of all CAPs
			   + (log(total_no_of_WORDS+1) - log(total_no_of_words+1)) // no of first Caps
			   + 5 * (log(max_page-min_page+1)) // pages spanned
			   + 2 * (log(has_title+1) - log(noi+1)) // titles such as article / section
			   - 15 * ((good_group == 0) ? 1 : 0)  // 1. I. A.
			   );
      fprintf(stderr
	      ,"         HHH0 seq=%3d: tot=%2.2f: noi=%d:%d: words=%d:(%2.2f) centered:%d: include(d):%d:%d:(%2.2f:%2.2f)  tot=%d:%d:%d:(%2.2f:%2.2f) fp=%d:%d: lp=%d:%d: nop=%d:(%2.2f) title:%d:(%2.2f) good_group=%d:\n"
	      , ii
	      , grand_total
	      , noi,rnoi, total_header_words, (log(total_header_words+1) - log(noi+1))
	      , seq_array[ii].centered
	      , seq_array[ii].include, seq_array[ii].included, log(seq_array[ii].include+1), (log(seq_array[ii].included +1))
	      , total_no_of_words, total_no_of_Words, total_no_of_WORDS, (log(total_no_of_Words+1) - log(total_no_of_words+1)), (log(total_no_of_WORDS+1) - log(total_no_of_words+1))
	      , first_page, min_page, last_page, max_page, max_page-min_page+1,  (log(max_page-min_page+1))
	      , has_title, (log(has_title+1) - log(noi+1)), good_group);
      seq_array[ii].header_words = total_header_words;
      if (max_total < grand_total) {
	max_total = grand_total;
	main_seq  = ii;
      }
    }
  } // ii
  // if the main level is hidden (this happens in 4236 b/c A is connected to C non-locally and including all of main_seq
  fprintf(stderr,"MAIN_SEQ: %d: lev=%d:%d: :%2.2f:\n",main_seq, (child_triple_array[main_seq].ptr)?child_triple_array[main_seq].ptr->level:-1, seq_array[main_seq].level, max_total);

  //if (!child_triple_array[main_seq].ptr || child_triple_array[main_seq].ptr->level != main_level) { // USED TO BE UZ 10/23/2022
  if (child_triple_array[main_seq].ptr && child_triple_array[main_seq].ptr->level != main_level) {    
    fprintf(stderr,"FIXING MAIN_SEQ: %d: lev=%d:%d: :%2.2f:\n",main_seq, (child_triple_array[main_seq].ptr)?child_triple_array[main_seq].ptr->level:-1, seq_array[main_seq].level, max_total);
    int a1 = seq_array[main_seq].level = main_level;
    fprintf(stderr,"FIXING MAIN_SEQ0:    \n");
    child_triple_array[main_seq].ptr->level = a1;
    fprintf(stderr,"FIXING MAIN_SEQ1:    \n");
    rearrange_nested_seqs(main_seq, 0); // THIS IS DONE RELATIVE TO THE MAIN_SEQ
    fprintf(stderr,"FIXING MAIN_SEQ2:    \n");    
  }
  fprintf(stderr,"FIXING MAIN_SEQ3:    \n");  
  return 0;
} // correct_main_seq() 

