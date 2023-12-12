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





/*********** Indent types of cluster ***********/
#define none_i 0
#define left_i 1
#define right_i 2
#define both_i 3
/*********** Indent types of cluster ***********/

struct Block_Cluster {  // cluster in order to rule out TOC situations
  int cluster_type[5]; // 0-none /1-left/2-right/3-both indent
  //int block_array[MAX_BLOCKS_IN_CLUSTER]; // REDUNDANT, same as block field in struct
  struct Block_Item {int block; int first_word; int last_word;/* int sn_in_line_of_first_word;*/} block_item_array[MAX_BLOCKS_IN_CLUSTER];  
  int block_no;  // how many block AND items exist in the list

  int last_under_point_five_total; // does the last word cross the center? total
  int first_over_point_five_total; // does the first word cross the center? total
  int total_right_side_indented; // is the right side indented uniformally? total

  float width_mean;
  float width_std;  
  float center_mean;
  float center_std;  
  int sequential_numbering; // items are enumerated?
  float ratio_sequential_numbering;  
  int x2_over_8000; // for TOC detect
  int x1_under_2000;   // probably enum   // for TOC detect

  int is_digit; // how many are pure digits: 12, 1, x, I, (i)
  int SECTION; // is Section or Article the first word?
  int no_first_in_line;  // how many are first in line
  float ratio_x2_over_8000; // divided by total_no
  float ratio_x1_under_2000; // divided by total_no  
  float ratio_is_digit;  // divided by total_no
  float mean_no_of_chars;
  float std_no_of_chars;

  float left_mean, left_std;
  float right_mean, right_std;
  float ratio_SECTION;
  float ratio_first_in_line;  

  int sequence_type;
  int first_enum_value;
  
} Block_cluster_array[MAX_PAGE][MAX_CLUSTERS];
int Block_cluster_no[MAX_PAGE];
int Real_Block_cluster_no[MAX_PAGE];


int ppp_block_item_cmp(const void *a, const void *b) {
  int x1 = (((struct Block_Item *)a)->block);
  int x2 = (((struct Block_Item *)b)->block);
  return(x1-x2);
}

int reset_sim_block_array(int pp) {
  int ll, kk;
  for (ll = 0; ll < page_properties_array[pp].no_of_blocks_in_page; ll++) {
    for (kk = ll+1; kk < page_properties_array[pp].no_of_blocks_in_page; kk++) {
      //sim_block_matrix[ll][kk] = 1000000;
    }
  }
  return 0;
}


// assuming page is scanned vertically
#define DELTAX_THRESHOLD 25 
int create_sim_block_array(int pp) {
  int ll;
  sim_block_no[pp] = 0;
  fprintf(stderr,"  GOING TO SIM PAGE:%d: no_of_blocks=%d: fl=%d:%d:\n",pp 
	  , page_properties_array[pp].no_of_blocks_in_page, page_properties_array[pp].first_block_in_page, page_properties_array[pp].last_block_in_page);
  for (ll = page_properties_array[pp].first_block_in_page; ll <= page_properties_array[pp].last_block_in_page; ll++) {
    int x1_ll = block_array[ll].my_x1;
    int kk;
    if (pp==1 || pp==2) fprintf(stderr,"   LLL:   bk=%d: x1=%d: t=%s:\n"
				, ll
				, block_array[ll].my_x1
				, block_array[ll].text);
    for (kk = ll+1; kk <= page_properties_array[pp].last_block_in_page; kk++) {
      int x1_kk = block_array[kk].my_x1;
      //sim_block_matrix[ll][kk] = abs(x1_kk - x1_ll);
      int my_sim = abs(x1_kk - x1_ll);
      if (pp==1 || pp==2) fprintf(stderr,"          KKK:  bk=%d: x1=%d: t=%s: del=%d:%d:\n"
				  , kk
				  , block_array[kk].my_x1
				  , block_array[kk].text
				  , my_sim
				  , DELTAX_THRESHOLD);



      //      if (sim_block_matrix[ll][kk] < DELTAX_THRESHOLD) {
      if (my_sim < DELTAX_THRESHOLD) {      
	sim_block_list[sim_block_no[pp]].sim = my_sim; //sim_block_matrix[ll][kk];

	sim_block_list[sim_block_no[pp]].ll_block = ll;
	sim_block_list[sim_block_no[pp]].kk_block = kk;
	/*
	sim_block_list[sim_block_no[pp]].ll_last = 0; //last_word_ll;
	sim_block_list[sim_block_no[pp]].kk_last = 0; //last_word_kk;

	sim_block_list[sim_block_no[pp]].ll = 0;// word_ll;
	sim_block_list[sim_block_no[pp]].kk = 0; //word_kk;
	*/


	sim_block_no[pp]++;	

      }
    }
  }
  return sim_block_no[pp];
} // create_sim_block_array(int pp) 

