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


int diff_rental_no;

int ppp_rental_cmp(const void *a, const void *b) {
  int x1 = (((struct Rental_Triple *)a)->diff);
  int x2 = (((struct Rental_Triple *)b)->diff);
  return((x1<x2) ? 0 : 1); 
}

int sort_diff_rental_pairs(int pp) {
  qsort((void *)&rental_list[pp][0], rental_no[pp],  sizeof(struct Rental_Triple), ppp_rental_cmp);
  return 0;
}



float coord_diff(int mm, int kk) {
  float ret = 0;
  int x1_mm = block_array[mm].my_x1;
  int x2_mm = block_array[mm].my_x2;    

  int x1_kk = block_array[kk].my_x1;
  int x2_kk = block_array[kk].my_x2;      

  ret = abs(x1_mm - x1_kk);// + abs(x2_mm-x2_kk);

  return ret;
} // coord_diff()


// in AWS periodically they add a dot.  "8" -> "8."
// so we accept it and weed it out as a list
/*
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
*/

// score = sequence - found_period
#define SECTION_OR_RENTAL_THRESHOLD 0.2
// assuming page is scanned vertically/ only little tilt, mostly felt when the enumeration gaps in the middle of the page
// for clustering
#define THRESH_ALL 180
#define THRESH_ALL 180
#define DIFF_RENTAL_THRESHOLD THRESH_ALL
#define ADD_RENTAL_THRESHOLD THRESH_ALL
#define MERGE_RENTAL_THRESHOLD THRESH_ALL

#define DIFF_RENTAL_THRESHOLD_MY 200
#define LEFT_LIMIT 2000
#define DIFF_RENTAL_X1 45

// the allowed size of the block
#define MAX_FIRST 2
int SIZE_THRESHOLD[MAX_FIRST];

int is_sectionf(char *text) { // section, article, part
  int ret = ((strncasecmp(text,"part",4) == 0 && strncasecmp(text,"party",5) != 0 && strncasecmp(text,"parties",7) != 0)
	     || strncasecmp(text,"section",7) == 0
	     || strncasecmp(text,"article",7) == 0
	     ) ? 1 : 0;
  //fprintf(stderr,"AAA:%d:---:%s:\n",ret,text);
  return ret;
}


int create_rental_diff_array(int pp, int first, int do_rf) { // first 0 is first / first 1 is second
    int first_block = page_properties_array[pp].first_block_in_page;
    int last_block = page_properties_array[pp].last_block_in_page;

    int ll1;
    for (ll1 = first_block;  do_rf && ll1 <= last_block; ll1++) {
      int my_line1 = block_array[ll1].my_line_in_doc;
      int first_in_line1 = line_property_array[my_line1].first_block_in_line;
      if (ll1 == first_in_line1+first) { // take only the first or the second
	if (first != 0 // either in second cluster
	    || (block_array[ll1].my_x2 - block_array[ll1].my_x1 > 300 // or text longer than 300
		&& is_sectionf(block_array[ll1].text) == 0)) { // and not a section
	char *text_mm;
	int size1;
	int fw1;
	text_mm = block_array[ll1].text;
	size1 = block_array[ll1].my_x2 - block_array[ll1].my_x1;
	int len_mm = (text_mm) ? strlen(text_mm) : 10000;
	int ll2;
	for (ll2 = ll1+1; ll2 <= last_block; ll2++) {
	  int my_line2 = block_array[ll2].my_line_in_doc;
	  int first_in_line2 = line_property_array[my_line2].first_block_in_line;
	  if (ll2 == first_in_line2+first) {


	    char *text_kk;
	    int size2;
	    int fw2;
	    text_kk = block_array[ll2].text;
	    size2 = block_array[ll2].my_x2 - block_array[ll2].my_x1;

	    int len_kk = (text_kk) ? strlen(text_kk) : 10000;
	    int diff = coord_diff(ll1, ll2); // the discrepancy -- not relative to the size
	    if (1
		&& diff < DIFF_RENTAL_X1
		&& size1 < SIZE_THRESHOLD[first] && size2 < SIZE_THRESHOLD[first]
	    
		) {
	      rental_list[pp][rental_no[pp]].diff = diff;
	      rental_list[pp][rental_no[pp]].ll = ll1;
	      rental_list[pp][rental_no[pp]].kk = ll2;
	      rental_list[pp][rental_no[pp]].page = pp;
	      rental_no[pp]++;
	    } // if
	  } // if first in line
	} // ll2
      } // if first in line
      }
    } // ll1
    
   return 0;
} // create_rental_diff_array(int pp) 


