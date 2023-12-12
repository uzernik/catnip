#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <mysql.h>
#include <stdlib.h>	/* for bsearch() */
#include "entity_functions.h"
#include "my_import_functions.h"
#include "align_functions.h"
#include "print_toc_functions.h"
#include "create_toc.h"

/********************************
PROBLEMS NOT SOLVED
1. toc_toc longer than one page
2. toc_toc not at the beginning
3. toc_toc not flat, hierarchical
4. no enumeration at all
5. what if TOC is not found? it tries and wastes 20 seconds
6. use Table of Contents title

********************************/
long long tt1 = 0;
long long current_timestamp(int nn) {
  struct timeval te;
  gettimeofday(&te, NULL); // get current time
  long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
  if (nn == 0) tt1 = milliseconds;
  fprintf(stderr,"********** NN:%d: milliseconds: %lld\n", nn, milliseconds);
  if (nn == 1) fprintf(stderr,"TIME:%lld:\n", milliseconds - tt1);
  return milliseconds;
}


// we don't look at long headers for toc-toc
#define legit_header_length 40
char *normalize(char *text, int my_keep_case) { // this is used only for TOC_TOC, not for REST_TOC
  int ii;
  int jj = 0;
  static char bext[500];
  int my_min = MIN(strlen(text), legit_header_length+1);
  if (text) {
    for (jj = 0, ii = 0; ii < my_min; ii++) {
      if (isalnum(text[ii]) != 0) { // if alnum 
	bext[jj++] = (my_keep_case == 1) ? text[ii] : tolower(text[ii]);
      } else { // remove if not alnum
	;
      }

    }
  } else {
    jj = 0;
  }
  bext[jj] = '\0';
  return strdup(bext);
} // normalize()

/* determine the start of the TOC and the strat of REST
** take the first pair that the TOC shares 5 items (assuming TOC is on one page) 
*/

int print_pairs() {
  int kk;

  for (kk = 0; kk < pair_no; kk++) {
    fprintf(stderr,"   GGGGGB:kk=%2d: ii=%d: ppii=%2d:%2d: ppjj=%3d:%3d:  ij=:%2d:%2d: same_page=%2d: mono=%2d:   :%s:%s: \n" 
	    , kk
	    , pair_array[kk].ii, item_array[pair_array[kk].ii].page_no, item_array[pair_array[kk].ii].pid
	    , item_array[pair_array[kk].jj].page_no, item_array[pair_array[kk].jj].pid
	    , pair_array[kk].ii, pair_array[kk].jj,      pair_array[kk].same_page, 0,       item_array[pair_array[kk].ii].clean_header,item_array[pair_array[kk].jj].clean_header);
  }

  return 0;
} // find_first_good_pair_to_fold


int find_first_good_pair_to_fold() { 
  int kk;

  for (kk = 0; kk < pair_no; kk++) { // go over the pairs
    int nn;
    pair_array[kk].same_page = 0;

    for (nn = 0; nn < pair_no; nn++) {
      if (item_array[pair_array[kk].ii].page_no == item_array[pair_array[nn].ii].page_no) {
	pair_array[kk].same_page++;
      }
    }

    fprintf(stderr,"   GGGGGA:kk=%2d: ii=%d: ppii=%2d:%2d: ppjj=%3d:%3d:  ij=:%2d:%2d: same_page=%2d: mono=%2d:   :%s:%s: \n" 
	    , kk
	    , pair_array[kk].ii, item_array[pair_array[kk].ii].page_no, item_array[pair_array[kk].ii].pid
	    , item_array[pair_array[kk].jj].page_no, item_array[pair_array[kk].jj].pid
	    , pair_array[kk].ii, pair_array[kk].jj,      pair_array[kk].same_page, 0,       item_array[pair_array[kk].ii].clean_header,item_array[pair_array[kk].jj].clean_header);
  }

  int ret = -1;
  for (kk = 0; kk < pair_no; kk++) {
    if (pair_array[kk].same_page >= 5) {
      ret = kk;
      break;
    }
  }
  return ret;
} // find_first_good_pair_to_fold

