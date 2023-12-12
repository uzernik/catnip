#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include <math.h>  
#include <ctype.h>
#include <unistd.h>
#include "create_toc_ocr.h"
#include "entity_functions.h"
#include "align_toc_functions.h"  
#define MAX(a,b) (a>b)?a:b
#define MIN(a,b) (a<b)?a:b  


struct Max_Align_Cell {
  int found; // found connection between prev and this
  int dist; // dist between prev and this
  int gap; // dist between prev and this
  int style_sim;
  int indent_sim;
  int para_dist;
  int score; // a combo of gap and dist and para and style:  10 - 2*gap - dist
} max_align_array[MAX_GROUP_ITEM][MAX_GROUP_ITEM];

struct Min_Transition { // to store the recursive cummulative_pred so we don't recalculate it in the recursion
  int pred;
  int score;
  int taken;
  int cummulative_value; // how many predecessors
} max_transition_array[500];

int new_seq(char text[], int kk) {
  if (g_seq_no >= MAX_SEQ-1) {
    fprintf(stderr,"Error: g_seq_no =%d >= MAX_SEQ\n",g_seq_no);
  } else if (group_no_array[kk]> 0) {
    g_seq_no++;
  } else {
    ;
  }
  return 0;
}

int align_forced_group(int kk) {
  //fprintf(stderr,"DO_ALIGN_FORCED:\n");
      int hh;
      if (kk == FILE_GROUP) {
	SEQ_70 = ++g_seq_no;
      } else if (kk == FUNDAMENTALS_GROUP) {
	SEQ_71 = ++g_seq_no;
	fprintf(stderr,"SEQ71=%d:\n",g_seq_no);
      }
      if (g_seq_no >= MAX_SEQ-1) {
	fprintf(stderr,"Error: g_seq_no =%d >= MAX_SEQ\n",g_seq_no);
      }

      seq_array[g_seq_no].no_of_items = group_no_array[kk];
      seq_array[g_seq_no].real_no_of_items = group_no_array[kk];
      seq_array[g_seq_no].group_no = kk;
      seq_array[g_seq_no].prev_item = -1;
      fprintf(stderr,"DO_ALIGN_FORCED1:\n");
      for (hh = 0; hh <= group_no_array[kk]; hh++) { // LOOP over ITEMs in GROUP
	int in = group_item_array[kk][hh].item_no;
	seq_item_array[g_seq_no][hh].my_enum = hh+1;

	seq_item_array[g_seq_no][hh].item_no = group_item_array[kk][hh].item_no;
	seq_item_array[g_seq_no][hh].group_no = kk;
	seq_item_array[g_seq_no][hh].group_item_no = hh;
	//fprintf(stderr,"      DO_ALIGN_FORCED2:gin=%d: sn=%d: in=%d: noi=%d:%d: sin=%d:\n",hh, g_seq_no, in, seq_array[g_seq_no].no_of_items, seq_item_array[g_seq_no][hh].item_no);
      }
      //fprintf(stderr,"DONE_ALIGN_FORCED:\n");
  return 0;
} // align_forced_group()


#define BAD_GAP 999
#define BAD_DIST 999
int init_max_align(int kk) {
  int ii, jj;

  for (ii = 0; ii < group_no_array[kk]; ii++) { // LOOP over ITEMs in GROUP, X coord  
    for (jj = 0; jj < group_no_array[kk]; jj++) { // LOOP over ITEMs in GROUP, Y coord
      max_align_array[ii][jj].dist = BAD_DIST;
      max_align_array[ii][jj].gap = BAD_GAP;
    }
  }
  return 0;
}

int calc_gap_and_dist_array(int kk) {
  int mm = 0;
  int ii, jj;  
  // build the array

  init_max_align(kk); // set default_gap to 99

  for (ii = 0; ii < group_no_array[kk]; ii++) { // CREATE THE BASIC DATA, LOOP over ITEMs in GROUP, X coord
    //int found = 0;
    int gap = 0;

    for (gap = 1; gap < 5; gap++) { // STEP1: CREATE THE MATRIX

      for (jj = 0; jj < ii; jj++) { // LOOP over ITEMs in GROUP, Y coord

	if (group_item_array[kk][ii].my_enum == group_item_array[kk][jj].my_enum + gap
	    && group_item_array[kk][jj].my_P_enum == group_item_array[kk][ii].my_P_enum) {
	  max_align_array[ii][jj].dist = ii - jj -1; // 1 is ideal
	  max_align_array[ii][jj].gap = gap -1; // 1 is ideal
	  mm++;
	}  else {
	  ;
	}

      } // jj

    } // for gap

  }
  return 0;
} // calc_gap_and_dist_array(int kk) 