int ppp_sim_cmp(const void *a, const void *b) {
  int x1 = (((struct Sim_Triple *)a)->sim);
  int x2 = (((struct Sim_Triple *)b)->sim);
  return(x1-x2);
}

int sort_sim_block_pairs(int pp) {
  qsort((void *)&sim_block_list[0], sim_block_no[pp],  sizeof(struct Sim_Triple), ppp_sim_cmp);
  return 0;
}



int found_item_in_block_cluster(int pp, int cc, int first_word, int block) {
  int nn;
  int found_in_cluster = 0;
  for (nn = 0; found_in_cluster == 0 && nn < Block_cluster_array[pp][cc].block_no; nn++) { // check all the items of the cluster
    // UUU if (Block_cluster_array[pp][cc].block_array[nn] == block) {
    //if (Block_cluster_array[pp][cc].block_item_array[nn].first_word == first_word) {
    if (Block_cluster_array[pp][cc].block_item_array[nn].block == block) {                  
      found_in_cluster = 1;
    }
  }
  return found_in_cluster;
}

int block_merge_clusters_J_into_I(int pp, int cI_in, int cJ_in) {
  int cI = MIN(cI_in, cJ_in);
  int cJ = MAX(cI_in, cJ_in);
  {
    int nII = Block_cluster_array[pp][cI].block_no;
    int nJJ;
    for (nJJ = 0; nJJ < Block_cluster_array[pp][cJ].block_no; nJJ++) {
      /*
      Block_cluster_array[pp][cI].block_array[nII] = Block_cluster_array[pp][cJ].block_array[nJJ];
      Block_cluster_array[pp][cI].block_array[nII] = Block_cluster_array[pp][cJ].block_array[nJJ];      
      */ //UZ 082521
      
      Block_cluster_array[pp][cI].block_item_array[nII].first_word = Block_cluster_array[pp][cJ].block_item_array[nJJ].first_word;
      Block_cluster_array[pp][cI].block_item_array[nII].first_word = Block_cluster_array[pp][cJ].block_item_array[nJJ].first_word;      

      Block_cluster_array[pp][cI].block_item_array[nII].block = Block_cluster_array[pp][cJ].block_item_array[nJJ].block;
      Block_cluster_array[pp][cI].block_item_array[nII].block = Block_cluster_array[pp][cJ].block_item_array[nJJ].block;      

      Block_cluster_array[pp][cI].block_item_array[nII].last_word = Block_cluster_array[pp][cJ].block_item_array[nJJ].last_word;
      Block_cluster_array[pp][cI].block_item_array[nII].last_word = Block_cluster_array[pp][cJ].block_item_array[nJJ].last_word;      

      nII++;
    }
    int kk;
    for (kk = 0; kk < 5; kk++) {
      Block_cluster_array[pp][cI].cluster_type[kk] += Block_cluster_array[pp][cJ].cluster_type[kk];
    }
    for (kk = 0; kk < 5; kk++) {
      Block_cluster_array[pp][cJ].cluster_type[kk] = 0;
    }

    Block_cluster_array[pp][cI].block_no = nII;
    Block_cluster_array[pp][cJ].block_no = 0;
  }
  return cI;
}

int is_digit(char *text) { // allow 1,25,37, etc or (i) or IX
  int ii;
  int OK_so_far = 1;
  int found_space = 0;
  for (ii = 0; OK_so_far == 1 && found_space == 0 && ii < strlen(text); ii++) {
    if (text[ii] == ' ') found_space = 1;
    if (!(text[ii] == 'I' || text[ii] == 'i' || text[ii] == 'v' || text[ii] == 'V' || text[ii] == 'X' || text[ii] == 'x' || text[ii] == '(' || text[ii] == '.' || text[ii] == ')' || isdigit(text[ii]) != 0)) OK_so_far = 0;
  }
  return OK_so_far;
} // is_digit()

int sn_in_line(int pp, int cc, int jj, int *first_word) {
  int block = Block_cluster_array[pp][cc].block_item_array[jj].block;
  *first_word = block_array[block].first_word; 
  int ret = word_array[*first_word].sn_in_line;
  fprintf(stderr,"YYYY:%d:%d:%d:  :%d:%d:\n",pp,cc,jj, *first_word, ret);
  return ret;
}


