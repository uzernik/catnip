#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include <ctype.h>
#include "recitals_functions.h"
extern int debug;
#define MIN(a,b) (a<b)?a:b
#define MAX(a,b) (a>b)?a:b
/*
first param is "do_print"
CALLING (../bin/get_preamble 0 3968 ../tmp/Index/index_file_3968 localhost dealthing root imaof3 1 < ../tmp/Xxx/aaa7) >& ttt
 */

  char *db_IP, *db_name, *db_pwd, *db_user_name; // for DB config

  /************** SQL ********************/
  //char *server = "localhost";
  //char *server = "54.241.17.226";
  //char *user = "root";
  //char *password = "imaof3";
  //char *database = "dealthing";


  MYSQL *conn;
  MYSQL_RES *res;
  MYSQL_ROW row;


  struct Para_Len {
    int pid;
    int len;
  } para_len_array[MAX_PARA];

  int my_para_len_no;

  #define MAX_SUMMARY_ITEM 100000
  struct Summary_Item {
    int sn_in_doc; // the serial number of this item in a document; start from 0
    int doc_id;
    int para_id;
    int curr_toc_sn; // the index
    int curr_toc_item_id; // the toc_item_id (id in the document iteslef)

    char *pheader;
    int pitem_id; //  // the toc_item_id (id in the document iteslef)
    char *psection;
    int ppara_id;
    int pgrp_id;
    char *ptitle;
    int penum;

    char *nheader;
    int nitem_id;
    char *nsection;
    int npara_id;
    int ngrp_id;
    char *ntitle;
    int nenum;

    char *name;
    char *text;
    char *feature;

 } summary_item_array[MAX_SUMMARY_ITEM];
  int my_summary_item_no = 0;

  #define MAX_LABEL 100000
  struct Label {
    int my_id;
    char *name; // pream/recit/sigblock
    char *feature; // whereas/witnesseth
    char *text;
    int doc_id;
    int para_id;
    int toc_item_no; // the serial number in the array
  } label_array[MAX_LABEL];
  int my_label_no = -1;

  #define MAX_DOC 100000
  struct Doc {
    char *doc_name;
    char *fd_name;
    char *fd1_name;
    int item_id_in_this_doc; // pointer to summary_item_array
  } doc_array[MAX_DOC];
  int my_doc_no = -1;


  #define MAX_TOC_ITEM 100000

char *clean_quotes(char *text) {
  int ii, jj;
  static char bext[6000];
  for (ii = 0, jj = 0; ii <strlen(text); ii++) {
    if (text[ii] != '\'' && text[ii] != '\"') {
      bext[jj++] = text[ii];
    }
  }
  bext[jj++] = '\0'; 
  return bext;
}


int populate_item_array(int kk, int aa, int curr_tt) {
  //int curr_toc_item_id = index_item_array[curr_tt].item_id; JJJJJ
  int curr_toc_item_id = curr_tt;

  summary_item_array[aa].sn_in_doc++;
  summary_item_array[aa].doc_id = label_array[kk].doc_id;
  summary_item_array[aa].para_id = label_array[kk].para_id;
  summary_item_array[aa].curr_toc_item_id = curr_toc_item_id;

  summary_item_array[aa].pheader = strdup((index_item_array[curr_tt].header) ? index_item_array[curr_tt].header : "_");
  summary_item_array[aa].pitem_id = curr_toc_item_id;
  summary_item_array[aa].psection = strdup((index_item_array[curr_tt].section) ? index_item_array[curr_tt].section : "_");
  summary_item_array[aa].ptitle = strdup((index_item_array[curr_tt].title) ? index_item_array[curr_tt].title : "_");
  summary_item_array[aa].ppara_id = index_item_array[curr_tt].pid;
  summary_item_array[aa].pgrp_id = index_item_array[curr_tt].grp_no;
  summary_item_array[aa].penum = index_item_array[curr_tt].my_enum;

  summary_item_array[aa].nheader = strdup((index_item_array[curr_tt+1].header) ? index_item_array[curr_tt+1].header : "_");
  summary_item_array[aa].nitem_id = curr_toc_item_id+1;
  summary_item_array[aa].nsection = strdup((index_item_array[curr_tt+1].section) ? index_item_array[curr_tt+1].section : "_");
  summary_item_array[aa].ntitle = strdup((index_item_array[curr_tt+1].title) ? index_item_array[curr_tt+1].title : "_");
  summary_item_array[aa].ngrp_id = index_item_array[curr_tt+1].grp_no;
  summary_item_array[aa].npara_id = index_item_array[curr_tt+1].pid;
  summary_item_array[aa].nenum = index_item_array[curr_tt+1].my_enum;
  summary_item_array[aa].name = label_array[kk].name;
  summary_item_array[aa].text = label_array[kk].text;
  summary_item_array[aa].feature = label_array[kk].feature;

  if (my_debug) fprintf(stderr,"POP:doc_id=%d: nm=%s: sn=%d: tt=%d: pid=%d: nsummary_item_id=%d npid=%d nsec=%s: nhd=%s: nenum=%d: feat=%s:\n"
			,summary_item_array[aa].doc_id
			,summary_item_array[aa].name
			,aa
			,curr_tt+1
			,summary_item_array[aa].para_id
			,summary_item_array[aa].nitem_id
			,summary_item_array[aa].npara_id
			,summary_item_array[aa].nsection
			,summary_item_array[aa].nheader
			,summary_item_array[aa].nenum
			,summary_item_array[aa].feature);
  return 0;
} // populate_item_array

int decide_place_on_seq(int index_no) {
  /* 
     see if the preamble is in a middle of a good sequence.
     check the items before and after and see if they are part of a longer seq (2 on each side)
     return 1 if it's a LONG seq
     return 0 if it is a short seq
  */
  int ret;
  if (my_debug) fprintf(stderr,"PREAM IS: \tBF index_no=%d: seq=%d: lev=%d:\n\t\tAT index_no=%d: seq=%d: lev=%d:\n"
	  ,index_no , (index_no==-1)?0:index_item_array[index_no].seq_no, (index_no==-1)?0:index_item_array[index_no].lev_no
	  ,index_no+1 , (index_no>=index_array_no)?0:index_item_array[index_no].seq_no, (index_no>=index_array_no)?0:index_item_array[index_no].lev_no);
  int next_seq_no = (index_no >= index_array_no) ? 0 : index_item_array[index_no].seq_no;
  int next_lev = (index_no >= index_array_no) ? 0: index_item_array[index_no].lev_no;
  int count_bf = 0;
  int count_at = 0;
  if (next_lev >= 3) {
    int ii;
    for (ii = 0; ii <= index_no; ii++) {
      if (index_item_array[ii].seq_no == next_seq_no) count_bf++;
    }

    for (ii = index_no+1; ii < index_array_no; ii++) {
      if (index_item_array[ii].seq_no == next_seq_no) count_at++;
    }
  }
  if (count_at >= 2 && count_bf >= 2) {
    ret = 1;  // 1 is BAD -- preamble cannot appear in a middle of a healthy sequence
  } else {
    ret = 0; // 0 is GOOD, Preamble is enabled
  }
  if (my_debug) fprintf(stderr,"CHECKED SEQ: index_no=%d next_seq_no=%d: next_lev=%d: count_bf=%d: count_at=%d: ret=%d:\n"
	  , index_no, next_seq_no, next_lev, count_bf, count_at, ret);
  return ret;
}