int print_max_align_array(int nn, int kk, int lim) {
  int ii, jj;
  fprintf(stderr,"PRINTING MAX ALIGN ARRAY%d: kk=%d: gno=%d: P[0]=%d:%d: P[1]=%d:%d: P[2]=%d:%d: P[3]=%d:%d: P[4]=%d:%d: P[5]=%d:%d: P[6]=%d:%d: : P[10]=%d:%d: P[11]=%d:%d: P[12]=%d:%d:\n", nn, kk, group_no_array[kk]
	  , group_item_array[2][0].my_P_enum, group_item_array[2][0].my_enum
	  , group_item_array[2][1].my_P_enum, group_item_array[2][1].my_enum
	  , group_item_array[2][2].my_P_enum, group_item_array[2][2].my_enum
	  , group_item_array[2][3].my_P_enum, group_item_array[2][3].my_enum
	  , group_item_array[2][4].my_P_enum, group_item_array[2][4].my_enum
	  , group_item_array[2][5].my_P_enum, group_item_array[2][5].my_enum
	  , group_item_array[2][6].my_P_enum, group_item_array[2][6].my_enum
	  , group_item_array[2][10].my_P_enum, group_item_array[2][10].my_enum
	  , group_item_array[2][11].my_P_enum, group_item_array[2][11].my_enum
	  , group_item_array[2][12].my_P_enum, group_item_array[2][12].my_enum	  	  	  
	  );
  fprintf(stderr,"      ");
  for (ii = 0; ii < group_no_array[kk] && ii < lim; ii++) { // LOOP over ITEMs in GROUP, X coord
    fprintf(stderr,"%2d:%2d  ", group_item_array[kk][ii].my_enum,ii);
  }
  fprintf(stderr,"\n");  
  for (jj = 0; jj < group_no_array[kk] && jj < lim; jj++) { // LOOP further to cancel items on this row
    fprintf(stderr,"%2d:%2d ", jj, group_item_array[kk][jj].my_enum);  
    for (ii = 0; ii < group_no_array[kk] && ii < lim; ii++) { // LOOP over ITEMs in GROUP, X coord
      int gap = max_align_array[ii][jj].gap;
      int next1 = ii - max_align_array[ii][jj].dist;
      int next = (next1 >= 0) ? next1 : -1;
      fprintf(stderr,"%2d:%2d  "
	      , ((gap > 9) ? -9 : gap)
	      , ((max_align_array[ii][jj].dist > 9) ? -9 : max_align_array[ii][jj].dist ) //next
	      //, ii
	      //, group_item_array[kk][ii].my_enum
	      );
    }
    fprintf(stderr,"\n");      
  }
  fprintf(stderr,"\n");    
  fprintf(stderr,"      ");
  for (ii = 0; ii < group_no_array[kk] && ii < lim; ii++) { // LOOP over ITEMs in GROUP, X coord
    fprintf(stderr,"%2d:%2d  ", group_item_array[kk][ii].my_enum,ii);
  }
  fprintf(stderr,"\n");  
  fprintf(stderr,"\n      ");  
  for (ii = 0; ii < group_no_array[kk] && ii < lim; ii++) { // LOOP over ITEMs in GROUP, X coord
    fprintf(stderr,"%2d:%2d  ", max_transition_array[ii].score, max_transition_array[ii].pred);
  }
  fprintf(stderr,"\n\n");  
  return 0;
} // print_max_align_array(int kk) 


struct Temp_Item {
  int my_enum, item_no, group_item_no, group_no;
} temp_seq[MAX_SEQ_NO];

int s_seq_order(int kk, int sn) {
  int ii;
  int noi = seq_array[sn].no_of_items;
  fprintf(stderr,"ORDER: k=%d: sn=%d:%d: ", kk, sn, noi);
  for (ii = 0; ii < noi; ii++) {
    fprintf(stderr," %d:%d:%d: ", seq_item_array[sn][ii].my_enum, seq_item_array[sn][ii].item_no, seq_item_array[sn][ii].group_item_no);
  } 
  fprintf(stderr,"\n");
  return 0;
}

