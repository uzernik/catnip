#include <math.h> 
#include <mysql.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <mysql.h>
#include <stdlib.h>	/* for bsearch() */
#include <calc_page_params.h>


int wrap_LD(char *line_buff1, char *line_buff2); 
/*************************************  FOOTER *******************************************/
/* this module deals with footer identification */
#define MAX_OL (MAX_PAGE*MAX_PAGE)
//int diff_footer_matrix[MAX_PAGE][MAX_PAGE];
struct Diff_Triple {
  int ll, kk; // first block
  int ll_page, kk_page; // the page
  float ol;
} ol_footer_list[MAX_OL], ol_header_list[MAX_OL];;
int ol_footer_no = 0;
int ol_header_no = 0;

int ppp_ol_footer_cmp(const void *a, const void *b) {
  float x1 = (((struct Diff_Triple *)a)->ol);
  float x2 = (((struct Diff_Triple *)b)->ol);
  return((x1>x2) ? 0 : 1); 
}

int ppp_ol_header_cmp(const void *a, const void *b) {
  float x1 = (((struct Diff_Triple *)a)->ol);
  float x2 = (((struct Diff_Triple *)b)->ol);
  return((x1>x2) ? 0 : 1); 
}

int sort_ol_footer_pairs() {
  qsort((void *)&ol_footer_list[0], ol_footer_no,  sizeof(struct Diff_Triple), ppp_ol_footer_cmp);
  return 0;
}

int sort_ol_header_pairs() {
  qsort((void *)&ol_header_list[0], ol_header_no,  sizeof(struct Diff_Triple), ppp_ol_header_cmp);
  return 0;
}



// assuming page is scanned vertically
#define THRESH_ALL 0.100
#define DIFF_FH_THRESHOLD THRESH_ALL
#define ADD_FH_THRESHOLD THRESH_ALL
#define MERGE_FH_THRESHOLD THRESH_ALL


int is_page_number_f(int pp, int bk, char *text) { // page 5, or -3- or 15 or IX or ii,  MUST BE FOR bottom lines ONLY
  static char bext[5000];
  int len = strlen(text);
  char *found_page = strcasestr(text,"page");
  int is_roman = 0;
  int is_number = 0;
  if (!found_page) {
    int ii, jj;
    for (ii = 0, jj = 0; ii < len; ii++) {
      if (isalnum(text[ii]) != 0) {
	bext[jj++];
      } 
    }
    bext[jj++] = '\0';
    if (jj < 4) {
      is_roman = 1;
      is_number = 1;
      for (ii = 0; ii < jj; ii++) {
	if (bext[ii] != 'i' && bext[ii] != 'I' && bext[ii] != 'L' && bext[ii] != 'v' && bext[ii] != 'V' && bext[ii] != 'x' && bext[ii] != 'X') {
	  is_roman = 0;
	}
	if (isalpha(bext[ii]) == 1) { // enough that one char is alpha then its not number
	  is_number = 0;
	}
      }
    }
  }
  int x2 = block_array[bk].my_x2;
  int x1 = block_array[bk].my_x1;
  int ret = ((!found_page && is_number == 0 && is_roman == 0) || len > 8 || x1 < 4000 || x2 > 6000) ? 0 : 1;
  if (pp == 13) fprintf(stderr,"JJJ: ret=%d: fp=%s: len=%d: isn=%d: ir=%d: t=%s: x=%d:%d: bk=%d:\n", ret, found_page, len,  is_number, is_roman, text, x1, x2, bk);
  return ret;
} // is_page_number_f


int is_exhibit_f(char *text) { // EXHIBIT on the TOP of the page is an exhibit
  int ret = (strcasestr(text,"exhibit") != NULL || strcasestr(text,"schedule") != NULL || strcasestr(text,"appendix") != NULL);
  return ret;
}

