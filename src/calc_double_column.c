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
#include "./calc_page_params.h"


/*****************************
 ** 1. split when a GROUP of consecutive lines has a gap in the center (between 4500 and 5500)
 ** 2. do not take TOC pages (where there is the page column at the right)  (page_properties_array[pp].no_toc_items < 3)
 ** 3. do not take lines where entire group is right or left (very short lines)
 ** 4. FOR NOW do not handle "address groups": a group of few lines that are doubled or tripled
 ** 5. in the future we need to detect charts so they are not taken as DC (example, 58275 p11 and p8)
 ** 6. for now do DC only if more than 80% of the page -- obviously a bad measure both for FP and FN
****************************/

int MY_PN = 2;

int right_margin = 10000, left_margin = 0;
int create_gap_array(int last_page_no) {
  int pp;
  if (debug) fprintf(stderr,"*************************IV.  CREATING GAPS\n");  
  for (pp = 1; pp <= last_page_no; pp++) {
    int ii;
    int line_no;
    int prev_line_no = -1;
    gap_no[pp] = 0;

    for (ii = 0; ii < MAX_GAP_PER_PAGE; ii++) { // INIT
      gap_array[pp][ii].block_r = -5;
      gap_array[pp][ii].block_l = -5;
      gap_array[pp][ii].size = -1;	            
    }

    int first_block = page_properties_array[pp].first_block_in_page;
    int last_block = page_properties_array[pp].last_block_in_page;
    if (0) fprintf(stderr,"GGGGGGGGGGGGGG: pp=%d: gn=%d: flb=%d:%d: \n",pp, gap_no[pp], first_block, last_block);    
    
    for (ii = first_block; ii <= last_block; ii++) {
      line_no = block_array[ii].my_line_in_doc;
      if (line_no != prev_line_no) {

	if (gap_no[pp] > 0) {
	  gap_array[pp][gap_no[pp]-1].right = right_margin;
	  gap_array[pp][gap_no[pp]-1].block_r = -2;	
	  gap_array[pp][gap_no[pp]-1].size = gap_array[pp][gap_no[pp]-1].right - gap_array[pp][gap_no[pp]-1].left;	
	}
      
	gap_array[pp][gap_no[pp]].left = left_margin;
	if (0 &&debug) fprintf(stderr,"GGG0:%d:%d:\n",gap_no[pp],  gap_array[pp][gap_no[pp]].left);
	gap_array[pp][gap_no[pp]].block_l = -1;
	prev_line_no = line_no;
	gap_no[pp]++;
      }

      gap_array[pp][gap_no[pp]-1].right = block_array[ii].my_x1;
      gap_array[pp][gap_no[pp]-1].block_r = ii;    
      gap_array[pp][gap_no[pp]-1].size = gap_array[pp][gap_no[pp]-1].right - gap_array[pp][gap_no[pp]-1].left;
    
      gap_array[pp][gap_no[pp]].left = block_array[ii].my_x2;
      if (0 && debug) fprintf(stderr,"GGG1:%d: ii=%d: xy=%d:%d:%d:%d: l=%d: r-1=%d:\n",gap_no[pp], ii, block_array[ii].my_x1, block_array[ii].my_x2, block_array[ii].my_y1, block_array[ii].my_y2, gap_array[pp][gap_no[pp]].left, gap_array[pp][gap_no[pp]-1].right);
      gap_array[pp][gap_no[pp]].block_l = ii;    

      gap_array[pp][gap_no[pp]].line_in_doc = block_array[ii].my_line_in_doc;
      gap_array[pp][gap_no[pp]].page = block_array[ii].page;    
      gap_array[pp][gap_no[pp]].block_in_doc = block_array[ii].block_in_doc;
    
      if (gap_array[pp][gap_no[pp]-1].line_in_doc == 0 && gap_array[pp][gap_no[pp]-1].line_in_doc != 1) {
	gap_array[pp][gap_no[pp]-1].line_in_doc = block_array[ii].my_line_in_doc;
	gap_array[pp][gap_no[pp]-1].page = block_array[ii].page;
      }

      if (0 && debug) fprintf(stderr,"LLLLL: ii=%d: gn=%d: block=%d:%d: ln=%d: x12=%d:%d: lr=%d:%d: size=%d: THR=%d:\n"
			 , ii, gap_no[pp]-1
			 , gap_array[pp][gap_no[pp]-1].block_l, gap_array[pp][gap_no[pp]-1].block_r
			 , gap_array[pp][gap_no[pp]-1].line_in_doc
			 , block_array[ii].my_x1, block_array[ii].my_x2
			 , gap_array[pp][gap_no[pp]-1].left, gap_array[pp][gap_no[pp]-1].right
			 , gap_array[pp][gap_no[pp]-1].size, GAP_TOP_THRESHOLD);
      if ( gap_array[pp][gap_no[pp]-1].size > GAP_TOP_THRESHOLD) { // TRYING???
	// if gap too large then shrink it, don't delete it
	gap_array[pp][gap_no[pp]-1].size = GAP_TOP_THRESHOLD;
	//gap_array[pp][gap_no[pp]-1].left = gap_array[pp][gap_no[pp]-1].right - GAP_TOP_THRESHOLD;
      }
      if (1 || gap_array[pp][gap_no[pp]-1].size > MIN_GAP_THRESHOLD) {
	gap_no[pp]++; 
      }
    }
    if (1) fprintf(stderr,"             GAPS for page: pp=%d: gn=%d: flb=%d:%d: \n",pp, gap_no[pp], first_block, last_block);
  } // pp 
  return 0;
} // create_gap_array()