#define MY_DIFF1 10
int is_pream_ok(int ii, int f_pream) {
  if (my_debug) fprintf(stderr,"\n\nIS_PREAM:ii=%d: pid=%d:%d ty=%s:%s: index_no=%d:\n"
			,ii,summary_item_array[ii].para_id,summary_item_array[ii+1].para_id
			,summary_item_array[ii].name,summary_item_array[ii+1].name, summary_item_array[ii].nitem_id);
  char *after_pream = NULL;
  int toc_after_enum = summary_item_array[ii].nenum;
  int ok_toc = (summary_item_array[ii].npara_id - summary_item_array[ii].para_id < MY_DIFF1) && (toc_after_enum == 1 || toc_after_enum == 9); // the first TOC item is nearby  
  int bf_recitals = (summary_item_array[ii+1].name 
		     && (strcmp(summary_item_array[ii+1].name,"Recitals") == 0
			 || strcasecmp(summary_item_array[ii+1].name,"Table_Of_Contents") == 0)
		     && summary_item_array[ii+1].para_id - summary_item_array[ii].para_id < MY_DIFF1);
  if (bf_recitals) {
    after_pream = "REC";
  } else if (ok_toc) {
    after_pream = "TOC";
  } else {
    after_pream = "NULL";
  }

  // the preamble is embedded inside a good seq so it should be cancelled
  int inside_good_seq = decide_place_on_seq(summary_item_array[ii].nitem_id); // rule out pream when it's embedded in a good sequence:  1., 2., 3. pream, 4., 5.,
  if (my_debug) fprintf(stderr,"BF_REC=%d: AFTER=%s\n",bf_recitals,after_pream);
  if (inside_good_seq == 0 && (bf_recitals || ok_toc)) { // we are OK, the last PREAM is followed by REC if (the next one is a recitals, or there is a Section 1 near by)

    edit_index_array[edit_index_array_no].first_pream = f_pream;
    edit_index_array[edit_index_array_no].last_pream = ii;

    edit_index_array[edit_index_array_no].first_pream_pid = summary_item_array[f_pream].para_id;
    if (strcmp(after_pream,"REC")==0) {

      edit_index_array[edit_index_array_no].last_pream_pid = MAX(summary_item_array[ii+1].para_id - 1,summary_item_array[ii].para_id); // go all the way to the REC, but in case the REC is a duplicate of the current PREAM then move 1 forward...
      if (my_debug) fprintf(stderr,"REC:PAI=%d: II=%d: CPID=%d: NPID=%d: TP=%s:%s:\n",edit_index_array_no,ii
	      ,summary_item_array[ii].para_id,summary_item_array[ii+1].para_id
	      ,summary_item_array[ii].name,summary_item_array[ii+1].name);
    } else if (strcmp(after_pream,"TOC")== 0) {

      if (summary_item_array[ii+1].name &&strcasecmp(summary_item_array[ii+1].name,"Table_Of_Contents") == 0) { // there is a TOC title

	edit_index_array[edit_index_array_no].last_pream_pid = MAX(summary_item_array[ii+1].para_id - 1,summary_item_array[ii].para_id); // go all the way to the REC, but in case the REC is a duplicate of the current PREAM then move 1 forward...

      } else { // there is a section I

	edit_index_array[edit_index_array_no].last_pream_pid = summary_item_array[ii].npara_id - 1; // go all the way to Section 1

      }
      if (my_debug) fprintf(stderr,"REC:PAI=%d: II=%d: CPID=%d: NPID=%d: TP=%s:%s:\n",edit_index_array_no,ii
	      ,summary_item_array[ii].para_id,summary_item_array[ii+1].para_id
	      ,summary_item_array[ii].name,summary_item_array[ii+1].name);

    } else {
      ;
    }

    edit_index_array[edit_index_array_no].end_type = strdup(after_pream);
    if (my_debug) fprintf(stderr,"\t\tGOOD TOC=%d: REC=%d: F=%d: L=%d: NEXT=%s:\n",ok_toc,bf_recitals
	    ,edit_index_array[edit_index_array_no].first_pream_pid,        edit_index_array[edit_index_array_no].last_pream_pid,  after_pream);
    edit_index_array_no++;

    float my_ratio = (float)((float)(summary_item_array[f_pream].curr_toc_item_id - summary_item_array[f_pream-1].curr_toc_item_id) 
			     / (float)(summary_item_array[f_pream].para_id - summary_item_array[f_pream-1].para_id)); // no_toc / no_pid

    float my_simple_ratio = (float)((float)(summary_item_array[f_pream].curr_toc_item_id) 
				    / (float)(summary_item_array[f_pream].para_id)); // no_toc / no_pid

    int bb;
    int total_len = 0;
    for (bb = index_item_array[1].pid; bb <= summary_item_array[f_pream].para_id-1; bb++) {
      total_len += para_len_array[bb].len;
    }

    // we calculate the number of CHARACTERS per TOC item. Note: a toc item can have several PIDs.
    float len_ratio = (float)((float)(total_len) / (float)(summary_item_array[f_pream].para_id - index_item_array[1].pid));
    if (my_debug) fprintf(stderr,"LEN_RATIO =%2.2f total_len=%d total_no_of_toc toc_len=%d my_ration=%2.2f\n"
			  ,len_ratio
			  ,total_len
			  ,summary_item_array[f_pream].curr_toc_item_id, my_ratio);

    if (my_debug /*&& f_pream > 0*/) { // just a printout
      fprintf(stderr,"REMOVE_PARAMS: Pid=%d:%d: Iid=%d:%d: simple_ratio=%2.2f len_ratio=%2.2f ib=%d:%s: %d:%d  %d:%d TOC_START (item, not pid):%d: TOC_SIZE=%d: MY_RATIO=%2.2f TOC_start=%d:\n"
	      , summary_item_array[f_pream].para_id,summary_item_array[f_pream-1].para_id
	      , summary_item_array[f_pream].curr_toc_item_id,summary_item_array[f_pream-1].curr_toc_item_id
	      , my_simple_ratio
	      , len_ratio
	      , f_pream,summary_item_array[f_pream-1].name
	      ,summary_item_array[f_pream].curr_toc_item_id,summary_item_array[f_pream-1].curr_toc_item_id
	      ,summary_item_array[f_pream].para_id,summary_item_array[f_pream-1].para_id
	      ,summary_item_array[f_pream-1].curr_toc_item_id
	      ,summary_item_array[f_pream].curr_toc_item_id - summary_item_array[f_pream-1].curr_toc_item_id
	      , my_ratio
	      , summary_item_array[f_pream-1].curr_toc_item_id
	      );
    }

    // two cases for the identification of an MS_TOC to be removed:
    /*fprintf(stderr,"OOO: fp=%d fitem=%d: tl=%d (%d-%d): lr=%2.2f: mr=%2.2f"
	    ,f_pream
	    , summary_item_array[f_pream-1].curr_toc_item_id
	    , summary_item_array[f_pream].curr_toc_item_id - summary_item_array[f_pream-1].curr_toc_item_id, summary_item_array[f_pream].curr_toc_item_id, summary_item_array[f_pream-1].curr_toc_item_id 
	    , len_ratio
	    , my_ratio);  */
    if (f_pream > 0 // check if the prev sum_item is a beginning of a TOC //  found "Table Of Contents"
	&& summary_item_array[f_pream-1].curr_toc_item_id < 100 // TOC must start in ID 0 or so (this is tokens not PID)
	//&& summary_item_array[f_pream].curr_toc_item_id - summary_item_array[f_pream-1].curr_toc_item_id > 11 // TOC must be substantial in size:  NEW!!!!! DOES IT CAUSE TROUBLE????
	&& ((len_ratio < 90 // allow more or less 90 char per TOC item (pretty much 2.5 paras per item)
	     && my_ratio > 0.4
	    )
	    || (len_ratio < 60 // allow more or less 100 char per TOC item (pretty much 3.3 paras per item, so less chars)
		&& my_ratio > 0.3
		)
	    )
        )
      {
      edit_index_array[edit_index_array_no].end_type = "REM_TOC";
      edit_index_array[edit_index_array_no].first_pream_pid = summary_item_array[f_pream-1].para_id;
      edit_index_array[edit_index_array_no].last_pream_pid = summary_item_array[f_pream].para_id-1;
      edit_index_array_no++;
      if (my_debug) fprintf(stderr,"REM TOC0=%d: REC=%d: F=%d: L=%d: NEXT=%s:\n",ok_toc,bf_recitals,edit_index_array[edit_index_array_no].first_pream_pid,edit_index_array[edit_index_array_no].last_pream_pid,after_pream);
      if (my_debug) fprintf(stderr,"FOUND MS_TOC ar PARA:%d",summary_item_array[f_pream-1].para_id);
    } else  if (summary_item_array[f_pream].curr_toc_item_id > 11 // see if the TOC is degenerate by the ratio// not found "Table Of Contents"
		&& my_simple_ratio > 0.7
		&& len_ratio < 50 // allow more or less 100 char per TOC item
		&& (index_item_array[1].section[0] == '1' || index_item_array[1].section[0] == 'I' || strcasestr(index_item_array[1].section,"ONE"))
		) {
      edit_index_array[edit_index_array_no].end_type = "REM_TOC";
      edit_index_array[edit_index_array_no].first_pream_pid = index_item_array[1].pid;
      edit_index_array[edit_index_array_no].last_pream_pid = summary_item_array[f_pream].para_id-1;
      edit_index_array_no++;
      if (my_debug) fprintf(stderr,"REM TOC1=%d: REC=%d: F=%d:%d: L=%d: NEXT=%s:msr=%2.2f: len_ratio=%2.2f\n",ok_toc,bf_recitals
			    ,index_item_array[1].pid,edit_index_array[edit_index_array_no].first_pream_pid,      edit_index_array[edit_index_array_no].last_pream_pid,   after_pream,my_simple_ratio, len_ratio);
      if (my_debug) fprintf(stderr,"FOUND_XXX MS_TOC ar PARA:%d",index_item_array[1].pid);
    } else {
      if (my_debug) fprintf(stderr,"NOT FOUND");
    }
  } // major if 
  if (my_debug) fprintf(stderr,"\nwhat's my edit_no? %d:\n",edit_index_array_no);
  return edit_index_array_no;
} // is_pream_ok()