int print_rental_diff_list(int pp, int rental_no_pp) {
  int ii;
  fprintf(stderr,"PRINTING RENTAL PAGE: p=%d: no=%d:\n"  , pp , rental_no_pp);
  for (ii = 0; ii < rental_no_pp; ii++) {
      fprintf(stderr,"RRR: ii=%d: diff=%d: p=%d: lk=%d:%d: t=%s:%s:\n"
	    , ii
	    , rental_list[pp][ii].diff

	    , pp

	    , rental_list[pp][ii].ll 
	    , rental_list[pp][ii].kk

	    , block_array[rental_list[pp][ii].ll].text
	    , block_array[rental_list[pp][ii].kk].text	    

	    );

  }
  return 0;
}


// we cluster on first and second block in line
#define MAX_RENTAL_CLUSTERS 10
struct Rental_Cluster { 
  struct Rental_Item {int block;int found_period;} enum_item_array[MAX_BLOCKS_IN_CLUSTER];  
  int enum_no;  // how many block AND items exist in the list
  float score; // mostly tease apart section numbers from enums
  float tot_digits, tot_roman, tot_ROMAN, tot_paren, tot_section, tot_size, tot_quote, tot_rental_size;
  float std_size, avg_size;
  float avg_xleft;
  float tot_sections;
  int letitbe;
} rental_cluster_array[MAX_PAGE][MAX_RENTAL_CLUSTERS][MAX_FIRST];
int rental_cluster_no[MAX_PAGE][MAX_FIRST];
int Real_rental_cluster_no[MAX_PAGE][MAX_FIRST];


int ppp_rent_item_cmp(const void *a, const void *b) {
  int x1 = (((struct Rental_Item *)a)->block);
  int x2 = (((struct Rental_Item *)b)->block);
  return(x1-x2);
}


int found_item_in_rental_cluster(int pp, int cc, int block, int first) {
  int nn;
  int found_in_cluster = 0;
  for (nn = 0; found_in_cluster == 0 && nn < rental_cluster_array[pp][cc][first].enum_no; nn++) { // check all the items of the cluster
    if (rental_cluster_array[pp][cc][first].enum_item_array[nn].block == block) {            
      found_in_cluster = 1;
    }
  }
  return found_in_cluster;
}

int merge_rental_clusters_J_into_I(int pp, int cI_in, int cJ_in, int first) {
  int cI = MIN(cI_in, cJ_in);
  int cJ = MAX(cI_in, cJ_in);
  {
    int nII = rental_cluster_array[pp][cI][first].enum_no;
    int nJJ;
    for (nJJ = 0; nJJ < rental_cluster_array[pp][cJ][first].enum_no; nJJ++) {

      rental_cluster_array[pp][cI][first].enum_item_array[nII].block = rental_cluster_array[pp][cJ][first].enum_item_array[nJJ].block;
      nII++;
    }

    rental_cluster_array[pp][cI][first].enum_no = nII;
    rental_cluster_array[pp][cJ][first].enum_no = 0;
  }
  return cI;
}


