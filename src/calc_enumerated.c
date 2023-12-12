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

/*******************
 ** 1. cluster of 1 ,2, 3, numbers
 ** 2. sequencing adds points
 ** 3. period takes points (FPs with TOC section)
 ** 4. found any lines right of this cluster takes points
 ** 5. charts?

 *******************/


/************************************* ENUMERATED  *******************************************/


int ppp_enum_cmp(const void *a, const void *b) {
  int x1 = (((struct Enum_Triple *)a)->diff);
  int x2 = (((struct Enum_Triple *)b)->diff);
  return((x1<x2) ? 0 : 1); 
}

int sort_diff_enum_pairs(int pp) {
  qsort((void *)&enumerated_list[pp][0], enumerated_no[pp],  sizeof(struct Enum_Triple), ppp_enum_cmp);
  return 0;
}






float calc_enum_diff(int mm, int kk) {
  float ret = 0;
  int x1_mm = block_array[mm].my_x1;
  int x2_mm = block_array[mm].my_x2;    

  int x1_kk = block_array[kk].my_x1;
  int x2_kk = block_array[kk].my_x2;      

  ret = abs(x1_mm - x1_kk) + abs(x2_mm-x2_kk);

  return ret;
} // calc_enum_diff()


// in AWS periodically they add a dot.  "8" -> "8."
// so we accept it and weed it out as a list
int is_just_digits(char *text, int *found_period) {
  int ii;
  int ret = 1;
  *found_period = 0;
  for (ii = 0; ii < strlen(text); ii++) {
    if (text[ii] == '.') *found_period = 1;    
    else if (isdigit(text[ii]) == 0) ret = 0;
  }
  return ret;
}

// score = sequence - found_period
#define SECTION_OR_ENUM_THRESHOLD 0.2
// assuming page is scanned vertically/ only little tilt, mostly felt when the enumeration gaps in the middle of the page
// for clustering
#define THRESH_ALL 180
#define DIFF_ENUM_THRESHOLD THRESH_ALL
#define ADD_ENUM_THRESHOLD THRESH_ALL
#define MERGE_ENUM_THRESHOLD THRESH_ALL

// the allowed size
#define SIZE_THRESHOLD 300
#define DIFF_ENUM_THRESHOLD_MY 200
#define LEFT_LIMIT 2000

#define BY_WORD 0

