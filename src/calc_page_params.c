/*

33 PAGES:
../bin/calc_page_params -d 71815 -Pudev1-from-prod-feb26.cu0erabygff3.us-west-2.rds.amazonaws.com -N dealthing -U root -W imaof333 -D 1 -scalc_page_params >& ttt


#UDEV3:
../bin/calc_page_params -d 117561 -Pudev3-from-prod-jul27.cu0erabygff3.us-west-2.rds.amazonaws.com  -N dealthing -U root -W imaof333 -D 1 -s calc_page_params -2dp -3/home/ubuntu/efs/media//CLS/test_tok/test_tok/aws_spdf_95048_LUOLZwe -M/home/ubuntu/efs/media/ -v120 -T../../dealthing/dtcstuff/data/WordCountDir/words_file_for_bigrams -cUSA >& ttt

UDEV3:
(../bin/calc_page_params -d 117570 -Pudev3-from-prod-jul27.cu0erabygff3.us-west-2.rds.amazonaws.com -N dealthing -U root -W imaof333 -D1 -scalc_page_params -2dp -3my_spdf -M/home/ubuntu/efs/media -v120 -T../data/WordCountDir/words_file_for_bigrams -cUSA > aaa61) >& www21 

OMER:
../bin/calc_page_params -d 71840 -Pudev1-from-prod-feb26.cu0erabygff3.us-west-2.rds.amazonaws.com -N dealthing -U root -W imaof333 -D 1 -scalc_page_params >& ttt_71840_calc

ENUM:
../bin/calc_page_params -d 71851 -Pudev1-from-prod-feb26.cu0erabygff3.us-west-2.rds.amazonaws.com -N dealthing -U root -W imaof333 -D 1 -scalc_page_params >& ttt_71840_calc

PAIN
../bin/calc_page_params -d 58275 -Pudev1-from-prod-feb26.cu0erabygff3.us-west-2.rds.amazonaws.com -N dealthing -U root -W imaof333 -D 1 -s calc_page_params >& ttt

#UDEV2:
../bin/calc_page_params -d 95048 -Pudev2-from-prod-aug12.cu0erabygff3.us-west-2.rds.amazonaws.com  -N dealthing -U root -W imaof333 -D 1 -s calc_page_params -2/home/ubuntu/efs/media/cls__france_/bellevue/my_test_exalog/displayfile_00095048 -3/home/ubuntu/efs/media/cls__france_/bellevue/my_test_exalog/aws_spdf_95048_LUOLZwe -M/home/ubuntu/efs/media/ -v120 -T../../dealthing/dtcstuff/data/WordCountDir/words_file_for_bigrams -cUSA >& ttt

UKDEV
date;../bin/calc_page_params -d60863 -P ukdev-2022-01-12.cgomfrnah8ae.eu-west-2.rds.amazonaws.com  -N dealthing -U root -W imaof333 -D 1 -s calc_page_params -2/home/ubuntu/efs/media/cls__france_/bellevue/my_test_exalog/displayfile_00060863 -3/home/ubuntu/efs/media/cls__france_/bellevue/my_test_exalog/aws_spdf_60863_LUOLZwe -M/home/ubuntu/efs/media/ -v120 -T../../dealthing/dtcstuff/data/WordCountDir/words_file_for_bigrams -cUSA >& ttt63; date

------------------------------------------------------------------------------------------------
  1. read ocrtoken table
  2. calculates para, double_col, header/footer, left margin, right_margin, resume


INPUT: DEALS_OCRBLOCK, DEALS_ocrtoken
OUTPUT: 1. reading word order: re_enumerate blocks and words
        2. footer/header are eliminated
        3. left line enumeration is eliminated

1.  read  words and blocks from SQL
2.  cluster blocks by footer header and mark as such -- so won't be re-enumerated (0, -1, -2)
3.  cluster blocks by line-enumeration and mark as such -- so TOC won't be re-enumerated
4.  cluster toc page-lines and mark as such -- so last number on line won't be taken as right side of double-column
5.  cluster gaps and take center caps in order to decide double-column groups

*/
  
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


int ENUM_LEFT_MARGIN = 4000; // ab units
int last_page_no = 0; // remember pn starts from 1! so 33 pages means the last page is 33!!!
int last_block_no = 0;
int last_line_no = 0;
char *displayfile_name;
char *spdffile_name;
char *media; 

float X_back_to_aby_factor = 0.0616375;
float Y_back_to_aby_factor = 0.0792393;

// we don't do enumerated lines in GBR, it catches sec items 1,2,3, no periods
char *doc_country = "USA";

char *dict_path;
int essential;
int in_dict(char *word) {
  return 1;
}

// decide what's stronger: regulation or regula-tion, mutual information
float split_dict(char *word1, char *word2) {
  char word0[200];
  sprintf(word0, "%s%s", word1, word2);
  int n_dict1 = in_dict(word1);
  int n_dict2 = in_dict(word2);
  int n_dict0 = in_dict(word0);
  float ret = (float)(float)(n_dict1 * n_dict2) / (float)(n_dict0 * n_dict0);
  return ret;
} // split_dict()


int fix_split_token(struct Obj_Struct word_array[], int word_count) {
  int ii;
  for (ii = 0; ii <= word_count; ii++) {
    if (word_array[ii].split_token == 1 && word_array[ii+1].text[0] == '-') { // thirty-day
      float is_dict = split_dict(word_array[ii].text, word_array[ii+2].text);
      int jj;
      fprintf(stderr, "\n");      
      fprintf(stderr,"SPLITED: w1=%s: w2=%s: is_dict=%4.2f:\n", word_array[ii].text, word_array[ii+2].text, is_dict);
      if (is_dict > 0.9) {
	char concat[200];
	sprintf(concat,"%s%s", word_array[ii].text, word_array[ii+2].text);
	word_array[ii].text = strdup(concat);
	word_array[ii+1].text = NULL;
	word_array[ii+2].text = NULL;	
	word_array[ii].essential = ESSENTIAL +30;
	for (jj = 0; jj < 6; ii++, jj++) {
	  fprintf(stderr,"   SPLIT:%d:  split=%d:  space=%d: text=%s:\n", ii, word_array[ii].split_token, word_array[ii].has_space_before, word_array[ii].text);
	}
	fprintf(stderr, "\n");
      }
    }
  }
  return 0;
} // fix_split_token()


int ppp_gap_cmp(const void *a, const void *b) { // gap_overlap_item
  int x1 = (((struct Gap_Struct *)a)->size);
  int y1 = (((struct Gap_Struct *)b)->size);
  return(x1<y1);
}

int sort_gap_array_by_size(int pp) {
  qsort((void *)&gap_array[pp][0], gap_no[pp], sizeof(struct Gap_Struct), ppp_gap_cmp);
  return 0;
}


int ppp_block_by_renumbered(const void *a, const void *b) { // block
  int x1 = (((struct Obj_Struct *)a)->renumbered);
  int y1 = (((struct Obj_Struct *)b)->renumbered);
  return((x1>y1) ? 1 :0 );
}

int sort_blocks_by_renumbered(struct Obj_Struct block_array[], int last_block_no) {
  qsort((void *)&block_array[0], last_block_no+1, sizeof(struct Obj_Struct), ppp_block_by_renumbered);
  return 0;
}


int ppp_block_by_page_and_renumbered(const void *a, const void *b) { // block
  int r1 = (((struct Obj_Struct *)a)->renumbered);
  int R1 = (((struct Obj_Struct *)b)->renumbered);
  int p1 = (((struct Obj_Struct *)a)->page);
  int P1 = (((struct Obj_Struct *)b)->page);
  return((p1>P1) ? 1
	 : ((p1 == P1)
	    ? ((r1>R1)
	       ? 1 : 0)
	    : 0));
}

int sort_blocks_by_page_and_renumbered(struct Obj_Struct block_array[], int last_block_no) {
  qsort((void *)&block_array[0], last_block_no+1, sizeof(struct Obj_Struct), ppp_block_by_page_and_renumbered);
  return 0;
}

void remove_last_comma(char text[]) {
  int ii, found;
  for (found = 0, ii = strlen(text); found == 0 && ii > 0; ii--) {
    if (text[ii] == ',') {
      text[ii] = '\0'; found = 1;
    } else if (isalnum(text[ii]) > 0) {
      found = 1;
    } else {
      ;
    }
  }
  return;
}


struct Page_Dimensions { // dimension in pixels
  int xx, yy;
  float xx_factor, yy_factor;
} page_dimensions_array[MAX_PAGE]; // indexed by page_no


int read_page_dimensions(int doc_id) {
  char query[5000];
  sprintf(query, "select \n\
                     page, xx, yy \n\
                     from deals_ocrpagesizeinpixel \n\
                     where (doc_id = %d) \n\
                          order by page asc "
	  ,doc_id);

  if (1 && debug) fprintf(stderr,"QUERY73 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY73 (%s): query=%s:\n",prog,query);
  }
  sql_res = mysql_store_result(conn);
  int nn = mysql_num_rows(sql_res);
  int pp;
  while ((sql_row = mysql_fetch_row(sql_res) )) {
    if (sql_row[0]) {
      pp = atoi(sql_row[0]);
      int xx = page_dimensions_array[pp].xx = (sql_row[1]) ? atoi(sql_row[1]) : -1;
      int yy = page_dimensions_array[pp].yy = (sql_row[2]) ? atoi(sql_row[2]) : -1;

      page_dimensions_array[pp].xx_factor = (float)((float)xx / (float)10000);            
      page_dimensions_array[pp].yy_factor = (float)((float)yy / (float)10000);            
    }
  }
  fprintf(stderr,"Found %d page dimensions:\n", pp);
  return pp+1;
}