int do_rental_clusters(int pp, int first) {
  rental_cluster_no[pp][first] = 0;
  Real_rental_cluster_no[pp][first] = 0;  

  int ss; // the similarity item
  int cc; // the cluster
  int cI = -1;
  int cJ = -1;

  for (ss = 0; ss < rental_no[pp]; ss++) { 
    //if (debug) fprintf(stderr, "**GO OVER SS=%d: CL_NO=%d:%d:\n", ss, rental_cluster_no[pp], Real_rental_cluster_no[pp]);
    int found_in_clusters = 0;
    cI = 0; cJ = 0;
    int foundI = 0, foundJ = 0;
    for (cc = 0; found_in_clusters < 2 && cc < rental_cluster_no[pp][first]; cc++) { // check if new sim is already in an EXISTING CLUSTER
      if (foundI == 0) {
	foundI = found_item_in_rental_cluster(pp, cc, rental_list[pp][ss].ll, first);
	if (foundI == 1) {
	  cI = cc;
	}
      }
      
      if (foundJ == 0) {
	foundJ = found_item_in_rental_cluster(pp, cc, rental_list[pp][ss].kk, first);
	if (foundJ == 1) {
	  cJ = cc;
	}
      }
      
    } // for all clusters



    //if (debug) fprintf(stderr,"       FOUND ss=%d: fIJ1=%d:%d: cIJ=%d:%d: sim=%f: CL_NO=%d:\n", ss, foundI, foundJ, cI, cJ, rental_list[pp][ss].diff, rental_cluster_no[pp]);
    int ii0 = rental_list[pp][ss].ll;
    int ii1 = rental_list[pp][ss].kk;
    //int found_period1= rental_list[pp][ss].found_period1;
    //int found_period2= rental_list[pp][ss].found_period2;
    float diff01 = rental_list[pp][ss].diff;
    if (foundI == 1 && foundJ == 0 && diff01 <= ADD_RENTAL_THRESHOLD) {

      rental_cluster_array[pp][cI][first].enum_item_array[rental_cluster_array[pp][cI][first].enum_no].block = ii1;
      rental_cluster_array[pp][cI][first].enum_no++;

      if (rental_cluster_array[pp][cI][first].enum_no >= MAX_BLOCKS_IN_CLUSTER-1) {
	fprintf(stderr,"No of itemd exceeded %d:\n", MAX_BLOCKS_IN_CLUSTER);
	exit(0);
      }

    } else if (foundJ == 1 && foundI == 0  && diff01 <= ADD_RENTAL_THRESHOLD) {

      rental_cluster_array[pp][cJ][first].enum_item_array[rental_cluster_array[pp][cJ][first].enum_no].block = ii0;
      rental_cluster_array[pp][cJ][first].enum_no++;

      if (rental_cluster_array[pp][cJ][first].enum_no >= MAX_BLOCKS_IN_CLUSTER-1) {
	fprintf(stderr,"No of itemd exceeded %d:\n", MAX_BLOCKS_IN_CLUSTER);
	exit(0);
      }
      
    } else if (foundI == 1 && foundJ == 1
	       && cI >= 0 && cJ >=0 && cI != cJ
	       && rental_list[pp][ss].diff <= MERGE_RENTAL_THRESHOLD) { // merge clusters // any conditions on merge? what if each group is happy with 15 items?


      merge_rental_clusters_J_into_I(pp, cI, cJ, first);
      Real_rental_cluster_no[pp][first]--;

    } else if (foundI == 0 && foundJ == 0 && diff01 <= ADD_RENTAL_THRESHOLD) { // create new cluster with 2 items for this similarity
      int cc_enum_no = 0;

      rental_cluster_array[pp][rental_cluster_no[pp][first]][first].enum_item_array[cc_enum_no].block = ii0;
      cc_enum_no++;
	

      rental_cluster_array[pp][rental_cluster_no[pp][first]][first].enum_item_array[cc_enum_no].block = ii1;
      cc_enum_no++;
	
      rental_cluster_array[pp][rental_cluster_no[pp][first]][first].enum_no = cc_enum_no;
	
      if (rental_cluster_no[pp][first] >= MAX_RENTAL_CLUSTERS-1) {
	fprintf(stderr,"No of cluster exceeded %d:\n", MAX_RENTAL_CLUSTERS);
	exit(0);
      }
      if (rental_cluster_no[pp][first] < MAX_RENTAL_CLUSTERS-2 && Real_rental_cluster_no[pp][first] < MAX_RENTAL_CLUSTERS-2) {
	rental_cluster_no[pp][first]++;
	Real_rental_cluster_no[pp][first]++;
      } else {
	fprintf(stderr,"Exceeded max no of clusters :%d:%d:%d:\n", MAX_RENTAL_CLUSTERS, rental_cluster_no[pp][first], Real_rental_cluster_no[pp][first]);
      }
    }
  } // SS

  // sort the blocks by order
  for (cc = 0; cc < rental_cluster_no[pp][first]; cc++) {
    qsort((void *)&(rental_cluster_array[pp][cc][first].enum_item_array),  rental_cluster_array[pp][cc][first].enum_no, sizeof(struct Rental_Item), ppp_rent_item_cmp);
  }


  rental_report_array[pp].real_no_of_clusters = 0;
  for (cc = 0; cc < rental_cluster_no[pp][first]; cc++) {
    if (rental_cluster_array[pp][cc][first].enum_no > 0) {
      rental_report_array[pp].real_no_of_clusters++;
    }
  }
  return rental_cluster_no[pp][first];
} // do_rental_clusters()