int reverse_seq_order(int kk, int sn) {
  int ii;
  int noi = seq_array[sn].no_of_items;
  //fprintf(stderr,"SBB:%d:%d:%d:\n",kk,sn,noi);
  for (ii = 0; ii < noi; ii++) {
    temp_seq[ii].my_enum = seq_item_array[sn][ii].my_enum;
    temp_seq[ii].item_no = seq_item_array[sn][ii].item_no;
    temp_seq[ii].group_item_no = seq_item_array[sn][ii].group_item_no;
    temp_seq[ii].group_no = seq_item_array[sn][ii].group_no;
  } 
  for (ii = 0; ii < noi; ii++) {
    seq_item_array[sn][ii].my_enum = temp_seq[noi-ii-1].my_enum;
    seq_item_array[sn][ii].item_no = temp_seq[noi-ii-1].item_no;
    seq_item_array[sn][ii].group_item_no = temp_seq[noi-ii-1].group_item_no;
    seq_item_array[sn][ii].group_no = temp_seq[noi-ii-1].group_no;
  } 
  return noi;
}

int insert_into_seq_in_max_align(int kk, int g_sn, int curr_seq_no, int *curr_seq_item) {
  int item_no = group_item_array[kk][g_sn].item_no;
  int my_enum = group_item_array[kk][g_sn].my_enum;
  seq_item_array[curr_seq_no][*curr_seq_item].my_enum = my_enum;
  seq_item_array[curr_seq_no][*curr_seq_item].item_no = item_no;
  seq_item_array[curr_seq_no][*curr_seq_item].group_item_no = g_sn;
  seq_item_array[curr_seq_no][*curr_seq_item].group_no = kk;
  if (0) fprintf(stderr,"IN_INSERT:sn=%d: sitem=%d:%d: enum=%d: in=%d: gin=%d: g=%d:\n"
	  , curr_seq_no, *curr_seq_item, seq_array[curr_seq_no].no_of_items, my_enum, item_no, g_sn, kk);
  group_item_array[kk][g_sn].seq_item_no = *curr_seq_item;
  group_item_array[kk][g_sn].my_seq_no = curr_seq_no;

  seq_array[curr_seq_no].no_of_items = ++(*curr_seq_item);
  seq_array[curr_seq_no].group_no = kk;
  return 0;
} // insert_into_seq_in_max_align

int calc_one_cummulative_value(int kk, int ii, int *max_cummulative_value) {
  int  jj;
  int max_jj = 0;
  *max_cummulative_value = 0;
  for (jj = 0; jj <= ii; jj++) { // find best predecessor for this item
    int gap = max_align_array[ii][jj].gap;
    int dist = max_align_array[ii][jj].dist;
    int score = gap + dist;
    int my_cummulative_value = max_transition_array[jj].cummulative_value + 10 - score ;
    if (my_cummulative_value > *max_cummulative_value)  {
      max_jj = jj;
      *max_cummulative_value = my_cummulative_value;
    }
  }
  return max_jj;
} // calc_one_cummulative_value(int kk, int ii)


int calc_cummulative_chain(int kk) {
  int ii;
  for (ii = 0; ii < group_no_array[kk]; ii++) { // LOOP over ITEMs in GROUP, X coord
    max_transition_array[ii].cummulative_value = 0;
    max_transition_array[ii].taken = 0;    
  }
  if (debug) fprintf(stderr,"\n CUMMULATIVE     \n");
  for (ii = 0; ii < group_no_array[kk]; ii++) { // LOOP over ITEMs in GROUP, X coord
    fprintf(stderr,"   CUMM0 \n");    
    int max_cumm = 0;
    int jj = calc_one_cummulative_value(kk, ii, &max_cumm);
    fprintf(stderr,"   CUMM1 \n");        
    max_transition_array[ii].cummulative_value = max_cumm;
    fprintf(stderr,"   CUMM2 \n");        
    int pred = jj;    
    int gap = max_align_array[ii][jj].gap;
    int dist = max_align_array[ii][jj].dist;
    int score = gap + dist;
    fprintf(stderr,"   CUMM3 \n");    
    if (debug) fprintf(stderr,"CCC0 kk=%d: ii=%3d: gin=%d: pid=%d:\n", kk, ii, group_item_array[kk][ii].item_no, item_array[group_item_array[kk][ii].item_no].pid);
    if (debug) fprintf(stderr,"CCC kk=%d: ii=%3d: gin=%d: pid=%d: sec=%s: en=%2d: cum=%4d: pred=%2d gap=%d: dist=%d: score=%d:\n", kk, ii, group_item_array[kk][ii].item_no, item_array[group_item_array[kk][ii].item_no].pid, item_array[group_item_array[kk][ii].item_no].section /*"seven"*/, 77/*group_item_array[kk][ii].my_enum*/, 78/*max_transition_array[ii].cummulative_value*/, pred, gap, dist, score);
    fprintf(stderr,"   CUMM4 \n");    
  }
  fprintf(stderr,"\n");  
  return 0;
} // calc_cummulative_chain(int kk) 