#define MY_DIFF0 5
// here we decide what are legitimate preambles and recitals based on TOC
// generate EDIT_INDEX_ARRAY, an array of first/last pid pairs for each preamble
int identify_preamble(int in, int doc_id) {
  int ii;

  int ok_toc = 1;
  int in_pream = 0;
  int in_recitals = 0;
  edit_index_array_no = 0;
  int f_pream = -1; // the first pream item
  for (ii = in // move on to the next line only if: (1) it is not more than 4 Paras down, (2) it is still a preamble
	 ; summary_item_array[ii].doc_id == doc_id && ii < my_summary_item_no
	 ; ii++) {
    //fprintf(stderr,"III %d:%d:%d\n",ii,summary_item_array[ii].para_id,summary_item_array[ii].npara_id);
    /******** in once cycle we can go in and out; we determine f_pream and then we do the ok_pream ***********/
    if (in_pream == 0 // GET IN clause
	&& summary_item_array[ii].name && strcmp(summary_item_array[ii].name,"Preamble") == 0) {
      if (my_debug) fprintf(stderr,"      GETIN IN PREAM %d:%d:%d\n",ii,summary_item_array[ii].para_id,summary_item_array[ii].npara_id);
      in_pream = 1;
      f_pream = ii;
    }
    if (in_pream == 1  // GET OUT clause: break based on the next item -- so the next item in turn can have its own life 
	&& ((summary_item_array[ii+1].name 
	     && (strcmp(summary_item_array[ii+1].name,"Preamble") != 0)) // this is better than enumerating all the others. (1) there can always be yet another guy say SIG, (2) a para can be marked both REC and PREAM
	     /*
	     && (strcmp(summary_item_array[ii+1].name,"Recitals") == 0
		 || strcasecmp(summary_item_array[ii+1].name,"Table_Of_Contents") == 0))
	     */
	    || summary_item_array[ii+1].name == NULL || ii >= my_summary_item_no // this guy is the last on the list
	    || summary_item_array[ii+1].para_id - summary_item_array[ii].para_id > MY_DIFF0)) {  
      if (my_debug) fprintf(stderr,"      GETOUT IN PREAM ii=%d: pid=%d:npid=%d f_pream=%d:\n",ii, summary_item_array[ii].para_id, summary_item_array[ii].npara_id, f_pream);
      /***************** IS_PREAM_OK generates the entry for PREAM plus its termination type: REC, TOC, NULL *********/
      is_pream_ok(ii,f_pream);      // now, getting out of PREAM, what's the verdict? Is it a legit pream?
      in_pream = 0;
    }

    /***************** SIG is the beginning of the SIGNATURE BLOCK ******************/
    if (summary_item_array[ii].name 
	&& (strcmp(summary_item_array[ii].name,"Sigblock") == 0)) {
      edit_index_array[edit_index_array_no].end_type = "SIG";
      edit_index_array[edit_index_array_no].first_pream_pid = summary_item_array[ii].para_id;
      edit_index_array[edit_index_array_no].last_pream_pid = summary_item_array[ii].para_id;
      edit_index_array_no++;
    }

    /***************** IN_REC is used to eliminate the A. B. at the beginning of the document *************/
    if (in_recitals == 0 
	&& summary_item_array[ii].name 
	&& strcmp(summary_item_array[ii].name,"Recitals") == 0 
	&& summary_item_array[ii].feature
	&& (strcmp(summary_item_array[ii].feature,"RECITALS") == 0  || strcmp(summary_item_array[ii].feature,"WITNESSETH") == 0))  {

      in_recitals = summary_item_array[ii].para_id;
      if (my_debug) fprintf(stderr,"FOUND REC WIT0: pid=%d: feat=%s:\n",in_recitals,summary_item_array[ii].feature);
    }

    if (in_recitals > 0 
	&& summary_item_array[ii].name 
	&& strcmp(summary_item_array[ii].name,"Recitals") == 0 
	&& summary_item_array[ii].feature
	&& (strcmp(summary_item_array[ii].feature,"NOW_THEREFORE") == 0 || strcmp(summary_item_array[ii].feature,"CONDITION") == 0 || strcmp(summary_item_array[ii].feature,"AGREE") == 0)) {
      if (my_debug) fprintf(stderr,"FOUND REC WIT1: pid=%d: feat=%s:\n",summary_item_array[ii].para_id,summary_item_array[ii].feature);
      if (summary_item_array[ii].para_id - in_recitals < 20) {
	edit_index_array[edit_index_array_no].end_type = "IN_REC";
	edit_index_array[edit_index_array_no].first_pream_pid = in_recitals;
	edit_index_array[edit_index_array_no].last_pream_pid = summary_item_array[ii].para_id;
	edit_index_array_no++;
	if (my_debug) fprintf(stderr,"FOUND REC_NOW (IN_REC) PAIR: pid=%d:%d:\n",in_recitals,summary_item_array[ii].para_id);
      }
      in_recitals = 0;
    }

    /******** preparing OK_TOC for next step **********/

    if (0 && my_debug) fprintf(stderr,"VISITING SUMMARY_ITEM: ii=%d:pid=%d:next_pid=%d: npid (toc)=%d:\n",ii,summary_item_array[ii].para_id,summary_item_array[ii+1].para_id, summary_item_array[ii].npara_id);

    { // prep for next stage
      int diff = summary_item_array[ii+1].para_id - summary_item_array[ii].para_id;
      int ok_diff = (diff <= MY_DIFF0);

      int within_pream = (summary_item_array[ii+1].name && strcmp(summary_item_array[ii+1].name,"Preamble") == 0);
      int toc_after_pid = summary_item_array[ii].npara_id;
      char *toc_after_title = summary_item_array[ii].ntitle;
      char *toc_after_section = summary_item_array[ii].nsection;
      int toc_after_enum = summary_item_array[ii].nenum;
      ok_toc = (toc_after_pid - summary_item_array[ii].para_id < MY_DIFF1) && (toc_after_enum == 1 || toc_after_enum == 9);
      if (0 && my_debug) fprintf(stderr,"VISITING SUMMARY_ITEM5\n");
      if (0 && my_debug) fprintf(stderr,"<TR><TD>KK:%d:%d:%d:</TD><TD>name=%s: ok_toc=%d diff=%d: ok_diff=%d</TD><TD>in_pre=%d: next=%s toc=%d:%s:%s: tap=%d pid=%d tae=%d</TD></TR>\n"
			 , ii, in, doc_id
			 , summary_item_array[ii].name, ok_toc, diff, ok_diff
			 , within_pream
			 , summary_item_array[ii+1].name
			 , toc_after_pid
			 , toc_after_title
			 , toc_after_section
			 ,toc_after_pid, summary_item_array[ii].para_id,toc_after_enum
			 );
    }
  } // for ii
  return edit_index_array_no;
} // identify_preamble