int is_digitsf(char *text) { // 1.  21. 21
  int len = strlen(text);
  int count_digit = 0;
  if (len < 4) {
    int ii;
    for (ii = 0; count_digit > -1 && ii < len; ii++) {
      if (isalpha(text[ii]) != 0) count_digit = -1;
      else if (isdigit(text[ii]) != 0) count_digit++;
    }
  } else {
    count_digit = -1;
  }
  return count_digit;
}

int is_slashf(char *text) { // '\\'
  int len = strlen(text);
  int count_digit = 0;
  if (len < 4) {
    int ii;
    for (ii = 0; count_digit > -1 && ii < len; ii++) {
      if (text[ii] =='\\') count_digit++;
    }
  } else {
    count_digit = -1;
  }
  return count_digit;
}

int is_quotef(char *text) { // "dxvdf fsdfs" 
  int len = strlen(text);
  int ret = (text[0] == '\"' && len > 0 && text[len-1] == '\"') ? 1 : 0;
  return ret;
}


int is_romanf(char *text) { // i iv xii
  int len = strlen(text);
  int count_rom = 0;
  if (len < 6) {
    int ii;
    for (ii = 0; count_rom > -1 && ii < len; ii++) {
      if (text[ii] == 'i' || text[ii] == 'v' || text[ii] == 'x' || text[ii] == 'l') count_rom++;
      else if (isalnum(text[ii]) != 0) count_rom = -1;
    }
  } else {
    count_rom = -1;
  }
  return count_rom;
}

int is_ROMANF(char *text) { // I IV XII
  int len = strlen(text);
  int count_rom = 0;
  if (len < 6) {
    int ii;
    for (ii = 0; count_rom > -1 && ii < len; ii++) {
      if (text[ii] == 'I' || text[ii] == 'V' || text[ii] == 'X' || text[ii] == 'L') count_rom++;
      else if (isalnum(text[ii]) != 0) count_rom = -1;
    }
  } else {
    count_rom = -1;
  }
  return count_rom;
}


#define ALLOWED_FIRST_CLUSTER 3000
#define NORMAL_FIRST_CLUSTER 1100
#define ALLOWED_SECOND_CLUSTER 30000