/*
int get_all_max_sequences(int kk) {
  int done = 0;
  int mm = 0;
  int start_nn = 0;
  g_seq_no++;
  while (done == 0 &&  mm < group_no_array[kk]) { // GO OVER GROUP next 0 and build a seq behind it
    if (debug) fprintf(stderr,"START_SEQ0:%d:\n",g_seq_no);
    int nn;
    int start_nn;
    // initiate array
    for (start_nn = -1, nn = 0; start_nn == -1 && nn < group_no_array[kk]; nn++) {//find the next 0
      if (max_transition_array[nn].cummulative_value == 0
	  &&  max_transition_array[nn].taken == 0) {
	start_nn = nn;
      }
    }
    if (debug) fprintf(stderr,"START_SEQ1:%d: start at %d:%d:\n",g_seq_no,  start_nn,  nn);
    int curr_seq_item = 0;
    if (start_nn == -1) { // if not found then terminate
      done = 1;
    } else { // build a seq from the 0 and up
      int ii = start_nn;
      if (debug) fprintf(stderr,"START_SEQ2=%d: start_nn=%d: \n",g_seq_no, start_nn);      
      max_transition_array[ii].taken = 1;
      insert_into_seq_in_max_align(kk, ii, g_seq_no, &curr_seq_item);
      ii++;
      int highest_cumm_value = 0;
      int highest_cumm_item = ii;
      int done_seq = 0;
      while (done_seq == 0 && ii < group_no_array[kk]) {
	if (max_transition_array[ii].taken == 1) {
	  done_seq = 1;
	} else {
	  if (debug) fprintf(stderr,"MID ii=%d: cv=%d: hcv=%d: \n"
			     , ii, max_transition_array[ii].cummulative_value,highest_cumm_value);
	  if (max_transition_array[ii].cummulative_value > highest_cumm_value) {
	    insert_into_seq_in_max_align(kk, ii, g_seq_no, &curr_seq_item);
	    max_transition_array[ii].taken = 1;
	    highest_cumm_value = max_transition_array[ii].cummulative_value;
	    highest_cumm_item = ii;
	  } else {
	    ; // we can visit it later, it might be noise or an embedded sequence
	  }
	  ii++;
	}
      } // while
      if (debug) fprintf(stderr,"START_SEQ30:%d: last item at %d: highest value=%d, highest item=%d:\n",g_seq_no, ii, highest_cumm_value, highest_cumm_item);            
    } // else
    g_seq_no++;
    mm++;
  }
  return 0;
} // get_all_max_sequences(int kk) {
*/