#define MIN_HEADER_FOR_COMPARE 2
int create_and_compare_toc_pairs_for_folding() { // compare each pair using Levenshtein in order to decide where to fold the list
  int ii, jj;
  int kk = 0;
  int wrap_LD_count = 0;
  for (ii = 0; ii < item_no; ii++) {
    if (strlen(item_array[ii].norm_header) < legit_header_length && strlen(item_array[ii].norm_header) > MIN_HEADER_FOR_COMPARE) {    // only long enough
      fprintf(stderr,"HH:%2d: pp=%3d:%2d: tit=%10s:  sec=%6s: hd=%30s:  nh=%30s:\n",ii,item_array[ii].pid, item_array[ii].page_no, item_array[ii].title, item_array[ii].section, item_array[ii].clean_header, item_array[ii].norm_header);
      int min_total_dist = 1000;
      int min_reg_dist = -1;
      int min_dist_jj = -1;
      for (jj = ii+1; jj < item_no; jj++) {
	if (strlen(item_array[jj].norm_header) < legit_header_length  && strlen(item_array[jj].norm_header) > MIN_HEADER_FOR_COMPARE) {    // only long enough
	  wrap_LD_count++;
	  int reg_dist = wrap_LD(item_array[ii].norm_header,item_array[jj].norm_header);
	  wrap_LD_count++;	  
	  int case_dist = wrap_LD(item_array[ii].case_norm_header,item_array[jj].case_norm_header);	  
	  int total_dist = reg_dist + case_dist;
	  if (total_dist < min_total_dist) {  // find the best match for this header
	    min_total_dist = total_dist;
	    min_dist_jj = jj;
	    min_reg_dist = reg_dist;	    
	  }
	}
      } // for jj

      int same_page = 0;
      if (min_reg_dist < 3) { // insert into pairs_array only items with small distance
	int jj;
	for (jj = 0; jj <= kk; jj++) {
	  if (item_array[ii].page_no == item_array[jj].page_no) {
	    same_page++;
	  }
	}
	if (1) fprintf(stderr,"   GGGGG:kk=%2d: ppii=%2d:%2d: ppjj=%3d:%3d:  ij=:%2d:%2d: D=%2d:%2d:   :%s:%s: :%d:\n"
		       , kk
		       , item_array[ii].page_no, item_array[ii].pid, item_array[min_dist_jj].page_no, item_array[min_dist_jj].pid
		       , ii, min_dist_jj,       min_total_dist, min_reg_dist,       item_array[ii].clean_header,  item_array[min_dist_jj].clean_header
		       , same_page);
	pair_array[kk].ii = ii;
	pair_array[kk].jj = min_dist_jj;	
	pair_no = kk++;
      }
      if (same_page > 7) { // once found a header with 6 brothers on the same page then we are done, SAVES TONS OF TIME (30 seconds)!!!!
	break;
      }
    } // only long enough
  } // for ii
  fprintf(stderr,"   WRAP_LD_COUNT=%d:\n", wrap_LD_count);
  return 0;
} // create_and_compare_toc_pairs_for_folding()

int normalize_headers() {
  int ii;
  fprintf(stderr,"HEADERS:%d:\n",item_no);
  for (ii = 0; ii < item_no; ii++) {
    item_array[ii].norm_header = normalize(item_array[ii].clean_header, 0);
    item_array[ii].case_norm_header = normalize(item_array[ii].clean_header, 1);    
    fprintf(stderr,"      HEADER:%d:%d:%s: :%s:  :%s:\n", ii,  item_array[ii].my_loc,  item_array[ii].clean_header,  item_array[ii].norm_header,  item_array[ii].case_norm_header);
  }
  fprintf(stderr,"DONE HEADERS:%d:\n",item_no);  
  return 0;
}

int index_line2position_of_rest_file(char rest_buff[]) {
  int pos_no = 0;
  int line_no = 0;  
  for (pos_no = 0; pos_no < strlen(rest_buff); pos_no++) {
    if (rest_buff[pos_no] == '$') {
      rest_line2position_array[line_no] = pos_no;
      rest_position2line_array[pos_no] = line_no;
      line_no++;
    } else {
      rest_position2line_array[pos_no] = line_no-1;
    }
  }
  return pos_no;
}