int mark_one_cluster(int pp, int cc, int *nn, int first) {

  int enum_no = rental_cluster_array[pp][cc][first].enum_no;
  if (enum_no > 0) {
    int bk1 = rental_cluster_array[pp][cc][first].enum_item_array[0].block;
    if (1) fprintf(stderr, "\n      RENTAL CLUSTER: pp=%d: cc=%d:%d: first=%d: score=%4.2f:%4.2f:  no_of_items=%d: first_t=%s:\n"
		   , pp ,(*nn)++,cc, first, rental_cluster_array[pp][cc][first].score, SECTION_OR_RENTAL_THRESHOLD, rental_cluster_array[pp][cc][first].enum_no, block_array[bk1].text

		   );
    int ii;
    rental_cluster_array[pp][cc][first].tot_digits = 0; rental_cluster_array[pp][cc][first].tot_roman = 0; rental_cluster_array[pp][cc][first].tot_ROMAN = 0;
    rental_cluster_array[pp][cc][first].tot_paren = 0; rental_cluster_array[pp][cc][first].tot_section = 0; rental_cluster_array[pp][cc][first].tot_size = 0;
    rental_cluster_array[pp][cc][first].tot_quote = 0; rental_cluster_array[pp][cc][first].tot_rental_size = 0;
    int tot_xleft = 0;
    int tot_sections= 0;

    for (ii = 0; ii < enum_no; ii++) { // COLLECTION LOOP

      int bk = rental_cluster_array[pp][cc][first].enum_item_array[ii].block;
      char *text = block_array[bk].text;
      int len = strlen(text);
      int xleft = block_array[bk].my_x1;
      int vtext = atoi(text);

      int line = block_array[bk].my_line_in_page;
      int is_digits = is_digitsf(text);
      int is_slash = is_slashf(text);
      rental_cluster_array[pp][cc][first].tot_digits += (is_digits > 0) ? 1 : 0;
      int is_toc_len = (len < 4);
      int is_roman = is_romanf(text); // ivx
      rental_cluster_array[pp][cc][first].tot_roman += (is_roman > 0) ? 1 : 0;	
      int is_ROMAN = is_ROMANF(text); // IVX
      rental_cluster_array[pp][cc][first].tot_ROMAN += (is_ROMAN > 0) ? 1 : 0;		
      int is_paren; // (12), (iv)

      rental_cluster_array[pp][cc][first].tot_paren += (is_paren > 0) ? 1 : 0;
      int is_quote = is_quotef(text);
      rental_cluster_array[pp][cc][first].tot_quote += (is_quote > 0) ? 1 : 0;				
      int is_section = is_sectionf(text); // section, article, part
      rental_cluster_array[pp][cc][first].tot_section += (is_section > 0) ? 1 : 0;				
      int xright = block_array[bk].my_x2;
      int size = xright - xleft;
      rental_cluster_array[pp][cc][first].tot_size += size;
      tot_xleft += xleft;

      int is_rental_size = (size < NORMAL_FIRST_CLUSTER);
      rental_cluster_array[pp][cc][first].tot_rental_size += (is_rental_size > 0)  ? 1 : 0;
      int found_section =  detect_toc_section(bk);
      block_array[bk].found_section = found_section;
      //rental_cluster_array[pp][cc][first].tot_sections += found_section;

      fprintf(stderr, "\n  BBB   :%d:%d:-:%s:%d: x1=%d: line=%d: dig=%d: sec=%d: rom=%d: ROM=%d: size=%d:%4.2f: section=%d: iq=%d:"
	      , ii, bk, text, vtext, xleft, line
	      , is_digits, is_section, is_roman, is_ROMAN, size, rental_cluster_array[pp][cc][first].tot_size, found_section, is_quote
	      );

    } // for
    float avg_size = (float)((rental_cluster_array[pp][cc][first].tot_size) / (float)(enum_no));
    float avg_xleft = (float)((tot_xleft) / (float)(enum_no));    
    float tot_size_diff_2 = 0;    
    for (ii = 0; ii < enum_no; ii++) { // DECISION LOOP
      int bk = rental_cluster_array[pp][cc][first].enum_item_array[ii].block;      
      int xright = block_array[bk].my_x2;
      int xleft = block_array[bk].my_x1;
      int size = xright - xleft;
      int size_diff_2 = (size - avg_size) * (size - avg_size);
      tot_size_diff_2 += size_diff_2;
    }
    rental_cluster_array[pp][cc][first].std_size = sqrt(tot_size_diff_2 / (float)enum_no); // std_size low with larg size means double columns (no enum)
    rental_cluster_array[pp][cc][first].avg_size = avg_size;
    rental_cluster_array[pp][cc][first].avg_xleft = avg_xleft;    
    fprintf(stderr, "\n ----TOT pp=%d: cc=%d: f=%d: avx1=%4.2f: avg=%4.2f: std=%4.2f: rom=%4.2f:%4.2f: dig=%4.2f:\n"
	    , pp, cc, first
	    , avg_xleft, avg_size, rental_cluster_array[pp][cc][first].std_size, rental_cluster_array[pp][cc][first].tot_roman/(float)enum_no
	    , rental_cluster_array[pp][cc][first].tot_ROMAN/(float)enum_no, rental_cluster_array[pp][cc][first].tot_digits/(float)enum_no
	    );
  }     // if no > 0

  return 0;
}