int get_all_rev_max_sequences(int kk) {
  int done = 0;
  int mm = 0;
  while (done == 0
	 &&  mm < group_no_array[kk]
	 ) { // next 0 and build a seq behind it
    g_seq_no++;
    if (debug) fprintf(stderr,"START_SEQ0:%d: kk=%d:\n", g_seq_no,kk);
    int nn;
    int curr_nn;
    int curr_value = -1;

    for (curr_nn = -1, nn = group_no_array[kk]; nn >=0 ; nn--) {//finding the next highest sequence
      if (max_transition_array[nn].cummulative_value > curr_value
	  && max_transition_array[nn].taken == 0) { // find the hight non-taken value
	curr_value = max_transition_array[nn].cummulative_value; 
	curr_nn = nn;
      }
    }
    if (debug) fprintf(stderr,"  BART_SEQ1:%d: start at %d:%d: csm=%d:\n",g_seq_no, curr_nn, nn, curr_value);
    if (curr_nn == -1) { // if not found then terminate
      done = 1;
    } else { // build a seq from the 0 and up
      int ii = curr_nn;
      int done_seq = 0;
      int next_highest_value = 0;
      int next_highest_item = -1;

      if (debug) fprintf(stderr,"            VISIT_SEQ6:%d: ii=%d:\n", g_seq_no, ii);            	
      //max_transition_array[ii].taken = 1;
      int curr_seq_item = 0;
      if (debug) fprintf(stderr,"START_SEQ2:%d: start at ii=%d: highest=%d: tkn=%d:\n",g_seq_no, ii, next_highest_value, max_transition_array[ii].taken);      
      int goon = 1;
      while (max_transition_array[ii].taken == 0
	     && ii >= 0
	     && goon == 1
	     && item_array[group_item_array[kk][ii].item_no].pid > 0 // under testing
	     ) { // collect items for this sequence, starting from the end, the highest
	if (debug) fprintf(stderr,"\n    BEARCHING0 for next highest on seq :%d: start at ii=%d: highest=%d:%d: pid=%d: enum=%d:%s:\n"
		, g_seq_no, ii, next_highest_value, curr_value
		, item_array[group_item_array[kk][ii].item_no].pid, group_item_array[kk][ii].my_enum, item_array[group_item_array[kk][ii].item_no].section);      
	max_transition_array[ii].taken = 1;
	insert_into_seq_in_max_align(kk, ii, g_seq_no, &curr_seq_item);
	int jj;
	int MM = group_item_array[kk][ii].my_enum;
	int NN = group_item_array[kk][ii-1].my_enum;
	if (debug) fprintf(stderr,"            VISIT_SEQ3:%d: last item at %d: highest value=%d, highest item=%d: AB=%d:%d:\n"
		, g_seq_no, ii, next_highest_value, next_highest_item, MM, NN);            	
	int tt,hh, done = 0;
	for (tt = 0, hh = ii-1; done == 0 && tt < 3 && hh >=0; hh--, tt++) { // we do this little loop so we start with an item that suits our M/N condition (monotonicity)
	  int MM = group_item_array[kk][ii].my_enum;
	  int NN = group_item_array[kk][hh].my_enum;
	  if (MM > NN) {
	    done = 1;
	    next_highest_item = hh;
	    next_highest_value = max_transition_array[hh].cummulative_value;
	  }
	  if (debug) fprintf(stderr,"            VISIT_SEQ13:%d: last item at %d:%d: highest value=%d, highest item=%d: AB=%d:%d:\n"
		  , g_seq_no, ii, hh, next_highest_value, next_highest_item, MM, NN);            	
	}
	if (done == 0) {
	  next_highest_item = ii-1;
	  next_highest_value = max_transition_array[ii-1].cummulative_value;
	}

	for (jj = ii-1; jj >= 0 && max_transition_array[jj].taken == 0; jj--) {
	  int AA = group_item_array[kk][ii].my_enum;
	  int BB = group_item_array[kk][jj].my_enum;

	  if ((ii == 28)) {
	    if (debug) fprintf(stderr,"       SEARCHING for next highest on seq :%d: start at jj=%d: njj=%d: highest=%d:-%d:%d: en=%d:%d: sec=%s:%s:\n"
		     ,g_seq_no, jj, next_highest_item, next_highest_value
		    , max_transition_array[jj].cummulative_value,   max_transition_array[jj].cummulative_value - (next_highest_item - jj)
		    , AA, BB, item_array[group_item_array[kk][ii].item_no].section, item_array[group_item_array[kk][jj].item_no].section
		    );
	  }
	  if (max_transition_array[jj].cummulative_value - (next_highest_item - jj) > next_highest_value
	      && AA > BB) {
	    next_highest_value = max_transition_array[jj].cummulative_value;
	    next_highest_item = jj;
	  }
	} // for jj


	int V1 = max_transition_array[ii].cummulative_value;
	int V2 = next_highest_value;
	int AA =  group_item_array[kk][ii].my_enum;
	int BB =  group_item_array[kk][next_highest_item].my_enum;	
	int diff = ii - next_highest_item;
	
	jj = next_highest_item;
	ii = jj;

	goon = (V1 - diff + 20 > V2 // cummulative calculation
		&& (AA > BB // don't go down: " a b c d ... c " 
		    || (AA == BB // but sometimes by mistake we have "1 2 3 3 4 5"
			&& AA != 1 // but too many times we find 1 1 1 2 3 4 so eliminate
			&& !(AA == 9 && kk == 28))) // (1) is taken as 9 in conversion to I
		&& AA - BB < 6) // don't take a gap larger than 5 "a b c d   ...  u v w"
	  ? 1 : 0;  // this is very important:  otherwise several sequences are bunched together // +20 is added so that seqs won't be chopped off in the middle, the other condition is more important
	if (debug) fprintf(stderr,"             BEARCHING1 for next highest on seq :%d: start at ii=%d: highest=%d:%d: tkn=%d: V1=%d: v2%d: diff=%d: AB=%d:%d: goon=%d:\n"
		,g_seq_no, ii, next_highest_value, curr_value, max_transition_array[ii].taken, V1, V2, diff, AA, BB, goon);
	next_highest_value = 0;	
      } // while

      //s_seq_order(kk,g_seq_no);
      if (debug) fprintf(stderr,"START_SEQ9:%d: last item at %d: highest value=%d, highest item=%d:\n"
	      , g_seq_no, ii, next_highest_value, next_highest_item);
      reverse_seq_order(kk,g_seq_no);
      //s_seq_order(kk,g_seq_no);
      next_highest_item = ii-1;
    } // else
    mm++;
  }
  return 0;
} // get_all_rev_max_sequences(int kk) 