#define MAX_GAP_OL 5000
struct Gap_Overlap {
  int the_overlap;
  int my_ii; // so after sort we know where we came from
  int my_jj;
} gap_overlap_array[MAX_PAGE][MAX_GAP_OL];
int gap_overlap_no[MAX_PAGE];
int real_gap_overlap_no[MAX_PAGE];

// int print_one_ol(int pp, int mm, int nn, int ii, int jj, int kk) {
int print_one_ol(int pp, int mm) {
  int ii = gap_overlap_array[pp][mm].my_ii;
  int jj = gap_overlap_array[pp][mm].my_jj;
  int nn = 0;

  int tli = gap_array[pp][ii].block_l;
  int tri = gap_array[pp][ii].block_r;  
  int tlj = gap_array[pp][jj].block_l;
  int trj = gap_array[pp][jj].block_r;  
  if ((pp == MY_PN || (pp == 1 && doc_id == 94611))) fprintf(stderr,"         :%d:  GAP_OL(%d): kk=%d: ol=%d: GAP1: ii=%d:-lr=%d:%d:-:%d:%d:%10s:%10s GAP2: jj=%d:-lr=%d:%d:-:%d:%d:%10s:%10s \n"
		     , mm, nn, mm, gap_overlap_array[pp][mm].the_overlap
		
		, ii,   gap_array[pp][ii].left, gap_array[pp][ii].right,               tli,tri,   block_array[tli].textl, block_array[tri].textf
		, jj,   gap_array[pp][jj].left, gap_array[pp][jj].right,               tlj,trj,   block_array[tlj].textl, block_array[trj].textf
	  );
  mm++;
  return mm;
}

int print_gap_ol(int page_no) {
  int pp;
  for (pp = 1; pp <= page_no; pp++) {
    if ((pp == MY_PN || (pp == 1 && doc_id == 94611))) {
      fprintf(stderr,"  PRINTING OL PAGE=%d: gn=%d: flb=%d:%d: gon=%d:\n",pp, gap_no[pp],  page_properties_array[pp].first_block_in_page, page_properties_array[pp].last_block_in_page, gap_overlap_no[pp]);    
      int kk;
      for (kk = 0; kk < gap_overlap_no[pp]; kk++) {
	print_one_ol(pp,kk);
      }
    }
  }
  return 0;
}