int index_line2position_of_toc_toc_file(char toc_toc_buff[], int first_toc) {
  int pos_no = 0;
  int line_no = 0;  
  for (pos_no = 0; pos_no < strlen(toc_toc_buff); pos_no++) {
    if (toc_toc_buff[pos_no] == '$') {
      toc_toc_line2position_array[line_no] = pos_no;
      toc_toc_position2line_array[pos_no] = line_no + first_toc;
      line_no++;
    } else {
      toc_toc_position2line_array[pos_no] = line_no-1 + first_toc;
    }
    //fprintf(stderr,"QQQQQQQQQQQQQ:%d:%d:\n",pos_no,line_no);
  }
  int ii;
  for (ii = 0; ii <= pos_no; ii++) {
    //fprintf(stderr,"QQQQQQQQQQQQQ1:%d:%d:\n",ii, toc_toc_position2line_array[ii]);
  }
  return pos_no;
}


// input aaa5 into buff
// the second half of buff goes to buff2
// processed by lex
// output to rest file
int scan_and_clean_rest_file(int ii1, int ii2, char *rest_name, int first_toc) {
  // step 0 copy aaa5 into buffer
  fprintf(stderr,"YYWRAP\n");
  yywrap();
  my_rewind_and_BEGIN(); // here we rewind yyin (aaa5)
  buff_ctr = 0;
  yylex(); // copy into buffer
  MY_TOTAL_file[buff_ctr] = '\0';

  char *buff = MY_TOTAL_file;
  static char rest_buff2[10000000];

  fprintf(stderr,"IIII00:%s:\n",buff);
  sprintf(rest_buff2,"%s",buff+ii2);
  buff[ii2] = '\0';
  fprintf(stderr,"IIII02:%s:\n",rest_buff2);    
  // step 1 remove PP and tag and case_normalize
  my_begin_yyyy1();
  yy_scan_string(rest_buff2);
  if (debug) fprintf(stderr,"DOING yylex12:%ld:\n",strlen(rest_buff2));
  buff_ptr = rest_buff12;
  strcpy(buff_ptr,"$ "); // insert first para that was dropped earlier
  yylex();
  fprintf(stderr,"IIII12:%s:\n",buff_ptr);

  // step 2 chop to 50 char
  my_begin_yyyy2();
  yy_scan_string(rest_buff12);
  if (debug) fprintf(stderr,"DOING yylex22:%ld:\n",strlen(rest_buff12));
  buff_ptr = rest_buff22;
  strcpy(buff_ptr,"");
  yylex();
  fprintf(stderr,"IIII22:%s:\n",buff_ptr);

  // step 3 multiple [\n]
  my_begin_yyyy3();
  yy_scan_string(rest_buff22);
  if (debug) fprintf(stderr,"DOING yylex32:%ld:\n",strlen(rest_buff22));
  buff_ptr = rest_buff32;
  strcpy(buff_ptr,"");
  yylex();
  fprintf(stderr,"IIII32:%s:\n",buff_ptr);

  // step 4 output buffers into rest_file
  FILE *rest_file = fopen(rest_name,"w");
  if (!rest_file) {
    fprintf(stderr,"Error: can't (w) open toc_toc file %s\n", rest_name);
    exit(0);
  }
  fprintf(rest_file,"%s",buff_ptr);
  fclose(rest_file);

  int rest_position_no = index_line2position_of_rest_file(rest_buff32);
  int toc_toc_position_no = index_line2position_of_toc_toc_file(toc_toc_buff, first_toc);  
  
  return rest_position_no;

} // scan_and_clean_rest_file(int ii1, int ii2)