// used to be 30 -- memory problems
#define PAGE_BATCH_SIZE 300
int read_ocrtoken_array(int doc_id, int *last_page_no, int *last_block_no, int *last_line_no) {
  int ret;
  char query[5000];
  int nn = 1;
  int my_line = -1;
  while (nn > 0) { // split the query to multiple runs as I ran into memory limitation with SQL_RES on UDEV2
    static int low_page = 0;
    int high_page = low_page + PAGE_BATCH_SIZE;    
    fprintf(stderr,"MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM:lh=%d:%d:  nn=%d:\n", low_page, high_page,  nn);
    if (1 && debug) fprintf(stderr,"***************************  I. QUERY OCRBLOCK and OCRTOKEN:%d:\n", doc_id);
    sprintf(query, "select tok.text, tok.my_x1, tok.my_x2, tok.my_y1, tok.my_y2, tok.sn_in_doc, tok.sn_in_page, tok.sn_in_block \n\
                        , tok.block_in_doc, tok.block_in_page, tok.doc_id, tok.source_program, tok.page \n\
                        , tok.block_in_doc, block.block_in_doc \n\
                        , block.my_line_in_doc \n\
                        , block.my_x1, block.my_x2, block.my_y1, block.my_y2 \n\
                        , block.text \n\
                        , block.id, tok.id \n\
                        , tok.my_line_in_page \n\
                        , tok.has_space_before, tok.inserted_after_sn_in_doc, tok.split_token \n\
                     from deals_ocrtoken as tok \n\
                     left join deals_ocrblock as block on (block.block_in_doc = tok.block_in_doc) \n\
                     where \n\
                        tok.doc_id = %d and tok.source_program = '%s' and block.doc_id = %d and block.source_program = '%s'\n\
                        and tok.page <= %d and block.page <= %d \n\
                        and tok.page >%d and block.page >%d "
	    ,doc_id, "AWS",doc_id, "AWS", high_page, high_page, low_page, low_page);

    if (1 && debug) fprintf(stderr,"QUERY80 (%s): query=%s:\n",prog,query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY80 (%s): query=%s:\n",prog,query);
    }
    sql_res = mysql_store_result(conn);
    static int ii = 0;
    nn = mysql_num_rows(sql_res);
    static int last_bk = -1, bk = 0, last_pn = 0, pn = 0;
    int prev_my_line = 0;
    if (nn <= 0)  {
      fprintf(stderr,"DDDDDDDDDDDD0:ii=%d: nn=%d:\n",ii, nn);
    } else {
      fprintf(stderr,"DDDDDDDDDDDD1:ii=%d: nn=%d:\n",ii, nn);      
      if (1 && debug) fprintf(stderr,"***************************  II. READING AWS TOKENS:%d:\n",nn);
      int essential = 6400;

      // *******************next line causing segfault*************88

      int prev_line_in_page = -1;
      int last_sn_in_line = -1;
      while ( (sql_row = mysql_fetch_row(sql_res) )) {
	word_array[ii].obj_id = atoi(sql_row[22]);
	word_array[ii].text = strdup(sql_row[0]);

	word_array[ii].my_x1 = atoi(sql_row[1]);
	word_array[ii].my_x2 = atoi(sql_row[2]);    
	word_array[ii].my_y1 = atoi(sql_row[3]);
	word_array[ii].my_y2 = atoi(sql_row[4]);    
	last_pn = pn;
	int pn1 = word_array[ii].page = atoi(sql_row[12]);
	pn = MAX(last_pn, pn1);
	if (pn > MAX_PAGE - 1) {
	  fprintf(stderr,"ERROR page number :%d: exceeded MAX_PAGE :%d: e=%d:\n", pn, MAX_PAGE, word_array[ii].essential);
	  exit(13);
	}
	word_array[ii].sn_in_doc = atoi(sql_row[5]);
	word_array[ii].sn_in_page = atoi(sql_row[6]);    
	int sn_in_block = word_array[ii].sn_in_block = atoi(sql_row[7]);

	last_bk = bk;
	int bk1 = word_array[ii].block_in_doc = atoi(sql_row[8]);
	bk = MAX(bk, bk1);
	word_array[ii].block_in_page = atoi(sql_row[9]);
    
	word_array[ii].doc_id = atoi(sql_row[10]);
	word_array[ii].source_program = strdup(sql_row[11]);
	word_array[ii].page = atoi(sql_row[12]); 
	word_array[ii].block_in_doc = atoi(sql_row[13]);
	block_array[bk].block_in_doc = atoi(sql_row[14]);
	block_array[bk].page = pn;
	my_line = word_array[ii].my_line_in_doc = block_array[bk].my_line_in_doc = atoi(sql_row[15]);
	word_array[ii].my_line_in_page = block_array[bk].my_line_in_page = atoi(sql_row[23]);
	if (word_array[ii].my_line_in_page != prev_line_in_page) { // UZ 082521
	  word_array[ii].sn_in_line = 1;
	  last_sn_in_line = 1;
	  line_array[my_line].first_word = ii;
	} else {
	  word_array[ii].sn_in_line = ++last_sn_in_line;
	}
	prev_line_in_page = word_array[ii].my_line_in_page;
	word_array[ii].has_space_before = atoi(sql_row[24]);        
	int inserted_after = atoi(sql_row[25]);
	word_array[ii].split_token = atoi(sql_row[26]);    
	if (sn_in_block == 0 && inserted_after == 0) {
	  //page_block_word_array[pn][bk].first_word = ii;
	  block_array[bk].first_word = ii;
	  block_array[bk].my_x1 = word_array[ii].my_x1;
	  block_array[bk].my_y1 = word_array[ii].my_y1;
	  block_array[bk].textf = strdup(word_array[ii].text);            
	  if (last_pn > 0 && last_bk > -1) {
	    //page_block_word_array[last_pn][last_bk].last_word = ii;
	    block_array[last_bk].last_word = ii-1;
	    block_array[last_bk].my_x2 = word_array[ii-1].my_x2;
	    block_array[last_bk].my_y2 = word_array[ii-1].my_y2;
	    block_array[last_bk].textl = strdup(word_array[ii-1].text);		
	  }
	}

	block_array[bk].my_x1 = atoi(sql_row[16]);
	block_array[bk].my_x2 = atoi(sql_row[17]);               
	block_array[bk].my_y1 = atoi(sql_row[18]);
	block_array[bk].my_y2 = atoi(sql_row[19]);               
	block_array[bk].text = strdup(sql_row[20]);               
	block_array[bk].obj_id = atoi(sql_row[21]);

	if (last_pn != pn) {

	  page_properties_array[last_pn].last_line_in_page = prev_my_line;
	  page_properties_array[pn].first_line_in_page = my_line;      

	  page_properties_array[pn].first_block_in_page = bk;
	  if (pn > 0) page_properties_array[last_pn].last_block_in_page = bk-1;
	  page_properties_array[last_pn].no_of_blocks_in_page = page_properties_array[last_pn].last_block_in_page - page_properties_array[last_pn].first_block_in_page;

	}

	if (my_line != prev_my_line) {
	  line_property_array[my_line].first_block_in_line = bk;
	  line_property_array[prev_my_line].last_block_in_line = bk-1;
	  prev_my_line = MAX(my_line, prev_my_line);
	}

	if (0)  fprintf(stderr,"JJLLL10:   ww=%d: lpn=%d: ml=%d:%d: bk=%d:%d: text=%s: pn=%d: :%s: snib=%d:  bkf=%d:%s e=%d:\n"
		       , ii, last_pn, my_line, prev_my_line, bk, bk1, word_array[ii].text, pn, sql_row[0]
		       , sn_in_block
		       , block_array[bk].first_word, word_array[block_array[bk].first_word].text, essential);

	if (ii < MAX_WORD -2) {
	  ii++;
	} else {
	  fprintf(stderr,"ERROR: no of words exceeded :%d:\n", MAX_WORD);
	  exit(43);
	}
      } // while ii

      block_array[last_bk].essential = essential;
      fprintf(stderr,"finished processing sql results\n");
      //page_block_word_array[last_pn][last_bk].last_word = ii;
      block_array[last_bk].last_word = ii-1;  
      block_array[last_bk].my_x2 = word_array[ii-1].my_x2;
      block_array[last_bk].my_y2 = word_array[ii-1].my_y2;
      block_array[last_bk].textl = strdup(word_array[ii-1].text);		

      line_property_array[prev_my_line].last_block_in_line = bk-1;
      page_properties_array[last_pn].last_line_in_page = prev_my_line;
      page_properties_array[last_pn].last_block_in_page = last_bk;
      page_properties_array[last_pn].no_of_blocks_in_page = page_properties_array[last_pn].last_block_in_page - page_properties_array[last_pn].first_block_in_page;

      if (debug) fprintf(stderr,"Returning array with aws tokens=%d:; blocks=%d:; pages=%d: ppa[%d]=%d:\n",ii, bk, pn, last_pn, prev_my_line);
      *last_page_no = pn+1;
      *last_block_no = bk+1;
      *last_line_no = my_line+1;

      fprintf(stderr,"   MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM:lh=%d:%d:  ii=%d:  nn=%d:\n", low_page, high_page, ii , nn);
      low_page+=PAGE_BATCH_SIZE;
      ret = ii;
    } // while
  } // nn > 0
  return ret;
} // read_ocrtoken_array() 


int print_block_array(int nn) {
  if (debug) fprintf(stderr,"***************************  PRINTING YBLOCKS:%d: lbn=%d:\n",nn , last_block_no);

  int ll;
  int last_page = -1;
  for (ll = 0; ll <= last_block_no ; ll++) {
    int fw = 0; //page_block_word_array[pp][ll].first_word;
    int fw1 = block_array[ll].first_word;
    int lw = 0;//page_block_word_array[pp][ll].last_word;      
    int lw1 = block_array[ll].last_word;
    int page = block_array[ll].page;
    if (page > last_page) {
      fprintf(stderr,"\n   PAGE %d:%d:\n",page, page_properties_array[page].last_block_in_page - page_properties_array[page].first_block_in_page +1);
    }
    if (block_array[ll].renumbered < 0) fprintf(stderr,"       ----");
    fprintf(stderr,"          BLOCK:%d: ll=%4d:%4d: pg=%2d: pr=%4d: h/f=%2d: my_ln=%4d: flw=%4d:%4d:  y=%4d:%4d: x=%4d:%4d: :%s:%s:\n"
	    , nn , ll
	    , block_array[ll].renumbered

	    , page
	    , block_array[ll].para	      
	    , block_array[ll].is_header_footer

	    , block_array[ll].my_line_in_doc

	    , fw1, lw1

	    , block_array[ll].my_y1
	    , block_array[ll].my_y2

	    , block_array[ll].my_x1
	    , block_array[ll].my_x2

	    , block_array[ll].textf
	    , block_array[ll].textl
	    );
    last_page = page;
  }
   return 0;
} // print_block_array(int nn) {