int identify_rentals(int pp) {
  // in each page find line correspondence between a cluster on the first-on-line and the cluster on the second-on-line
  int line1 = 0;
  int found_rental_in_this_page = 0;

  static int prev_avg_xleft0 = 0, avg_xleft0 = 0;
  static int prev_avg_xleft1 = 0, avg_xleft1 = 0;  
  static int prev_pp = 0;
  static int prev_real_total_in_cluster, real_total_in_cluster = 0;
  static int prev_aa = 0;

  int cc0;
  for (cc0 = 0; cc0 < rental_cluster_no[pp][0] && cc0 < 10; cc0++) { // for each one of the first-on-line clusters
    fprintf(stderr,"CCCCCC0: pp=%d: cc=%d: f=%d: no=%d: avx1=%4.2f: avsize=%4.2f: std=%4.2f:\n"
	    , pp, cc0, 0, rental_cluster_array[pp][cc0][0].enum_no
	    , rental_cluster_array[pp][cc0][0].avg_xleft, rental_cluster_array[pp][cc0][0].avg_size, rental_cluster_array[pp][cc0][0].std_size);
    // CONDITIONS ON C0
    if (rental_cluster_array[pp][cc0][0].enum_no > 0
	&& (rental_cluster_array[pp][cc0][0].tot_digits / rental_cluster_array[pp][cc0][0].enum_no) < 0.5 // cant use 11, 1, 3
	//&& (rental_cluster_array[pp][cc0][0].tot_sections / rental_cluster_array[pp][cc0][0].enum_no) < 0.5 // can't use A., B., Section 5
	) { // digits only clusters must not exceed 50%

      avg_xleft0 = rental_cluster_array[pp][cc0][0].avg_xleft;
      int cc1;
      for (cc1 = 0; cc1 < rental_cluster_no[pp][1] && cc1 < 10; cc1++) { // for each one of the second-on-line clusters	
	fprintf(stderr,"   CCCCCC1: pp=%d: cc=%d: f=%d: no=%d: avx1=%4.2f: avsize=%4.2f: std=%4.2f:\n"
		, pp, cc1, 1,  rental_cluster_array[pp][cc1][1].enum_no
		, rental_cluster_array[pp][cc1][1].avg_xleft, rental_cluster_array[pp][cc1][1].avg_size, rental_cluster_array[pp][cc1][1].std_size);

	// CONDITIONS ON C1 
	if (rental_cluster_array[pp][cc1][1].enum_no > 0
	    && (rental_cluster_array[pp][cc1][1].avg_xleft < 6000)) { // can't start too much to the right
	
	  int aa = 1; // the toc section number

	  int ii0;
	  int real_total_in_cluster = 0;

	  avg_xleft1 = rental_cluster_array[pp][cc1][1].avg_xleft;



	  if (0) fprintf(stderr,"   CCCCCCB: pp=%d: cc=%d: f=%d: avx1=%4.2f: avsize=%4.2f: std=%4.2f: no0=%d: no1=%d:\n"
		  , pp, cc1, 1
		  , rental_cluster_array[pp][cc1][1].avg_xleft, rental_cluster_array[pp][cc1][1].avg_size, rental_cluster_array[pp][cc1][1].std_size
		  , rental_cluster_array[pp][cc0][0].enum_no,  rental_cluster_array[pp][cc1][1].enum_no);



	  // *********** SUMMATION LOOP, determine real_total_in_cluster  *********** 
	  for (ii0 = 0; ii0 < rental_cluster_array[pp][cc0][0].enum_no; ii0++) { // for each one of the items
	    int bk0 = rental_cluster_array[pp][cc0][0].enum_item_array[ii0].block;
	    int line0 = block_array[bk0].my_line_in_page;
	    if (block_array[bk0].found_section == 0) { // CONDITION ON INDIVIDUAL ITEM I0
	      int ii1;

	      for (ii1 = 0; // for each one of the items
		   (ii1 < rental_cluster_array[pp][cc1][1].enum_no
		    && block_array[rental_cluster_array[pp][cc1][1].enum_item_array[ii1].block].my_line_in_page <= line0);
		   ii1++) {
		int bk1 = rental_cluster_array[pp][cc1][1].enum_item_array[ii1].block;
		int line1 = block_array[bk1].my_line_in_page;
		if (line0 == line1) {
		  real_total_in_cluster++;		  
		  if (0) fprintf(stderr,"LLLLLLB:%d:\n", real_total_in_cluster);
		} // if lines
	      } // for ii1
	    } // if I0
	  } // for ii0 

	  // *********** END SUMMATION LOOP,  *********** 


	  // *********** REAL LOOP,  *********** 	  
	  // CONDITIONS FOR CONTINUATION BETWEEN PAGES
	  // allow if the number of items bigger than 2 or it's a continuation from a previous page, similar tabs
	  int condition2 = (prev_real_total_in_cluster > 0
			    && abs(prev_avg_xleft0 - avg_xleft0)  < 120
			    && abs(prev_avg_xleft1 - avg_xleft1)  < 120
			    && abs(prev_pp - pp) < 4) ? 1 : 0;
	  fprintf(stderr,"LLLLLLC: cond2=%d: ptot=%d: px0=%d:%d: px1=%d:%d: pa=%d: real=%d: pp=%d:%d: pa=%d:\n"
		  , condition2, prev_real_total_in_cluster, prev_avg_xleft0, avg_xleft0, prev_avg_xleft1, avg_xleft1, prev_aa, real_total_in_cluster, prev_pp, pp, summary_point_page_array[pp]);

	  if (real_total_in_cluster >= 2
	      && (condition2 == 1 || summary_point_page_array[pp] == 1) // either it's a continuation from prev page or it says "Cover Page" or "Lease Summary"
	      ) {

	    aa = (condition2 == 1) ? prev_aa : 1; // continue the section count if necessary
	    for (ii0 = 0; ii0 < rental_cluster_array[pp][cc0][0].enum_no; ii0++) { // for each one of the items
	      int bk0 = rental_cluster_array[pp][cc0][0].enum_item_array[ii0].block;
	      int line0 = block_array[bk0].my_line_in_page;
	      char *text0 = block_array[bk0].text;
	      if (block_array[bk0].found_section == 0) { // CONDITION ON INDIVIDUAL ITEM I0
		int ii1;

		for (ii1 = 0; // for each one of the items
		     (ii1 < rental_cluster_array[pp][cc1][1].enum_no
		      && block_array[rental_cluster_array[pp][cc1][1].enum_item_array[ii1].block].my_line_in_page <= line0);
		     ii1++) {
		  int bk1 = rental_cluster_array[pp][cc1][1].enum_item_array[ii1].block;
		  int line1 = block_array[bk1].my_line_in_page;
		  char *text1 = block_array[bk1].text;
		  if (line0 == line1) {
		    total_no_of_rentals++;
		    found_rental_in_this_page = 1;
		    rental_point_array[rental_point_no].given_section_no = aa++;
		    prev_aa = aa;
		    rental_point_array[rental_point_no].block_no = bk0;
		    rental_point_array[rental_point_no].line_no = line0;
		    rental_point_array[rental_point_no].word_no = block_array[bk0].first_word;
		    rental_point_array[rental_point_no].page_no = block_array[bk0].page;
		    index2rental_point_array[block_array[bk0].page][block_array[bk0].my_line_in_page] = rental_point_no;
		    rental_point_no++;	      
		    fprintf(stderr,"          LLLLLL: cc=%d: ii=%d:%d: bk=%d:%d: ll=%d:%d: TT=%s:%s: PL=%d:%d: x1=%d: aa=%d:\n", cc0,  ii0,ii1,  bk0,bk1,  line0,line1, text0,text1, block_array[bk0].page,block_array[bk0].my_line_in_page, block_array[bk0].my_x1, aa-1);
		  } // if lines
		} // for ii1
	      } // if I0
	    } // for ii0 
	    prev_avg_xleft1 = avg_xleft1;	    
	    prev_avg_xleft0 = avg_xleft0;
	    prev_real_total_in_cluster += real_total_in_cluster;
	    prev_pp = pp;
	    // *********** END REAL LOOP,  *********** 	  
	  } // condition on continuation between clusters
	} // cond for cc1
      } // for cc1
    } // cond for cc0
  } // for cc0
  fprintf(stderr,"DDDDDDD\n");
  if (found_rental_in_this_page) total_no_of_rental_pages++;
  return 0;
} // identify_rentals()