int print_one_item(int aa) {
  int f_p = edit_index_array[0].first_pream_pid;
  int l_p = edit_index_array[0].last_pream_pid;
	char *color = "yellow";
	if (aa >= f_p && aa <= l_p) {
	  color = "green";
	} else if (summary_item_array[aa].name && strcasecmp(summary_item_array[aa].name,"preamble") == 0) {
	  color = "blue";
	} else if (summary_item_array[aa].name && strcasecmp(summary_item_array[aa].name,"recitals") == 0) {
	  color = "red";
	} else {
	  color = "gray";
	}

	if (debug) fprintf(stderr,"<TR><TD>%d:<font color=%s><b>%d:<span title='%s'>(%d %d %s:%s %d %d)</span>:<span title='%s'>(%d %d %s:%s %d %d)</span></b></font></TD><TD>n=%s</TD><TD>t=%.*s</TD><TR>\n"
	       , summary_item_array[aa].doc_id
	       , color
	       , summary_item_array[aa].para_id

	       , summary_item_array[aa].pheader
	       , summary_item_array[aa].curr_toc_item_id
	       , summary_item_array[aa].ppara_id
	       , summary_item_array[aa].ptitle
	       , summary_item_array[aa].psection
	       , summary_item_array[aa].pgrp_id
	       , summary_item_array[aa].penum

	       , summary_item_array[aa].nheader
	       , summary_item_array[aa].curr_toc_item_id+1
	       , summary_item_array[aa].npara_id
	       , summary_item_array[aa].ntitle
	       , summary_item_array[aa].nsection
	       , summary_item_array[aa].ngrp_id
	       , summary_item_array[aa].nenum

	       , summary_item_array[aa].name
	       , 70,summary_item_array[aa].text);
	return 0;
}

int sync_up_recitals_and_toc() { // generate the summary_item_array
  int ii; // doc_id runner
  int sn = 0; 
  int kk = 0;
  int curr_tt; // toc_item runner
  
  curr_tt = -1;

  for (sn = 0, ii = 0; ii <= my_doc_no; ii++, sn++) { // go over the DOC_ARRAY, II is the REAL doc_id, SN is the index in the sparse array

    if (doc_array[ii].doc_name != NULL) { // make sure there is a DOC in this place in this sparse array

      summary_item_array[my_summary_item_no].sn_in_doc = -1;   // this is the array of doc_parts (preamble, sig, recitals, toc)
      doc_array[ii].item_id_in_this_doc = my_summary_item_no;

      { // bring the TOC_ARRAY all the way up to the current doc
	int nn = 0;
	while (nn++ < 10000 && my_doc_no < ii) {

	  curr_tt++; // the pointer to the INDEX_ITEM_ARRAY
	}
	curr_tt--;
      }

      while (label_array[kk].doc_id <= ii && kk <= my_label_no) { // run up KK, the label_array runner together with tt, the toc_item runner
	// move the toc_item counter to run in sync with the label counter

	int tt = 0;
	while (tt < index_array_no // don't exceed the array
	       && (my_doc_no < ii
		   || (my_doc_no == ii
		       && index_item_array[tt].pid <= label_array[kk].para_id))) {
	  if (my_doc_no == ii) { // don't take if not the same doc yet
	    curr_tt = tt;
	  }
	  tt++;
	} 

	populate_item_array(kk, my_summary_item_no, curr_tt);
	my_summary_item_no++;

	label_array[kk].toc_item_no = curr_tt;  // insert the toc_id just before this pid
	kk++; 

      }
      if (my_debug) fprintf(stderr,"</TABLE>\n");

    } // if

  } // for sn
  if (my_debug) fprintf(stderr,"Sync_UP generated %d summary_items\n",my_summary_item_no);
  return 0;
} // // sync_up_recitals_and_toc

int scan_recitals_from_array(struct Pream_Pair edit_index_array[]) { // populate edit_array

  int ii;
  int sn = 0;
  if (debug) fprintf(stderr,"\n\nPRINTING DOCS:%d:\n",my_doc_no);
  int prev_ii = 0;
  for (ii = 0; ii < my_doc_no +2; ii++) {

    if (doc_array[ii].doc_name) {
      if (debug) fprintf(stderr,"<P>NEW DOC:%d::: DOC_ID=%d -- PATH= %s / %s / %s</P><TABLE>\n"
	     ,sn++
	     ,prev_ii
	     ,doc_array[prev_ii].fd1_name
	     ,doc_array[prev_ii].fd_name
	     ,doc_array[prev_ii].doc_name);
      
      int in = doc_array[ii].item_id_in_this_doc; // since there can be many strung documents
      edit_index_array_no = identify_preamble(in, summary_item_array[in].doc_id);
      while (ii == summary_item_array[in].doc_id) {
	print_one_item(in);
	in++;
      }
      if (debug) fprintf(stderr,"\n</TABLE>\n");
      prev_ii = ii;
    } // if
  }
  return edit_index_array_no;
}