int print_word_array(int nn, int total_renumbered_words) {
  if (debug) fprintf(stderr,"\n\n***************************  PRINTING XWORDS:%d: total=%d:\n",nn, total_renumbered_words);  

  int ll;
  int last_page = -1;
  for (ll = 0; ll <= total_renumbered_words; ll++) {
    int page = word_array[ll].page;
    if (page > last_page) {
      fprintf(stderr,"\n   PAGE %d:\n",page);
    }
    fprintf(stderr,"          WORD:%d: sn=%d: ll=%4d:%4d: pg=%2d: para=%4d: ln=%4d:!!:%4d: split=%d: space=%d: text=%s: sn_in_line=%d:\n"
	    , nn
	    , word_array[ll].sn_in_doc
	    , ll, word_array[ll].renumbered

	    , page, word_array[ll].para	      
	    , word_array[ll].my_line_in_doc
            , word_array[ll].my_line_in_page_renumbered
	    
	    , word_array[ll].split_token, word_array[ll].has_space_before

	    , word_array[ll].text
	    , word_array[ll].sn_in_line	    
	    );
    last_page = page;
  }
  if (debug) fprintf(stderr,"***************************  PRINTED XWORDS:%d: total=%d:\n",nn, total_renumbered_words);     
  return 0;
} // print_word_array(int nn) {

int print_rf_word_array(int nn, int total_renumbered_words) {
  if (debug) fprintf(stderr,"***************************  PRINTING YWORDS:%d: total=%d:\n",nn, total_renumbered_words);  

  int ll;
  int last_page = -1;
  for (ll = 0; ll <= total_renumbered_words; ll++) {
    int page = word_array[ll].page;
    if (page > last_page) {
      fprintf(stderr,"\n   PAGE %d:\n",page);
    }
    fprintf(stderr,"          WORD:%d: ll=%4d:%4d: pg=%2d: para=%4d: ln=%4d: split=%d: space=%d: text=%s:\n"
	    , nn
	    , ll, rf_word_array[ll].renumbered

	    , page, rf_word_array[ll].para	      
	    , rf_word_array[ll].my_line_in_doc

	    , rf_word_array[ll].split_token, rf_word_array[ll].has_space_before

	    , rf_word_array[ll].text
	    );
    last_page = page;
  }
  if (debug) fprintf(stderr,"***************************  PRINTED YWORDS:%d: total=%d:\n\n",nn, total_renumbered_words);  
  return 0;
} // print_rf_word_array(int nn) {



int create_line2block_index() {
  int bb;
  for (bb = 0; bb <= last_block_no; bb++) {
    int line_no = block_array[bb].my_line_in_doc;
    line2block_index[line_no][line2block_no[line_no]++] = bb;
    last_line_no = MAX(block_array[bb].my_line_in_doc, last_line_no);
  }
  return 0;
}

int get_params(int argc, char **argv) {
  int get_opt_index;
  int c_getopt;
  prog = argv[0];
  opterr = 0;
  debug = 0;
  SHOW_PAGE = 0; 
  DELTA_VERT_FOR_PARA  = 70;  

  while ((c_getopt = getopt (argc, argv, "d:P:N:U:W:D:s:X:Y:Z:n:p:2:3:x:y:v:T:M:c:")) != -1) {
    fprintf(stderr,"GO: %c:%s:\n",c_getopt, optarg);
    switch (c_getopt) {
    case 'd':
      doc_id = atoi(optarg);
      break;
    case 'D':
      debug = atoi(optarg);
      break;
    case 'P':
      db_IP = strdup(optarg);
      break;
    case 'N':
      db_name = strdup(optarg);
      break;
    case 'U':
      db_user_name = strdup(optarg);
      break;
    case 'W':
      db_pwd = strdup(optarg);
      break;
    case 's':
      source_prog = strdup(optarg);
      break;
    case 'p':
      SHOW_PAGE = atoi(optarg);
      break;
    case '2':
      displayfile_name = strdup(optarg);
      break;
    case '3':
      spdffile_name = strdup(optarg);
      fprintf(stderr,"SPDFFILE=%s:\n",spdffile_name);
      spdffile_name = ":bbb";
      break;
    case 'M':
      media = strdup(optarg);

      break;
    case 'x': // the BACK abbyy factor
      X_back_to_aby_factor = atof(optarg);
      break;
    case 'y': // the BACK abbyy factor
      Y_back_to_aby_factor = atof(optarg);
      break;
    case 'v': // the BACK abbyy factor
      DELTA_VERT_FOR_PARA = atoi(optarg);
      break;
    case 'T': // the BACK abbyy factor
      dict_path = strdup(optarg);
      break;
    case 'c': // the BACK abbyy factor
      doc_country = strdup(optarg);
      break;

    } //while
  }

  fprintf (stderr,"%s took: doc_id = %d, debug=%d db_IP =%s, db_name =%s, db_user_name =%s, db_pwd=%s: displayfile=%s: spdffile=%s: media=%s:\n",
	   argv[0], doc_id, debug, db_IP, db_name, db_user_name, db_pwd, displayfile_name, spdffile_name, media);

  for (get_opt_index = optind; get_opt_index < argc; get_opt_index++) {
    printf ("Non-option argument %s\n", argv[get_opt_index]);
  }

  conn = mysql_init(NULL);

  /* now connect to database */
  if (!mysql_real_connect(conn,db_IP,db_user_name,db_pwd,db_name,0,NULL,0)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }
  return 0;
}

int ppp(int nn) {
  fprintf(stderr,"      GGGGG%d:%d:\n", nn, word_array[0].renumbered);
  return 0;
}

/* traverse and RENUMBER blocks according to the folding at the top of the group */
int traverse_words_double_column(int last_block_no) {
  fprintf(stderr,"**************************** XII. TRAVERSE WORDS (double column) \n");
  int ww;
  for (ww = 0; ww < word_count; ww++) { // initialize
    word_array[ww].renumbered = -3;
  }
  
  int bb;
  // go over all the ignore blocks, footers/headers/enumerated
  for (bb = 0; block_array[bb].renumbered < 0 && bb <= last_block_no; bb++) {
    fprintf(stderr,"ZZZ0: bb=%d: l=%d: ren=%d: text=%s:\n",bb, last_block_no, block_array[bb].renumbered, block_array[bb].text);
  }
  fprintf(stderr,"ZZZ1: bb=%d: l=%d: ren=%d: text=%s:\n",bb, last_block_no, block_array[bb].renumbered, block_array[bb].text);  
  int first_real_bb = bb;
  int renumbered_ww = 0;
  // now go over real blocks
  fprintf(stderr,"LAST_BLOCK_NUMBER: first=%d: %d:\n",first_real_bb, last_block_no);
  int nn = 0;
  for (bb = first_real_bb; bb <= last_block_no; bb++) {
    //if (renumbered_ww > 760 && renumbered_ww < 780) fprintf(stderr,"   YYY:%d:\n",bb);
    int first_word = block_array[bb].first_word;
    int last_word = block_array[bb].last_word;    
    int prev_ww = 0;
    fprintf(stderr,"XBLOCK: bb=%d: t=%s: f=%d:%s: l=%d:%s:\n",bb, block_array[bb].text, first_word, word_array[first_word].text,  last_word, word_array[last_word].text);
    if (block_array[bb].text) {
      for (ww = first_word; ww <= last_word; ww++) {
	word_array[ww].renumbered = renumbered_ww++;
	word_array[ww].my_line_in_page_renumbered = block_array[bb].my_line_in_page_renumbered;
	fprintf(stderr,"      DDD:UD2: bb=%d: ww=%d: renumbered=%d:!!:%d:  text=%s:\n", bb, ww, word_array[ww].renumbered, word_array[ww].my_line_in_page_renumbered, word_array[ww].text);
	prev_ww = ww;
      }
      nn++;
    } 
  }
  fprintf(stderr,"TOTAL RENUMBERED WORDS=%d:\n",renumbered_ww);
  return renumbered_ww;
} // traverse_words_double_column()

int print_first_last_line_in_page_array(int last_page_no) {
  int pp;
  for (pp = 0; pp <+ last_page_no; pp++) {
    fprintf(stderr,"FLP:%3d: :%3d: :%3d:\n",
	    pp, page_properties_array[pp].first_line_in_page, page_properties_array[pp].last_line_in_page);
  }

  return 25;
}
struct Double_Col_Rep {
  int no_of_reordered_words;
  int no_of_left_lines;
  int no_of_right_lines;    
} double_column_report_array[MAX_PAGE];