int create_enum_diff_array(int pp) {
    int first_block = page_properties_array[pp].first_block_in_page;
    int last_block = page_properties_array[pp].last_block_in_page;


    int ll1;
    for (ll1 = first_block; ll1 <= last_block; ll1++) {
      int my_line1 = block_array[ll1].my_line_in_doc;
      int first_in_line1 = line_property_array[my_line1].first_block_in_line;
      if (first_in_line1 == ll1) {
	int found_period1;

	char *text_mm;
	int size1;
	int fw1;
	if (BY_WORD) {
	  fw1 = block_array[ll1].first_word;
	  text_mm = word_array[fw1].text;
	  size1 = word_array[fw1].my_x2 - word_array[fw1].my_x1;
	} else {
	  text_mm = block_array[ll1].text;
	  size1 = block_array[ll1].my_x2 - block_array[ll1].my_x1;
	}
	int len_mm = (text_mm) ? strlen(text_mm) : 10000;
	int just_digits_mm = (text_mm) ? is_just_digits(text_mm, &found_period1) : 0;

	int ll2;
	for (ll2 = ll1+1; ll2 <= last_block; ll2++) {
	  int my_line2 = block_array[ll2].my_line_in_doc;
	  int first_in_line2 = line_property_array[my_line2].first_block_in_line;
	  if (first_in_line2 == ll2) {
	    int found_period2;

	    char *text_kk;
	    int size2;
	    int fw2;
	    if (BY_WORD) {
	      fw2 = block_array[ll2].first_word;
	      text_kk = word_array[fw2].text;
	      size2 = word_array[fw2].my_x2 - word_array[fw2].my_x1;
	    } else {
	      text_kk = block_array[ll2].text;
	      size2 = block_array[ll2].my_x2 - block_array[ll2].my_x1;
	    }
	    int len_kk = (text_kk) ? strlen(text_kk) : 10000;
	    int just_digits_kk = (text_kk) ? is_just_digits(text_kk, &found_period2) : 0;

	    int diff = calc_enum_diff(ll1, ll2); // the discrepancy -- not relative to the size
	    if (diff < DIFF_ENUM_THRESHOLD_MY
		&& size1 < SIZE_THRESHOLD && size2 < SIZE_THRESHOLD
		&& (BY_WORD == 0 && block_array[ll1].my_x2 < LEFT_LIMIT && block_array[ll2].my_x2 < LEFT_LIMIT
		    || BY_WORD == 1 && word_array[fw1].my_x2 < LEFT_LIMIT && word_array[fw2].my_x2 < LEFT_LIMIT)

		&& len_mm < 3 && len_kk < 3
		&& just_digits_kk == 1 && just_digits_mm == 1
	    
		) {
	      enumerated_list[pp][enumerated_no[pp]].diff = diff;
	      enumerated_list[pp][enumerated_no[pp]].found_period1 = found_period1;
	      enumerated_list[pp][enumerated_no[pp]].found_period2 = found_period2;	      	      
	      enumerated_list[pp][enumerated_no[pp]].ll = ll1;
	      enumerated_list[pp][enumerated_no[pp]].kk = ll2;
	      enumerated_list[pp][enumerated_no[pp]].page = pp;
	      enumerated_list[pp][enumerated_no[pp]].size1= size1;
	      enumerated_list[pp][enumerated_no[pp]].size2= size2;	  	  
	      enumerated_no[pp]++;
	    } // if
	  } // if first in line
	} // ll2
      } // if first in line
    } // ll1
    
   return 0;
} // create_enum_diff_array(int pp) 


int print_enum_diff_list(int pp, int enumerated_no_pp) {
  int ii;
  fprintf(stderr,"PRINTING ENUM PAGE: p=%d: no=%d:\n"  , pp , enumerated_no_pp);
  for (ii = 0; ii < enumerated_no_pp; ii++) {
    fprintf(stderr,"ENUM: ii=%d: diff=%d: size=%d:%d: p=%d: lk=%d:%d: t=%s:%s: fp=%d:%d:\n"
	    , ii
	    , enumerated_list[pp][ii].diff

	    , enumerated_list[pp][ii].size1
	    , enumerated_list[pp][ii].size2
	    
	    , pp

	    , enumerated_list[pp][ii].ll 
	    , enumerated_list[pp][ii].kk

	    , block_array[enumerated_list[pp][ii].ll].text
	    , block_array[enumerated_list[pp][ii].kk].text	    

	    , enumerated_list[pp][ii].found_period1
	    , enumerated_list[pp][ii].found_period2
	    );
  }
  return 0;
}



struct Enum_Cluster { 
  struct Enum_Item {int block;int found_period;} enum_item_array[MAX_BLOCKS_IN_CLUSTER];  
  int enum_no;  // how many block AND items exist in the list
  float score; // mostly tease apart section numbers from enums
} enum_cluster_array[MAX_PAGE][MAX_CLUSTERS];
int enum_cluster_no[MAX_PAGE];
int Real_enum_cluster_no[MAX_PAGE];


int ppp_enum_item_cmp(const void *a, const void *b) {
  int x1 = (((struct Enum_Item *)a)->block);
  int x2 = (((struct Enum_Item *)b)->block);
  return(x1-x2);
}


int found_item_in_enum_cluster(int pp, int cc, int block) {
  int nn;
  int found_in_cluster = 0;
  for (nn = 0; found_in_cluster == 0 && nn < enum_cluster_array[pp][cc].enum_no; nn++) { // check all the items of the cluster
    if (enum_cluster_array[pp][cc].enum_item_array[nn].block == block) {            
      found_in_cluster = 1;
    }
  }
  return found_in_cluster;
}