int is_SECTION(char *text) {
  return (strncasecmp(text,"section", 7) == 0
		       || strncasecmp(text,"article", 7) == 0
		       || strncasecmp(text,"exhibit", 7) == 0
		       || strncasecmp(text,"addendum", 7) == 0
		       || strncasecmp(text,"amendment", 7) == 0
		       || strncasecmp(text,"paragraph", 7) == 0
		       || strncasecmp(text,"clause", 7) == 0
		       || strncasecmp(text,"schedule", 7) == 0		       		       		       
		       ) ? 1 : 0;
}
#define MIN_TOC_CLUSTER_SCORE 0.5
int decide_block_clusters(int pp) {
  int cc;
  fprintf(stderr," PRINTING CLUSTERS PAGE:%d:%d:%d:\n",pp, Block_cluster_no[pp], Real_Block_cluster_no[pp]);
  for (cc= 0; cc < Block_cluster_no[pp]; cc++) {
    Block_cluster_array[pp][cc].left_mean = 0;
    Block_cluster_array[pp][cc].right_mean = 0 ;	

    Block_cluster_array[pp][cc].x2_over_8000 = 0;
    Block_cluster_array[pp][cc].x1_under_2000 = 0;    
    if (Block_cluster_array[pp][cc].block_no > 0) {
      fprintf(stderr, "                CLUSTER:%2d:  block_no=%2d: types:1-%2d:2-%2d:3-%2d: \n"
	      , cc, Block_cluster_array[pp][cc].block_no
	      , Block_cluster_array[pp][cc].cluster_type[1], Block_cluster_array[pp][cc].cluster_type[2], Block_cluster_array[pp][cc].cluster_type[3]

	      );
      int jj;
      int block_no = Block_cluster_array[pp][cc].block_no;
      int last_atoi_text = 0;
      for (jj = 0; jj < block_no; jj++) {
	int block = Block_cluster_array[pp][cc].block_item_array[jj].block;
	int wf = Block_cluster_array[pp][cc].block_item_array[jj].first_word;
	int wl =  Block_cluster_array[pp][cc].block_item_array[jj].last_word;
	int real_line = block_array[block].my_line_in_doc;
	int no_of_char = (int)(block_array[block].text == NULL) ? 0 : strlen(block_array[block].text);
	int width = block_array[block].my_x2 - block_array[block].my_x1;
	int center = (block_array[block].my_x2 + block_array[block].my_x1) / 2;	
	int my_digit = (block_array[block].text == NULL) ? 0 : is_digit(block_array[block].text);
	int first_word;
	int my_sn_in_line = sn_in_line(pp,cc,jj,&first_word);
	int is_first_in_line = (my_sn_in_line == 1) ? 1 : 0;
	int SECTION = is_SECTION(block_array[block].text);
	fprintf(stderr,"                       LL=%2d:  l=%2d: wf=%5d:%4d:  wl=%5d:%4d:  sz=%d: rl=%d: no_of_words=%d: t=%d:%s: bn=%d: isdig=%d: sec=%d: is_fil=%d:%d:%d:\n"
		, jj
		, Block_cluster_array[pp][cc].block_item_array[jj].block
		, wf, block_array[block].my_x1
		, wl, block_array[block].my_x2
		, width
		, real_line
		, wl-wf
		, no_of_char
		, block_array[block].text
		, block_no
		, my_digit
		, SECTION
		, first_word, my_sn_in_line, is_first_in_line
		);
	int atoi_text = (block_array[block].text) ? atoi(block_array[block].text) : 0;
	Block_cluster_array[pp][cc].sequential_numbering += ((atoi_text > 0) ? (atoi_text >= last_atoi_text) : 0.0);
	last_atoi_text = atoi_text;

	Block_cluster_array[pp][cc].left_mean += block_array[block].my_x1 ;
	Block_cluster_array[pp][cc].right_mean += block_array[block].my_x2 ;	
	Block_cluster_array[pp][cc].x2_over_8000 += block_array[block].my_x2 > 8000;
	Block_cluster_array[pp][cc].x1_under_2000 += block_array[block].my_x1 < 2000;	
	Block_cluster_array[pp][cc].is_digit += (float)my_digit; // how many are pure digits: 12, 1, x, I, (i)
	Block_cluster_array[pp][cc].mean_no_of_chars += (float)no_of_char;
	Block_cluster_array[pp][cc].width_mean += (float)width;
	Block_cluster_array[pp][cc].center_mean += (float)center;
	Block_cluster_array[pp][cc].SECTION += SECTION;
	Block_cluster_array[pp][cc].no_first_in_line += is_first_in_line;	

      } // for jj
      if (block_no > 0) {
	Block_cluster_array[pp][cc].ratio_sequential_numbering = Block_cluster_array[pp][cc].sequential_numbering/(float)block_no;
	Block_cluster_array[pp][cc].ratio_x2_over_8000 = Block_cluster_array[pp][cc].x2_over_8000 / (float)block_no;
	Block_cluster_array[pp][cc].ratio_x1_under_2000 = Block_cluster_array[pp][cc].x1_under_2000 / (float)block_no;	
	Block_cluster_array[pp][cc].ratio_is_digit = Block_cluster_array[pp][cc].is_digit / (float)block_no;
	Block_cluster_array[pp][cc].mean_no_of_chars = Block_cluster_array[pp][cc].mean_no_of_chars / (float)block_no;
	Block_cluster_array[pp][cc].width_mean = Block_cluster_array[pp][cc].width_mean / (float)block_no;
	Block_cluster_array[pp][cc].center_mean = Block_cluster_array[pp][cc].center_mean / (float)block_no;	
	Block_cluster_array[pp][cc].right_mean  = (float)Block_cluster_array[pp][cc].right_mean / (float)block_no;
	Block_cluster_array[pp][cc].left_mean  = (float)Block_cluster_array[pp][cc].left_mean / (float)block_no ;
	Block_cluster_array[pp][cc].ratio_SECTION  = (float)Block_cluster_array[pp][cc].SECTION / (float)block_no ;
	Block_cluster_array[pp][cc].ratio_first_in_line  = (float)Block_cluster_array[pp][cc].no_first_in_line / (float)block_no ;		
      }
      int total_right_dev = 0;
      int total_left_dev = 0;
      int total_width_dev = 0;
      int total_center_dev = 0;                  
      for (jj = 0; jj < block_no; jj++) { // sum deviaton
	int block = Block_cluster_array[pp][cc].block_item_array[jj].block;	
	total_right_dev +=((int)block_array[block].my_x2 - (int)Block_cluster_array[pp][cc].right_mean) * ((int)block_array[block].my_x2 - (int)Block_cluster_array[pp][cc].right_mean);
	total_left_dev += ((int)block_array[block].my_x1 - (int)Block_cluster_array[pp][cc].left_mean) * ((int)block_array[block].my_x1 - (int)Block_cluster_array[pp][cc].left_mean);
	total_width_dev += ((int)block_array[block].my_x2 - (int)block_array[block].my_x1 - (int)Block_cluster_array[pp][cc].width_mean) * ((int)block_array[block].my_x2 - (int)block_array[block].my_x1- (int)Block_cluster_array[pp][cc].width_mean);
	total_center_dev += (((int)block_array[block].my_x2 - (int)block_array[block].my_x1)  / 2 - (int)Block_cluster_array[pp][cc].width_mean) * (((int)block_array[block].my_x2 - (int)block_array[block].my_x1) / 2 - (int)Block_cluster_array[pp][cc].width_mean);      	
	if (pp == 2) fprintf(stderr,"OOO pp=%d: cc=%d: jj=%d: bl=%d: x2=%d: mean=%d: diff=%d: tot=%d:\n", pp, cc, jj, block, (int)block_array[block].my_x2, (int)Block_cluster_array[pp][cc].right_mean,   (int)(block_array[block].my_x2 - Block_cluster_array[pp][cc].right_mean), (int)total_left_dev);
      }
      if (block_no > 1) {
	Block_cluster_array[pp][cc].right_std = sqrt((double)(total_right_dev / (float)(block_no - 1)));
	Block_cluster_array[pp][cc].left_std = sqrt((double)((float)total_left_dev / (float)(block_no - 1)));
	Block_cluster_array[pp][cc].width_std = sqrt((double)((float)total_width_dev / (float)(block_no - 1)));
	Block_cluster_array[pp][cc].center_std = sqrt((double)((float)total_center_dev / (float)(block_no - 1)));		
	fprintf(stderr, "RIGHT: totl=%d: bn=%d: ratio=%4.2f: sqrt=%4.2f:%4.2f: \n", total_right_dev, block_no, (float)(total_right_dev / (float)(block_no - 1)), sqrt((double)(total_right_dev / (float)(block_no - 1))), (float)Block_cluster_array[pp][cc].right_std);
      }


      float cluster_toc_total = 0
	+ 0.05 * Block_cluster_array[pp][cc].ratio_sequential_numbering 
	+ 0.4 * Block_cluster_array[pp][cc].ratio_x2_over_8000 
	//- 0.9 * Block_cluster_array[pp][cc].ratio_x1_under_2000 
	+ 0.2 * Block_cluster_array[pp][cc].ratio_is_digit 
	+ 0.2 * (Block_cluster_array[pp][cc].mean_no_of_chars < 3.0) 
	+ 0.3 * (Block_cluster_array[pp][cc].width_mean < 300
	+ 0.5 * Block_cluster_array[pp][cc].ratio_SECTION       ) ;

      fprintf(stderr, "                CLUSTER PROP: TOT=%4.2f:  pp=%d: cc=%d: seq=%4.2f: over_8000:%4.2f: under_2000=%4.2f: is_digit=%4.2f: no_of_char=%4.2f: width=%4.2f: SEC=%4.2f: sn_in_line_first_word:%d:%4.2f:\n"
	      , cluster_toc_total, pp, cc
	      , Block_cluster_array[pp][cc].ratio_sequential_numbering
	      , Block_cluster_array[pp][cc].ratio_x2_over_8000
	      , Block_cluster_array[pp][cc].ratio_x1_under_2000 	      
	      , Block_cluster_array[pp][cc].ratio_is_digit 
	      , Block_cluster_array[pp][cc].mean_no_of_chars
	      , Block_cluster_array[pp][cc].width_mean
	      , Block_cluster_array[pp][cc].ratio_SECTION
	      , Block_cluster_array[pp][cc].no_first_in_line
	      , Block_cluster_array[pp][cc].ratio_first_in_line
	      );
      if (cluster_toc_total > MIN_TOC_CLUSTER_SCORE) {
	page_properties_array[pp].no_toc_clusters++;
	page_properties_array[pp].no_toc_items += block_no;
      }
      fprintf(stderr,"\n");
    } // if
  } // cc
  fprintf(stderr," PAGE TOC? toc_cl=%d: toc_items=%d: line_no=%d: ratio=%4.2f:%4.2f:\n"
	  , page_properties_array[pp].no_toc_clusters, page_properties_array[pp].no_toc_items
	  , page_properties_array[pp].last_line_in_page - page_properties_array[pp].first_line_in_page
	  , (float)((float)(page_properties_array[pp].no_toc_items) / (float)(page_properties_array[pp].last_line_in_page - page_properties_array[pp].first_line_in_page))
	  , MAX_TOC_RATIO
	  ); 
  return 0;
} // decide_block_clusters(int pp) 