#define OL_FOOTER_THRESHOLD 0.1
#define OL_HEADER_THRESHOLD 0.1
float my_is_overlap(int pp1, int pp2, int mm, int kk, int d_x, int d_y) {
  float ret = 0;
  int x1_mm = block_array[mm].my_x1+d_x;
  int x2_mm = block_array[mm].my_x2+d_x;    
  int y1_mm = block_array[mm].my_y1+d_y;
  int y2_mm = block_array[mm].my_y2+d_y;    
  float mm_area = (float)((x2_mm - x1_mm) * (y2_mm - y1_mm));

  int x1_kk = block_array[kk].my_x1;
  int x2_kk = block_array[kk].my_x2;      
  int y1_kk = block_array[kk].my_y1;
  int y2_kk = block_array[kk].my_y2;      
  float kk_area = (float)((x2_kk - x1_kk) * (y2_kk - y1_kk));

  int y_edge =
    (y2_kk >= y2_mm && y1_mm >= y1_kk) ? (y2_mm - y1_mm) // total inclusion mm inside kk
    : (y2_mm >= y2_kk && y2_kk >= y1_mm && y1_mm >= y1_kk) ? (y2_kk - y1_mm) // portrusion of mm on right
    : (y1_mm <= y1_kk && y1_kk <= y2_mm && y2_mm <= y2_kk) ? (y2_mm - y1_kk) // portrusion of mm on left
    : (y2_mm >= y2_kk && y1_kk >= y1_mm) ? (y2_kk - y1_kk) // total inclusion kk inside mm
    : 0;
  int x_edge =
    (x2_kk >= x2_mm && x1_mm >= x1_kk) ? (x2_mm - x1_mm) // total inclusion
    : (x2_mm >= x2_kk && x2_kk >= x1_mm && x1_mm >= x1_kk) ? (x2_kk - x1_mm) // portrusion on right
    : (x1_mm <= x1_kk && x1_kk <= x2_mm && x2_mm <= x2_kk) ? (x2_mm - x1_kk) // portrusion on left    
    : (x2_mm >= x2_kk && x1_kk >= x1_mm) ? (x2_kk - x1_kk) // total inclusion rare    
    : 0;
  float overlap_area = x_edge * y_edge;
  ret = (overlap_area * overlap_area) / (kk_area * mm_area);
  if (pp1 == 1000) fprintf(stderr,"    BOX: ret=%4.2f: mk=%d:%d:  mkarea=%4.0f:%4.0f: sq=%4.0f: olarea=%4.0f: sq=%4.0f: xyedge=%d:%d:  xm=%d:%d: xk=%d:%d: ym=%d:%d: yk=%d:%d:\n"
		 ,   ret,   mm, kk
		 ,   mm_area, kk_area, mm_area * kk_area
		 ,   overlap_area, overlap_area * overlap_area
		 ,   x_edge,y_edge
		 ,   x1_mm, x2_mm
		 ,   x1_kk, x2_kk
		 ,   y1_mm, y2_mm
		 ,   y1_kk, y2_kk);
  return ret;
} // my_is_overlap()

float MAX9(float a0, float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8) {
  float max_a= -1;
    max_a = MAX(a0, a1);
    max_a = MAX(max_a,a2);
    max_a = MAX(max_a,a3);
    max_a = MAX(max_a,a4);
    max_a = MAX(max_a,a5);
    max_a = MAX(max_a,a6);
    max_a = MAX(max_a,a7);
    max_a = MAX(max_a,a8);
  return max_a;
} // MAX9()


char *normalize(char *text) {
  static char bext[5000];
  if (!text)  {
    strcpy(bext,"");
  } else {
    int jj, ii;
    for (ii = 0, jj = 0; ii < strlen(text); ii++) {
      if (isalnum(text[ii]) != 0) {
	bext[jj++] = text[ii];
      }
    }
    bext[jj++] = '\0';
  }
  return strdup(bext);
}
    


// with wide blocks OL is not enough, we need to test also  HORIZ_SHIFT
#define HORIZ_SHIFT_LIMIT 200
// with wide blocks OL is not enough, we need to test also  HORIZ_SHIFT
#define VERT_SHIFT_LIMIT 150