// we don't look at GAPs below MIN_GAP_THRESHOLD
// gap_overlap is 1D emulating a 2D array [ii][jj]
int calc_overlap_matrix_of_gaps(int last_page_no) {
  int pp;
  if (debug) fprintf(stderr,"*****************************  VII.  OVERLAP: gap_no=%d: lpn=%d:\n",gap_no[pp], last_page_no);      
  for (pp = 1; pp <= last_page_no; pp++) {
    int kk = 0;
    int ii,jj;
    int test1 = 0;

    int first_gap = -1;
    int last_gap = gap_no[pp];  
    int gg = 0;

    //fprintf(stderr,"  GAPPING OL PAGE=%d: gn=%d: flb=%d:%d:\n",pp, gap_no[pp],  page_properties_array[pp].first_block_in_page, page_properties_array[pp].last_block_in_page);
    for (gg = 0; gg <= gap_no[pp]; gg++) {
      if ((pp == MY_PN || (pp == 1 && doc_id == 94611))) fprintf(stderr,"       BBB GAP OVERLAP:%d: tl=%d: tr=%d: pn=%d:%d: \n"
		     , gg,  gap_array[pp][gg].block_l,  gap_array[pp][gg].block_r
		     ,block_array[gap_array[pp][gg].block_l].page, block_array[gap_array[pp][gg].block_r].page);
      if (gap_array[pp][gg].block_r >= page_properties_array[pp].first_block_in_page && first_gap == -1) {
	first_gap = gg;
      } else if (gap_array[pp][gg].block_l >= page_properties_array[pp].last_block_in_page) {
	last_gap = gg; 
	break;
      }
    }
  
    if ((pp == MY_PN || (pp == 1 && doc_id == 94611))) fprintf(stderr,"CREATE GAP OVERLAP:%d: fg=%d: lg=%d:\n",gap_no[pp], first_gap, last_gap);
    for (ii = first_gap; ii <= last_gap; ii++) {

      if (1
	  // we allow a "gap" that starts all the way to the left of the page, but we want to make sure it's a double-column
	  // in other words, a double column line where the left side is empty
	  // this is important so we can get the correct read-order
	  && (0 || (((gap_array[pp][ii].block_l >= 0 && gap_array[pp][ii].left < 5000 /* && gap_array[pp][ii].size > MIN_GAP_THRESHOLD*/) 
		     || (gap_array[pp][ii].block_l == -1  && gap_array[pp][ii].right < 8000))
		    && ((gap_array[pp][ii].block_r >= 0 && gap_array[pp][ii].right > 5000 /* && gap_array[pp][ii].size > MIN_GAP_THRESHOLD*/)
			|| ((gap_array[pp][ii].block_r == -2 || gap_array[pp][ii].block_r == -5) && gap_array[pp][ii].left > 1000))))
	  ) {

	for (jj = ii+1; jj <= last_gap; jj++) {

	  int tli = gap_array[pp][ii].block_l;
	  int tri = gap_array[pp][ii].block_r;  
	  int tlj = gap_array[pp][jj].block_l;
	  int trj = gap_array[pp][jj].block_r;  

	  if (0 && pp == SHOW_PAGE) fprintf(stderr,"            CREATE0: ii=%d: sz=%d: c=%d:%d: b=%d:%d: :%s:%s:       jj=%d: sz=%d:  c=%d:%d: b=%d:%d: :%s:%s:\n"
				       , ii, gap_array[pp][ii].size, gap_array[pp][ii].left, gap_array[pp][ii].right, gap_array[pp][ii].block_l, gap_array[pp][ii].block_r, block_array[tri].textf, block_array[tli].textl
				       , jj, gap_array[pp][jj].size, gap_array[pp][jj].left, gap_array[pp][jj].right, gap_array[pp][jj].block_l, gap_array[pp][jj].block_r, block_array[trj].textf, block_array[tlj].textl);


	  if (1
	      && (0 || (((gap_array[pp][jj].block_l >= 0 && gap_array[pp][jj].left < 5000 /* && gap_array[pp][jj].size > MIN_GAP_THRESHOLD*/)
			 || ((gap_array[pp][jj].block_l == -1 || gap_array[pp][jj].block_l == -5) && gap_array[pp][jj].right < 8000))
			&& ((gap_array[pp][jj].block_r >= 0 && gap_array[pp][jj].right > 5000 /* && gap_array[pp][jj].size > MIN_GAP_THRESHOLD*/)
			    || ((gap_array[pp][jj].block_r == -2 || gap_array[pp][jj].block_r == -5) && gap_array[pp][jj].left > 1000))))
	      ) {

	    if (0 && /*ii < 300 && jj < 300 &&*/ pp == SHOW_PAGE) {
	      int tli = gap_array[pp][ii].block_l;
	      int tri = gap_array[pp][ii].block_r;  
	      int tlj = gap_array[pp][jj].block_l;
	      int trj = gap_array[pp][jj].block_r;  
	    
	      fprintf(stderr,"            CREATE1: ii=%d: sz=%d: c=%d:%d: b=%d:%d: :%s:%s:            jj=%d: sz=%d: c=%d:%d: b=%d:%d: :%s:%s:\n"
		      , ii, gap_array[pp][ii].size, gap_array[pp][ii].right, gap_array[pp][ii].left, gap_array[pp][ii].block_r, gap_array[pp][ii].block_l, block_array[tri].textf, block_array[tli].textl
		      , jj, gap_array[pp][jj].size, gap_array[pp][jj].right, gap_array[pp][jj].left, gap_array[pp][jj].block_r, gap_array[pp][jj].block_l, block_array[trj].textf, block_array[tlj].textl);
	    }

	    int ok = 0;

	    if (gap_array[pp][jj].left > gap_array[pp][ii].right
		|| gap_array[pp][ii].left > gap_array[pp][jj].right) {

	      ok = 0;
	    } else if (gap_array[pp][ii].left <= gap_array[pp][jj].left
		       && gap_array[pp][ii].right >= gap_array[pp][jj].right) {
	      gap_overlap_array[pp][kk].the_overlap = gap_array[pp][jj].size;

	      ok = 1;
	    } else if (gap_array[pp][jj].left <= gap_array[pp][ii].left
		       && gap_array[pp][jj].right >= gap_array[pp][ii].right) {
	      gap_overlap_array[pp][kk].the_overlap = gap_array[pp][ii].size;

	      ok = 1;
	    } else if (gap_array[pp][jj].left >= gap_array[pp][ii].left
		       && gap_array[pp][jj].right >= gap_array[pp][ii].right) { // jj OLs above ii
	      gap_overlap_array[pp][kk].the_overlap = gap_array[pp][ii].right - gap_array[pp][jj].left;
	      gap_overlap_no[pp]++;

	      ok = 1;
	    } else if (gap_array[pp][ii].left >= gap_array[pp][jj].left
		       && gap_array[pp][ii].right >= gap_array[pp][jj].right) {   // jj OLs below ii
	      gap_overlap_array[pp][kk].the_overlap = gap_array[pp][jj].right - gap_array[pp][ii].left;
	      gap_overlap_no[pp]++;

	      ok = 1;
	    }
	    if (ok) {
	      gap_overlap_array[pp][kk].my_ii = ii;
	      gap_overlap_array[pp][kk].my_jj = jj;

	      //if (debug) print_one_ol(pp, kk);
	      kk++;
	    }
	  }
	} // for jj
      }
    } // for ii
    gap_overlap_no[pp] = kk;
    if ((pp == MY_PN || (pp == 1 && doc_id == 94611))) fprintf(stderr,"        OVERLAPs for page:%d: gap_overlap_no=%d::\n",gap_no[pp], gap_overlap_no[pp]);    
  } // pp
  return  0;  
} // calc_overlap_matrix_of_gaps()