int do_align(char *toc_toc_name, char *rest_name, char *results_name) {
  static char cmd[4000];
  sprintf(cmd,"python %s/deals/coordinates_extractor/diff.py '1234' '%s' '%s' '%s' ;", app_path, toc_toc_name, rest_name, results_name); // align aaa and bbb into ccc
  fprintf(stderr,"CMD:%s\n",cmd);
  system(cmd);

  return 0;
}
int found_first_number(char *buff) { // the buff goes: "13..." or "$ 13...."
  int ret = 0;
  if (isdigit(buff[0]) != 0) {
    ret = 1;
  } else if (buff[0] == '$' && buff[1] == ' ' && isdigit(buff[2]) != 0)  {
    ret = 1;
  } else {
    ;
  }
  return ret;
}

int transfer_toc_toc_into_file(int first_toc,int first_rest, char *toc_toc_name, char toc_toc_buff[]) {
  int ii;
  static char buff[3000];
  fprintf(stderr,"PRINTING TOC_TOC:%d:%d:\n",first_toc,first_rest);
  for (ii = first_toc; ii < first_rest; ii++) {
    sprintf(buff,"$%s %s %s\n"
	    , ((strcmp(item_array[ii].title,"_") == 0) ? "" : item_array[ii].title)
	    , item_array[ii].section
	    , item_array[ii].case_norm_header);
    strcat(toc_toc_buff,buff);
    toc_toc_line2item_no_array[ii] = ii/*+first_toc*/; // this is bc the first '$' corresponds to the FIRST item
    fprintf(stderr," TOC_TOC:%d:%d:%s:%s:%s: ii=%d: ftoc=%d: v=%d:\n",ii, item_array[ii].page_no, item_array[ii].title,item_array[ii].section,item_array[ii].case_norm_header
	    , ii, first_toc, toc_toc_line2item_no_array[ii]);
  }
  fprintf(stderr,"IIII30:%s:\n",toc_toc_buff);
  
  FILE *toc_toc_file = fopen(toc_toc_name,"w");
  if (!toc_toc_file) {
    fprintf(stderr,"Error: can't (w) open toc_toc file %s\n", toc_toc_name);
    exit(0);
  }
  fprintf(toc_toc_file,"%s",toc_toc_buff);
  fclose(toc_toc_file);

  return 0;
} // transfer_toc_toc_into_file(int first_toc,int first_rest) 

// if entry includes "$ (a) blabla" or "$ 4.1 bla bla" or " (a) blabla" or " 4.1 bla bla"  then it's an indepedent entry and should not be merged
int independent_entry(char *text) {
  char aaa[5000];
  int nn = sscanf(text,"$ %[a-zA-Z] ",aaa);
  if (nn == 0) nn = sscanf(text," %[a-zA-Z] ",aaa);
  if (nn == 0) nn = sscanf(text,"$ %[0-9] ",aaa);
  if (nn == 0) nn = sscanf(text," %[0-9] ",aaa);      
  return nn;
}