// allowing tolerance DD of 100 so if the pages were scanned with a shift.
// adds FPs
#define DD 100
// testing max 10 blocks at the top and 10 blocks at the bottom
#define NO_OF_TESTED_BLOCKS 10
int create_ol_footer_array(int no_of_pages) {
  int pp1;
  for (pp1 = 1; pp1 <= no_of_pages; pp1++) {
    int last_block_mm = page_properties_array[pp1].last_block_in_page;

    int ll1;
    fprintf(stderr,"F NEW PAGE: pn=%d: no_of_pages=%d: last_bl=%d:%d:\n",pp1,no_of_pages, last_block_mm, NO_OF_TESTED_BLOCKS);

    for (ll1 = last_block_mm; ll1 > 0 && ll1 > last_block_mm-NO_OF_TESTED_BLOCKS; ll1--) {
      if (pp1 == 13) fprintf(stderr,"\n    GGGRRR pp=%d: ll1=%d: last_block=%d: x12=%d:%d: y12=%d:%d:  t=%s: desig=%d:\n", pp1, ll1, last_block_mm
		, block_array[ll1].my_x1, block_array[ll1].my_x2, block_array[ll1].my_y1, block_array[ll1].my_y2
		, block_array[ll1].text, block_array[ll1].is_header_footer);
      if (block_array[ll1].my_y2 > 5000 && block_array[ll1].is_header_footer != ENUM_DESIGNATION) { // so on an empty page we don't start grabbing headers
	char *text_mm = block_array[ll1].text;
	char *normalized_text_mm = normalize(text_mm);	
	int len_mm = (normalized_text_mm) ? strlen(normalized_text_mm) : 0;
	int pp2;
	fprintf(stderr,"\nPPPf pp=%d: ll1=%d: last_block=%d: x12=%d:%d: y12=%d:%d:  t=%s: desig=%d: len=%d:\n", pp1, ll1, last_block_mm
		, block_array[ll1].my_x1, block_array[ll1].my_x2, block_array[ll1].my_y1, block_array[ll1].my_y2
		, block_array[ll1].text, block_array[ll1].is_header_footer, len_mm);

	for (pp2 = pp1+1; pp2 <= pp1+3; pp2++) {
	  int last_block_kk = page_properties_array[pp2].last_block_in_page;
	  int first_block_kk = page_properties_array[pp2].first_block_in_page;	  
	  int ll2;

	  int failures = 0; // DON'T ALLOW A DEEPER FOOTER IF LOWER ONES FAILED
	  for (ll2 = last_block_kk; ll2 > 0 && ll2 > last_block_kk - NO_OF_TESTED_BLOCKS && ll2 > first_block_kk; ll2--) {
	    if (block_array[ll2].my_y2 > 5000 && block_array[ll2].is_header_footer != ENUM_DESIGNATION) {	   // so on an empty page we don't start grabbing headers
	      char *text_kk = block_array[ll2].text;
	      char *normalized_text_kk = normalize(text_kk);
	      int len_kk = (normalized_text_kk) ? strlen(normalized_text_kk) : 0;
	      float overlap_00 = my_is_overlap(pp1, pp2, ll1, ll2, 0 , 0); // the discrepancy -- not relative to the size
	      float overlap_01 = my_is_overlap(pp1, pp2, ll1, ll2, 0 , DD); // the discrepancy -- not relative to the size
	      float overlap_02 = my_is_overlap(pp1, pp2, ll1, ll2, 0 , -1*DD); // the discrepancy -- not relative to the size	      	      

	      float overlap_10 = my_is_overlap(pp1, pp2, ll1, ll2, DD , 0); // the discrepancy -- not relative to the size
	      float overlap_11 = my_is_overlap(pp1, pp2, ll1, ll2, DD , DD); // the discrepancy -- not relative to the size
	      float overlap_12 = my_is_overlap(pp1, pp2, ll1, ll2, DD , -1*DD); // the discrepancy -- not relative to the size	      	      

	      float overlap_20 = my_is_overlap(pp1, pp2, ll1, ll2, -1*DD , 0); // the discrepancy -- not relative to the size
	      float overlap_21 = my_is_overlap(pp1, pp2, ll1, ll2, -1*DD , DD); // the discrepancy -- not relative to the size
	      float overlap_22 = my_is_overlap(pp1, pp2, ll1, ll2, -1*DD , -1*DD); // the discrepancy -- not relative to the size	      

	      if (pp1 == 13) fprintf(stderr, "MRR: :pp2=%d: ll2=%d: y2=%d: ed=%d:%d: t=%s:\n", pp2, ll2,   block_array[ll2].my_y2, block_array[ll2].is_header_footer, ENUM_DESIGNATION, text_kk);


	      if (0 // allowing shift in all directions
		  || overlap_00 > OL_FOOTER_THRESHOLD
		  || overlap_01 > OL_FOOTER_THRESHOLD
		  || overlap_02 > OL_FOOTER_THRESHOLD

		  || overlap_10 > OL_FOOTER_THRESHOLD
		  || overlap_11 > OL_FOOTER_THRESHOLD
		  || overlap_12 > OL_FOOTER_THRESHOLD		  

		  || overlap_20 > OL_FOOTER_THRESHOLD
		  || overlap_21 > OL_FOOTER_THRESHOLD
		  || overlap_22 > OL_FOOTER_THRESHOLD		  
		  ) {

		int LD = wrap_LD(normalized_text_kk, normalized_text_mm);
		float ld_diff = (float)(LD * LD) / (float)(len_kk * len_mm);
		int is_page_no_kk = is_page_number_f(pp2, ll2, text_kk);
		int is_page_no_mm = is_page_number_f(pp1, ll1, text_mm);
		int horiz_diff1 = abs(block_array[ll1].my_x1 - block_array[ll2].my_x1);
		int horiz_diff2 = abs(block_array[ll1].my_x2 - block_array[ll2].my_x2); // with wide blocks OL is not enough. We need to test also for horiz shift limit.
		int vert_diff1 = abs(block_array[ll1].my_y1 - block_array[ll2].my_y1);
		int vert_diff2 = abs(block_array[ll1].my_y2 - block_array[ll2].my_y2); // with wide blocks OL is not enough. We need to test also for horiz shift limit.
		if (pp1 == 13) fprintf(stderr, "   MRR1: :pp2=%d: ll2=%d: y2=%d: ed=%d:%d: t=%s: ipn=%d:%d:\n", pp2, ll2,   block_array[ll2].my_y2, block_array[ll2].is_header_footer, ENUM_DESIGNATION, text_kk, is_page_no_kk, is_page_no_mm);
		if ((ld_diff < 0.25 || is_page_no_mm == 1 || is_page_no_kk == 1)
		    && horiz_diff1 < HORIZ_SHIFT_LIMIT
		    && horiz_diff2 < HORIZ_SHIFT_LIMIT
		    && vert_diff1 < VERT_SHIFT_LIMIT
		    && vert_diff2 < VERT_SHIFT_LIMIT
		    && failures < 3
		    )  {
		  /* 
		     1.  either strings are similar or it has a page number BOTTOM
		     2.  don't allow EXHIBIT on page top
		     3.  strings have to be LD-similar
		     4.  pages must be in proximity (you can't have similar footers in far parts of the document)
		  */

		  ol_footer_list[ol_footer_no].ol = MAX9(overlap_00 , overlap_01, overlap_02 , overlap_10, overlap_11 , overlap_12, overlap_20 , overlap_21, overlap_22);
		  fprintf(stderr,"                  QQQf: ii=%d: pp=%d: ll1=%d: ll2=%d:  last_kk=%d:  t=%s:%s: rol=%4.2f: ol=%4.2f:%4.2f:%4.2f:%4.2f:%4.2f: ld=%d:%d:%d:%4.2f: is_pg=%d: is_pg=%d: len=%d:\n"
			  , ol_footer_no, pp2, ll1, ll2, last_block_kk, text_mm, text_kk, ol_footer_list[ol_footer_no].ol, overlap_00, overlap_10, overlap_20, overlap_01, overlap_02, LD, len_mm, len_kk, ld_diff, is_page_no_kk, is_page_no_mm, len_kk); 

		  ol_footer_list[ol_footer_no].ll = ll1;
		  ol_footer_list[ol_footer_no].kk = ll2;

		  ol_footer_list[ol_footer_no].ll_page = pp1;
		  ol_footer_list[ol_footer_no].kk_page = pp2;	
	    
		  ol_footer_no++;
		} else {
		  failures++;
		}
	      }  else {
		failures++;
	      }
	    }
	  }
	}
      }
    } // ll1
  } // pp1
  fprintf(stderr,"RRR:%d:\n",ol_footer_no);
  return ol_footer_no;
} // create_ol_footer_array(int pp) 