int insert_tabbed_column_into_sql(int doc_id, int page_no) {
  char query[5000000];
  sprintf(query,"delete from deals_page_tabbed_column \n\
           where doc_id = '%d' "
	  , doc_id);
  if (debug) fprintf(stderr,"QUERY67=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY67=%s\n",mysql_error(conn));
    exit(1);
  }

  sprintf(query,"insert into deals_page_tabbed_column \n\
                 (doc_id, page_no, col_no, no_of_items,     seq, over_8000, under_2000, is_digit, SECTION,     left_mean, left_std, right_mean, right_std,     center_mean, center_std, width, width_std,       sequence_type,  first_enum_value, mean_no_of_chars, std_no_of_chars) \n\
                 values \n "); 

  int pp, cc;
  for (pp = 0; pp < page_no; pp++) {
    for (cc = 0; cc < Block_cluster_no[pp]; cc++) {
      if (Block_cluster_array[pp][cc].block_no > 1) {
	char buff[500000];
	sprintf(buff,"(%d, %d, %d, %d,     %4.2f, %4.2f, %4.2f, %4.2f, %4.2f,       %4.2f, %4.2f, %4.2f, %4.2f,      %4.2f, %4.2f, %4.2f, %4.2f,     %d, %d, %4.2f, %4.2f), \n"
		, doc_id
		, pp
		, cc
		, Block_cluster_array[pp][cc].block_no

		, Block_cluster_array[pp][cc].ratio_sequential_numbering
		, Block_cluster_array[pp][cc].ratio_x2_over_8000
		, Block_cluster_array[pp][cc].ratio_x1_under_2000
		, Block_cluster_array[pp][cc].ratio_is_digit
		, Block_cluster_array[pp][cc].ratio_SECTION		

		, Block_cluster_array[pp][cc].left_mean
		, Block_cluster_array[pp][cc].left_std
		, Block_cluster_array[pp][cc].right_mean
		, Block_cluster_array[pp][cc].right_std

		, Block_cluster_array[pp][cc].center_mean
		, Block_cluster_array[pp][cc].center_std
		, Block_cluster_array[pp][cc].width_mean
		, Block_cluster_array[pp][cc].width_std

		, Block_cluster_array[pp][cc].sequence_type
		, Block_cluster_array[pp][cc].first_enum_value
		, Block_cluster_array[pp][cc].mean_no_of_chars
		, Block_cluster_array[pp][cc].std_no_of_chars	      
		);
	      
	strcat(query,buff);
      }// noi > 1
    } //cc
  } // pp
  remove_last_comma(query);    
  if (debug) fprintf(stderr,"QUERY08: query=%s:\n", query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY08: query=%s: \n", query);
  } 

  return 0;
} // insert_tabbed_column_into_sql(int doc_id, int page_no) {