int enum_merge_clusters_J_into_I(int pp, int cI_in, int cJ_in) {
  int cI = MIN(cI_in, cJ_in);
  int cJ = MAX(cI_in, cJ_in);
  {
    int nII = enum_cluster_array[pp][cI].enum_no;
    int nJJ;
    for (nJJ = 0; nJJ < enum_cluster_array[pp][cJ].enum_no; nJJ++) {

      enum_cluster_array[pp][cI].enum_item_array[nII].block = enum_cluster_array[pp][cJ].enum_item_array[nJJ].block;
      enum_cluster_array[pp][cI].enum_item_array[nII].found_period = enum_cluster_array[pp][cJ].enum_item_array[nJJ].found_period;      

      nII++;
    }

    enum_cluster_array[pp][cI].enum_no = nII;
    enum_cluster_array[pp][cJ].enum_no = 0;
  }
  return cI;
}


int do_enum_clusters(int pp) {
  enum_cluster_no[pp] = 0;
  Real_enum_cluster_no[pp] = 0;  

  int ss; // the similarity item
  int cc; // the cluster
  int cI = -1;
  int cJ = -1;

  fprintf(stderr, "ENUM CLUSTERING PAGE=%d:  no=%d:\n", pp, enumerated_no[pp]);
  for (ss = 0; ss < enumerated_no[pp]; ss++) { 
    //if (debug) fprintf(stderr, "**GO OVER SS=%d: CL_NO=%d:%d:\n", ss, enum_cluster_no[pp], Real_enum_cluster_no[pp]);
    int found_in_clusters = 0;
    cI = 0; cJ = 0;
    int foundI = 0, foundJ = 0;
    for (cc = 0; found_in_clusters < 2 && cc < enum_cluster_no[pp]; cc++) { // check if new sim is already in an EXISTING CLUSTER
      if (foundI == 0) {
	foundI = found_item_in_enum_cluster(pp, cc, enumerated_list[pp][ss].ll);
	if (foundI == 1) {
	  cI = cc;
	}
      }
      
      if (foundJ == 0) {
	foundJ = found_item_in_enum_cluster(pp, cc, enumerated_list[pp][ss].kk);
	if (foundJ == 1) {
	  cJ = cc;
	}
      }
      
    } // for all clusters



    //if (debug) fprintf(stderr,"       FOUND ss=%d: fIJ1=%d:%d: cIJ=%d:%d: sim=%f: CL_NO=%d:\n", ss, foundI, foundJ, cI, cJ, enumerated_list[pp][ss].diff, enum_cluster_no[pp]);
    int ii0 = enumerated_list[pp][ss].ll;
    int ii1 = enumerated_list[pp][ss].kk;
    int found_period1= enumerated_list[pp][ss].found_period1;
    int found_period2= enumerated_list[pp][ss].found_period2;
    int found_period= enumerated_list[pp][ss].found_period2;
    
    float diff01 = enumerated_list[pp][ss].diff;
    if (foundI == 1 && foundJ == 0 && diff01 <= ADD_ENUM_THRESHOLD) {

      enum_cluster_array[pp][cI].enum_item_array[enum_cluster_array[pp][cI].enum_no].block = ii1;
      enum_cluster_array[pp][cI].enum_item_array[enum_cluster_array[pp][cI].enum_no].found_period = found_period2;            
      enum_cluster_array[pp][cI].enum_no++;

      if (enum_cluster_array[pp][cI].enum_no >= MAX_BLOCKS_IN_CLUSTER-1) {
	fprintf(stderr,"No of itemd exceeded %d:\n", MAX_BLOCKS_IN_CLUSTER);
	exit(0);
      }

    } else if (foundJ == 1 && foundI == 0  && diff01 <= ADD_ENUM_THRESHOLD) {

      enum_cluster_array[pp][cJ].enum_item_array[enum_cluster_array[pp][cJ].enum_no].block = ii0;
      enum_cluster_array[pp][cJ].enum_item_array[enum_cluster_array[pp][cJ].enum_no].found_period = found_period1;      
      enum_cluster_array[pp][cJ].enum_no++;

      if (enum_cluster_array[pp][cJ].enum_no >= MAX_BLOCKS_IN_CLUSTER-1) {
	fprintf(stderr,"No of itemd exceeded %d:\n", MAX_BLOCKS_IN_CLUSTER);
	exit(0);
      }
      
    } else if (foundI == 1 && foundJ == 1
	       && cI >= 0 && cJ >=0 && cI != cJ
	       && enumerated_list[pp][ss].diff <= MERGE_ENUM_THRESHOLD) { // merge clusters // any conditions on merge? what if each group is happy with 15 items?


      enum_merge_clusters_J_into_I(pp, cI, cJ);
      Real_enum_cluster_no[pp]--;

    } else if (foundI == 0 && foundJ == 0 && diff01 <= ADD_ENUM_THRESHOLD) { // create new cluster with 2 items for this similarity
      int cc_enum_no = 0;

      enum_cluster_array[pp][enum_cluster_no[pp]].enum_item_array[cc_enum_no].block = ii0;
      enum_cluster_array[pp][enum_cluster_no[pp]].enum_item_array[cc_enum_no].found_period = found_period1;
      cc_enum_no++;
	

      enum_cluster_array[pp][enum_cluster_no[pp]].enum_item_array[cc_enum_no].block = ii1;
      enum_cluster_array[pp][enum_cluster_no[pp]].enum_item_array[cc_enum_no].found_period = found_period2;
      cc_enum_no++;
	
      enum_cluster_array[pp][enum_cluster_no[pp]].enum_no = cc_enum_no;
	
      if (enum_cluster_no[pp] >= MAX_CLUSTERS-1) {
	fprintf(stderr,"No of cluster exceeded %d:\n", MAX_CLUSTERS);
	exit(0);
      }
      enum_cluster_no[pp]++;
      Real_enum_cluster_no[pp]++;      
    }
  } // SS
  if (debug) fprintf(stderr, "FINISHED PAGE clusters :%d:\n", cc);

  // sort the blocks by order
  for (cc = 0; cc < enum_cluster_no[pp]; cc++) {
    qsort((void *)&(enum_cluster_array[pp][cc].enum_item_array),  enum_cluster_array[pp][cc].enum_no, sizeof(struct Enum_Item), ppp_enum_item_cmp);
  }


  enum_report_array[pp].real_no_of_clusters = 0;
  for (cc = 0; cc < enum_cluster_no[pp]; cc++) {
    if (enum_cluster_array[pp][cc].enum_no > 0) {
      enum_report_array[pp].real_no_of_clusters++;
    }
  }


  return enum_cluster_no[pp];
} // do_enum_clusters()