int get_recitals_from_sql(MYSQL *conn, int in_doc_id) {
    int ii = 0;
    MYSQL_RES *sql_res;
    MYSQL_ROW sql_row;
    static char query[10000];
    int nn;


    /************* get PARA_LEN info */
    sprintf(query,"select pl.para_no, pl.len \n\
                          from deals_paragraphlen as pl \n\
                          left join deals_document as dd on (dd.id = pl.doc_id) \n\
                          where dd.name not like \'%cAbstract%c\' \n\
                          and dd.id = \'%d\' \n\
                          order by pl.doc_id ASC, pl.para_no ASC "
	    , '%', '%' , in_doc_id
	    );
    if (my_debug) fprintf(stderr,"QUERY21=%s\n",query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      exit(1);
    }

    sql_res = mysql_store_result(conn);
    nn = mysql_num_rows(sql_res);
    if (my_debug) fprintf(stderr,"NUM_ROWS0=%d\n",nn);
    if (nn == 0) {
      //fprintf(stderr,"Error: PARA_LEN item not found in SQL\n");
    } else {
      int ii = 0;
      while (ii < nn && ii < MAX_PARA) {

	sql_row = mysql_fetch_row(sql_res);	

	para_len_array[ii].pid = atoi(sql_row[0]);
	para_len_array[ii].len = atoi(sql_row[1]);
	
	//if (my_debug) fprintf(stderr,"TOC_ITEM: ii=%d pid=%d: did=%d enum=%d: section=%s: hdr=%s:\n",ii, index_item_array[ii].pid,index_item_array[ii].doc_id,index_item_array[ii].my_enum,index_item_array[ii].section,index_item_array[ii].header);
	ii++;
      }
      my_para_len_no = ii;
    }
    

    /********* GET ALL PREAM/RECIT/SIGBLOCK INSTANCES ***************/ 
    sprintf(query,"select de.id as my_id, de.name, dp.value, dp.doc_id, dp.para_id, dd.name as doc_name, fd.name as fd_name, fd1.name as fd1_name, rf.feature \n\
                          from deals_entity_text as dp \n\
                          left join deals_entity as de on (de.id = dp.belongs_in_name_id) \n\
                          left join deals_recitals_feature as rf on (dp.id = rf.instance_id) \n\
                          left join deals_document as dd on (dd.id = dp.doc_id) \n\
                          left join deals_folder as fd on (fd.id = dd.folder_id) \n\
                          left join deals_folder as fd1 on (fd1.id = fd.parentfolder_id) \n\
                          where dd.name not like \'%cAbstract%c\' \n\
                          and (%d = -1 or dd.id = \'%d\') \n\
                          and (de.name like \'Recitals\' or de.name like \'Sigblock\' or de.name like \'Preamble\' or de.name like \'Table_Of_Contents\') \n\
                          order by dp.doc_id ASC, dp.para_id ASC "
	    , '%', '%', in_doc_id, in_doc_id
	    );

    if (my_debug) fprintf(stderr,"QUERY31=%s\n",query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      exit(1);
    }

    sql_res = mysql_store_result(conn);
    nn = mysql_num_rows(sql_res);
    if (my_debug) fprintf(stderr,"NUM_ROWS0=%d\n",nn);
    if (nn == 0) {
      //fprintf(stderr,"Error: labels not found in SQL\n");
    } else {
      while (ii < nn && ii < MAX_LABEL) {

	sql_row = mysql_fetch_row(sql_res);	
	int my_id = atoi(sql_row[0]);
	label_array[ii].my_id = my_id;
	char *my_name = sql_row[1];
	label_array[ii].name = strdup(my_name);
	char *my_text = sql_row[2];
	label_array[ii].text = strdup(my_text);
	my_doc_no = atoi(sql_row[3]);
	label_array[ii].doc_id = my_doc_no;
	int para_id = atoi(sql_row[4]);
	label_array[ii].para_id = para_id;
	label_array[ii].feature = (sql_row[8]) ? strdup(sql_row[8]) : "NULL";

	doc_array[my_doc_no].doc_name = strdup(sql_row[5]);
	doc_array[my_doc_no].fd_name = strdup(sql_row[6]);
	doc_array[my_doc_no].fd1_name = (sql_row[7])? strdup(sql_row[7]):"";

	ii++;
      }
      my_label_no = ii;
    }
    return ii;
} // get_recitals_from_sql()

int read_index(FILE *index_file) {
  static char line[4000];
  if (my_debug) fprintf(stderr,"READING INDEX\n");
  int in = 0;
  largest_seq_no = 0;
  while (fgets(line,800,index_file)) {
    int seqn, pid, ln, lev_no,gn, my_enum, center, indent, too_long, my_loc, page_no, line_no, is_special;
    static char article[100];
    static char section[100];
    int a1, a2, a3, a4;
    static char clean_header[1000];
    int toc_page_no;
    int toc_page_no_type;
    int toc_page_no_coord;				

    sscanf(line,"%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%[^\t\n]\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n"
	   ,&seqn,&my_enum,&ln,&gn,&pid,&lev_no,article,section,clean_header, &indent, &too_long, &center, &my_loc, &line_no, &page_no, &a1, &a2, &a3, &is_special
	   , &toc_page_no, &toc_page_no_type, &toc_page_no_coord
	   );
    if (debug) fprintf(stderr,"\tYYY:%s:",line);
    if (debug) fprintf(stderr,"XXXX:sqn=%d\tme=%d\tln=%d\tgn=%d\tpid=%d\tlev=%d\tart=%s\tsec=%s\thead=%s\tind=%d\ttoo=%d\tcent=%d\tml=%d\tln=%d\tpn=%d\ta1=%d--\ta2=%d\ta3=%d\tis_spec=%d page=%d:%d:%d:\n"
		       , seqn,  my_enum, ln, gn, pid, lev_no,article,section,clean_header,  indent,  too_long,  center,  my_loc,  line_no,  page_no,  a1,  a2,  a3, is_special
		       , toc_page_no, toc_page_no_type, toc_page_no_coord
		       );
    if (lev_no != 10) {
	index_item_array[in].my_enum = my_enum;
	index_item_array[in].line_no = ln;
	index_item_array[in].grp_no = gn;
	index_item_array[in].seq_no = seqn;
	index_item_array[in].pid = pid;
	index_item_array[in].lev_no = lev_no;
	index_item_array[in].title = ((strcmp(article,"-")==0) ? "" : strdup(article));
	index_item_array[in].section = strdup(section);
	index_item_array[in].clean_header = strdup(clean_quotes(clean_header));
	index_item_array[in].indent = indent;
	index_item_array[in].too_long = too_long;	
	index_item_array[in].center = center;
	index_item_array[in].my_loc = my_loc;
	index_item_array[in].line_no = line_no;
	index_item_array[in].page_no = page_no;
	index_item_array[in].a1 = a1;	
	index_item_array[in].a2 = a2;
	index_item_array[in].a3 = a3;	
	index_item_array[in].is_special = is_special;				

	index_item_array[in].toc_page_no = toc_page_no;
	index_item_array[in].toc_page_no_type = toc_page_no_type;
	index_item_array[in].toc_page_no_coord = toc_page_no_coord;				
	
	in++;
	if (seqn > largest_seq_no) largest_seq_no = seqn;
	if (lev_no > 3 && pid >=0) {
	  no_good_array[pid] = 1; // no_good_paras cannot be preamble or recitals
	}
    }
  }
  return in;
} // read_index


  struct Index_Item *first_index = NULL; 
  struct Index_Item *last_index = NULL; 

  struct Index_Item *new_index_item() {
    struct Index_Item *new_index = (struct Index_Item *)malloc(sizeof(struct Index_Item));
    if (!new_index) {
      fprintf(stderr,"Error: failed to alloc new_index\n");
      exit(0);
    }
    return(new_index);
 }


  struct Index_Item *insert_index_item_prev(struct Index_Item *new_index, struct Index_Item *ptr) {
    if (ptr == NULL) {
      if (first_index == NULL) { // the list is empty
	new_index->prev = NULL;
	new_index->next = NULL;
	first_index = new_index;
	last_index = new_index;
	if (first_index == NULL) {
	  first_index = new_index;
	}
      } 
    } else {
      new_index->next = ptr;
      new_index->prev = ptr->prev;
      if (ptr->prev) {
	ptr->prev->next = new_index;
      } else {
	first_index = new_index;
      }
      ptr->prev = new_index;
    }
    return(new_index);
  }


  struct Index_Item *insert_index_item_next(struct Index_Item *new_index, struct Index_Item *ptr) {
    if (ptr == NULL) {
      if (first_index == NULL) { // the list is empty
	new_index->prev = NULL;
	new_index->next = NULL;
	first_index = new_index;
	last_index = new_index;
      } 
      if (my_debug) fprintf(stderr,"insert1: new=%d bf=%d top=%d last=%d:\n",new_index->line_no, new_index->prev->line_no, first_index->line_no, last_index->line_no);
    } else {
      new_index->prev = ptr;
      new_index->next = ptr->next;
      if (ptr->next) {
	if (my_debug) fprintf(stderr,"insert3: next exists\n");
	ptr->next->prev = new_index;
      } else {
	if (my_debug) fprintf(stderr,"insert4: no next exists\n");
	last_index = new_index;
      }
      ptr->next = new_index;
      if (my_debug) fprintf(stderr,"insert2:\n");
    }
    return(new_index);
  }