#define ADD_THRESHOLD 25
#define MERGE_THRESHOLD 25
int do_block_clusters(int pp) {
  Block_cluster_no[pp] = 0;
  Real_Block_cluster_no[pp] = 0;  

  int ss; // the similarity item
  int cc; // the cluster
  int cI = -1;
  int cJ = -1;

  fprintf(stderr, "CLUSTERING  %d:\n", sim_block_no[pp]);
  for (ss = 0; ss < sim_block_no[pp]; ss++) { 
    //if (debug) fprintf(stderr, "**GO OVER SS=%d: CL_NO=%d:%d:\n", ss, Block_cluster_no[pp], Real_Block_cluster_no[pp]);
    int found_in_clusters = 0;
    cI = 0; cJ = 0;
    int foundI = 0, foundJ = 0;
    for (cc = 0; found_in_clusters < 2 && cc < Block_cluster_no[pp]; cc++) { // check if new sim is already in an EXISTING CLUSTER
      if (foundI == 0) {
	foundI = found_item_in_block_cluster(pp, cc, sim_block_list[ss].ll, sim_block_list[ss].ll_block);
	if (foundI == 1) {
	  cI = cc;
	}
      }
      
      if (foundJ == 0) {
	foundJ = found_item_in_block_cluster(pp, cc, sim_block_list[ss].kk, sim_block_list[ss].kk_block);
	if (foundJ == 1) {
	  cJ = cc;
	}
      }
      
    } // for all clusters

    //if (debug) fprintf(stderr,"       FOUND ss=%d: fIJ1=%d:%d: cIJ=%d:%d: sim=%d: CL_NO=%d:\n", ss, foundI, foundJ, cI, cJ, sim_block_list[ss].sim, Block_cluster_no[pp]);
    int ii0 = sim_block_list[ss].ll;
    int ii1 = sim_block_list[ss].kk;
    int ll0 = sim_block_list[ss].ll_block;
    int ll1 = sim_block_list[ss].kk_block;    
    int xx0 = sim_block_list[ss].ll_last;
    int xx1 = sim_block_list[ss].kk_last;    
    int sim01 = sim_block_list[ss].sim;
    if (foundI == 1 && foundJ == 0 && sim01 < ADD_THRESHOLD) {

      // Block_cluster_array[pp][cI].block_array[Block_cluster_array[pp][cI].block_no] = ii1;     //UZ 082521
      Block_cluster_array[pp][cI].block_item_array[Block_cluster_array[pp][cI].block_no].first_word = ii1;
      Block_cluster_array[pp][cI].block_item_array[Block_cluster_array[pp][cI].block_no].block = ll1;
      Block_cluster_array[pp][cI].block_item_array[Block_cluster_array[pp][cI].block_no].last_word = xx1;      
      Block_cluster_array[pp][cI].cluster_type[left_i]++;
      Block_cluster_array[pp][cI].block_no++;

      if (Block_cluster_array[pp][cI].block_no >= MAX_BLOCKS_IN_CLUSTER-1) {
	fprintf(stderr,"No of itemd exceeded %d:\n", MAX_BLOCKS_IN_CLUSTER);
	exit(0);
      }

    } else if (foundJ == 1 && foundI == 0  && sim01 < ADD_THRESHOLD) {

      // Block_cluster_array[pp][cJ].block_array[Block_cluster_array[pp][cJ].block_no] = ii0; //UZ 082521
      Block_cluster_array[pp][cJ].block_item_array[Block_cluster_array[pp][cJ].block_no].first_word = ii0;
      Block_cluster_array[pp][cJ].block_item_array[Block_cluster_array[pp][cJ].block_no].block = ll0;
      Block_cluster_array[pp][cJ].block_item_array[Block_cluster_array[pp][cJ].block_no].last_word = xx0;      
      Block_cluster_array[pp][cJ].cluster_type[left_i]++;
      Block_cluster_array[pp][cJ].block_no++;

      if (Block_cluster_array[pp][cJ].block_no >= MAX_BLOCKS_IN_CLUSTER-1) {
	fprintf(stderr,"No of itemd exceeded %d:\n", MAX_BLOCKS_IN_CLUSTER);
	exit(0);
      }
      
    } else if (foundI == 1 && foundJ == 1
	       && cI >= 0 && cJ >=0 && cI != cJ
	       && sim_block_list[ss].sim < MERGE_THRESHOLD) { // merge clusters // any conditions on merge? what if each group is happy with 15 items?


      block_merge_clusters_J_into_I(pp, cI, cJ);
      Real_Block_cluster_no[pp]--;

    } else if (foundI == 0 && foundJ == 0 && sim01 < ADD_THRESHOLD) { // create new cluster with 2 items for this similarity
      int cc_block_no = 0;

      // Block_cluster_array[pp][Block_cluster_no[pp]].block_array[cc_block_no] = ii0; //UZ 082521
      Block_cluster_array[pp][Block_cluster_no[pp]].block_item_array[cc_block_no].first_word = ii0;
      Block_cluster_array[pp][Block_cluster_no[pp]].block_item_array[cc_block_no].block = ll0;
      Block_cluster_array[pp][Block_cluster_no[pp]].block_item_array[cc_block_no].last_word = xx0;      
      cc_block_no++;
	

      // Block_cluster_array[pp][Block_cluster_no[pp]].block_array[cc_block_no] = ii1; //UZ 082521
      Block_cluster_array[pp][Block_cluster_no[pp]].block_item_array[cc_block_no].first_word = ii1;
      Block_cluster_array[pp][Block_cluster_no[pp]].block_item_array[cc_block_no].block = ll1;
      Block_cluster_array[pp][Block_cluster_no[pp]].block_item_array[cc_block_no].last_word = xx1;      
      cc_block_no++;
	
      Block_cluster_array[pp][Block_cluster_no[pp]].block_no = cc_block_no;
      Block_cluster_array[pp][Block_cluster_no[pp]].cluster_type[left_i] = 2;      
	
      if (Block_cluster_no[pp] >= MAX_CLUSTERS-1) {
	fprintf(stderr,"No of cluster exceeded %d:\n", MAX_CLUSTERS);
	exit(0);
      }

      Block_cluster_no[pp]++;
      Real_Block_cluster_no[pp]++;      
    }
  } // SS, sim
  if (debug) fprintf(stderr, "FINISHED PAGE clusters :%d:\n", cc);

  // sort the blocks by order
  for (cc = 0; cc < Block_cluster_no[pp]; cc++) {
    qsort((void *)&(Block_cluster_array[pp][cc].block_item_array),  Block_cluster_array[pp][cc].block_no, sizeof(struct Block_Item), ppp_block_item_cmp);
  }

  if (pp == 2) {
    fprintf(stderr,"BLOCK CLUSTER0: pp=%d: ff=%d:\n", pp, Block_cluster_no[pp]);
    for (cc = 0; cc < Block_cluster_no[pp]; cc++) {
      int ii;
      int block_no = Block_cluster_array[pp][cc].block_no;
      fprintf(stderr,"BLOCK CLUSTER0: pp=%d: cc=%d: bn=%d:\n", pp, cc, block_no);
      for (ii = 0; ii < block_no; ii++) { // go over all the items, for each indicate the sn_in_line_of_first word
	int block = Block_cluster_array[pp][cc].block_item_array[ii].block; 
	fprintf(stderr,"BLOCK CLUSTER: pp=%d: cc=%d: ii=%d: bl=%d: fw=%d: lw=%d:\n", pp,cc,ii, block, block_array[block].first_word,block_array[block].last_word);
	/*Block_cluster_array[pp][cc].block_item_array[ii].sn_in_line_of_first_word = word_array[first_word].sn_in_line; */
      }
    }
  }
  return Block_cluster_no[pp];
} // do_block_clusters()