int create_leftmost_coord_array(int pp, int mean_x1_of_enum) {
  /* eliminate enums in cases;
     1 Jggggg
     Gfdgdgdg
     2 Hdfdf
     Fffffff

OR
     2 Hfef
       2.1 xcsc
       2.2 ssf
     3 Kfefef
   */
  
  if (enum_report_array[pp].real_no_of_clusters > 0) {
  int first_block = page_properties_array[pp].first_block_in_page;
  int last_block = page_properties_array[pp].last_block_in_page;
  int ll;
  int last_enum = -1;
  int sub_enum = -1;
  int total_subsection = 0, total_long_line = 0;
  for (ll = first_block; ll <= last_block; ll++) {
    int my_line = block_array[ll].my_line_in_doc;
    int first_in_line = line_property_array[my_line].first_block_in_line;
    if (ll == first_in_line) {
      int found_period;

      char *text;
      int size;
      int fw;
      text = block_array[ll].text;
      int vtext = atoi(text);
      size = block_array[ll].my_x2 - block_array[ll].my_x1;
      int xleft = block_array[ll].my_x1;
      int len_mm = (text) ? strlen(text) : 10000;
      int just_digits = (text) ? is_just_digits(text, &found_period) : 0;
      int diff = xleft - mean_x1_of_enum;
      char *xxx = (diff < 80) ? "    " : "";
      int jj0,jj1; char tt[50000];
      int d_d = sscanf(text,"%d.%d%s",&jj0,&jj1,tt);
      sub_enum = 0;
      if (d_d == 1) { 
	last_enum = vtext;
      } else if (d_d == 2) {
	sub_enum = (vtext == last_enum) ? 1 : -1;
      }

      int long_line = 0; //WHAT IS THIS?? OUT FOR NOW... 12/23/2020
      if (diff < 80 && d_d == 0 && len_mm > 5) {
	long_line = 1;
	enum_report_array[pp].total_long_lines_same_x1++;
      }

      int subsection = 0;
      if (d_d == 2 && sub_enum == 1) {
	subsection = 1;
	enum_report_array[pp].total_subsection++; // "4.5" starts anywhere following a "4" enum
      }
      
     
      if (0) fprintf(stderr, " %s  LN=%d: cc=%d: x=%d:%d: diff=%d: sz=%d: len=%d: jd=%d: fp=%d: vt=%d: type=%d: last_enum=%d: sub_enum=%d: long_line=%d: subsection=%d: t=%s:\n"
	      , xxx, ll, enum_cluster_no[pp], xleft, mean_x1_of_enum, diff, size, len_mm, just_digits, found_period, vtext, d_d, last_enum, sub_enum, long_line, subsection, text);
    }
  }
  fprintf(stderr, " CONFLICT ENUM: pp=%d: subsections=%d: long_lines=%d:\n", pp, total_subsection, total_long_line);
  }
  return 0;
} // create_leftmost_coord_array(int pp, int mean_x1_of_enum) {