int print_gap_array(int last_page_no) {
  int pp;
  if (debug) fprintf(stderr,"******************************* V.  PRINTING GAPS for specific pages \n");      
  for (pp = 1;  pp <= last_page_no; pp++) {
    if (pp == MY_PN || (pp == 1 && doc_id == 94611)) {
      if (debug) fprintf(stderr,"       GAPS FOR PAGE:%d:%d: \n",pp, gap_no[pp]);    
      int ii;
      for (ii = 0; ii < gap_no[pp]; ii++) {
	int tli = gap_array[pp][ii].block_l;
	int tri = gap_array[pp][ii].block_r;  
    

	if (debug) fprintf(stderr,"     GAP: %3d: sz=%8d:  lr=:%6d:%6d: line=%d: pg=%d: glinesn=%d: tid=:%3d:%3d: t=:%10s:%10s \n"
			   , ii, gap_array[pp][ii].size
			   , gap_array[pp][ii].left, gap_array[pp][ii].right
			   , gap_array[pp][ii].line_in_doc,  gap_array[pp][ii].page,  gap_array[pp][ii].block_in_doc
			   , tli,tri
			   , (tli > -1) ? block_array[tli].textl : ""
			   , (tri > -1) ? block_array[tri].textf : ""
			   );
      }
      if (debug) fprintf(stderr,"\nGON=%d: gon=%d: \n", gap_no[pp], gap_overlap_no[pp]);
    } // pp
  }
  return 0;
} //  print_gap_array(int gap_no) 

int ppp2_cmp(const void *a, const void *b) { // gap_overlap_item
  int x1 = (((struct Gap_Overlap *)a)->the_overlap);
  int y1 = (((struct Gap_Overlap *)b)->the_overlap);
  //fprintf(stderr,"\nComparing <%d> <%d> <%d>\n",x1,y1, y1-x1);
  return(x1<y1);
}

int sort_gap_overlap_array_by_the_overlap(int page_no) {
  int pp;
  for (pp = 1; pp <= page_no; pp++) {
    qsort((void *)&gap_overlap_array[pp][0], gap_overlap_no[pp],  sizeof(struct Gap_Overlap),  ppp2_cmp);
  }
  return 0;
}

struct Overlap_Cluster {
  int gap_item_no;
  int gap_item_array[MAX_GAP_PER_PAGE]; // stores the gaps on this overlap_cluster
} overlap_cluster_array[MAX_PAGE][5];
int overlap_cluster_no[MAX_PAGE];

int reset_line_property_array() {
  int ll;
  for (ll = 0; ll < MAX_PAGE*MAX_LINE_PER_PAGE; ll++) {
    line_property_array[ll].gap_no_in_cluster = -1;
    line_property_array[ll].belongs_in_group = -2;
  }
  return 0;
}

/************* EMPTY FOR NOW ***************************/
int first_line_in_cluster_of_page[MAX_PAGE];
int last_line_in_cluster_of_page[MAX_PAGE];


int get_first_block_on_cluster(int pp) {
  int my_first_line_in_cluster = first_line_in_cluster_of_page[pp];
  int first_block_in_cluster = line2block_index[pp][0];
  return first_block_in_cluster;
}

int get_last_block_on_cluster(int pp) {
  int my_last_line_in_cluster = last_line_in_cluster_of_page[pp];
  int last_block_in_cluster = line2block_index[pp][line2block_no[pp]];  
  return last_block_in_cluster;
}

int add_val_to_overlap_cluster(int pp, int cluster_id, int val) { // a cluster of gaps is a channel of white or a tab in a chart
  int cc;
  int exists = 0; // testing existence of JJ in cluster so no duplications
  cc = cluster_id;
  for (cc = 0; cc < overlap_cluster_no[pp] && exists == 0; cc++)  {
    int item_no = overlap_cluster_array[pp][cc].gap_item_no;
    int kk;
    for (kk = 0; kk < item_no; kk++) { // see if val exists already
      if (val == overlap_cluster_array[pp][cc].gap_item_array[kk]) {
	exists = 1;
	break;
      }
    }
  }

  int my_item_no = overlap_cluster_array[pp][cluster_id].gap_item_no;
  if (exists == 0) { // add val to overlap_cluster
    overlap_cluster_array[pp][cluster_id].gap_item_array[my_item_no] = val; 
    overlap_cluster_array[pp][cluster_id].gap_item_no++;
    if ((pp == MY_PN || (pp == 1 && doc_id == 94611))) fprintf(stderr,"                OOOOOOOOX!: ADDED: my_ii=%d: to cl=%d: in=%d:\n", val, cluster_id, my_item_no);      
  }
  return 0;
} // add_val_to_overlap_cluster(int cluster_id, int val) 


int line_of_gap_exists_in_cluster(int pp, int my_ii, int cluster_id) {
  int found = 0;
  int kk;
  int item_no = overlap_cluster_array[pp][cluster_id].gap_item_no;
  for (kk = 0; kk < item_no; kk++) {
    int my_jj = overlap_cluster_array[pp][cluster_id].gap_item_array[kk];
    if (0) fprintf(stderr," :%d:%d: ", kk, my_jj);
  }
  if (0) fprintf(stderr,"\n");
  
  for (kk = 0; kk < item_no; kk++) {
    int my_jj = overlap_cluster_array[pp][cluster_id].gap_item_array[kk];
    if (my_ii != my_jj && gap_array[pp][my_ii].line_in_doc == gap_array[pp][my_jj].line_in_doc) {
      found = 1;
      break;
    }
  }
  return found;
}
  