int create_ol_header_array(int no_of_pages) {
  int pp1;

  for (pp1 = 1; pp1 <= no_of_pages; pp1++) { // PP1
    fprintf(stderr,"--------------H NEW PAGE: pn=%d: no_of_pages=%d: \n" ,pp1 ,no_of_pages);
    int first_block_mm = page_properties_array[pp1].first_block_in_page;
    int last_block_mm = page_properties_array[pp1].last_block_in_page;    

    int ll1;

    for (ll1 = first_block_mm; ll1 < last_block_mm && ll1 < first_block_mm+NO_OF_TESTED_BLOCKS; ll1++) { // LL1
      if (block_array[ll1].my_y2 < 5000) { // so on an empty page we don't start grabbing headers
	char *text_mm = block_array[ll1].text;
	char *normalized_text_mm = normalize(text_mm);	
	int len_mm = (normalized_text_mm) ? strlen(normalized_text_mm) : 0;
	int pp2;
	fprintf(stderr,"\nPPPh pp=%d: ll1=%d: first_block=%d: t=%s:\n", pp1, ll1, first_block_mm, block_array[ll1].text);

	for (pp2 = pp1+1; pp2 <= pp1+3; pp2++) { // PP2
	  int last_block_kk = page_properties_array[pp2].last_block_in_page;
	  int first_block_kk = page_properties_array[pp2].first_block_in_page;	  
	  int ll2;

	  int failures = 0; // DON'T ALLOW A DEEPER HEADER IF HIGHER ONES FAILED
	  for (ll2 = first_block_kk; ll2 < last_block_kk && ll2 < first_block_kk + NO_OF_TESTED_BLOCKS; ll2++) { // LL2
	    //fprintf(stderr,"    LLL0:%d:\n",failures);
	    if (block_array[ll2].my_y2 < 5000) {	   // so on an empty page we don't start grabbing headers
	      char *text_kk = block_array[ll2].text;
	      char *normalized_text_kk = normalize(text_kk);
	      int len_kk = (normalized_text_kk) ? strlen(normalized_text_kk) : 0;
	      float overlap = my_is_overlap(pp1, pp2, ll1, ll2, 0, 0); // the discrepancy -- not relative to the size
	      if (overlap > OL_HEADER_THRESHOLD) {
		int LD = wrap_LD(normalized_text_kk, normalized_text_mm);
		float ld_diff = (float)(LD * LD) / (float)(len_kk * len_mm);
		int is_exhibit_kk = is_exhibit_f(text_kk);	// only for header
		int is_exhibit_mm = is_exhibit_f(text_mm);	// only for header
		int horiz_diff1 = abs(block_array[ll1].my_x1 - block_array[ll2].my_x1);
		int horiz_diff2 = abs(block_array[ll1].my_x2 - block_array[ll2].my_x2); // with wide blocks OL is not enough. We need to test also for horiz shift limit.
		if (ld_diff < 0.25
		    && horiz_diff1 < HORIZ_SHIFT_LIMIT
		    && horiz_diff2 < HORIZ_SHIFT_LIMIT
		    && is_exhibit_kk == 0 && is_exhibit_mm == 0
		    && failures < 2
		    )  {
		  ol_header_list[ol_header_no].ol = overlap; 
		  fprintf(stderr,"                  QQQh: ii=%d: pp=%d:  ll1=%d: ll2=%d:  first=%d:  t=%s:%s: ol=%4.2f: ld=%d:%d:%d:%4.2f: is_%d:%d:\n"
			  , ol_header_no, pp2, ll1, ll2, first_block_kk, text_mm, text_kk, overlap, LD, len_kk, len_mm, ld_diff, is_exhibit_kk, is_exhibit_mm); 

		  ol_header_list[ol_header_no].ll = ll1;
		  ol_header_list[ol_header_no].kk = ll2;

		  ol_header_list[ol_header_no].ll_page = pp1;
		  ol_header_list[ol_header_no].kk_page = pp2;	
	    
		  ol_header_no++;
		} else {// IF conditions
		  failures++;
		  if (0) fprintf(stderr,"                           f=%d: SSSh: ii=%d: pp=%d:  ll1=%d: ll2=%d:  first=%d:  t=%s:%s: ol=%4.2f: \n"
			  , failures, ol_header_no, pp2, ll1, ll2, first_block_kk, text_mm, text_kk, overlap); 
		}		  
	      } else {// IF geometric overlap condition
		  failures++;
		  if (0) fprintf(stderr,"                           f=%d: RRRh: ii=%d: pp=%d:  ll1=%d: ll2=%d:  first=%d:  t=%s:%s: ol=%4.2f: \n"
			  , failures, ol_header_no, pp2, ll1, ll2, first_block_kk, text_mm, text_kk, overlap); 

	      }
	    } // IF < 5000
	  }
	}
      }
    } // ll1
  } // pp1
  fprintf(stderr,"RRR:%d:\n",ol_header_no);
  return ol_header_no;
} // create_ol_header_array(int pp) 