int stats_rental_cluster(int pp, int first) {
  int cc;
  int nn = 0;
  for (cc = 0; cc < rental_report_array[pp].real_no_of_clusters+5/*rental_cluster_no[pp]*/; cc++) {
    mark_one_cluster(pp, cc, &nn, first);
  } // cc
  //Xcreate_leftmost_coord_array(pp, min_mean_x1, first);
  return 0;
} // stats_rental_cluster()


int print_rental_report(int last_page_no) {
  fprintf(stderr,"RENTAL REPORT: no_of_pages=%d: no_of_enums=%d:\n", total_no_of_rental_pages, total_no_of_rentals);
  int pp;
  for (pp = 1; pp <= last_page_no; pp++) {
    if (rental_report_array[pp].cluster_no_of_enums > 0) {
      fprintf(stderr,"    PAGE RENTAL REPORT: pp=%d: no_clusters=%d: no_enums=%d: score=%d: subsection=%d: ll_x1=%d:\n"
	      , pp, rental_report_array[pp].real_no_of_clusters, rental_report_array[pp].cluster_no_of_enums
	      , rental_report_array[pp].score, rental_report_array[pp].total_subsection, rental_report_array[pp].total_long_lines_same_x1);
    }
  }
  return 0;
}

int reset_index2rental_point_array() {
  int ii;
  for (ii = 0; ii < MAX_PAGE; ii++) {
    int jj;
    for (jj = 0; jj < MAX_LINE_PER_PAGE; jj++) {    
      index2rental_point_array[ii][jj] = -1;
    }
  }
  return 0;
}