// if my_ii exists on overlap_cluster then add also my_jj
// make sure this is not a "cousin" (they both sit on the same line) of another gap on this cluster -- that 
int find_overlap_gaps_in_cluster_and_add_if_pass(int pp, int my_ii, int my_jj, int cluster_id) {
  int found = 0;
  int kk;
  int item_no = overlap_cluster_array[pp][cluster_id].gap_item_no;
  for (kk = 0; kk < item_no; kk++) {
    int eI= line_of_gap_exists_in_cluster(pp, my_ii, cluster_id);
    int eJ= line_of_gap_exists_in_cluster(pp, my_jj, cluster_id);
    if (my_ii == overlap_cluster_array[pp][cluster_id].gap_item_array[kk] && eJ == 0) { // does II exists on cluster? and no line coincidence for JJ?
      add_val_to_overlap_cluster(pp, cluster_id, my_jj); // add jj if doesn't exist already ON THIS CLUSTER
      found = 1;
      break;
    } else if (my_jj == overlap_cluster_array[pp][cluster_id].gap_item_array[kk] && eI == 0) {  // does JJ exists on cluster? and no line coincidence for II?
      add_val_to_overlap_cluster(pp, cluster_id, my_ii); // // add ii if doesn't exist already ON THIS CLUSTER
      found = 1;
      break;
    }
  }
  return found;
} // find_overlap_gaps_in_cluster_and_add_if_pass()


int gap_exists_in_any_overlap_cluster(int pp, int my_gap_id) {
  int cc;
  int found = 0;
  for (cc = 0; cc < overlap_cluster_no[pp]; cc++) { // go over overlap clusters
    int ii;
    for (ii = 0; found == 0 && ii < overlap_cluster_array[pp][cc].gap_item_no; ii++) { // check for each of the items of CC1 if it's included in some CC2
      if (overlap_cluster_array[pp][cc].gap_item_array[ii] == my_gap_id) {
	found = 1;
	break;
      } 
    }  // ii
  } // cc
  return found;
}

int create_new_cluster(int pp, int ol1) {
  overlap_cluster_array[pp][overlap_cluster_no[pp]].gap_item_no = 0;
  overlap_cluster_array[pp][overlap_cluster_no[pp]].gap_item_array[overlap_cluster_array[pp][overlap_cluster_no[pp]].gap_item_no] = gap_overlap_array[pp][ol1].my_ii;
  overlap_cluster_array[pp][overlap_cluster_no[pp]].gap_item_no++;
  overlap_cluster_array[pp][overlap_cluster_no[pp]].gap_item_array[overlap_cluster_array[pp][overlap_cluster_no[pp]].gap_item_no] = gap_overlap_array[pp][ol1].my_jj;
  overlap_cluster_array[pp][overlap_cluster_no[pp]].gap_item_no++;
  if (0 && debug) fprintf(stderr,"\n\n             OOCLUSTER: cl_no=%d:  gap_item_array[0]=%d: gap_item_array[1]=%d: :%d:%d:\n"
		 , overlap_cluster_no[pp], overlap_cluster_array[pp][0].gap_item_array[0], overlap_cluster_array[pp][0].gap_item_array[1]
		 , gap_overlap_array[pp][ol1].my_ii, gap_overlap_array[pp][ol1].my_jj);
  overlap_cluster_no[pp]++;
  return 0;
} // create_new_cluster() {



/*********************************** ^^ NEW ^^ ********************************************/


#define MAX_GAPS_PER_LINE 10
int do_overlap_clusters(int page_no) {
  int pp;
  if (debug) fprintf(stderr,"***************************  VIII.  CREATING OL CLUSTERS: no_of_pages=%d:\n", page_no);
  for (pp = 1; pp <= page_no; pp++) {
    int ol1;
    overlap_cluster_no[pp] = 0;
    int new_cluster_pending = 1;
    int safety_no_of_loops = 0;
    if (0) fprintf(stderr,"CREATING OL CLUSTERS for page=%d:\n", pp);    
    //while (new_cluster_pending == 1 && safety_no_of_loops++ < 1 && safety_no_of_loops < MAX_GAPS_PER_LINE) { // on each pass we allow only one create_cluster so we exhaust all it's gaps bf we create a new cluster
      new_cluster_pending = 0;
      int done_new_cluster = 0; // in an ol1 loop we do only one new cluster
      if (0) fprintf(stderr,"   START A NEW CLUSTER LOOP:%d:\n", overlap_cluster_no[pp]);
      for (ol1 = 0; ol1 < gap_overlap_no[pp] /*&& gap_overlap_array[ol1].the_overlap > MIN_GAP_THRESHOLD*/; ol1++) {
	int left = gap_array[pp][gap_overlap_array[pp][ol1].my_ii].left;
	int right = gap_array[pp][gap_overlap_array[pp][ol1].my_jj].right;	
	if (left < 5000 && (right > 5000 || right == 0)) { // don't look at non-median gaps for now
	  int found; // don't add to 2 clusters
	  int cc2;
	  if (0) fprintf(stderr,"       GOING OVER OLS: search this gap on existing clusters: ol1=%d: ol=%d:  how_many_clusters=%d:\n",ol1, gap_overlap_array[pp][ol1].the_overlap,   overlap_cluster_no[pp]);
	  for (cc2 = 0, found = 0; cc2 < overlap_cluster_no[pp]; cc2++) { // search for this gap on the existing clusters
	    int kk;
	    int item_no = overlap_cluster_array[pp][cc2].gap_item_no;
	    if (0) fprintf(stderr,"  CL::%d: ", cc2);
	    for (kk = 0; kk < item_no; kk++) {
	      int my_ii = overlap_cluster_array[pp][cc2].gap_item_array[kk];
	      if (0) fprintf(stderr," :%d:%d:",kk, my_ii);
	    }
	    //if (debug) fprintf(stderr,"\n");
      
	    found = find_overlap_gaps_in_cluster_and_add_if_pass(pp, gap_overlap_array[pp][ol1].my_ii, gap_overlap_array[pp][ol1].my_jj, cc2);
	    if (found == 1) {
	      found = 1;
	      break;
	    }
	    found = find_overlap_gaps_in_cluster_and_add_if_pass(pp, gap_overlap_array[pp][ol1].my_jj, gap_overlap_array[pp][ol1].my_ii, cc2);
	    if (found == 1) {
	      found = 1;
	      break;
	    }
	  }
	  //if (debug) fprintf(stderr,"\n");
	  if (found == 0
	      && gap_exists_in_any_overlap_cluster(pp, gap_overlap_array[pp][ol1].my_ii) == 0 // make sure my_ii really doesn't exist anywhere else
	      && gap_exists_in_any_overlap_cluster(pp, gap_overlap_array[pp][ol1].my_jj) == 0 // make sure my_jj really doesn't exist anywhere else
	      ) { // if neither gap found on any cluster, then start a new cluster
	    if (done_new_cluster == 0) {
	      create_new_cluster(pp,ol1);
	      done_new_cluster = 1;
	    } else {
	      new_cluster_pending = 1;
	    }
	  } else {
	    ;
	  }
	} // if median gap
      } // for ol1
      //} // while pending
    if (debug) fprintf(stderr,"              CLUSTERS per page: page=%d: no_of_ols=%d: no_of_clusters=%d:\n", pp, gap_overlap_no[pp], overlap_cluster_no[pp]);
  } // pp
  return 0;
} // do_overlap_clusters()