int make_max_align(int kk) { // go over the group and identify sequences via the max_matrix

  //fprintf(stderr,"MAKE_MAX_ALIGN:%d: no=%d:\n", kk, group_no_array[kk]);
  if (group_no_array[kk] > 0) {
    int lim = 13;
    calc_gap_and_dist_array(kk); // STEP1 CALC GAP and DIST MATRIX
    if (debug) {
      print_max_align_array(0, kk, lim);
    }
    calc_cummulative_chain(kk); // STEP2 calculate cummulative value for each item
    get_all_rev_max_sequences(kk); // STEP3 collect the sequences based on momotonous cumm_value 
    //get_all_max_sequences(kk); // STEP3 collect the sequences based on momotonous cumm_value

  }

  return 0;
} // make_max_align(int kk) 




void seq_badness() {
  int ii;
  for (ii = 0; ii <= g_seq_no && ii < MAX_SEQ; ii++) { // seq ii

    int total_diff = 0;
    int total_gap = 0;
    int max_gap = 0;
    int max_diff = 0;
    int noi = seq_array[ii].no_of_items;
    int jj;
    for (jj = 0; jj <= noi; jj++) {
      int gn = seq_item_array[ii][jj].group_no;
      int gin_m = (jj > 0) ? seq_item_array[ii][jj-1].group_item_no :seq_item_array[ii][jj].group_item_no-1 ;
      int gin = seq_item_array[ii][jj].group_item_no;
      int val_m =  (jj > 0) ? group_item_array[gn][gin_m].my_enum : 0;
      int val = group_item_array[gn][gin].my_enum;
      int curr_gap = (gin - gin_m - 1)*(gin - gin_m - 1);
      int curr_diff = (val - val_m -1)*(val - val_m -1); 
      total_diff += curr_diff;
      total_gap += curr_gap;
      max_diff = (max_diff > (val - val_m -1)) ? max_diff : (val - val_m -1);
      max_gap = (max_gap > (gin - gin_m - 1)) ? max_gap : (gin - gin_m - 1);


      int gin_p = (jj < noi) ? seq_item_array[ii][jj+1].group_item_no : (gin + 1);
      int val_p = (jj < noi) ? seq_item_array[ii][jj+1].my_enum : (val + 1);
      seq_item_array[ii][jj].badness = (gin - gin_m) + (gin_p - gin) + (val - val_m) + (val_p - val) - 4;

    }

    //    seq_array[ii].total_diff = 10 * total_diff / ((seq_array[ii].no_of_items+1)*(seq_array[ii].no_of_items+1));
    //    seq_array[ii].total_gap = 10 * total_gap / ((seq_array[ii].no_of_items+1)*(seq_array[ii].no_of_items+1));
    seq_array[ii].total_diff = total_diff;
    seq_array[ii].total_gap = total_gap;
    seq_array[ii].max_diff = max_diff;
    seq_array[ii].max_gap = max_gap;
  }
} // seq_badness()