#define POINT_THRESHOLD 35
/* traverse and RENUMBER blocks according to the folding at the top of the group */
int traverse_double_column_blocks_in_groups(int last_page_no) {
  int pp;
  fprintf(stderr,"**************************** XII. TRAVERSE (double column) \n");
  int bb;
  for (bb = 0; bb <= last_block_no; bb++) { // initialize
    block_array[bb].renumbered = -3;
  }
  
  int last_renumbered_block = 0;
  for (pp = 1; pp <= last_page_no; pp++) {    
    // don't do pages that are TOC
    float toc_ratio = (float)((float)(page_properties_array[pp].no_toc_items) / (float)(page_properties_array[pp].last_line_in_page - page_properties_array[pp].first_line_in_page +1));
    { //     if (toc_ratio < MAX_TOC_RATIO) {
      int first_line_in_page = (page_properties_array[pp-1].last_line_in_page == 0) ? page_properties_array[pp].first_line_in_page : page_properties_array[pp-1].last_line_in_page+1;
      int ll = (pp > 1) ? first_line_in_page : 0; //page_properties_array[pp].first_line_in_page;
      //int ll = (pp > 1) ? page_properties_array[pp-1].last_line_in_page+1 : 0; //page_properties_array[pp].first_line_in_page;
      int prev_ll = -1;
      fprintf(stderr,"   TRAVERSE B PAGE: pp=%d: first_line=%d:%d: last_line=%d: last_renumbered_block=%d toc_ratio=%4.2f no:%d:%d: \n"
	      , pp, ll, page_properties_array[pp].first_line_in_page, page_properties_array[pp].last_line_in_page, last_renumbered_block
	      , toc_ratio, page_properties_array[pp].no_toc_items, page_properties_array[pp].last_line_in_page - page_properties_array[pp].first_line_in_page +1);
      int dc_line = -1; // double-column line
      while (ll > prev_ll // to prevent infinite loop
	     && ll <= page_properties_array[pp].last_line_in_page) {
	prev_ll = ll;
	fprintf(stderr,"  LINE IN PAGE:  ll=%d: last=%d: group=%d: group/page_ratio=%4.2f:%4.2f:\n"
		,  ll, page_properties_array[pp].last_line_in_page
		,      line_property_array[ll].belongs_in_group, line_group_array[pp][line_property_array[ll].belongs_in_group].line_ratio, MIN_DC_LINE_RATIO);


	// regular lines
	int last_ll = ll;
	fprintf(stderr, "\n");	
	for (ll = last_ll
	       ; (ll <= page_properties_array[pp].last_line_in_page
		  && (line_property_array[ll].belongs_in_group == -2
		      || (line_property_array[ll].belongs_in_group >= 0
			  && line_group_array[pp][line_property_array[ll].belongs_in_group].line_ratio < MIN_DC_LINE_RATIO)))
		 
	       ; ll++) {
	  dc_line++;
	  int ii;
	  int fbol = line_property_array[ll].first_block_in_line;
	  int lbol = line_property_array[ll].last_block_in_line;
	  if (lbol < line_property_array[ll+1].first_block_in_line-1 || lbol-fbol > POINT_THRESHOLD) lbol = fbol; // protection 
	  if (lbol < fbol)  lbol = fbol;  // protection 
	  fprintf(stderr,"ll=%3d:   REGULAR LINE  gr=%d: flbol=%d:%d: tfl=%s:%s:\n"
		  ,ll, line_property_array[ll].belongs_in_group
		  , fbol
		  , lbol
		  ,  ((fbol>=0) ? block_array[fbol].textf : "_") ,          ((lbol) ? block_array[lbol].textl : "_"));
	  if ((fbol > 0 || ll == 0)) { // what is this condition trying to prevent?
	    for (ii = fbol; ii <= lbol; ii++) {
	      int bb = ii; // line2block_index[ll][ii];
	      if (block_array[bb].is_header_footer == 0) { // keep header/footer unnumbered
		block_array[bb].renumbered = last_renumbered_block++;
	      } else {
		block_array[bb].renumbered = -1 * block_array[bb].is_header_footer;
	      }		
	      block_array[bb].my_line_in_page_renumbered = dc_line;
	      fprintf(stderr,"     @ ii=%d: bb=%d: ren=%d: !!line=%d: ft=%s: 0:%d:",ii,bb,block_array[bb].renumbered, block_array[bb].my_line_in_page_renumbered,  block_array[bb].textf, block_array[0].renumbered);
	    } // ii -- block in line
	  }
	  fprintf(stderr,"\n");
	} // for ll -- line in doc

	fprintf(stderr,"     OI0 ii=%d: bb=%d: ren=%d: !!line=%d: ft=%s: :%s:",3, 3 ,block_array[3].renumbered, block_array[3].my_line_in_page_renumbered,  block_array[3].textf, block_array[3].textl);

	// LHS
	int belongs_in_group = line_property_array[ll].belongs_in_group;    //  now we are in a group
	int lhs_rhs = 0;
	int first_group_ll = ll;
	for (ll = first_group_ll; ll <= line_group_array[pp][belongs_in_group].last_line; ll++) {
	  //if (line2block_l_of_gap[ll] != -1) { // don't grab when left does not exist
	  if (line2block_l_of_gap[ll] > 0) { // don't grab when left does not exist	    
	    dc_line++;
	    int ii;
	    int last_block_on_line = (line2block_l_of_gap[ll] == -1  && ll < page_properties_array[pp].last_line_in_page) ? line_property_array[ll+1].first_block_in_line-1 : line2block_l_of_gap[ll]; //if there's no RHS then first block on next line
	    fprintf(stderr,"ll=%d:   LEFT LINE  PP=%d: gr=%d: f=%d:    b_l%d:  last=%d: fl=%s:%s:\n"
		    , ll, pp, line_property_array[ll].belongs_in_group, line_property_array[ll].first_block_in_line
		    , line2block_l_of_gap[ll],   last_block_on_line, block_array[last_block_on_line].textf, block_array[last_block_on_line].textl);	
	    double_column_report_array[pp].no_of_left_lines++;
	    int fbol = line_property_array[ll].first_block_in_line;
	    if ((fbol > 0 || ll == 0)) {
	      for (ii = fbol; ii <= /* line2block_l_of_gap[ll]*/ last_block_on_line; ii++) {
		int bb = ii;//line2block_index[ll][ii];
		if (block_array[bb].is_header_footer == 0) {
		  block_array[bb].renumbered = last_renumbered_block++;
		} else {
		  block_array[bb].renumbered = -1 * block_array[bb].is_header_footer;
		}		
		block_array[bb].my_line_in_page_renumbered = dc_line;
		fprintf(stderr,"   %d:%d:%d:%d: ",ii,bb,block_array[bb].renumbered, block_array[bb].my_line_in_page_renumbered);	  
		double_column_report_array[pp].no_of_reordered_words += (block_array[bb].last_word - block_array[bb].first_word +1);
	      }
	    }
	    fprintf(stderr,"\n");	
	  }
	} // we finished the LHS of the group

	fprintf(stderr,"     OI1 ii=%d: bb=%d: ren=%d: !!line=%d: ft=%s: :%s:",3, 3 ,block_array[3].renumbered, block_array[3].my_line_in_page_renumbered,  block_array[3].textf, block_array[3].textl);
	// RHS
	fprintf(stderr,"\n");		
	for (ll = first_group_ll; ll <= line_group_array[pp][belongs_in_group].last_line; ll++) {
	  //if (line2block_r_of_gap[ll] != -2) { // don't grab when left does not exist
	  if (line2block_r_of_gap[ll] > 0) { // don't grab when left does not exist	    
	    dc_line++;
	    int ii;
	    int first_block_on_line = (line2block_r_of_gap[ll] < 0 && ll > 0) ? line_property_array[ll-1].last_block_in_line+1 : line2block_r_of_gap[ll]; //if there's no LHS then last block on prev line
	    fprintf(stderr,"ll=%d:    RIGHT LINE  PP=%d  gr=%d: fl=%d:%d: fl=%s:%s:\n"
		    ,ll, pp, line_property_array[ll].belongs_in_group,  line2block_r_of_gap[ll],  line_property_array[ll].last_block_in_line, block_array[first_block_on_line].textf, block_array[first_block_on_line].textl);		
	    double_column_report_array[pp].no_of_right_lines++;
	    if ((first_block_on_line > 0 || ll == 0)) {	    
	      for (ii = first_block_on_line; ii <= line_property_array[ll].last_block_in_line; ii++) {
		int bb = ii; //line2block_index[ll][ii];
		if (block_array[bb].is_header_footer == 0) {
		  block_array[bb].renumbered = last_renumbered_block++;
		} else {
		  block_array[bb].renumbered = -1 * block_array[bb].is_header_footer;
		}		
		block_array[bb].my_line_in_page_renumbered = dc_line;
		fprintf(stderr,"   %d:%d:%d:%d: ",ii,bb,block_array[bb].renumbered, block_array[bb].my_line_in_page_renumbered);
		double_column_report_array[pp].no_of_reordered_words += (block_array[bb].last_word - block_array[bb].first_word +1);
	      }
	    }
	    fprintf(stderr,"\n");	
	  } // we finished the RHS of the group
	}
	fprintf(stderr,"     OI2 ii=%d: bb=%d: ren=%d: !!line=%d: ft=%s: :%s:",3, 3 ,block_array[3].renumbered, block_array[3].my_line_in_page_renumbered,  block_array[3].textf, block_array[3].textl);
      } // while finished the lines in the page
    }
    fprintf(stderr,"     OI3 ii=%d: bb=%d: ren=%d: !!line=%d: ft=%s: :%s:",3, 3 ,block_array[3].renumbered, block_array[3].my_line_in_page_renumbered,  block_array[3].textf, block_array[3].textl);
    fprintf(stderr,"\n\n");		    
  } // pp - pages
  fprintf(stderr,"     OI4 ii=%d: bb=%d: ren=%d: !!line=%d: ft=%s: :%s:",3, 3 ,block_array[3].renumbered, block_array[3].my_line_in_page_renumbered,  block_array[3].textf, block_array[3].textl);
  return 0;
} // traverse_double_column_blocks_in_groups() 




// update the entire set of blocks at once
int update_block_reorder_sql_at_once(int last_page_no) {
  char query[500000];
  char buff[500];  
  strcpy(query,"update deals_ocrblock set reorder_in_doc = (case id \n ");
  /*case id when 344807 then -7 when 344808 then -8 when 344809 then -9 \n\ end) where id in (344807, 344808, 344809) \n"); */
  int ii, mm;
  for (ii = 0, mm = 0; ii <= last_block_no; ii++) {
    if (1 || block_array[ii].page == 3) {
      sprintf(buff,"when %d then %d \n ", block_array[ii].obj_id, block_array[ii].renumbered);
      strcat(query,buff);
      mm++;
    }
  }
  strcat(query," end)\n " );

  if (debug) fprintf(stderr,"QUERY870 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY870 (%s): query=%s:\n",prog,query);
  }
  int affected = mysql_affected_rows(conn);
  fprintf(stderr,"AFFECTED REORDER ii=%d:%d: aff=%d: lbn=%d:\n",ii, mm, affected, last_block_no);
  return 0;
} // update_block_reorder_sql_at_once

// we do the update in batches of 20 pages so not to blow up the max lackage allowed by AWS
#define PAGE_BATCH 20
int update_block_reorder_sql_by_page(int last_page_no) {
  int pp;
  int qq = 0;
  for (pp = 0; pp <= last_page_no; pp+=PAGE_BATCH) {
    int fbk = page_properties_array[pp].first_block_in_page;
    int last_pp = MIN((pp+PAGE_BATCH-1), last_page_no);
    int lbk = page_properties_array[last_pp].last_block_in_page;
    fprintf(stderr,"ROUND OF: pp=%d:%d: (lpn=%d:)  qq=%d: flbk=%d:%d\n", pp, last_pp, last_page_no, qq++,  fbk,lbk);
    char query[500000];
    char buff[500];  
    strcpy(query,"update deals_ocrblock set reorder_in_doc = (case id \n ");
    /*case id when 344807 then -7 when 344808 then -8 when 344809 then -9 \n\ end) where id in (344807, 344808, 344809) \n"); */
    int ii, mm;
    for (ii = fbk, mm = 0; ii <= lbk; ii++) {
      sprintf(buff,"when %d then %d \n ", block_array[ii].obj_id, block_array[ii].renumbered);
      strcat(query,buff);
      //fprintf(stderr,"KK:%d:%s:QQ:%s:\n",mm,buff,query);
      mm++;
    }
    strcat(query," end)\n " );
    sprintf(buff, "     where doc_id = %d and page >=%d and page < %d ", doc_id, pp, pp+PAGE_BATCH);
    strcat(query, buff);    

    if (debug) fprintf(stderr,"QUERY871 (%s): query=%s:\n",prog,query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY871 (%s): query=%s:\n",prog,query);
    }
    int affected = mysql_affected_rows(conn);
    fprintf(stderr,"AFFECTED REORDER ii=%d:%d: aff=%d: lbn=%d:\n",ii, mm, affected, last_block_no);
  } // pp
  return 0;
} // update_block_reorder_sql_by_page()