int ppp_int_cmp(const void *a, const void *b) { // gap_overlap_item
  int x1 = ((*(int *)a));
  int y1 = ((*(int *)b));
  return(x1>y1);
}

int sort_cluster_items(int item_array[], int item_no) {
  qsort((void *)&item_array[0], item_no, sizeof(int), ppp_int_cmp);
  return 0;
}



int in_seq_number = 0; // the seq no in which this chart is found
int my_seq_number = 0;

int stats_overlap_clusters(int last_page_no) {
  int pp;
  if (debug) fprintf(stderr,"\n\n***********************************  IX. STATS OVERLAP CLUSTERS\n");    
  for (pp = 1; pp <= last_page_no; pp++) {
    if (1 || pp == SHOW_PAGE) {
      if (debug) fprintf(stderr,"\n\n    MEDIAN GAP CLUSTERS FOR PAGE  pp=%d: :%d: toc=%d:%d:\n", pp, overlap_cluster_no[pp], page_properties_array[pp].no_toc_clusters, page_properties_array[pp].no_toc_items);  
      int cc;
      for (cc = 0; cc < overlap_cluster_no[pp]; cc++) {
	int gap_item_no = overlap_cluster_array[pp][cc].gap_item_no;
	if (gap_item_no > 0) {
	  if (debug) fprintf(stderr,"       GAP_OL_CLUSTER CL=%d:%d:--\n",cc,  overlap_cluster_array[pp][cc].gap_item_no);
	  int ii;
	  sort_cluster_items(overlap_cluster_array[pp][cc].gap_item_array, gap_item_no);
	  int prev_line = 0;
	  int x2_l_array[MAX_GAP_PER_PAGE];
	  int x1_r_array[MAX_GAP_PER_PAGE];
	  int my_line_array[MAX_GAP_PER_PAGE];

	  for (ii = 0; ii < gap_item_no; ii++) { // loop to calculate the x2_l_array
	    int gap_id = overlap_cluster_array[pp][cc].gap_item_array[ii];  // the elem_id
	    int block_l = gap_array[pp][gap_id].block_l;
	    int line_l = block_array[block_l].my_line_in_doc;
	    int x1_l = block_array[block_l].my_x1;
	    int x2_l = block_array[block_l].my_x2;
	    int block_r = gap_array[pp][gap_id].block_r;
	    int line_r = block_array[block_r].my_line_in_doc;
	    int x1_r = block_array[block_r].my_x1;
	    int x2_r = block_array[block_r].my_x2;	  
	    x2_l_array[ii] = x2_l;
	    x1_r_array[ii] = x1_r;
	    my_line_array[ii] = MAX(line_r, line_l);	  
	  }


	  first_line_in_cluster_of_page[pp] = my_line_array[0];
	  last_line_in_cluster_of_page[pp] = my_line_array[gap_item_no-1];
	  int is_median_cluster = 0;	  
	  for (ii = 0; ii < gap_item_no; ii++) {

	    int gap_id = overlap_cluster_array[pp][cc].gap_item_array[ii];
	    int block_l = gap_array[pp][gap_id].block_l;
	    int line_l = block_array[block_l].my_line_in_doc;
	    int x1_l = block_array[block_l].my_x1;
	    int x2_l = block_array[block_l].my_x2;
	    int block_r = gap_array[pp][gap_id].block_r;
	    int line_r = block_array[block_r].my_line_in_doc;
	    int x1_r = block_array[block_r].my_x1;
	    int x2_r = block_array[block_r].my_x2;	  

	    int yr = block_array[block_r].my_y1;
	    int yl = block_array[block_l].my_y1;

	    int total_indent_r = 0;
	    int total_indent_l = 0;	  
	    int jj;
	    for (jj = MAX(ii-6,0); jj < ii+6 && jj < gap_item_no; jj++) {
	      total_indent_l += ((ii != jj && x2_l_array[jj] > 0 && abs(x2_l_array[jj] - x2_l_array[ii]) < 20) ? 1 : 0);
	      total_indent_r += ((ii != jj && x1_r_array[jj] > 0 && abs(x1_r_array[jj] - x1_r_array[ii]) < 20) ? 1 : 0);	    
	    }

	  
	    char *fwl = (block_l >= 0) ? word_array[block_array[block_l].first_word].text : "-";
	    char *lwl = (block_l >= 0) ? word_array[block_array[block_l].last_word].text : "-";	  	  
	    char *fwr = (block_r >= 0) ? word_array[block_array[block_r].first_word].text : "-";
	    char *lwr = (block_r >= 0) ? word_array[block_array[block_r].last_word].text : "-";	  	  
	    //int my_line = MAX(line_r,line_l);
	    int my_y = MAX(yr, yl);	  	  
	    //int dl = ((dl >= gap_item_no-1) ? 9 : (my_line_array[ii+1] - my_line_array[ii]));
	    int dl = my_line_array[ii+1] - my_line_array[ii]; 	    

	    line_property_array[my_line_array[ii]].gap_no_in_cluster = ii;
	    line_property_array[my_line_array[ii]].gap_id = ii;	    
	    line_property_array[my_line_array[ii]].gap_size = MAX(x1_r-x2_l, 0);
	    line_property_array[my_line_array[ii]].block_l = block_l;

	    line_property_array[my_line_array[ii]].block_r = block_r;
	    line_property_array[my_line_array[ii]].block_l_size = x2_l-x1_l;
	    line_property_array[my_line_array[ii]].block_r_size = x2_r-x1_r;
	    //	    line_property_array[my_line_array[ii]].gap_my_line = my_line_array[my_line_array[ii]];
	    line_property_array[my_line_array[ii]].gap_distance = dl;
	    line_property_array[my_line_array[ii]].total_indent_l = total_indent_l;
	    line_property_array[my_line_array[ii]].total_indent_r = total_indent_r;	    

	    line_property_array[my_line_array[ii]].gap_x1 = x2_l;
	    line_property_array[my_line_array[ii]].gap_x2 = x1_r;
	    line_property_array[my_line_array[ii]].gap_y  = my_y;
	    line_property_array[my_line_array[ii]].page  = pp;

	    prev_line = my_line_array[ii];
	    line2block_r_of_gap[my_line_array[ii]] = block_r;
	    line2block_l_of_gap[my_line_array[ii]] = block_l;

	    is_median_cluster += (x2_l < 5000 && (x1_r > 5000 || x1_r == 0)) ? 1 : 0;

	    if (debug) fprintf(stderr,"STAT:  ii=%2d: linearr=%d: gg=%3d: dl=%2d: ti=%2d:%2d ln=%3d: x1=%4d: x2=%4d: gap=%4d: y=%4d: bl=%4d: br=%4d: sz=%4d:%4d: wl=%s:%s:  wr=%s:%s: is_mc=%d:\n"
			       , ii, my_line_array[ii], gap_id, dl, total_indent_r, total_indent_l, my_line_array[ii], x2_l, x1_r, MAX(x1_r-x2_l, 0), my_y, block_l, block_r, x2_l-x1_l, x2_r-x1_r, fwl, lwl, fwr, lwr, is_median_cluster);



	  } // for ii
	  if (debug) fprintf(stderr,"\n");

	} // if (gap_item_no > 0) 
      } // for cc
    } // for pp
  } // pp
  return 0;
} // stats_overlap_clusters(int last_page_no)