int only_spaces(char *text) {
  int ii; 
  int done;
  for (ii = 0, done = 1; ii < strlen(text) && done == 1; ii++) {
    if (text[ii] != ' ' && text[ii] != '\t') done = 0;
  }
  return done;
}

int create_linked_list_index(int index_no) {

  if (my_debug) fprintf(stderr,"CREATING LINKED LIST INDEX:%d:\n",index_no);
  int ii;
  first_index = last_index = NULL;
  for (ii = 0; ii < index_no ; ii++) { // insert in LIST
    struct Index_Item *new_index = new_index_item();
    if (first_index) {
      new_index->next = NULL;
      new_index->prev = last_index;
      last_index->next = new_index;
      last_index = new_index;
    } else {
      new_index->next = NULL;
      new_index->prev = NULL;
      first_index = new_index;
      last_index = new_index;
    }

    new_index->seq_no = index_item_array[ii].seq_no;
    new_index->indent = index_item_array[ii].indent;
    new_index->center = index_item_array[ii].center;
    new_index->page_no = index_item_array[ii].page_no;
    new_index->line_no = index_item_array[ii].line_no;            
    new_index->too_long = index_item_array[ii].too_long;
    new_index->is_special = index_item_array[ii].is_special;
    new_index->my_loc = index_item_array[ii].my_loc;
    new_index->a1 = index_item_array[ii].a1;
    new_index->a2 = index_item_array[ii].a2;
    new_index->a3 = index_item_array[ii].a3;        
    new_index->my_enum = index_item_array[ii].my_enum;
    new_index->line_no = index_item_array[ii].line_no;
    new_index->grp_no = index_item_array[ii].grp_no;
    new_index->pid = index_item_array[ii].pid;
    new_index->lev_no = index_item_array[ii].lev_no;
    new_index->title = (strlen(index_item_array[ii].title) == 0) ? "-" : strdup(index_item_array[ii].title);
    new_index->section = strdup(index_item_array[ii].section);
    new_index->clean_header = (only_spaces(index_item_array[ii].clean_header)) ? "-" : strdup(index_item_array[ii].clean_header);

    new_index->toc_page_no = index_item_array[ii].toc_page_no;
    new_index->toc_page_no_type = index_item_array[ii].toc_page_no_type;
    new_index->toc_page_no_coord = index_item_array[ii].toc_page_no_coord;
    if (my_debug) fprintf(stderr,"INDEX_ITEM ii=%d: sn=%d: enum=%d: line=%d: lev=%d: pid=%d: title=%s: section=%s: header=%s: too_long=%d: indent=%d: center=%d: page=%d:%d:%d:\n"
			  , ii,new_index->seq_no,new_index->my_enum,new_index->line_no,new_index->lev_no
			  , new_index->pid, new_index->title, new_index->section, new_index->clean_header, new_index->too_long, new_index->indent, new_index->center
			  , new_index->toc_page_no, new_index->toc_page_no_type, new_index->toc_page_no_coord
			  );
  }
  return 0;
}

int rem_index_item(struct Index_Item *ptr) {
  if (ptr->prev) ptr->prev->next = ptr->next;
  if (ptr->next) ptr->next->prev = ptr->prev;
  return 0;
}


int check_if_REM_TOC_exists(int last_pid) { // check the entire edit_index_array if there is a REM_TOC item that starts after this one 
  int kk;
  int ret = 0;
  for (kk = 0; kk < edit_index_array_no; kk++) { 
    if (edit_index_array[kk].first_pream_pid == last_pid+1 && strcmp(edit_index_array[kk].end_type,"REM_TOC") == 0) {
      ret = 1;
    }
  }
  return ret;
}