int print_ol_footer_list(int ol_footer_no) {
  int ii;
  for (ii = 0; ii < ol_footer_no; ii++) {
    fprintf(stderr,"FOOTER: ii=%d: ol=%f: p12=%d:%d: lk=%d:%d: t=%s:%s:\n"
	    , ii
	    , ol_footer_list[ii].ol
	    , ol_footer_list[ii].ll_page
	    , ol_footer_list[ii].kk_page
	    , ol_footer_list[ii].ll 
	    , ol_footer_list[ii].kk
	    , block_array[ol_footer_list[ii].ll].text
	    , block_array[ol_footer_list[ii].kk].text	    
	    );
  }
  return 0;
}

int print_ol_header_list(int ol_header_no) {
  int ii;
  for (ii = 0; ii < ol_header_no; ii++) {
    fprintf(stderr,"HEADER: ii=%d: ol=%f: p12=%d:%d: lk=%d:%d: t=%s:%s:\n"
	    , ii
	    , ol_header_list[ii].ol
	    , ol_header_list[ii].ll_page
	    , ol_header_list[ii].kk_page
	    , ol_header_list[ii].ll 
	    , ol_header_list[ii].kk
	    , block_array[ol_header_list[ii].ll].text
	    , block_array[ol_header_list[ii].kk].text	    
	    );
  }
  return 0;
}