/* break the cluster into groups, where the folding happens at the top of the group */
int establish_groups_in_overlap_clusters(int last_page_no) { // find beginning
  int pp;
  int last_renumbered_block = -1;
  for (pp = 1; pp <= last_page_no; pp++) {
    if (page_properties_array[pp].no_toc_clusters == 0  || page_properties_array[pp].no_toc_items < 3)  { // make sure it's not a TOC page
      int ll;
      int in_group = 0;
      int first_line = page_properties_array[pp].first_block_in_page;
      int last_line = page_properties_array[pp].last_block_in_page;

      first_line = first_line_in_cluster_of_page[pp];
      last_line = last_line_in_cluster_of_page[pp];      

      if (1) fprintf(stderr,"ESTABLISHING in PAGE: p=%d: fblock=%d: lblock=%d:\n",pp,  first_line, last_line);
      for (ll = first_line; ll <= last_line; ll++) {
	if ((pp == MY_PN || (pp == 1 && doc_id == 94611))) {
	  fprintf(stderr,"\nEST ll=%d: gd=%d: sec_cond=%d: gin=%d: <=  gnic=%d:", ll
		  , line_property_array[ll].gap_distance
		  , (overlap_cluster_array[pp][0].gap_item_no <= line_property_array[ll].gap_no_in_cluster-1)
		  , overlap_cluster_array[pp][0].gap_item_no, line_property_array[ll].gap_no_in_cluster);
	}
	if (line_property_array[ll].gap_no_in_cluster > -1) { //meaning this line is in a cluster at all
	  int last_line = (line_property_array[ll].gap_distance == 1
			   || overlap_cluster_array[pp][0].gap_item_no <= line_property_array[ll].gap_no_in_cluster-1)
	    ? 0 : 1;
	  if ((pp == MY_PN || (pp == 1 && doc_id == 94611))) fprintf(stderr,"                 ESTABLISH00 ll=%d: last_line=%d: gd=%d: sec_cond=%d:  in_cluster=%d:\n"
			       , ll
			       , last_line
			       , line_property_array[ll].gap_distance			       
			       , (overlap_cluster_array[pp][0].gap_item_no <= line_property_array[ll].gap_no_in_cluster-1)
			       , line_property_array[ll].gap_no_in_cluster);
	  /**********************************  IN A GROUP *************************/
	  if (in_group == 1) {
	    line_group_array[pp][line_group_no[pp]].how_many_lines++;	  
	    line_property_array[ll].belongs_in_group = line_group_no[pp];

	    if (last_line == 1) { // next line is not connected
	      line_group_array[pp][line_group_no[pp]].last_line = ll;
	      line_group_array[pp][line_group_no[pp]].page = pp;	  
	      if ((pp == MY_PN || (pp == 1 && doc_id == 94611))) fprintf(stderr,"   --ESTABLISH last in group ll=%d: gp=%d: how-many=%d:\n",ll , line_property_array[ll].gap_no_in_cluster, line_group_array[pp][line_group_no[pp]].how_many_lines );
	      line_group_no[pp]++;
	      in_group = 0;
	    } else { // keep going
	      ;
	    }
	  /**********************************  NOT IN A GROUP *************************/
	  } else if (in_group == 0) {
	    int good_line = 1;
	    if (debug) fprintf(stderr,"LLL0 pp=%d: gl=%d: ll=%d:\n", pp, good_line, last_line);
	    if (good_line == 1 && last_line != 1) { // good grade, we kill it right away if its the first and the last, we dont want a one-line group
	      line_group_array[pp][line_group_no[pp]].how_many_lines++;	  
	      line_property_array[ll].belongs_in_group = line_group_no[pp];

	      line_group_array[pp][line_group_no[pp]].first_line = ll;
	      if (debug) fprintf(stderr,"    ++ESTABLISH pp=%d: first in group ll=%d: gap_no=%d: group_no=%d:\n",pp, ll, line_property_array[ll].gap_no_in_cluster, line_group_no[pp] );
	      in_group = 1;
	    } else { // keep going
	      ;
	    }
	  }
	  /**********************************  NOT IN A GROUP *************************/	  
	}
      }
    } else {// if not TOC
      if ((pp == MY_PN || (pp == 1 && doc_id == 94611))) fprintf(stderr,"   NOT ESTABLISHING TOC PAGE: p=%d:\n",pp);
    }
    int ii;
    for (ii = 0; ii < line_group_no[pp]; ii++) { // indicate which lines are not full-page DC
      int no_of_lines_in_page = page_properties_array[pp].last_line_in_page - page_properties_array[pp].first_line_in_page +1;
      line_group_array[pp][ii].line_ratio = (float)((float)(line_group_array[pp][ii].how_many_lines) / (float)(no_of_lines_in_page));
    }
  } // pp
  return 0;
} // establish_groups_in_overlap_clusters()