int print_block_pairs(int pp) {
  int ii;
  fprintf(stderr,"  TOC LIST for PAGE=%d: no=%d:\n",pp, sim_block_no[pp]);
  for (ii = 0; ii < sim_block_no[pp] ; ii++) {
      fprintf(stderr,"     TOC PAIR : ii=%d: sim=%d: lk=%d:%d: t=%s:%s:\n"
	      , ii
	      , sim_block_list[ii].sim

	      , sim_block_list[ii].ll_block 
	      , sim_block_list[ii].kk_block

	      , block_array[sim_block_list[ii].ll_block].text
	      , block_array[sim_block_list[ii].kk_block].text	    
	      );
  }
  return 0;
}

int report_toc_pages(int last_page_no) {
  int pp;
  int toc_pages = 0;
  fprintf(stderr,"\n");  
  for (pp = 1; pp <= last_page_no; pp++) {
    if ((float)((float)(page_properties_array[pp].no_toc_items) / (float)(page_properties_array[pp].last_line_in_page - page_properties_array[pp].first_line_in_page)) >  MAX_TOC_RATIO) {
      fprintf(stderr,"    REPORT:  PAGE IS TOC PP=%d: toc_cl=%d: toc_items=%d: line_no=%d: ratio=%4.2f:%4.2f:\n"
	      , pp, page_properties_array[pp].no_toc_clusters, page_properties_array[pp].no_toc_items
	      , page_properties_array[pp].last_line_in_page - page_properties_array[pp].first_line_in_page
	      , (float)((float)(page_properties_array[pp].no_toc_items) / (float)(page_properties_array[pp].last_line_in_page - page_properties_array[pp].first_line_in_page))
	      , MAX_TOC_RATIO
	      );
      toc_pages++;      
    }
  }
  fprintf(stderr,"REPORT:  FOUND :%d: TOC PAGES\n", toc_pages);
  return 0;
}