int mark_and_print_enum_one_cluster(int pp, int cc, int *nn) {
  if (enum_cluster_array[pp][cc].enum_no > 0) {
    int bk1 = enum_cluster_array[pp][cc].enum_item_array[0].block;
    fprintf(stderr, "      ENUM CLUSTER: pp=%d: cc=%d:%d: score=%4.2f:%4.2f:  no_of_items=%d: first_t=%s:\n"
	    , pp ,(*nn)++,cc, enum_cluster_array[pp][cc].score, SECTION_OR_ENUM_THRESHOLD, enum_cluster_array[pp][cc].enum_no, block_array[bk1].text);
    int ii;
    for (ii = 0; ii < enum_cluster_array[pp][cc].enum_no; ii++) {
      if (0 ||
	  (enum_cluster_array[pp][cc].score > SECTION_OR_ENUM_THRESHOLD
	   && enum_report_array[pp].total_subsection < 2
	   //&& enum_report_array[pp].total_long_lines_same_x1 < 0
	   )
	  ) {
	int bk = enum_cluster_array[pp][cc].enum_item_array[ii].block;
	char *text = block_array[bk].text;
	int xleft = block_array[bk].my_x1;
	int vtext = atoi(text);
	fprintf(stderr, "   :%d:%d:-:%s:%d: x=%d: CC:%d:%d:%d:", ii, enum_cluster_array[pp][cc].enum_item_array[ii].block, text, vtext, xleft
		, enum_cluster_array[pp][cc].score > SECTION_OR_ENUM_THRESHOLD
		, enum_report_array[pp].total_subsection < 2
		, enum_report_array[pp].total_long_lines_same_x1 < 0
		);
	block_array[bk].is_header_footer = ENUM_DESIGNATION;
	total_no_of_enums++;
      }
    }
    fprintf(stderr, "\n");
    enum_report_array[pp].cluster_serial_no = cc;
    enum_report_array[pp].cluster_no_of_enums = enum_cluster_array[pp][cc].enum_no;
    if (enum_cluster_array[pp][cc].enum_no > 0) total_no_of_enum_pages++;

  }     // if no > 0

  return 0;
}