int print_line_groups(int last_page_no) {
  int pp;
  if (debug) fprintf(stderr,"\n\n***********************************  X. GROUPS\n");    
  for (pp = 1; pp <= last_page_no; pp++) {
    int ii;
    fprintf(stderr,"       PAGE GROUPS:%d:%d:\n",pp, line_group_no[pp]);
    for (ii = 0; ii < line_group_no[pp]; ii++) {
      fprintf(stderr,"                     GROUPS:ii=%d: pp=%d: fl_line=%d:%d: fl_line_in_page=%d:%d: no=%d: total_lines_in_page=%d:\n"
	      , ii, line_group_array[pp][ii].page
	      , line_group_array[pp][ii].first_line,  line_group_array[pp][ii].last_line
	      , line_group_array[pp][ii].first_line - page_properties_array[pp].first_line_in_page,  line_group_array[pp][ii].last_line - page_properties_array[pp].first_line_in_page
	      , line_group_array[pp][ii].how_many_lines, page_properties_array[pp].last_line_in_page - page_properties_array[pp].first_line_in_page);
    }
  }
  return 0;
} // print_line_groups(int last_page_no) 



int create_DC_overlap_clusters(int last_page_no) {
  create_gap_array(last_page_no); // result is gap_no[pp]
  print_gap_array(last_page_no);
  calc_overlap_matrix_of_gaps(last_page_no);   // result is gap_overlap_no[pp]
  sort_gap_overlap_array_by_the_overlap(last_page_no);  
  print_gap_ol(last_page_no);
  reset_line_property_array();  
  do_overlap_clusters(last_page_no);
  stats_overlap_clusters(last_page_no);
  establish_groups_in_overlap_clusters(last_page_no); // LINE_GROUP_ARRAY
  print_line_groups(last_page_no);

  return 0;
} // create_median_overlap_clusters() 