int is_contents_OK(char *ftext, char *ltext) {
  int ret = (strcasecmp(ltext,"page") == 0 || strcasecmp(ltext,"contents") == 0 || strcasecmp(ftext,"section") == 0) ? 1 : 0;
  return ret;
}


int count_toc_bad_lines(int pp) {
  int ll;
  int ret = 0;

  fprintf(stderr,"YYYY: pp=%d: fl=%d:%d:\n", pp, page_properties_array[pp].first_line_in_page, page_properties_array[pp].last_line_in_page  );
  for (ll = page_properties_array[pp].first_line_in_page; ll <= page_properties_array[pp].last_line_in_page; ll++) {
    int fww = line_array[ll].first_word;
    int lww = line_array[ll+1].first_word-1;
    if (word_array[fww].text && word_array[lww].text) {
      int left_OK = is_digit(word_array[fww].text) || is_SECTION(word_array[fww].text);
      int right_OK = is_digit(word_array[lww].text);
      int contents_OK = is_contents_OK(word_array[fww].text, word_array[lww].text);
      int found_OK = left_OK || right_OK || contents_OK;
      ret += !found_OK;
      fprintf(stderr,"  WWW: pp=%d: flw=%d: :%d:%s:  :%d:%s: OK=%d: ret=%d:\n", pp, ll, fww, word_array[fww].text, lww, word_array[lww].text, found_OK, ret);
    } else {
      fprintf(stderr,"  ERROR: null text, WWW: pp=%d: ll=%d: fl=%d:%d: ret=%d:\n", pp, ll, fww, lww, ret);      
      ret++;
    }
  }
  return ret;
}