// update the entire set of blocks at once
int update_word_reorder_sql_at_once(int last_page_no, int total_renumbered_words) {
  char query[500000];
  char buff[500];  
  strcpy(query,"update deals_ocrtoken set reorder_in_doc = (case id \n ");
  /*case id when 344807 then -7 when 344808 then -8 when 344809 then -9 \n\ end) where id in (344807, 344808, 344809) \n"); */
  int ii, mm;
  for (ii = 0, mm = 0; ii < total_renumbered_words; ii++) {
    if (word_array[ii].page == 3) {
      sprintf(buff,"when %d then %d \n ", word_array[ii].obj_id, word_array[ii].renumbered);
      strcat(query,buff);
      mm++;
    }
  }
  strcat(query," end)\n " );

  if (debug) fprintf(stderr,"QUERY88 (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY88 (%s): query=%s:\n",prog,query);
  }
  int affected = mysql_affected_rows(conn);
  fprintf(stderr,"AFFECTED REORDER ii=%d:%d: aff=%d: trw=%d:\n",ii, mm, affected, total_renumbered_words);
  return 0;
} // update_word_reorder_sql_at_once


int update_word_reorder_sql_per_page(int last_page_no, int total_renumbered_words) {
  char query[500000];
  char buff[500];
  int pp;
  int mm = 0;
  int last_ii = -1;
  for (pp = 1; pp <= last_page_no; pp+=DELTA_PP) { // update in groups of DELTA_PP
    strcpy(query,"update deals_ocrtoken set reorder_in_doc = (case id \n ");
    /*case id when 344807 then -7 when 344808 then -8 when 344809 then -9 \n\ end) where id in (344807, 344808, 344809) \n"); */
    int ii;
    int nn;
    for (ii = last_ii+1, nn=0; ii < total_renumbered_words; ii++) {
      if (word_array[ii].page >= pp && word_array[ii].page < pp+DELTA_PP) {
	sprintf(buff,"when %d then %d \n ", word_array[ii].obj_id, word_array[ii].renumbered);
	strcat(query,buff);
	mm++;
	nn++;
	last_ii = ii;
      }
    }
    strcat(query," end)\n " );

    sprintf(buff, "     where doc_id = %d and page >=%d and page < %d ", doc_id, pp, pp+DELTA_PP);
    strcat(query, buff);    

    if (debug) fprintf(stderr,"QUERY88 pp=%d: (%s): query=%s:\n",pp,prog,query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY88 (%s): query=%s:\n",prog,query);
    }
    int affected = mysql_affected_rows(conn);
    fprintf(stderr,"AFFECTED WORD REORDER pp=%d: ii=%d: total_words_done=%d: done_in_this_batch=%d: affected=%d: trw=%d:\n", pp ,ii, mm, nn, affected, total_renumbered_words);
  }
  return 0;
} // update_word_reorder_sql_per_page


int insert_new_paras_into_sql(int last_para, int space_no, int doc_id, int org_id) {
  int pp;
  char query[1000000];

  /****************************************** paragraphtoken ************************************************/

  fprintf(stderr,"LLL0\n");
  sprintf(query,"delete from deals_paragraphtoken \n\
                 where doc_id = %d /*and source_program = 'aws' */"
	 , doc_id);

  
  if (debug) fprintf(stderr,"QUERY92: query=%s:\n", query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY92: query=%s: \n", query);
  }

  fprintf(stderr,"LLL1\n");
  sprintf(query,"insert into deals_paragraphtoken \n\
                (para_no, token_id, num_tokens, doc_id, organization_id, page_no, line_no, all_upper, renumbered_line_in_page, source_program) \n\
                values \n");
    
  for (pp = 0; pp <= last_para; pp++) {
    char buff[5000];
    fprintf(stderr,"QQQ0: pp=%d: lp=%d:\n", pp, last_para);
    sprintf(buff,"(%d, %d, %d, %d, %d, %d, %d, %d, %d, '%s'), \n"
	    , pp
	    , para_array[pp].token
	    , para_array[pp].num_tokens
	    , doc_id
	    , org_id
	    , para_array[pp].page
	    , para_array[pp].line	    
	    , para_len_array[pp].is_all_upper
	    , 0 // para_len_array[pp].	    
	    , "AWS"
	    );
    fprintf(stderr,"QQQ1: pp=%d: lp=%d: buff=%s:\n", pp, last_para, buff);
    strcat(query,buff);
  }
  remove_last_comma(query);    
  if (debug) fprintf(stderr,"QUERY93: query=%s:\n", query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY93: query=%s: \n", query);
  }

  /****************************************** paragraphlen ************************************************/

  sprintf(query,"delete from deals_paragraphlen \n\
           where doc_id = '%d' /* and source_program='aws'*/ "
	  , doc_id);
  if (debug) fprintf(stderr,"QUERY02=%s\n",query);
  if (mysql_query(conn, query)) {
      fprintf(stderr,"QUERY02=%s\n",mysql_error(conn));
      exit(1);
  }

  sprintf(query,"insert into deals_paragraphlen \n\
           (doc_id, organization_id, para_no, len, all_upper, source_program) \n\
           values "); 

  for (pp = 0; pp <= last_para; pp++) {
    int all_upper = is_all_upper(pp);
    char buff[500];
    sprintf(buff,"(%d, %d, %d, %d, %d, '%s'), \n"
	    , doc_id
	    , org_id
	    , pp
	    , para_array[pp].num_tokens
	    , all_upper
	    , "AWS"
	    );
    strcat(query,buff);
  }

  remove_last_comma(query);    
  if (debug) fprintf(stderr,"QUERY03: query=%s:\n", query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY03: query=%s: \n", query);
  }


  /****************************************** verticalspacetoken ************************************************/

  sprintf(query,"delete from deals_verticalspacetoken \n\
           where doc_id = '%d' and source_program='aws' "
	  , doc_id);
  if (debug) fprintf(stderr,"QUERY07=%s\n",query);
  if (mysql_query(conn, query)) {
      fprintf(stderr,"QUERY07=%s\n",mysql_error(conn));
      exit(1);
  }

  sprintf(query,"insert into deals_verticalspacetoken \n\
           (space_size, token_no, line_no, page_no, doc_id, source_program) \n\
           values "); 

  for (pp = 0; pp < space_no; pp++) {
    char buff[500];
    sprintf(buff,"(%d, %d, %d, %d, %d, '%s'), \n"
	    , space_array[pp].space_size
	    , space_array[pp].token_no
	    , space_array[pp].line_no
	    , space_array[pp].page_no 	    
	    , doc_id
	    , "AWS"
	    );
    strcat(query,buff);
  }

  remove_last_comma(query);    
  if (debug) fprintf(stderr,"QUERY08: query=%s:\n", query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY08: query=%s: \n", query);
  }


  /********************************* page2para *********************************************************/

  sprintf(query,"delete from deals_page2para \n\
                 where doc_id = %d and source_program = 'aws' "
	 , doc_id);
    
  if (debug) fprintf(stderr,"QUERY96: query=%s:\n", query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY96: query=%s: \n", query);
  }


  sprintf(query,"insert into deals_page2para \n\
                (para, doc_id, page_no, source_program) \n\
                values \n");
    
  int last_page = 0;
  for (pp = 0; pp <= last_para; pp++) {
    int page = para_array[pp].page;
    if (page > last_page) {
      char buff[500];
      sprintf(buff,"(%d, %d, %d, '%s'), \n"
	      , pp
	      , doc_id
	      , para_array[pp].page	    
	      , "AWS"
	      );
      strcat(query,buff);
    }
    last_page = page;
  }
  remove_last_comma(query);    
  if (debug) fprintf(stderr,"QUERY97: query=%s:\n", query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY97: query=%s: \n", query);
  }
  return 0;
} // insert_new_paras_into_sql(int last_para, int doc_id, int org_id) 



int select_from_regular_token_sql(int doc_id) {
  char query[5000];
  sprintf(query,"select sn, id from deals_tokenrelcoord \n\
                     where doc_id = %d and sn < 50 "
	    , doc_id);

    if (debug) fprintf(stderr,"QUERY77B : query=%s:\n",query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY77B: query=%s: (end QUERY77A)\n",query);
    }


    sql_res = mysql_store_result(conn);
    while ((sql_row = mysql_fetch_row(sql_res) )) {
      fprintf(stderr,"EEE: sn=%s: id=%s: \n", sql_row[0], sql_row[1]);
    }
    return 0;
} // select_from_regular_token_sql(int doc_id) 


int delete_from_regular_token_sql(int doc_id) {
  char query[5000];
  sprintf(query,"delete from deals_tokenrelcoord \n\
                     where doc_id = %d "
	    , doc_id);

    if (debug) fprintf(stderr,"QUERY77A : query=%s:\n",query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY77A: query=%s: (end QUERY77A)\n",query);
    }

  sprintf(query,"delete from deals_token \n\
                     where doc_id = \'%d\' /* and source_program='AWS' */ "
	    , doc_id);

    if (debug) fprintf(stderr,"QUERY77 : query=%s:\n",query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY77: query=%s: (end QUERY77)\n",query);
    }



    return 0;
} // delete_from_regular_token_sql(int doc_id) 


char *escape_quote_aws(char *text) {
  char *ret;
  if (text) {
    static char bext[500];
    int ii, jj;
    for (ii = 0, jj = 0; ii < strlen(text); ii++) {
      if (text[ii] == '\\' && (text[ii+1] == '\"' || text[ii+1] == '\'')) { // since aws already escapes quotes for me SOMETIMES
	bext[jj++] = text[ii];
	bext[jj++] = text[++ii];
      } else if (text[ii] == '\'' || text[ii] == '\"') {
	bext[jj++] = '\\';
	bext[jj++] = text[ii];
      } else if (text[ii] == '\\') {
	bext[jj++] = '\\';
	bext[jj++] = text[ii];
      } else if ((text+ii, "Â",1) == 0) {
	bext[jj++] = '\\';
	bext[jj++] = text[ii];
      } else if (text[ii] == '\n') {
	//bext[jj++] = ' ';
      } else {
	bext[jj++] = text[ii];
      }
    }
    bext[jj++] = '\0';
    ret = bext;
  } else {
    ret = "-";
  }
  return ret;
}