int add_seq_to_item(int in, int seq_no, int seq_in) {
  int nos = item_array[in].no_of_seqs;
  item_array[in].seq_array[nos].seq_no = seq_no; // update the seq_no where this items appears
  item_array[in].seq_array[nos].seq_item_no = seq_in; // update the seq_item_no where this items appears
  int group_no = seq_item_array[seq_no][seq_in].group_no;
  int group_item_no = seq_item_array[seq_no][seq_in].group_item_no;  
  item_array[in].seq_array[nos].my_enum = group_item_array[group_no][group_item_no].my_enum;
  if (nos < MAX_CONT_SEQ-1) {
    nos++;
  } else {
    fprintf(stderr,"ERROR: nos exceeded MAX_CONT_SEQ:%d:\n",MAX_CONT_SEQ);
  }
  item_array[in].no_of_seqs = nos;
  item_array[in].selected_seq = seq_no;
  return 0;
}

// go over all the seq_items in each sequence and hang it on the array_item on seq_array
int collect_contending_interpretations() {
  int ii;
  int print_contend = 0;
  for (ii = 0; ii <= g_seq_no && ii < MAX_SEQ; ii++) { // seq ii
    int moi1 = seq_array[ii].no_of_items;
    int jj;
    if (0) fprintf(stderr,"NEW BEQ ii=%d  nos1:%d:\n",ii, moi1);
    for (jj = 0; jj < moi1; jj++) { // seq_item jj
      int in = seq_item_array[ii][jj].item_no;
      int nos = item_array[in].no_of_seqs;
      //fprintf(stderr,"NEW SEQxITEM sni=%d sinj=%d in=%d: nos:%d: snj=%d: sinj=%d:\n",ii,jj, in, nos, item_array[in].seq_array[0].seq_no, item_array[in].seq_array[0].seq_item_no);
      
      int sa, sia, diff, gap, bad;
      int pp;
      if (debug) fprintf(stderr,"ADDING NEW ITEM IN=%d seq_no=%d: sin=%d: nos=%d:\n",in,ii, jj, nos);
      add_seq_to_item(in,ii,jj);
    } // for jj
  } // for ii
  return 0;
} // collect_contending_interpretations