/******* take the EDIT_INDEX_ARRAY and according to the END_TYPE edit the linked list INDEX ********************/
int modify_index(int edit_index_array_no) { // F08 NN-2
  if (my_debug) fprintf(stderr,"FUNCTION 08: MODIFY_INDEX\n");
  int kk;
  if (my_debug) fprintf(stderr,"\n\nSTART EDIT ARRAY:%d:\n",edit_index_array_no);
  for (kk = 0; kk < edit_index_array_no; kk++) {
    if (my_debug) fprintf(stderr,"   Array_Item:%d: Range=%d-%d: Type=%s\n",kk,edit_index_array[kk].first_pream_pid,edit_index_array[kk].last_pream_pid,edit_index_array[kk].end_type);
  }
  if (my_debug) fprintf(stderr,"\n\n");

  int ii;
  for (kk = 0; kk < edit_index_array_no; kk++) { // FIRST take care of REM_TOC. we remove all the TOC items that referred to the MS_TOC
    int first_preamble_pid = edit_index_array[kk].first_pream_pid;
    int last_preamble_pid = edit_index_array[kk].last_pream_pid;
    char *end_type = strdup(edit_index_array[kk].end_type);
    if (my_debug) fprintf(stderr,"   PPP0:%d: %d-%d: %s\n",edit_index_array_no,edit_index_array[kk].first_pream_pid,edit_index_array[kk].last_pream_pid,edit_index_array[kk].end_type);

    if (end_type  && strcmp(end_type,"REM_TOC") == 0) {

      struct Index_Item *ptr_index = first_index;
      ii = 0;
      while (ptr_index) { 
	struct Index_Item *my_next = ptr_index->next;
	if (ptr_index->pid >= first_preamble_pid && ptr_index->pid <= last_preamble_pid) {
	  if (ptr_index->line_no > 0 && strcmp(ptr_index->title,"FILE") != 0) { // do not remove the first line with the FILE thing so not to disturb the DIV structure
	    if (my_debug) fprintf(stderr,"   REMOVING toc items inside MS_TOC:pid=%d:\n",ptr_index->pid);
	    rem_index_item(ptr_index);
	  } else {
	    if (my_debug) fprintf(stderr,"   NOT REMOVING toc items inside MS_TOC:pid=%d:\n",ptr_index->pid);
	  }
	}
	ptr_index = my_next;
	ii++;
      } // while
    } // if REM_TOC

    else if (end_type  && strcmp(end_type,"IN_REC") == 0) { // REMOVE TOC ITEMS (i.e., A. B.) inside a RECITALS
      struct Index_Item *ptr_index = first_index;
      ii = 0;
      while (ptr_index) { 
	struct Index_Item *my_next = ptr_index->next;
	if (ptr_index->pid >= first_preamble_pid && ptr_index->pid <= last_preamble_pid) {
	  rem_index_item(ptr_index);
	  if (my_debug) fprintf(stderr,"   REMOVING toc items inside RECITALS:pid=%d:\n",ptr_index->pid);
	}
	ptr_index = my_next;
	ii++;
      } // while
    } // if IN_REC

    else if (end_type  && strcmp(end_type,"SIG") == 0) { // do SIGBLOCK
      struct Index_Item *ptr_index = first_index;

      ii = 0;
      int done = 0;
      while (ptr_index) { 
	struct Index_Item *my_next = ptr_index->next;
	if (done == 0 && ptr_index->pid >= first_preamble_pid) {
	  struct Index_Item *new_index = new_index_item();
	  insert_index_item_prev(new_index,ptr_index); 
	  new_index->seq_no = ++largest_seq_no;
	  new_index->line_no = 0;
	  new_index->grp_no = 0;
	  new_index->pid = first_preamble_pid;
	  new_index->lev_no = MAX(ptr_index->lev_no,3); 
	  new_index->title = "_";
	  new_index->section = "0.";
	  new_index->clean_header = "Signature Block";
	  done = 1;
	  if (my_debug)  fprintf(stderr,"   CREATING a Signature Block (SIG):pid=%d lev=%d:\n",first_preamble_pid,ptr_index->lev_no);
	}

	ptr_index = my_next;
	ii++;
      } // while
    } // if SIG
  } // for look kk -- for each item in the edit_index_array


  for (kk = 0; kk < edit_index_array_no; kk++) { // SECOND, we take care of marking up PREAMBLE, RECITALS, and TOC
    int first_preamble_pid = edit_index_array[kk].first_pream_pid;
    int last_preamble_pid = edit_index_array[kk].last_pream_pid;
    char *end_type = strdup(edit_index_array[kk].end_type);

    if (my_debug) fprintf(stderr,"   PPP1:%d: %d-%d: %s\n",edit_index_array_no,edit_index_array[kk].first_pream_pid,edit_index_array[kk].last_pream_pid,edit_index_array[kk].end_type);

    if (end_type  && (strcmp(end_type,"REM_TOC") == 0 || strcmp(end_type,"SIG") == 0 || strcmp(end_type,"IN_REC") == 0)) {
      ; // DO nothing
    } else { // not REM_TOC
      if (last_preamble_pid < first_preamble_pid) {
	fprintf(stderr,"Warning: last (%d) is smaller than first (%d). Fixing it for now!\n",last_preamble_pid,first_preamble_pid); 
	last_preamble_pid = first_preamble_pid;
      }

      int done_first = 0;
      int done_last = 0;
      int first_level = -1;

      struct Index_Item *ptr_index = first_index;
      ii = 0;

      if (ptr_index==NULL && done_last == 0) { // if it's an empty TOC
	  struct Index_Item *new_index = new_index_item();
	  insert_index_item_prev(new_index,ptr_index);
	  new_index->seq_no = ++largest_seq_no;
	  new_index->line_no = 0;
	  new_index->grp_no = 0;
	  new_index->pid = first_preamble_pid;
	  new_index->lev_no = 3; // assign the same level as the next in the list, but not 2 which is the FILE level
	  first_level = 3;
	  new_index->title = "_";
	  new_index->section = "0.";
	  new_index->clean_header = "Preamble";
	  done_first = 1;
	  if (my_debug) fprintf(stderr,"   CREATING a new PREAMBLE: pid=%d lev=%d:\n",first_preamble_pid,new_index->lev_no);


      } else while (ptr_index && done_last == 0) { // insert PREAM by the first item "first_pream_pid"
	struct Index_Item *my_next = (ptr_index) ? ptr_index->next : NULL;
	if (done_first == 0 && first_preamble_pid > -1 && ptr_index && ptr_index->pid > first_preamble_pid) { // EASY CASE: insert pream BEFORE the first TOC item
	  struct Index_Item *new_index = new_index_item();
	  insert_index_item_prev(new_index,ptr_index);
	  new_index->seq_no = ++largest_seq_no;
	  new_index->line_no = 0;
	  new_index->grp_no = 0;
	  new_index->pid = first_preamble_pid;
	  new_index->lev_no = MAX(ptr_index->lev_no,3); // assign the same level as the next in the list, but not 2 which is the FILE level
	  first_level = MAX(ptr_index->lev_no,3);
	  new_index->title = "_";
	  new_index->section = "0.";
	  new_index->clean_header = "Preamble";
	  done_first = 1;
	  if (my_debug) fprintf(stderr,"   CREATING a new PREAMBLE: pid=%d lev=%d:\n",first_preamble_pid,new_index->lev_no);
	}  else if (done_first == 0 && first_preamble_pid > -1 && ptr_index && ptr_index->pid == first_preamble_pid) { // TOUGH CASE: pream BEFORE the first TOC item
	  if (1) { // in case this is item 0 on the list then insert is as 1 (after 0) -- let's see it doesn't cause problems UZ 092313
	    struct Index_Item *new_index = new_index_item();
	    insert_index_item_prev(new_index,ptr_index->next);
	    new_index->seq_no = ++largest_seq_no;
	    new_index->line_no = 0;
	    new_index->grp_no = 0;
	    new_index->pid = first_preamble_pid;
	    new_index->lev_no = MAX(ptr_index->lev_no,3); // assign the same level as the next in the list, but not 2 which is the FILE level
	    first_level = MAX(ptr_index->lev_no,3);
	    new_index->title = "_";
	    new_index->section = "0.";
	    new_index->clean_header = "Preamble";
	    done_first = 1;
	    if (my_debug) fprintf(stderr,"   CREATING a new PREAMBLE: pid=%d lev=%d:\n",first_preamble_pid,new_index->lev_no);
	  } else {
	    ptr_index->pid = first_preamble_pid;
	    //ptr_index->lev_no = MAX(ptr_index->lev_no,3); // assign the same level as the next in the list
	    first_level = MAX(ptr_index->lev_no,3);
	    ptr_index->title = "_";
	    ptr_index->section = "0.";
	    ptr_index->clean_header = "Preamble";
	    done_first = 1;
	    if (my_debug) fprintf(stderr,"   MODIFYING existing TOC item to make a PREAM: pid=%d lev=%d:\n",first_preamble_pid,ptr_index->lev_no);
	  }
	}  else { // if (ptr_index == NULL) {
	  ;
	}

	ptr_index = my_next;
	ii++;
      } // while

      ptr_index = first_index;
      ii = 0;

      // deal with the last entry: insert (or fix) one PREAM item in TOC
      while (ptr_index && done_first == 1 && done_last == 0) { // insert RECITALS OR TOC by the first item "first_pream_pid"
	struct Index_Item *my_next = ptr_index->next;
	if (done_last == 0 && last_preamble_pid > -1 && ptr_index->pid > last_preamble_pid+1) { // EASY CASE: insert pream BEFORE the first TOC item
	  int found_REM_TOC = check_if_REM_TOC_exists(last_preamble_pid);
	  struct Index_Item *new_index = new_index_item();
	  insert_index_item_prev(new_index,ptr_index);
	  new_index->seq_no = ++largest_seq_no;
	  new_index->line_no = 0;
	  new_index->grp_no = 0;
	  new_index->pid = last_preamble_pid+1;
	  new_index->lev_no = first_level; // assign the same level as the PREAM
	  new_index->title = "_";
	  new_index->section = "0.";
	  // if there is a REM_TOC block after this PREAM then override the default REC title and insert TOC title for the block
	  new_index->clean_header = (found_REM_TOC || (end_type && strcmp(end_type,"TOC") == 0)) ? "Table Of Contents" : "Recitals"; //"Recitals";
	  done_last = 1;
	  if (my_debug) fprintf(stderr,"   CREATING a RECitals item at the end of PREAM: pid=%d lev=%d: end_type=%s:\n",last_preamble_pid+1,new_index->lev_no, end_type);
	}  else if (done_last == 0 && last_preamble_pid > -1 && ptr_index->pid == last_preamble_pid+1) { //DO NOTHING!!!
	  done_last = 1;
	} else if (done_last == 0  && last_preamble_pid > -1 && ptr_index->pid > last_preamble_pid + 10) { // don't let it go more than 10 after pream started.
	  struct Index_Item *new_index = new_index_item();
	  insert_index_item_next(new_index,ptr_index);
	  new_index->seq_no = ++largest_seq_no;
	  new_index->line_no = 0;
	  new_index->grp_no = 0;
	  new_index->pid = last_preamble_pid+1;
	  new_index->lev_no = first_level; // assign the same level as the PREAM
	  new_index->title = "_";
	  new_index->section = "0.";
	  new_index->clean_header = (end_type && strcmp(end_type,"TOC") == 0) ? "Table Of Contents" : "Recitals"; //"Recitals";
	  done_last = 1;
	  if (my_debug) fprintf(stderr,"   CREATING REC or TOC (%s) at the end of PREAM: pid=%d lev=%d:\n",new_index->clean_header,last_preamble_pid+1,new_index->lev_no);
	}  else {
	  ; // do nothing, the next level will take care
	}
	ptr_index = my_next;
	ii++;
      } // while 
    }
  } // for kk < edit_index_array_no
  if (my_debug) fprintf(stderr,"END EDIT ARRAY:%d:\n",ii);
  return ii;
} // modify