int stats_cluster(int pp) {
  int cc;
  fprintf(stderr," STATING ALL CLUSTERS:%d:%d: PAGE:%d:\n"
	  , enum_report_array[pp].real_no_of_clusters,  enum_cluster_no[pp], pp);  
  int min_mean_x1 = 10000;
  int nn = 0;
  for (cc = 0; cc < enum_report_array[pp].real_no_of_clusters/*enum_cluster_no[pp]*/; cc++) {

    int ii;
    int total_x1 = 0;
    int total_x2 = 0;
    int total_size = 0;
    int total_found_periods = 0;
    int first_item = 0;
    int total_sequenced = 0;
    int last_num = 0;
    int total_item = enum_cluster_array[pp][cc].enum_no;
    fprintf(stderr,"    STATING ONE CLUSTER:%d: PAGE:%d: no_of_items=%d:\n",cc,pp, total_item);
    for (ii = 0; ii < total_item; ii++) {
      int bk = enum_cluster_array[pp][cc].enum_item_array[ii].block;
      int fw = block_array[bk].first_word;
      total_x1 += ((BY_WORD == 0) ? block_array[bk].my_x1 : word_array[fw].my_x1);
      total_x2 += ((BY_WORD == 0) ? block_array[bk].my_x2 : word_array[fw].my_x2);
      total_size += ((BY_WORD == 0) ? (block_array[bk].my_x2 - block_array[bk].my_x1) : (word_array[fw].my_x2 - word_array[fw].my_x1));
      int my_num = atoi(block_array[bk].text);
      if (ii == 0) {
	first_item = my_num;
      } else if (ii > 0) {
	total_sequenced += ((my_num - last_num == 1) ? 1 : ((my_num - last_num == 0) ? -1 : -1 * (my_num - last_num)));
      }
      last_num = my_num;
      total_found_periods += enum_cluster_array[pp][cc].enum_item_array[ii].found_period; // periods mean it's TOC section, not enum
    } // ii
    int mean_x1 = (int)((float)total_x1 / (float)total_item);
    int mean_x2 = (int)((float)total_x2 / (float)total_item);    
    int total_lines_delta_below_mean_x1 = 0;
    int total_lines_below_mean_x1 = 0;
    int bb;
    int mm;
    for (bb = page_properties_array[pp].first_block_in_page, mm=0; bb <= page_properties_array[pp].last_block_in_page; bb++,mm++) {
      if (0) fprintf(stderr,"               ENUM LINE pp=%d: cc=%d: bb=%d: mm=%d: text=%s: x1=%d: mean_x1=%d:\n"
	      , pp, cc, bb, mm, block_array[bb].text, block_array[bb].my_x1, mean_x1);

      int ii;
      int found_bb_in_cluster = 0;
      for (ii = 0; found_bb_in_cluster == 0 && ii < total_item; ii++) { // make sure you don't include portruding enums
	int bk = enum_cluster_array[pp][cc].enum_item_array[ii].block;
	if (bb == bk) found_bb_in_cluster = 1;
      }

      if (found_bb_in_cluster == 0 && block_array[bb].my_x1 < mean_x1) {
	total_lines_delta_below_mean_x1 += ( mean_x1 - block_array[bb].my_x1);
	total_lines_below_mean_x1++;
	int fw = block_array[bb].first_word;
	if (0) fprintf(stderr,"PORTRUDE ENUM LINE pp=%d: cc=%d: bb=%d: mm=%d: fw=%d: text=%s:%s: x1=%d: mean_x1=%d: total=%d:\n"
		, pp, cc, bb, mm, fw, block_array[bb].text, word_array[fw].text, block_array[bb].my_x1, mean_x1, total_lines_below_mean_x1);
      }
    }

    int total_lines_in_page = page_properties_array[pp].last_line_in_page - page_properties_array[pp].first_line_in_page - total_item; // from the total number of lines subtract the number of item

    float relative_lines_below_mean_x1 = (float)((float)total_lines_below_mean_x1/(float)((total_lines_in_page < 1) ? 1 : total_lines_in_page)); // lines on the left of the enums mean it's not an enum cluster
    float relative_lines_delta_below_mean_x1 = (float)((float)total_lines_delta_below_mean_x1/(float)((total_lines_in_page < 1) ? 1 : total_lines_in_page));    

    float relative_sequenced = (float)((float)total_sequenced/(float)((total_item < 1) ? 1 : total_item-1)); // enums come in a sequence
    float relative_found_periods = (float)((float)total_found_periods/(float)((total_item < 1) ? 1 : total_item));
    float score = 0
      + 1.0 * (float)relative_sequenced
      - 1.0 * (float)relative_found_periods
      - 0.1 * (float)total_lines_below_mean_x1
      ;
    enum_cluster_array[pp][cc].score = score;    
    enum_report_array[pp].score = score;

    if (0 && total_item > 0) fprintf(stderr,"    STATS POST CLUSTER=%d:%d: no=%d: fi=%d: x1=%d: mean=%d: x2=%d:%d: s=%d:%d: fp=%d: FPR=%4.2f: seq=%d: SEQR=%4.2f: lines=%d:  blwr=%d:%4.2f: BLWT/10=%4.2f: del_belw=%d:%4.2f: SCORE=%4.2f:\n"
				     , pp, cc
				     , total_item, first_item
				     , total_x1, mean_x1
				     , total_x2, mean_x2
				     , total_size, (total_size/(total_item < 1) ? 1 : total_item)
				     , total_found_periods, relative_found_periods
				     , total_sequenced,  relative_sequenced
				     , total_lines_in_page
				     , total_lines_below_mean_x1, relative_lines_below_mean_x1, (float)total_lines_below_mean_x1*0.1
				     , total_lines_delta_below_mean_x1, relative_lines_delta_below_mean_x1
				     , score
				);

    if (mean_x1 > 0 && mean_x1 < 5000) min_mean_x1 = MIN(mean_x1, min_mean_x1);
    mark_and_print_enum_one_cluster(pp, cc, &nn);
  } // cc
  create_leftmost_coord_array(pp, min_mean_x1);


  return min_mean_x1;
} // stats_cluster()