int create_rental_forms(int last_page_no) {
  int do_rf = 0; // for WF don't do RF TESTING
  get_summary_page(); // populate page_array[] using deals_ocrtoken

  reset_index2rental_point_array();
  total_no_of_rental_pages = total_no_of_rentals = 0;
  int pp;
  fprintf(stderr,"***START RENTALS:\n");
  int first;
  SIZE_THRESHOLD[0] = ALLOWED_FIRST_CLUSTER;
  SIZE_THRESHOLD[1] = ALLOWED_SECOND_CLUSTER;
  for (pp = 1; pp <= last_page_no; pp++) {
    for (first = 0; first < 2; first++) { // create separately the clusters of the first block-on-line, and the second-block-on-line
      fprintf(stderr,"********** DOING FIRST :%d:\n", first);  

      rental_no[pp] = 0;
      create_rental_diff_array(pp, first, do_rf);
      if (1) {
	fprintf(stderr,"********************* PAGE:%d: PRINTING RENTALS BEFORE SORT:%d:\n", pp, rental_no[pp]);  
	if (pp == 3) print_rental_diff_list(pp, rental_no[pp]);
      }
      sort_diff_rental_pairs(pp);
      if (1) {
	fprintf(stderr,"********************* PAGE:%d: PRINTING RENTALS AFTER SORT:%d:\n", pp, rental_no[pp]);
	if (pp == 3) print_rental_diff_list(pp, rental_no[pp]);
      }
      int nn = do_rental_clusters(pp, first);
      if (1) {
	stats_rental_cluster(pp, first);
      }
    }
    fprintf(stderr,"\n\n********************* IDENTIFY PAGE:%d:\n", pp);
    identify_rentals(pp);
    fprintf(stderr,"********************* DONE IDENTIFY\n\n");      
  }
  print_rental_report(last_page_no);
  fprintf(stderr,"*** DONE RENTALS\n");
  return 0; 
} // create_footer_clusters()


/*****************************************************************************/
 