int find_clumps_in_results_buff(struct NYU_Struct nyu_results_array[], int nyu_results_no) {
  int ii;
  fprintf(stderr,"SCORing:%d:\n",nyu_results_no);
  for (ii = 0; ii < nyu_results_no; ii++) {
    int len = nyu_results_array[ii].nyu_len = strlen(nyu_results_array[ii].nyu_buff);
    nyu_results_array[ii].nyu_score =
      len // length
      + ((nyu_results_array[ii].nyu_buff[0] == '$') ? 2 : 0) // starts with '$'
      + ((found_first_number(nyu_results_array[ii].nyu_buff) == 1) ? 2 : 0) // has a starting number
      + (len > 0 && (nyu_results_array[ii].nyu_buff[len-1] == '$') ? 2 : 0) // ends with '$'
      + (len > 1 && (nyu_results_array[ii].nyu_buff[len-2] == '$') ? 2 : 0) // ends with '$'
      ;
    nyu_results_array[ii].nyu_dist = (ii < nyu_results_no - 1) ? (nyu_results_array[ii+1].nyu_ptr2 - (nyu_results_array[ii].nyu_ptr2 + len)) : 10000;

    fprintf(stderr,"  SCORE:%d: sc=%d: l=%d: dist=%d:   ptr=%d:%d: :%d:%s:\n"
	    , ii,    nyu_results_array[ii].nyu_score, len, nyu_results_array[ii].nyu_dist

	    , nyu_results_array[ii].nyu_ptr1, nyu_results_array[ii].nyu_ptr2

	    , strlen(nyu_results_array[ii].nyu_buff)
	    , nyu_results_array[ii].nyu_buff
	    );
  }
  fprintf(stderr,"\nHEADER RESULTS:%d:\n",nyu_results_no);
  int yy = 0;
  for (ii = 0; ii < nyu_results_no; ii++) { // now loop and merge
    if (nyu_results_array[ii].nyu_merged == 0) { // only if not merged already
      char total_merged_header[5000];
      int total_score = nyu_results_array[ii].nyu_score;
      strcpy(total_merged_header,nyu_results_array[ii].nyu_buff);
      int kk = 0;
      while (nyu_results_array[ii+kk].nyu_dist < 3
	     && independent_entry(nyu_results_array[ii+kk+1].nyu_buff) == 0
	     ) {
	nyu_results_array[ii+kk+1].nyu_merged = 1;
	strcat(total_merged_header,nyu_results_array[ii+kk+1].nyu_buff);	
	total_score += nyu_results_array[ii+kk+1].nyu_score;	
	kk++;
	nyu_results_array[ii].nyu_merger_no = kk;
	nyu_results_array[ii].nyu_total_merged_header = strdup(total_merged_header);			
      }
      if (total_score > 11) {
	nyu_results_array[ii].nyu_good_header = 1;
	fprintf(stderr,"  SCOREA:%d:%d:   sc=%d:%d: l=%d: dist=%d:   ptr=%d:%d: :%d:%s: tot=%s:\n"
		, yy++, ii

		, total_score, nyu_results_array[ii].nyu_score, nyu_results_array[ii].nyu_len, nyu_results_array[ii].nyu_dist

		, nyu_results_array[ii].nyu_ptr1, nyu_results_array[ii].nyu_ptr2

		, strlen(nyu_results_array[ii].nyu_buff)
		, nyu_results_array[ii].nyu_buff, nyu_results_array[ii].nyu_total_merged_header
		);
      }
      
    } // if not merged already
  } // for ii
  return yy;
} // find_clumps_in_results_buff(struct NYU_Struct nyu_results_array[], int nyu_results_no) 

int import_results_file(char *results_name, struct NYU_Struct nyu_results_array[], char *results_ptr) {
  my_begin_yyyy6();
  char *results_buffer = my_fopen(results_name);
  yy_scan_string(results_buffer);
  if (debug) fprintf(stderr,"DOING yylex6:%ld:\n",strlen(results_buffer));
  nyu_results_no = -1;
  yylex();
  fprintf(stderr,"IIII6:%d:\n",nyu_results_no);
  int ii;
  for (ii = 0; ii < nyu_results_no; ii++) {
    while (nyu_results_array[ii].nyu_buff && nyu_results_array[ii].nyu_buff[0] == ' ') { // if the first char is " " (used to be "\n"), then move up by one
      nyu_results_array[ii].nyu_buff++; // move one up
      nyu_results_array[ii].nyu_ptr2++; 
      nyu_results_array[ii].nyu_ptr1++;     
    }
    fprintf(stderr,"NYU:%d:\t:%d:\t:%s:\n"
	    , nyu_results_array[ii].nyu_ptr1, nyu_results_array[ii].nyu_ptr2, nyu_results_array[ii].nyu_buff);
  }
  return nyu_results_no;
} // import_results_file() 