int is_real_word(int ww) {
  int is_real = 1;
  int len = 0;
  char *text = word_array[ww].text;
  if (!text) {
    is_real = 0;
  }
  else {
    int ii;

    for (ii = 0; ii <= strlen(word_array[ww].text); ii++) {
      if (isalpha(text[ii]) != 0)  {
	is_real = 0;
      }
      len++;
    }
  }
    
  return (is_real && len > 2);
}



int is_all_upper_word(int ww) {
  char *text = word_array[ww].text;
  int is_upper = 1;
  if (!text) {
    is_upper = 1;
  }
  else {
    int ii;

    for (ii = 0; ii <= strlen(word_array[ww].text); ii++) {
      if (isalpha(text[ii]) != 0 && islower(text[ii]) != 0) {
	is_upper = 0;
      }
    }
  }
    
  //fprintf(stderr,"ISU:%d:%d:  :%d:%d:  %s:\n",is_upper, text);
  return is_upper;
}


int is_first_upper_word(int ww) {
  char *text = word_array[ww].text;
  int is_upper = 1;
  if (!text) {
    is_upper = 1;
  }
  else {
    if (isalpha(text[0]) != 0 && islower(text[0]) != 0) {
      is_upper = 0;
    }
  }
    
  //fprintf(stderr,"ISU:%d:%d:  :%d:%d:  %s:\n",is_upper, text);
  return is_upper;
}

int insert_into_para_arrays(int doc_id, int word_count) {
  
  int pp; 
  for (pp = 0; pp < MAX_PARA; pp++) { // clear para_array
	para_array[pp].num_tokens = 0;
	para_array[pp].token = 0;		
  }


  int ii;
  int para;
  int prev_para = -1;
  int page;

  for (ii = 0; ii <= word_count; ii++) {
    char buff[5000];
    int ww = rf_word_array[ii].renumbered;
    if (ww >= 0 && rf_word_array[ii].page > 0) {
      page = rf_word_array[ii].page;
      para = rf_word_array[ii].para;
      if (para > prev_para) {
	para_len_array[para].is_all_upper = 1;
	para_len_array[para].page = page;
	para_len_array[para].is_all_first_upper = 0;
	para_len_array[para].first_token = ww;		
	para_len_array[para].num_tokens = 0;
	para_array[para].num_tokens = 0;
	para_array[para].token = ww;		
      }
      int is_all_upper = para_len_array[para].is_all_upper;
      int is_all_upperf = is_all_upper_word(ii);
      para_len_array[para].is_all_upper = MIN(is_all_upper, is_all_upperf);
      para_len_array[para].no_of_all_upper += is_all_upperf;
      int is_all_first_upper = para_len_array[para].is_all_first_upper;
      int is_all_first_upperf = is_first_upper_word(ii);
      para_len_array[para].is_all_first_upper = MIN(is_all_first_upper, is_all_first_upperf);
      para_len_array[para].no_of_all_first_upper += is_all_first_upperf;
      
      para_len_array[para].num_tokens++;
      para_len_array[para].no_of_real_words += is_real_word(ii);
      para_array[para].num_tokens++;
      prev_para = para;
    }
  }
  return para+1;
} // insert_into_para_arrays()



#define CHUNK 2500
#define MAX_CHUNK_NO 100
char query_line[MAX_CHUNK_NO][100000000 / MAX_CHUNK_NO]; // this query blows up at 50,000 lines at AMAZON packet size. therefore I do the query in two pieces
char rel_query_line[MAX_CHUNK_NO][100000000 / MAX_CHUNK_NO]; // for rel_coord
long int last_id[MAX_CHUNK_NO];




int insert_extra_token_for_rental_point_into_rf_word(int ii, int idx, int *in_jj, int *delta_points) {

  char temp[50];
  sprintf(temp,"%c.", 'a'+rental_point_array[idx].given_section_no-1); // OLD
  sprintf(temp,"RF%d", rental_point_array[idx].given_section_no);  

  int jj = *in_jj;
  rf_word_array[jj].text = strdup(temp);
  rf_word_array[jj].page = word_array[ii].page;
  rf_word_array[jj].para = word_array[ii].para;
  rf_word_array[jj].my_x1 = word_array[ii].my_x1;
  rf_word_array[jj].my_x2 = word_array[ii].my_x2;      
  rf_word_array[jj].my_y1 = word_array[ii].my_y1;
  rf_word_array[jj].my_y2 = word_array[ii].my_y2;       
  rf_word_array[jj].renumbered = word_array[ii].renumbered + *delta_points;
  rf_word_array[jj].my_line_in_page_renumbered = word_array[ii].my_line_in_page_renumbered;
  rf_word_array[jj].my_line_in_page = word_array[ii].my_line_in_page;    
  rf_word_array[jj].loc = word_array[ii].loc;
  rf_word_array[jj].has_space_before = word_array[ii].has_space_before;
  rf_word_array[jj].block_in_doc = word_array[ii].block_in_doc;
  rf_word_array[jj].loc = word_array[ii].loc;
  rf_word_array[jj].has_space_before = word_array[ii].has_space_before;
  rf_word_array[jj].block_in_doc = word_array[ii].block_in_doc;
  rf_word_array[jj].sn_in_doc = word_array[ii].sn_in_doc; // ZZZZ
  
  (*in_jj)++;
  (*delta_points)++;
  return 0;
} // insert_extra_token_for_rental_point_into_rf_word(



int print_rf(int nn, int to_no) {
  int ii;
  fprintf(stderr,"PRRRR:UDEV2:   :%d:%d:\n",nn,to_no);
  for (ii = 0; ii <= to_no; ii++) {
    if (rf_word_array[ii].renumbered >=0) {
      fprintf(stderr, "WWW:ii=%4d: sn=%d: %20s:\t:%d:\t:%d:\t:%d:\t:%d:\t:%d:\t:%d:\t:REN=%d:\t:RENIP!!!=%d:\t:IP=%d:\t:%d:\n"
	      ,ii
	    ,rf_word_array[ii].sn_in_doc
	    ,rf_word_array[ii].text
	    ,rf_word_array[ii].page
	    ,rf_word_array[ii].para
	    ,rf_word_array[ii].my_x1
	    ,rf_word_array[ii].my_x2
	    ,rf_word_array[ii].my_y1
	    ,rf_word_array[ii].my_y2
	    ,rf_word_array[ii].renumbered
	    ,rf_word_array[ii].my_line_in_page_renumbered
	    ,rf_word_array[ii].my_line_in_page
	    ,rf_word_array[ii].loc
	    );
    }
  } // for ii

  return 0;
}


int copy_word_into_rf_word(int word_count) {
  int ii, jj;
  int delta_points = 0; // if we stick points in the middle we must increase the token SN
  int prev_page = 0;
  int prev_line = -1;
  int page = 0;
  int line = -1;

  for (ii = 0; ii <= word_count; ii++) {
    prev_page= page;
    prev_line = line;
    page = word_array[ii].page;
    line = word_array[ii].my_line_in_page;
    int idx = index2rental_point_array[page][line]; // IDX is > 0 only when an RF exists
    if (idx > -1
	&& double_column_report_array[page].no_of_reordered_words == 0 // don't insert RF in DC situations
	&& (prev_page != page || prev_line != line)) { // insert the RFi only at the first word of the line
      fprintf(stderr,"XXinserting extra rental point: ii=%d: idx=%d: sec=%d: pl=%d:%d: text=%s:\n", ii, idx, rental_point_array[idx].given_section_no
	      , word_array[ii].page, word_array[ii].my_line_in_page
	      , word_array[ii].text);
      total_no_of_rentals_inserted++;
      total_no_of_rental_pages_inserted[page]++;

      insert_extra_token_for_rental_point_into_rf_word(ii, idx, &jj, &delta_points);
    }
    
    rf_word_array[jj].text = (word_array[ii].text) ? strdup(word_array[ii].text) : "(null)";
    rf_word_array[jj].page = word_array[ii].page;
    rf_word_array[jj].para = word_array[ii].para;
    rf_word_array[jj].my_x1 = word_array[ii].my_x1;
    rf_word_array[jj].my_x2 = word_array[ii].my_x2;      
    rf_word_array[jj].my_y1 = word_array[ii].my_y1;
    rf_word_array[jj].my_y2 = word_array[ii].my_y2;       
    rf_word_array[jj].renumbered = word_array[ii].renumbered + delta_points;
    rf_word_array[jj].my_line_in_page_renumbered = word_array[ii].my_line_in_page_renumbered;
    rf_word_array[jj].my_line_in_page = word_array[ii].my_line_in_page;    
    rf_word_array[jj].loc = word_array[ii].loc;
    rf_word_array[jj].has_space_before = word_array[ii].has_space_before;
    rf_word_array[jj].block_in_doc = word_array[ii].block_in_doc;
    rf_word_array[jj].loc = word_array[ii].loc;
    rf_word_array[jj].has_space_before = word_array[ii].has_space_before;
    rf_word_array[jj].block_in_doc = word_array[ii].block_in_doc;
    rf_word_array[jj].sn_in_doc = word_array[ii].sn_in_doc; // ZZZZ
    
    jj++;
  } // for ii

  return jj;
} // copy_word_into_rf_word(int word_count) 

char *insert_new_token(char *text) {
   char *bext = text;
   if (text && strlen(text) > 1 && strchr(text,'\\')) {
     bext = text;
   }
   return bext;
 }