int write_index(int in) {  // function # 10 NN
  if (my_debug) fprintf(stderr,"FUNCTION 10: WRITE_INDEX\n");
  static char out_index_name[100];
  sprintf(out_index_name,"%s.out",index_name);
  FILE *out_index_file;
  out_index_file = fopen(out_index_name,"w");

  // print it out from LIST
  struct Index_Item *ptr_index = first_index;
  int ii = 0;
  FILE *my_file = stdout;  
  my_file =   out_index_file;
  if (debug)fprintf(stderr,"\nPRINTING NEW INDEX\n");
  while (ptr_index) {
    fprintf(my_file,"%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n"
	    ,ptr_index->seq_no
	    ,ptr_index->my_enum

	    ,ptr_index->line_no

	    ,ptr_index->grp_no
	    ,ptr_index->pid
	    ,ptr_index->lev_no
	    ,(strlen(ptr_index->title)>0) ? ptr_index->title : "-"
	    ,(strlen(ptr_index->section) > 0) ? ptr_index->section : "0"
	    ,(strlen(ptr_index->clean_header) > 0) ? ptr_index->clean_header : "-"
	    ,ptr_index->indent
	    ,ptr_index->too_long	    
	    ,ptr_index->center	
	    ,ptr_index->my_loc	    		           
	    ,ptr_index->line_no
	    
	    ,ptr_index->page_no

	    ,ptr_index->a1
	    ,ptr_index->a2
	    ,ptr_index->a3		       		       
	    ,ptr_index->is_special	    	    

	    ,ptr_index->toc_page_no
	    ,ptr_index->toc_page_no_type	       		       
	    ,ptr_index->toc_page_no_coord
	    
	    );
    
    if (debug) fprintf(stderr,"RRR: ---%d\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d a1=%d a2=%d a3=%d spec=%d: page=%d:%d:%d:\n"
	    ,ptr_index->seq_no
	    ,ptr_index->my_enum
	    ,ptr_index->line_no
	    ,ptr_index->grp_no
	    ,ptr_index->pid
	    ,ptr_index->lev_no
	    ,(strlen(ptr_index->title)>0) ? ptr_index->title : "-"
	    ,(strlen(ptr_index->section) > 0) ? ptr_index->section : "0"
	    ,(strlen(ptr_index->clean_header) > 0) ? ptr_index->clean_header : "-"
	    ,ptr_index->indent
	    ,ptr_index->too_long	    		       
	    ,ptr_index->center
	    ,ptr_index->my_loc	    		       
	    ,ptr_index->line_no
	    ,ptr_index->page_no
	    ,ptr_index->a1
	    ,ptr_index->a2
	    ,ptr_index->a3		       		       
	    ,ptr_index->is_special	    	    		       

	    ,ptr_index->toc_page_no
	    ,ptr_index->toc_page_no_type	       		       
	    ,ptr_index->toc_page_no_coord

	    );

    ii++;
    ptr_index = ptr_index->next;
  }
  if (my_debug) fprintf(stderr,"DONE WRITING INDEX:%d\n",in);
  return 0;
} // write_index

int insert_parts_into_sql(MYSQL *conn, int doc_id) { // function 9 NN-1
  if (my_debug) fprintf(stderr,"FUNCTION 09: INSERT_PARTS_INTO_SQL\n");
  //MYSQL_RES *sql_res;
  //MYSQL_ROW sql_row;

  static char query[200000];
  static char buff[2000];
  int org_id[2];
  int folder_id = get_did_oid(conn, doc_id, org_id);
  int affected;
  sprintf(query,"delete from deals_selected_doc_part \n\
                     where doc_id = \'%d\' "
	  , doc_id);

  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
  }

  affected = mysql_affected_rows(conn);

  if (my_debug) fprintf(stderr,"Info: Done delete selected_doc_part: doc_id=%d: affected=%d:\n",doc_id, affected);

  char clean_toc_query[400000];
  sprintf(query,"insert into deals_selected_doc_part \n\
           (doc_id, organization_id, folder_id, begin_para, end_para, name, insertion_date) \n\
           values "); 

  int ii;

  strcpy(clean_toc_query,"");
  //fprintf(stderr,"PID=%d:\n",pid);
  for (ii = 0; ii < edit_index_array_no; ii++) {
    int bpid, epid;
    char *name;
    char cc = (ii == edit_index_array_no-1) ? ' ' : ',';
    if (strcmp(edit_index_array[ii].end_type,"TOC") == 0) {
      epid = edit_index_array[ii].last_pream_pid;
      bpid = edit_index_array[ii].first_pream_pid;
      name = "PREAM";
    } else if (strcmp(edit_index_array[ii].end_type,"REC") == 0) {
      epid = edit_index_array[ii].last_pream_pid;
      bpid = edit_index_array[ii].first_pream_pid;
      name = "PREAM";
      /******************if the pream ends in REC then it doesn't have a separate edit array entry ****************/
      /*
      ii++;
      epid++;
      bpid = epid;
      name = "REC";
      */
    } else if (strcmp(edit_index_array[ii].end_type,"REM_TOC") == 0) {
      epid = edit_index_array[ii].last_pream_pid;
      bpid = edit_index_array[ii].first_pream_pid;
      name = "TOC";
    } else if (strcmp(edit_index_array[ii].end_type,"SIG") == 0) {
      epid = edit_index_array[ii].last_pream_pid;
      bpid = edit_index_array[ii].first_pream_pid;
      name = "SIG";
    }

    sprintf(buff,"(%d, %d, %d, %d, %d, \'%s\', now())%c ",doc_id, org_id[0], folder_id, bpid, epid, name,cc);
    strcat(clean_toc_query,buff);
  }
  strcat(query,clean_toc_query); 

  if (my_debug) fprintf(stderr,"QUERY4=%s\n",query);
  if (edit_index_array_no > 0) {
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
    }
    affected = mysql_affected_rows(conn);
    fprintf(stderr,"Info: Done insert selected_doc_part: doc_id=%d: affected=%d:\n",doc_id, affected);
  }

  return 0;
}