/*  
**  FIRST FIND WHERE TO "FOLD" EXISTING HEADER LIST (at the first matching pair)
**  SECOND IDENTIFY THE TOP-LEVEL ITEMS
**  THIRD ENTER THEM INTO ITEM_ARRAY and GROUP_ARRAY
**  FOURTH TAKE CARE IN OLD_ALIGN
**  1. get the given TOC from SQL into a buffer: toc_toc_buffer
**  2. find overlap between toc_toc and toc (create_and_compare)
**  3. find first toc_toc and first toc by finding first overlap with 5 brothers on page, print to toc_toc_file)
**  4. LEX: get the aaa5 file into a buffer, process, and output into rest file (input_split)
**  5. ALIGN toc_toc VS. rest into results_file
**  6. import into nyu_results_array
**  7. identify headers in results array, store in nyu_results_array[].merger_no
**  8. map headers into aaa5 file using nyu_ptr2 (pos in rest file) --> results_index (para in aaa5 file)
**  9. update LISTS
*/
int align_by_toc_toc() {

  sprintf (toc_toc_name,"%s/%s/%s_%d",app_path, "tmp", TOC_TOC_PATH, doc_id);
  sprintf (rest_name,"%s/%s/%s_%d",app_path, "tmp", REST_PATH, doc_id);
  sprintf (results_name,"%s/%s/%s_%d",app_path, "tmp", RESULTS_PATH, doc_id);    


  current_timestamp(900);
  fprintf(stderr,"ORG1: normalize headers\n");
  normalize_headers(); // step 1:  normalize toc header with and w/o case
  current_timestamp(901);
  fprintf(stderr,"ORG2: create and compare pairs:\n");  
  create_and_compare_toc_pairs_for_folding(); // step 2:  find matching toc header pairs, stop after a sequence of 5 -- what if it is not found???
  current_timestamp(902);
  print_pairs();
  fprintf(stderr,"ORG3: find first pair\n");
  int first_pair = find_first_good_pair_to_fold(); // step 3: get the first in the sequence
  //first_pair = -1;
  if (first_pair >-1) {
    int ii = pair_array[first_pair].ii;
    int jj = pair_array[first_pair].jj;
    fprintf(stderr,"FIRST_PAIR:kk=%d:  ii=%d:%d: jj=%d:%d:\n",first_pair,ii,item_array[ii].my_loc,jj, item_array[jj].my_loc);
    transfer_toc_toc_into_file(ii,jj, toc_toc_name, toc_toc_buff); // step 4: get the toc list into a physical file
    int bii = item_array[ii].my_loc;
    int bjj = item_array[jj].my_loc;
    int position_no = scan_and_clean_rest_file(bii, bjj, rest_name, ii); // step 5: using a sequence of LEX, process the aaa5 file into a clean file (rest_name)
    do_align(toc_toc_name, rest_name, results_name); // step 6: the main step, run align between the toc_toc and the rest files into results file
    char *results_ptr;
    nyu_results_no = import_results_file(results_name, nyu_results_array, results_ptr);  //step 7: using LEX import results into an array
    fprintf(stderr,"YYY0:%d:\n", nyu_results_no);
    nyu_good_results_no = find_clumps_in_results_buff(nyu_results_array, nyu_results_no); // step 8:  collect together separate clumps into good size headers
  } else {
    fprintf(stderr,"BAD FIRST_PAIR:kk=%d: \n",first_pair);
  }
  current_timestamp(910);
  return first_pair;
} // align_by_toc_toc() 