struct FH_Cluster { 
  int footer_array[MAX_BLOCKS_IN_CLUSTER];
  struct FH_Item {int block; int page;} fh_item_array[MAX_BLOCKS_IN_CLUSTER];  
  int fh_no;  // how many block AND items exist in the list
} footer_cluster_array[MAX_CLUSTERS], header_cluster_array[MAX_CLUSTERS];
int footer_cluster_no, header_cluster_no;
int Real_footer_cluster_no, Real_header_cluster_no;


int ppp_fh_item_cmp(const void *a, const void *b) {
  int x1 = (((struct FH_Item *)a)->block);
  int x2 = (((struct FH_Item *)b)->block);
  return(x1-x2);
}


int found_item_in_fh_cluster(int cc, int block, struct FH_Cluster footer_cluster_array[]) {
  int nn;
  int found_in_cluster = 0;
  for (nn = 0; found_in_cluster == 0 && nn < footer_cluster_array[cc].fh_no; nn++) { // check all the items of the cluster
    if (footer_cluster_array[cc].fh_item_array[nn].block == block) {            
      found_in_cluster = 1;
    }
  }
  return found_in_cluster;
}

int fh_merge_clusters_J_into_I(int cI_in, int cJ_in, struct FH_Cluster footer_cluster_array[]) {
  int cI = MIN(cI_in, cJ_in);
  int cJ = MAX(cI_in, cJ_in);
  {
    int nII = footer_cluster_array[cI].fh_no;
    int nJJ;
    for (nJJ = 0; nJJ < footer_cluster_array[cJ].fh_no; nJJ++) {

      footer_cluster_array[cI].fh_item_array[nII].block = footer_cluster_array[cJ].fh_item_array[nJJ].block;
      footer_cluster_array[cI].fh_item_array[nII].page = footer_cluster_array[cJ].fh_item_array[nJJ].page;      

      nII++;
    }

    footer_cluster_array[cI].fh_no = nII;
    footer_cluster_array[cJ].fh_no = 0;
  }
  return cI;
}