int insert_into_regular_token_sql(int doc_id, int my_page_no, int word_count) {
  delete_from_regular_token_sql(doc_id);
  int bb;
  for (bb = 0; bb < MAX_CHUNK_NO; bb++) {
    strcpy(query_line[bb],"");
    strcpy(rel_query_line[bb],"");    
  }

  int ii;
  int mm;
  int buff_no = 0;
  int delta_points = 0; // if we stick points in the middle we must increase the token SN
  int prev_page = 0;
  int prev_line = -1;
  int page = 0;
  int line = -1;
  for (ii = 0, mm = 0; ii <= word_count; ii++, mm++) {
    prev_page= page;
    prev_line = line;
    page = rf_word_array[ii].page;
    line = rf_word_array[ii].my_line_in_page;
    int idx = index2rental_point_array[page][line];
    if (idx > -1 && (prev_page != page || prev_line != line)) { // insert the RFi only at the first word of the line
      fprintf(stderr,"inserting extra rental point: ii=%d: idx=%d: sec=%d: pl=%d:%d: text=%s:\n", ii, idx, rental_point_array[idx].given_section_no
	      , rf_word_array[ii].page, rf_word_array[ii].my_line_in_page
	      ,  rf_word_array[ii].text);

    }
    char buff[5000];
    if (rf_word_array[ii].renumbered >= 0 && rf_word_array[ii].page > 0) {
      insert_new_token(rf_word_array[ii].text);
      buff_no = mm / CHUNK;
      int page = rf_word_array[ii].page;
      sprintf(buff,"(/*bn=%d: */'%s', %d,     %4.3f, %4.3f, %4.3f, %4.3f,     %d, %d, %d, %d, %d, %d, %d, '%s'), \n"
	      , buff_no
	      , escape_quote_aws(rf_word_array[ii].text)
	      , rf_word_array[ii].renumbered + delta_points

	      /*
	      , rf_word_array[ii].my_x1 * X_back_to_aby_factor
	      , rf_word_array[ii].my_x2 * X_back_to_aby_factor
	      , rf_word_array[ii].my_y1 * Y_back_to_aby_factor
	      , rf_word_array[ii].my_y2 * Y_back_to_aby_factor
	      */
	      //#define NEW_FACTOR 4.2
	      #define NEW_FACTOR 1.0	      
	      , rf_word_array[ii].my_x1 * page_dimensions_array[page].xx_factor * NEW_FACTOR
	      , rf_word_array[ii].my_x2 * page_dimensions_array[page].xx_factor * NEW_FACTOR
	      , rf_word_array[ii].my_y1 * page_dimensions_array[page].yy_factor * NEW_FACTOR
	      , rf_word_array[ii].my_y2 * page_dimensions_array[page].yy_factor * NEW_FACTOR
	      
	      , rf_word_array[ii].page
	      , doc_id
	      , rf_word_array[ii].para	    
	      , rf_word_array[ii].my_line_in_page_renumbered
	      , rf_word_array[ii].loc
	      , rf_word_array[ii].has_space_before
	      , rf_word_array[ii].block_in_doc
	      , "AWS"
	      );
      strcat(query_line[buff_no],buff);

    } // if
  } // for ii
  char query[1000000];
  for (bb = 0; bb <= buff_no; bb++) {
    strcpy(query,"insert into deals_token \n\
                (text, sn, x1, x2, y1, y2,    page_no, doc_id, para_no, line_no, loc, has_space_before, my_grouping, source_program) \n\
                values \n");
    strcat(query, query_line[bb]);

    remove_last_comma(query);    
    if (1 && debug) fprintf(stderr,"QUERY78: chunk=%d: deb=%d: query=%s:\n",bb, debug, query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n", mysql_error(conn));
      fprintf(stderr,"QUERY78: chunk=%d: query=%s: \n", bb, query);
    }
    last_id[bb] = mysql_insert_id(conn);
    fprintf(stderr,"777 bb=%d: lid=%ld:\n",bb, last_id[bb]);
  }

  for (ii = 0, mm = 0; ii <= word_count; ii++, mm++) {
    char buff[5000];    
    if (rf_word_array[ii].renumbered >= 0 && rf_word_array[ii].page > 0) {
      buff_no = mm / CHUNK;
      // sprintf(buff,"(%d, %d, %ld, %d, %d, %d, %d), \n" OOOOOOUZ
      sprintf(buff,"(%d, %d, %d, %d, %d, %d), \n"	      
	      , doc_id
	      , rf_word_array[ii].renumbered + delta_points	      
	      // , last_id[buff_no]++ // OOOOOOUZ
	      , rf_word_array[ii].my_x1
	      , rf_word_array[ii].my_x2
	      , rf_word_array[ii].my_y1
	      , rf_word_array[ii].my_y2
	      );
      strcat(rel_query_line[buff_no], buff);
    }
  }

  for (bb = 0; bb <= buff_no; bb++) {
    strcpy(query,"insert into deals_tokenrelcoord \n\
            /*    (doc_id, sn, token, x1, x2, y1, y2)  OOOOOOUZ */  \n\
                (doc_id, sn, x1, x2, y1, y2) \n\
                values \n");
    strcat(query, rel_query_line[bb]);

    remove_last_comma(query);    
    if (1 && debug) fprintf(stderr,"QUERY78A: chunk=%d: deb=%d: query=%s:\n",bb, debug, query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n", mysql_error(conn));
      fprintf(stderr,"QUERY78A: chunk=%d: query=%s: \n", bb, query);
    }
  }

  select_from_regular_token_sql(doc_id);
  return 0;
} // insert_into_regular_token_sql()


int print_prove_tokenrel() {
  return 0;
}
int print_token_array(int nn, int total_renumbered_words) {
  fprintf(stderr,"************************PRINT WORDS:%d:\n",nn);
  int last_page = -1;
  int last_para = 0;
  int my_last_para = 0;
  int ww;
  int ii;
  for (ww = 0; ww < total_renumbered_words + 10000; ww++) {
    if (word_array[ww].renumbered >= 0 && word_array[ww].text && strcmp(word_array[ww].text,"(null)") != 0) {
      ii++;
      int page = word_array[ww].page;
      int para = word_array[ww].para;      
      if (page > last_page) {
	fprintf(stderr,"\n\nPAGE pn=%d>\n", page);
      }
      last_page = page;
      if (para > last_para) {
	fprintf(stderr,"\n\nPP para=%d ww=%d:\n", para, ww);
      }
      last_para = para;
      if (para != 0 && last_para != 0) my_last_para = last_para;
      fprintf(stderr,"   ww=%d: ren=%d: para=%d:%d:%d: t=%s:\n", ww, word_array[ww].renumbered, word_array[ww].para, last_para, my_last_para, word_array[ww].text);
    }
  }
  return 0;
}

int fprintf_clean_buff(FILE *outfile, char buff[]) {
  char bext[5000];
  int ii;
  int in_bra = 0;
  for (ii = 0; ii < strlen(buff); ii++) {
    if (in_bra == 0) {
      if (buff[ii] == '<') {
	in_bra = 1;
	bext[ii] = ' ';
      } else {
	bext[ii] = buff[ii];	
      }
    } else if (in_bra == 1) {
      if (buff[ii] == '>') {
	in_bra = 0;
      }
      bext[ii] = ' ';
    }
  }
  bext[ii] = '\0';
  fprintf(outfile, "%s", bext);
  return 0;
}

int print_htmlfile(int total_renumbered_words) {
  char nbuff[5000];
  strcpy(nbuff, displayfile_name);
  char *pp = strrchr(nbuff,'/');
  fprintf(stderr,"NBUFF=%s: pp=%s:\n", nbuff, pp);
  pp[0] = '\0';
  sprintf(pp,"%s/aaa5_db.%d",nbuff,doc_id);
  FILE *outfile = fopen(pp,"w");
  if (!outfile) {
    fprintf(stderr,"Hey wrong directory for :%s:\n",pp);
    exit(0);
  }
  fprintf(stderr,"Hey opened heml_db=%s:\n",pp);
  
  printf("<HTML><BODY>\n\
<DIV class=\"H1\" lev=\"1\" name=\"_\">\n");
  int new_page_flag = 1;  // we want to avoid inserting the end of para tag after a new <HR>.  It has to come before the HR.
  int last_page = -1;
  int last_para = -1;  
  int ww;
  //int ii;
  int loc = 1; // why do we start from 1??? b/c that's how the printout works

  for (ww = 0; ww < total_renumbered_words + 10000; ww++) {
    fprintf(stderr,"YYY0 ii=%d: ww=%d: t=%s: para=%d:\n", ww, rf_word_array[ww].renumbered, rf_word_array[ww].text, rf_word_array[ww].para);
    if (rf_word_array[ww].renumbered >= 0 && rf_word_array[ww].text && strcmp(rf_word_array[ww].text,"(null)") != 0) {
      //ii++;
      int page = rf_word_array[ww].page;
      int para = rf_word_array[ww].para;      
      fprintf(stderr,"    YYY1 ii=%d: ww=%d: t=%s: page=%d: para=%d: lp=%d:\n",  ww, rf_word_array[ww].renumbered,  rf_word_array[ww].text,  page,  para,  last_para);      
      char buff[80];
      if (page > last_page) {
	sprintf(buff,"%s\n\n<HR pn=%d>\n", ((new_page_flag == 0) ? "</p>" : ""), page);
	printf("%s", buff);
	fprintf_clean_buff(outfile, buff);
	new_page_flag = 1;
      } else {
	strcpy(buff,"");
      }
      loc += strlen(buff);
      last_page = page;
      if (para > last_para) {
	sprintf(buff,"%s\n\n<p id=\"%d\">", ((new_page_flag == 0) ? "</p>" : ""), para);
	printf("%s", buff);
	fprintf_clean_buff(outfile, buff); 
	new_page_flag = 0;		
      } else {
	strcpy(buff,"");
      }
      loc += strlen(buff);      
      last_para = para;
      char *text = rf_word_array[ww].text;
      rf_word_array[ww].loc = loc;      
      loc += strlen(text) + ((rf_word_array[ww].has_space_before) ? 1 : 0);
      sprintf(buff,"%s%s", ((rf_word_array[ww].has_space_before) ? " " : ""), text);

      printf("%s",buff);
      fprintf(outfile,"%s",buff);
      //printf("%s%s:%d:%d:", ((rf_word_array[ww].has_space_before) ? " " : ""), rf_word_array[ww].text, rf_word_array[ww].renumbered, rf_word_array[ww].sn_in_doc);      
    }
  }
  printf("</p>\n");
  printf("</DIV>\n</BODY></HTML>\n");
  fclose(outfile);
  return 0;
} // print_htmlfile(int total_renumbered_words) {