// go over all the items in a sequence and resolve the conflicts for each item
int resolve_contending_interpretations() {
  int ii;
  if (debug) fprintf(stderr,"CONTENDING:%d:\n",item_no);
  for (ii = 0; ii <= item_no; ii++) { // item ii
    int nos = item_array[ii].no_of_seqs;
    int my_enum = item_array[ii].my_enum;
    char *section = item_array[ii].section;
    char *header = item_array[ii].clean_header;
    int pid = item_array[ii].pid;
    if (debug) fprintf(stderr,"  CONTEND: ii=%d: nos=%d:-:%s:%s:-:E=%d:%d:-",ii, nos, section, header, pid, my_enum);
    int jj;
    int min_diff = 100000;
    int min_sy = -1;
    int min_siy = -1;

    // check all the contending seqs hanging on this item
    for (jj = 0; jj < nos; jj++) { // contending seq jj
      my_enum = item_array[ii].seq_array[jj].my_enum;
      int sy = item_array[ii].seq_array[jj].seq_no;
      int siy = item_array[ii].seq_array[jj].seq_item_no;

      int noi1 = seq_array[sy].no_of_items;

      int inB = (siy > 0) ? seq_item_array[sy][siy-1].item_no : -1; // before
      int inA = (siy < noi1-1) ? seq_item_array[sy][siy+1].item_no : -2; // after

      // the PID is objective per item_no
      int pidB = (inB != -1) ? item_array[inB].pid : -1;
      int pidA = (inA != -2) ? item_array[inA].pid : -2;

      
      // the ENUM is subjetive.  I, V, X could be 1, 5, 10, or 10, 22, 23
      // so we need to find them per the seq they belong to
      int enumB = (inB != -1) ? item_array[inB].my_enum : -1; // the prev seq item
      int nosB = (inB != -1) ? item_array[inB].no_of_seqs : -1;
      int kk;
      int seqB = 0;
      // if we have a seq "i ii iii iv v vi vii viii ix x" then we want to compare the right enum.
      // so is X going with IX or with V?
      // we need to compare the corresponding enum.  Meaning the enum the corresponds to this seq: 5 (v) goes with 10 (x);  22 (v) goes with 24 (x)
      for (kk = 0; kk < nosB; kk++) {
	seqB = item_array[inB].seq_array[kk].seq_no;
	if (seqB == sy) {
	  enumB = item_array[inB].seq_array[kk].my_enum; // find the enum of the corresponding seq
	  break;
	}
	//fprintf(stderr,"        III:kk=%d: seqb=%d: sy=%d: eB=%d:%d:\n",kk, seqB, sy, enumB, my_enum);	
      }

      int enumA = (inA != -2) ? item_array[inA].my_enum : -2; // the default
      int nosA = (inA != -2) ? item_array[inA].no_of_seqs : -2;
      //fprintf(stderr,"    HHH:jj=%d: sy=%d: siy=%d: nos=%d:%d\n",jj, sy, siy, nosB,nosA);
      int seqA = 0;
      for (kk = 0; kk < nosA; kk++) {
	seqA = item_array[inA].seq_array[kk].seq_no;
	if (seqA == sy) {
	  enumA = item_array[inA].seq_array[kk].my_enum;
	  break;
	}
      }


      int diffB = (pidB != -1) ? (pid - pidB) : 1; // if it's the first item on the seq then do 1 and not 0, bc 1 is the minimal difference
      int diffA = (pidA != -2) ? (pidA - pid) : 1;      
      int enB = (inB != -1) ? (my_enum - enumB) : 1;
      int enA = (inA != -2) ? (enumA - my_enum) : 1;

      int diff_en = abs(enB)+abs(enA);
      int diff_pid = diffA+diffB;
      if (0) fprintf(stderr,"\n            CAND:%d: s=%d:%d:nos=%d:(PID=%d:%d:%d: SEQ=%d:%d:%d: EN=%d:%d:%d:):D=%d:%d: ",jj,sy,siy,noi1,     pid,pidB, pidA,   sy,seqB, seqA,      my_enum, enumB, enumA,     diff_pid, diff_en);
      if (noi1 > 1 && diff_pid + diff_en < min_diff) { // don't select a seq with a single member
	min_diff = diff_pid + diff_en;
	min_sy = sy;
	min_siy = siy;
      }
    } // jj (seq on seq_array on item ii)
    item_array[ii].selected_seq = min_sy;
    if (0) fprintf(stderr," --:%d:%d:%d:\n",min_sy,min_siy,min_diff);
    remove_item_from_seq_contenders(ii ,min_sy);
  } // ii (item)
  return 0;
} // resolve_contending_interpretations

int remove_item_from_seq_contenders(int in, int selected_seq) {
  // each contender that is not selected s/b removed from its seq
  int ii;
  for (ii = 0; ii < item_array[in].no_of_seqs; ii++) {
    int seq_no = item_array[in].seq_array[ii].seq_no;
    int seq_item_no = item_array[in].seq_array[ii].seq_item_no;
    if (seq_no != selected_seq) {
      if (seq_no != SEQ_70) seq_item_array[seq_no][seq_item_no].removed = 1;
      //fprintf(stderr,"                                REM:  in=%d: sn=%d: sin=%d:\n",in,seq_no,seq_item_no);
      seq_array[seq_no].real_no_of_items--;
    }
  }
  return 0;
}

int check_seq() {
  // calculate diff, gap, 
  seq_badness(); // input is seq_array; output is seq_array.max_gap, etc
  tally_headers_caps_converts(); // output is tool_long, total_cap, noi
  fprintf(stderr,"\n      STEP4.1: indent and center:\n");
  
  calculate_indent_and_center(); // center, lexfX AND gap_allowed
  fprintf(stderr,"\n      STEP4.2: resolve contending interpretations:\n");
  collect_contending_interpretations(); // go over each seq and place it in items
  fprintf(stderr,"\n      STEP4.4: mark include relations:\n");
  resolve_contending_interpretations();
  fprintf(stderr,"\n      STEP4.3: first/last:\n");
  calculate_first_and_last_on_seq();  // fn, ln due to removed items
  item_array[0].selected_seq = SEQ_70;
  mark_include_relations(); // calculate incl_array and prev, .include, .overlap, .awkward, rank
  if (debug) print_include_relations();
  return 0;
} // check_seq()