int do_fh_clusters(int last_page_no, int *fh_cluster_no, int *Real_fh_cluster_no, int ol_fh_no, struct Diff_Triple ol_fh_list[], struct FH_Cluster fh_cluster_array[]) {
  *fh_cluster_no = 0;
  *Real_fh_cluster_no = 0;  

  int ss; // the similarity item
  int cc; // the cluster
  int cI = -1;
  int cJ = -1;

  fprintf(stderr, "CLUSTERING  %d:\n", ol_fh_no);
  for (ss = 0; ss < ol_fh_no; ss++) { 
    //if (debug) fprintf(stderr, "**GO OVER SS=%d: CL_NO=%d:%d:\n", ss, *fh_cluster_no, *Real_fh_cluster_no);
    int found_in_clusters = 0;
    cI = 0; cJ = 0;
    int foundI = 0, foundJ = 0;
    for (cc = 0; found_in_clusters < 2 && cc < *fh_cluster_no; cc++) { // check if new sim is already in an EXISTING CLUSTER
      if (foundI == 0) {
	foundI = found_item_in_fh_cluster(cc, ol_fh_list[ss].ll, fh_cluster_array);
	if (foundI == 1) {
	  cI = cc;
	}
      }
      
      if (foundJ == 0) {
	foundJ = found_item_in_fh_cluster(cc, ol_fh_list[ss].kk, fh_cluster_array);
	if (foundJ == 1) {
	  cJ = cc;
	}
      }
    } // for all clusters

    if (debug) fprintf(stderr,"       FOUND ss=%d: fIJ1=%d:%d: cIJ=%d:%d: sim=%f: CL_NO=%d:\n", ss, foundI, foundJ, cI, cJ, ol_fh_list[ss].ol, *fh_cluster_no);
    int ii0 = ol_fh_list[ss].ll;
    int ii1 = ol_fh_list[ss].kk;
    int pp0 = ol_fh_list[ss].ll_page;
    int pp1 = ol_fh_list[ss].kk_page;
    float sim01 = ol_fh_list[ss].ol;
    if (foundI == 1 && foundJ == 0 && sim01 >= ADD_FH_THRESHOLD) {

      fh_cluster_array[cI].fh_item_array[fh_cluster_array[cI].fh_no].block = ii1;
      fh_cluster_array[cI].fh_item_array[fh_cluster_array[cI].fh_no].page = pp1;      
      fh_cluster_array[cI].fh_no++;

      if (fh_cluster_array[cI].fh_no >= MAX_BLOCKS_IN_CLUSTER-1) {
	fprintf(stderr,"No of itemd exceeded %d:\n", MAX_BLOCKS_IN_CLUSTER);
	exit(0);
      }

    } else if (foundJ == 1 && foundI == 0  && sim01 >= ADD_FH_THRESHOLD) {

      fh_cluster_array[cJ].fh_item_array[fh_cluster_array[cJ].fh_no].block = ii0;
      fh_cluster_array[cJ].fh_item_array[fh_cluster_array[cJ].fh_no].page = pp0;      
      fh_cluster_array[cJ].fh_no++;

      if (fh_cluster_array[cJ].fh_no >= MAX_BLOCKS_IN_CLUSTER-1) {
	fprintf(stderr,"No of itemd exceeded %d:\n", MAX_BLOCKS_IN_CLUSTER);
	exit(0);
      }
      
    } else if (foundI == 1 && foundJ == 1
	       && cI >= 0 && cJ >=0 && cI != cJ
	       && ol_fh_list[ss].ol >= MERGE_FH_THRESHOLD) { // merge clusters // any conditions on merge? what if each group is happy with 15 items?


      fh_merge_clusters_J_into_I(cI, cJ, fh_cluster_array);
      (*Real_fh_cluster_no)--;

    } else if (foundI == 0 && foundJ == 0 && sim01 >= ADD_FH_THRESHOLD) { // create new cluster with 2 items for this similarity
      int cc_fh_no = 0;

      fh_cluster_array[*fh_cluster_no].fh_item_array[cc_fh_no].block = ii0;
      fh_cluster_array[*fh_cluster_no].fh_item_array[cc_fh_no].page = pp0;      
      cc_fh_no++;
	

      fh_cluster_array[*fh_cluster_no].fh_item_array[cc_fh_no].block = ii1;
      fh_cluster_array[*fh_cluster_no].fh_item_array[cc_fh_no].page = pp1;      
      cc_fh_no++;
	
      fh_cluster_array[*fh_cluster_no].fh_no = cc_fh_no;
	
      if (*fh_cluster_no >= MAX_CLUSTERS-1) {
	fprintf(stderr,"No of cluster exceeded %d:\n", MAX_CLUSTERS);
	exit(0);
      }
      (*fh_cluster_no)++;
      (*Real_fh_cluster_no)++;      
      fprintf(stderr,"------------CREATED NEW: cl_nl=%d:\n", *fh_cluster_no);
    }
  } // SS
  if (debug) fprintf(stderr, "FINISHED PAGE clusters :%d:\n", cc);

  // sort the blocks by order
  for (cc = 0; cc < *fh_cluster_no; cc++) {
    qsort((void *)&(fh_cluster_array[cc].fh_item_array),  fh_cluster_array[cc].fh_no, sizeof(struct FH_Item), ppp_fh_item_cmp);
  }
  
  return *fh_cluster_no;
} // do_fh_clusters()