int insert_bad_lines_into_sql(int doc_id, int page_no) {
  char query[50000];
  sprintf(query,"delete from deals_bad_toc_lines \n\
           where doc_id = '%d' "
	  , doc_id);
  if (debug) fprintf(stderr,"QUERY47=%s\n",query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"QUERY47=%s\n",mysql_error(conn));
    exit(1);
  }

  sprintf(query,"insert into deals_bad_toc_lines \n\
                 (doc_id, page_no, no_of_bad_lines) \n\
                 values \n ");


  int pp;
  for (pp = 0; pp < page_no; pp++) {
	char buff[500000];
	sprintf(buff,"(%d, %d, %d) , \n "
		, doc_id
		, pp
		, page_properties_array[pp].no_of_bad_toc_lines
		);
	      
	strcat(query,buff);
  } // pp 

  remove_last_comma(query);    
  if (debug) fprintf(stderr,"QUERY06: query=%s:\n", query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY06: query=%s: \n", query);
  } 
  
  return 0;
}

int create_TOC_block_clusters(int last_page_no, int *essential) {
  int pp;
  fprintf(stderr,"create_TOC_block_clusters=%d:\n", last_page_no);  
  for (pp = 1; pp <= last_page_no; pp++) {
    fprintf(stderr,"\n\nPAGE:%d:%d:\n",pp,debug);
    fprintf(stderr,"*******1.  RESET PAGE:%d: deb=%d:\n", pp, debug);
    reset_sim_block_array(pp);
    fprintf(stderr,"*******2.  SIM PAGE:%d:%d:\n", pp,debug);    
    sim_block_no[pp] = create_sim_block_array(pp);
    fprintf(stderr,"*******3.  SORT PAGE:%d:%d: debug=%d:\n", pp, sim_block_no[pp], debug);    
    sort_sim_block_pairs(pp);
    print_block_pairs(pp);
    fprintf(stderr,"*******4.  CLUSTERS PAGE:%d:%d: :%d:\n", pp,debug, *essential);    
    int nn = do_block_clusters(pp);
    fprintf(stderr,"*******5.  DECIDE CLUSTERS :%d nn=%d: deb=%d:\n", pp, nn,debug);        
    decide_block_clusters(pp);    
    fprintf(stderr,"*******5.  DONE CLUSTER PAGE:%d nn=%d: deb=%d:\n", pp, nn,debug);        
    page_properties_array[pp].no_of_bad_toc_lines = count_toc_bad_lines(pp);
  }
  report_toc_pages(last_page_no);
  insert_tabbed_column_into_sql(doc_id, last_page_no+1);
  insert_bad_lines_into_sql(doc_id, last_page_no+1);  
  return *essential;
}