int print_displayfile(int word_count, char *displayfile_name) {
  fprintf(stderr,"ZZZA :%d:\n",word_count);
  FILE *displayfile = fopen(displayfile_name,"w");
  if (!displayfile) {
    fprintf(stderr,"Error: failed write open display_file :%s:\n",displayfile_name);
    exit(0);
  }
      
  fprintf(displayfile,"<HTML><BODY>\n\
<DIV class=\"H1\" lev=\"1\" name=\"_\">\n");
  int new_page_flag = 1;  // we want to avoid inserting the end of para tag after a new <HR>.  It has to come before the HR.
  int last_page = -1;
  int last_para = -1;  
  int ii;
  fprintf(stderr,"ZZZ :%d:\n",word_count);
  for (ii = 0; ii <= word_count; ii++) {
    fprintf(stderr,"    ZZZ0 ww=%d: t=%s: para=%d:\n", rf_word_array[ii].renumbered, rf_word_array[ii].text, rf_word_array[ii].para);
    // are we losing the first word renumbered >= 0 ?
    if (rf_word_array[ii].renumbered > 0 && rf_word_array[ii].text && strcmp(rf_word_array[ii].text,"(null)") != 0) {

      int page = rf_word_array[ii].page;
      int para = rf_word_array[ii].para;      
      char buff[80];
      if (page > last_page) {
	sprintf(buff,"%s\n\n<HR pn=%d>\n", ((new_page_flag == 0) ? "</p>" : ""), page);
	fprintf(displayfile,"%s", buff);
	new_page_flag = 1;
      } else {
	strcpy(buff,"");
      }
      last_page = page;
      if (para > last_para) {
	sprintf(buff,"%s\n\n<p id=\"%d\">", ((new_page_flag == 0) ? "</p>" : ""), para);
	fprintf(displayfile,"%s", buff);
	new_page_flag = 0;		
      } else {
	strcpy(buff,"");
      }
      last_para = para;
      char *text = rf_word_array[ii].text;
      fprintf(displayfile,"%s<span id=sp_%d>%s</span>", ((rf_word_array[ii].has_space_before) ? " " : ""),   rf_word_array[ii].renumbered, text);
    }
  }
  fprintf(displayfile,"</p>\n");
  fprintf(displayfile,"</DIV>\n</BODY></HTML>\n");
  fclose(displayfile);
  return 0;
} // print_displayfile(int word_count) 





int print_double_column_report(int last_page_no) {
  fprintf(stderr,"DOUBLE COLUMN REPORT\n");
  int pp;
  int total_words = 0;
  int total_pages = 0;
  for (pp = 1; pp <= last_page_no; pp++) {
    if (double_column_report_array[pp].no_of_reordered_words > 0) {
      fprintf(stderr,"    DC PAGE: page=%d: left_lines=%d: right_lines=%d reordered_words=%d:\n"
	      ,pp,double_column_report_array[pp].no_of_left_lines,double_column_report_array[pp].no_of_right_lines ,double_column_report_array[pp].no_of_reordered_words);
      total_pages ++;
      total_words += double_column_report_array[pp].no_of_reordered_words;
    }
  }
  fprintf(stderr,"    report DC TOTAL: pages=%d: reordered_words=%d:\n"  , total_pages, total_words);
  return 0;
}

int print_para_array(int para_no) {
  fprintf(stderr,"****************** PRINTING PARAS:%d:\n",para_no);
  int pp;
  for (pp = 0; pp < para_no; pp++) {
    fprintf(stderr," PID :%d: token:%d: num=%d: page=%d:\n", pp, para_array[pp].token, para_array[pp].num_tokens, para_array[pp].page);
  }
  fprintf(stderr," DONE PRINT PID pp=%d: para_no=%d:\n", pp, para_no);
  return 0;
}


int main(int ac , char **av) {
  get_params(ac, av);
  word_count = read_ocrtoken_array(doc_id, &last_page_no, &last_block_no, &last_line_no); // read from sql into WORD_ARRAY, BLOCK_ARRAY

  if (1 && debug) fprintf(stderr,"Returning0 pn=%d: ppa[%d]=%d:\n", last_page_no, 4, page_properties_array[4].last_line_in_page);

  print_block_array(0);
  print_word_array(0, word_count);
  fprintf(stderr,"\n********************* STY 1.  CALL create index \n");
  create_line2block_index(); // create LINE2BLOCK_INDEX
  fprintf(stderr,"Counted  words=%d: blocks=%d: pages=%d: lines=%d:\n",word_count, last_block_no, last_page_no, last_line_no);
  fprintf(stderr,"\n********************* STY 2.  CALL DOING SIMPLE BLOCK CLUSTERS -- is the entire page a TOC?\n");
  int essential_token = 0;
  int NEW_OCR = 1;
  if (NEW_OCR == 0) {
  create_TOC_block_clusters(last_page_no, &essential_token); // create Block_cluster_array, decide makes page_properties_array[pp].no_toc_clusters -- weather the entire page is TOC
  }
  
  if (debug) fprintf(stderr,"Returning1 pn=%d: ppa[%d]=%d:\n", last_page_no, 4, page_properties_array[4].last_line_in_page);
  
  fprintf(stderr,"\n********************* STY 3.0 CALL DOING REWNTAL FORMS\n");
  create_rental_forms(last_page_no); 
  print_word_array(1, word_count); 
  if (strcasecmp(doc_country,"GBR") != 0) {
    fprintf(stderr,"\n********************* STY 3. CALL DOING ENUMERATED_LINES\n");
    create_enumerated_clusters(last_page_no); // create enumerated lines
  }

  if (debug) fprintf(stderr,"Returning2 pn=%d: ppa[%d]=%d:\n", last_page_no, 4, page_properties_array[4].last_line_in_page);
  
  fprintf(stderr,"\n********************* STY 4. CALL DOING FOOTER\n");
  create_footer_clusters_new(last_page_no); // create footer_cluster_array and mark BLOCK_ARRAY[bb].IS_HEADER_FOOTER  
  fprintf(stderr,"\n********************* STY 5. CALL DOING HEADER\n");
  create_header_clusters_new(last_page_no) ;
  fprintf(stderr,"\n********************* STY 6. CALL DOING MEDIAN GAPS FOR DOUBLE COLUMN\n");

  if (debug) fprintf(stderr,"Returning3 pn=%d: ppa[%d]=%d:\n", last_page_no, 4, page_properties_array[4].last_line_in_page);
  print_word_array(2, word_count); 
    
  create_DC_overlap_clusters(last_page_no); // OVERLAP_CLUSTER_ARRAY
  fprintf(stderr,"********************* IDENTIFY DC GROUPS\n");  
  fprintf(stderr,"\n********************* STY 7. INSERT REORDER IN BLOCKS\n");
  print_block_array(1);

  print_first_last_line_in_page_array(last_page_no);
  if (debug) fprintf(stderr,"Returning pn=%d: ppa[%d]=%d:\n", last_page_no, 4, page_properties_array[4].last_line_in_page);
  traverse_double_column_blocks_in_groups(last_page_no);  // BLOCK_ARRAY[bb].RENUMBERED
  print_block_array(2);

  fprintf(stderr,"\n********************* STY 8. UPDATE BLOCKS IN SQL\n");
  update_block_reorder_sql_by_page(last_page_no);
  print_block_array(3);
  fprintf(stderr,"\n********************* STY 9. INSERT REORDER IN WORDS\n");
  sort_blocks_by_renumbered(block_array, last_block_no); // sort blocks so you can renumber words

  print_block_array(4);
  print_word_array(3, word_count); 
  fprintf(stderr,"\n********************* STY 10. TRAVERSE WORDS\n");
  int max_renumbered_words = traverse_words_double_column(last_block_no);  // WORD_ARRAY[bb].RENUMBERED
  int total_renumbered_words = word_count+1;
  print_word_array(4, total_renumbered_words);
  print_double_column_report(last_page_no);
  update_word_reorder_sql_per_page(last_page_no, total_renumbered_words);
  fprintf(stderr,"\n********************* STY 11. SORT BLOCKS\n");
  sort_blocks_by_page_and_renumbered(block_array, last_block_no); // sort blocks so you can see them REORDERED by PAGE

  fprintf(stderr,"\n********************* STY 12. INSERT PARAS\n");
  int para_no = insert_new_paras(last_block_no);
  int space_no = insert_spaces(last_block_no);  
  int org_id = get_org_id(doc_id);
  insert_paras_into_sql(para_no, space_no, doc_id, org_id);

  print_block_array(5);
  fprintf(stderr,"\n********************* STY 13. UPDATE WORDS: tot=%d:\n", total_renumbered_words);

  //update_word_para_sql_per_page(last_page_no, total_renumbered_words);

  fprintf(stderr,"\n********************* STY 14. SORT WORDS\n");
  print_word_array(7, word_count);
  sort_blocks_by_renumbered(word_array, word_count); // sort blocks so you can renumber words
  fix_split_token(word_array, word_count);
  print_word_array(8, total_renumbered_words);  

  fprintf(stderr,"\n********************* STY 15. INSERT RF WORDS:%d:\n",word_count);

  int rf_word_count = copy_word_into_rf_word(word_count);
  print_rf(4,9000);
  fprintf(stderr,"\n********************* STY 16. PRINT HTMLFILE (aaa61):wc=%d:%d:\n",word_count, rf_word_count);  
  print_htmlfile(rf_word_count);

  fprintf(stderr,"\n********************* STY 17. PRINT DISPLAYFILE\n");
  print_displayfile(rf_word_count, displayfile_name);
  
  //update_word_para_sql_per_page_rf(last_page_no, rf_word_count);

  int my_page_no = read_page_dimensions(doc_id);
  fprintf(stderr,"\n********************* STY 18. INSERT INTO DEALS_TOKEN page_no=%d:\n",my_page_no);

  insert_into_regular_token_sql(doc_id, my_page_no, rf_word_count);
  print_prove_tokenrel();
  fprintf(stderr,"\n********************* STY 18. INSERT INTO DEALS_PARAG\n");
  insert_into_para_arrays(doc_id, rf_word_count);
  print_para_array(para_no);
  fprintf(stderr," DONE PRINT1 PID para_no=%d: %d: %d: %d:\n", para_no, space_no, doc_id, org_id);  
  insert_new_paras_into_sql(para_no, space_no, doc_id,  org_id);

  fprintf(stderr,"********************* STY 19. DONE CALC\n");
  fprintf(stderr," no_of_enums=%d:%d:\n",total_no_of_enums,total_no_of_enum_pages);
  fprintf(stderr," no_of_rentals=%d:%d:\n",total_no_of_rentals, total_no_of_rental_pages);

  int pp;
  int trp = 0;
  for (pp = 1; pp < MAX_PAGE; pp++) {
    if (total_no_of_rental_pages_inserted[pp] > 0) {
      trp++;
    }
  }
  fprintf(stderr," no_of_rentals=%d:%d: pages:",total_no_of_rentals_inserted, trp);    
  for (pp = 1; pp < MAX_PAGE; pp++) {
    if (total_no_of_rental_pages_inserted[pp] > 0) {
      fprintf(stderr," P=%2d:%2d: ", pp, total_no_of_rental_pages_inserted[pp]);
    }
  }
  fprintf(stderr,"\n");
	  
  int ww;
  if (0) for (ww = 0; ww < rental_point_no; ww++) {
    fprintf(stderr,"rental: ww=%d: wn=%d: sn=%d:\n",ww, rental_point_array[ww].word_no, rental_point_array[ww].given_section_no);
  }
  return 0;
} // main()