int print_enum_report(int last_page_no) {
  fprintf(stderr,"ENUM REPORT: no_of_pages=%d: no_of_enums=%d:\n", total_no_of_enum_pages, total_no_of_enums);
  int pp;
  for (pp = 1; pp <= last_page_no; pp++) {
    if (enum_report_array[pp].cluster_no_of_enums > 0) {
      fprintf(stderr,"    PAGE ENUM REPORT: pp=%d: no_clusters=%d: no_enums=%d: score=%d: subsection=%d: ll_x1=%d:\n"
	      , pp, enum_report_array[pp].real_no_of_clusters, enum_report_array[pp].cluster_no_of_enums
	      , enum_report_array[pp].score, enum_report_array[pp].total_subsection, enum_report_array[pp].total_long_lines_same_x1);
    }
  }
  return 0;
}


int create_enumerated_clusters(int last_page_no) {  
  total_no_of_enum_pages = total_no_of_enums = 0;
  int pp;
  fprintf(stderr,"***START ENUMS:\n");
  for (pp = 1; pp <= last_page_no; pp++) {
    create_enum_diff_array(pp);
    fprintf(stderr,"********************* PRINTING ENUMS BEFORE SORT:%d:\n", enumerated_no[pp]);  
    if (pp == 6) print_enum_diff_list(pp,enumerated_no[pp]);  
    sort_diff_enum_pairs(pp);
    fprintf(stderr,"********************* PRINTING ENUMS AFTER SORT:%d:\n", enumerated_no[pp]);
    if (pp == 6) print_enum_diff_list(pp,enumerated_no[pp]);
    fprintf(stderr,"********************* CLUSTERING ENUMS:%d:\n", enumerated_no[pp]);    
    int nn = do_enum_clusters(pp);
    fprintf(stderr,"********************* MARKING ENUMS:%d:\n", enumerated_no[pp]);    
    stats_cluster(pp);
    fprintf(stderr,"********************* LEFTMOST\n");        
  }
  print_enum_report(last_page_no);
  fprintf(stderr,"*** DONE ENUMS\n");
  return 0; 
} // create_footer_clusters()


/*****************************************************************************/
 