int mark_and_print_fh_clusters(int fh_cluster_no, struct FH_Cluster fh_cluster_array[], int designation) {
  int cc;
  fprintf(stderr, "PRINTING %s CLUSTERS: cc=%d:\n",  ((designation==1)?"FOOTER":"HEADER"), fh_cluster_no);
  for (cc = 0; cc < fh_cluster_no; cc++) {
    if (fh_cluster_array[cc].fh_no > 0) {
      int bk1 = fh_cluster_array[cc].fh_item_array[1].block;
      fprintf(stderr, "      %s CLUSTER:%d:%d: t0=%s:\n", ((designation==1)?"FOOTER":"HEADER") ,cc, fh_cluster_array[cc].fh_no, block_array[bk1].text);
      int ii;
      for (ii = 0; ii < fh_cluster_array[cc].fh_no; ii++) {
	int bk = fh_cluster_array[cc].fh_item_array[ii].block;
	int fw = block_array[bk].first_word;
	int lw = block_array[bk].last_word;	

	fprintf(stderr, "  :%d:%d:%d:-:%d:%d:", ii, fh_cluster_array[cc].fh_item_array[ii].block, fh_cluster_array[cc].fh_item_array[ii].page, fw, lw);
	block_array[bk].is_header_footer = designation;
	
      }
      fprintf(stderr, "\n");
    }    
  }
  return 0;
}


int create_footer_clusters_new(int last_page_no) {  

  fprintf(stderr,"********************* IN DOING FOOTER\n");
  ol_footer_no = create_ol_footer_array(last_page_no);
  fprintf(stderr,"********************* PRINTING FOOTER BEFORE SORT:%d:\n", ol_footer_no);  
  print_ol_footer_list(ol_footer_no);  
  sort_ol_footer_pairs();
  fprintf(stderr,"********************* PRINTING FOOTER AFTER SORT:%d:\n", ol_footer_no);  
  print_ol_footer_list(ol_footer_no);  
  int nn = do_fh_clusters(last_page_no, &footer_cluster_no, &Real_footer_cluster_no, ol_footer_no, ol_footer_list, footer_cluster_array);
  fprintf(stderr,"********************* PRINTING CL FOOTR:%d:\n", ol_header_no);  
  mark_and_print_fh_clusters(footer_cluster_no, footer_cluster_array, FOOTER_DESIGNATION);

  return 0;
} // create_footer_clusters()

int create_header_clusters_new(int last_page_no) {  

  fprintf(stderr,"********************* IN DOING HEADER\n");
  ol_header_no = create_ol_header_array(last_page_no);
  fprintf(stderr,"********************* PRINTING HEADER BEFORE SORT:%d:\n", ol_header_no);  
  print_ol_header_list(ol_header_no);  
  sort_ol_header_pairs();
  fprintf(stderr,"********************* PRINTING HEADER AFTER SORT:%d:\n", ol_header_no);  
  print_ol_header_list(ol_header_no);  
  int nn = do_fh_clusters(last_page_no, &header_cluster_no, &Real_header_cluster_no, ol_header_no, ol_header_list, header_cluster_array);
  fprintf(stderr,"********************* PRINTING CL HEADR:%d:\n", ol_header_no);  
  mark_and_print_fh_clusters(header_cluster_no, header_cluster_array, HEADER_DESIGNATION);

  return 0;
} // create_header_clusters()


/*****************************************************************************/
 