int add_top_seq_to_insert_item_array_is_special() { // add info from NYU_RESULTS_ARRAY into INSERT_ITEM_ARRAY is_special 12/13
  int ii; // the nyu items, good and bad
  int kk; // good nyu_items
  fprintf(stderr,"MAPPING:\n");
  for (kk = 0, ii = 0; ii < nyu_results_no; ii++) {
    if (nyu_results_array[ii].nyu_good_header == 1) {
      int pos2 = nyu_results_array[ii].nyu_ptr2;
      int line2 = rest_position2line_array[pos2];
      int para2 = rest_line2para_no_array[line2];
      int is_special = 13;
      char *merged_header = (nyu_results_array[ii].nyu_total_merged_header) ? nyu_results_array[ii].nyu_total_merged_header : nyu_results_array[ii].nyu_buff;
      kk++;
      nyu_results_array[ii].nyu_rest_pid = para2;      
      int pos1 = nyu_results_array[ii].nyu_ptr1;
      int line1 = toc_toc_position2line_array[pos1];
      int toc_item1 = toc_toc_line2item_no_array[line1];

      int jj = 0;
      static int last_jj = 0; // where the count over item_array stopped last time
      int found = 0;
      for (jj = last_jj; jj < item_no; jj++) {
	if (insert_item_array[jj].pid == para2) {
	  insert_item_array[jj].is_special = 13;  // if item is exists in the toc then just mark it as special
	  if (do_take_headers_from_toc_toc == 1) {
	    insert_item_array[jj].my_text = item_array[toc_item1].section;
	    insert_item_array[jj].my_group = item_array[toc_item1].grp_no;
	    insert_item_array[jj].header = item_array[toc_item1].header; // or keep it the original??? merged_header???
	  }
	  last_jj = jj + 1;
	  found = 1;
	  break;
	} else if (insert_item_array[jj].pid > para2) {
	  last_jj = jj;
	  found = 0;
	  break;
	}
      } // for jj
      if (found == 0) {  
	add_one_extra_item_to_insert_item_array(merged_header, para2, pos2, line2, is_special, toc_item1); // add only if it does not already exist
      }
      nyu_results_array[ii].nyu_toc_item_no = toc_item1;      
      insert_item_array[toc_item1].is_special = 12;
      fprintf(stderr,"MAP  :%d:%d:    pos2=%d: line2=%d: para2=%d: ||  pos1:%d: line1:%d: item1=%d: text1=%s:%s:|| :%s:\n",
	      ii, kk-1, pos2, line2, para2, pos1, line1, toc_item1, item_array[toc_item1].section, item_array[toc_item1].header, merged_header);
    } // if good header
  } // for ii
  return 0;
} // add_top_seq_to_insert_item_array_is_special()

int count_found_toc_toc_items_per_group(int found_toc13_in_group[],  int found_toc12_in_group[]) {
  int kk;
  for (kk = 0; kk < MAX_GROUP; kk++) { // LOOP over GROUP
    found_toc12_in_group[kk] = 0;
    found_toc13_in_group[kk] = 0;      
    if (kk == FILE_GROUP) { // EXHIBIT, force it in
      ;
    } else if (kk == FUNDAMENTALS_GROUP) { // force it in
      ;
    } else { // don't use alignment for tiny seqs OR for 6.13, they requires some extra treatment (check also the prev span)
      int g_sn;
      for (g_sn = 0; g_sn < group_no_array[kk]; g_sn++) { // LOOP over ITEMs in GROUP
	int in = group_item_array[kk][g_sn].item_no;
	char *sec = item_array[in].section;
	if (item_array[in].is_special == 13) {
	  fprintf(stderr,"   FOUND TOC_REST 13: gr_no=%d: g-sn=%d: in=%d: :%s:\n"
		  ,kk, g_sn, in, sec);
	  found_toc13_in_group[kk]++;
	} else 	if (item_array[in].is_special == 12) {
	  fprintf(stderr,"    FOUND TOC_TOC 12: gr_no=%d: g-sn=%d: in=%d: :%s:\n"
		  ,kk, g_sn, in, sec);
	  found_toc12_in_group[kk]++;	  
	}
      }
    }
    if (found_toc12_in_group[kk] > 0) {
      fprintf(stderr,"TOTAL FOUND TOC_TOC 12: gr_no=%d: total=%d:\n",kk,found_toc12_in_group[kk]);
    }
    if (found_toc13_in_group[kk] > 0) {
      fprintf(stderr,"TOTAL FOUND TOC_TOC 13: gr_no=%d:  total=%d:\n",kk,found_toc13_in_group[kk]);
    }
  } // for kk 
  return 0;
} // check_toc_toc_group() {


int bi_ppp_cnt(const void *a, const void *b) {
  int x = ((struct Insert_Item_Struct *)a)->pid;
  int y = ((struct Insert_Item_Struct *)b)->pid;
  return(y<x);
}

int sort_insert_item_array_by_count(struct Insert_Item_Struct insert_item_array[], int insert_item_no) {
  qsort((void *)&insert_item_array[0], insert_item_no,sizeof(struct Insert_Item_Struct),bi_ppp_cnt);
  return 0;
}

