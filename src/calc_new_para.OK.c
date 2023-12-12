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

/**********************************

  ** 1. distance to next line
  ** 4. section (1.3, (b), etc)
  ** 2. start_para indentation 
  ** 3. short-line before
  ** 5. centered header
  ** 6. short lines, indented
  ** 7. charts??

 *********************************/


int detect_toc_section(int bk) { // don't mark items that come after TOC sections
  int ret = 0;
  char *text = block_array[bk].text;
  if (!text) {
    fprintf(stderr,"Error: text is NULL\n");
    return 0;
  }

  char ww0[20]; char cc0; int dd0;
  char ww1[20]; char cc1; int dd1;
  char ww2[20]; char cc2; int dd2;
  char ww3[20]; char cc3; int dd3;
  char ww4[20]; char cc4; int dd4;
  char ww5[20]; char cc5; int dd5; 
  char ww6[60]; char cc6; int dd6;
  char ww7[20]; char cc7; int dd7;
  char ww8[20]; char cc8; int dd8;
  char ww9[20]; char cc9; int dd9;
  char ww51[20]; char cc51; int dd51;          

  char ww20[20]; char cc20; int dd20;
  char ww30[20]; char cc30; int dd30;
  char ww40[20]; char cc40; int dd40;
  char ww50[20]; char cc50; int dd50;
  char ww60[20]; char cc60; int dd60;
  char ww70[20]; char cc70; int dd70;
  char ww80[20]; char cc80; int dd80;
  char ww90[20]; char cc90; int dd90;    

  strcpy(ww51,"");

  int nn0 = sscanf(text,"%d%c", &dd0, &cc0); // 1.
  int nn1 = sscanf(text,"%[ixvl]%c", ww1, &cc1); // ixv.
  int nn2 = sscanf(text,"%[IXVL]%c", ww2, &cc2); // IXV.
  int nn3 = sscanf(text,"%c%c", ww3, &cc3); // A., a., 1.  A- a) 1)
  int nn4 = sscanf(text,"(%[ixlv]%c", ww4, &cc4); // (ixl)
  int nn5 = sscanf(text,"(%d%c", &dd5, &cc5); // (12)
  int nn51 = sscanf(text,"(%[a-zA-Z]%c", ww51, &cc51); // (a)    
  
  int mm20 = sscanf(text,"Section %d", &dd20);  // Section 1
  int mm21 = sscanf(text,"Section %[IXVL]", ww20);  // Section IX
  int mm22 = sscanf(text,"Section %c%c", ww20, &cc20);  // Section B.

  int MM120 = sscanf(text,"SECTION %d", &dd20);  // Section 1
  int MM121 = sscanf(text,"SECTION %[IXVL]", ww20);  // Section IX
  int MM122 = sscanf(text,"SECTION %c%c", ww20, &cc20);  // Section B.

  int mm30 = sscanf(text,"Article %d", &dd30);  // Article 1
  int mm31 = sscanf(text,"Article %[IXVL]", ww30);  // Article IX
  int mm32 = sscanf(text,"Article %c%c", ww30, &cc30);  // Article B.

  int MM130 = sscanf(text,"ARTICLE %d", &dd30);  // Article 1
  int MM131 = sscanf(text,"ARTICLE %[IXVL]", ww30);  // Article IX
  int MM132 = sscanf(text,"ARTICLE %c%c", ww30, &cc30);  // Article B.

  int mm40 = sscanf(text,"Schedule %d", &dd40);  // Schedule 1
  int mm41 = sscanf(text,"Schedule %[IXVL]", ww40);  // Schedule IX
  int mm42 = sscanf(text,"Schedule %c%c", ww40, &cc40);  // Schedule B.

  int MM140 = sscanf(text,"SCHEDULE %d", &dd40);  // Schedule 1
  int MM141 = sscanf(text,"SCHEDULE %[IXVL]", ww40);  // Schedule IX
  int MM142 = sscanf(text,"SCHEDULE %c%c", ww40, &cc40);  // Schedule B.

  int mm50 = sscanf(text,"Part %d", &dd50);  // Part 1
  int mm51 = sscanf(text,"Part %[IXVL]", ww50);  // Part IX
  int mm52 = sscanf(text,"Part %c%c", ww50, &cc50);  // Part B.

  int MM150 = sscanf(text,"PART %d", &dd50);  // Part 1
  int MM151 = sscanf(text,"PART %[IXVL]", ww50);  // Part IX
  int MM152 = sscanf(text,"PART %c%c", ww50, &cc50);  // Part B.
  
  int mm60 = sscanf(text,"Amendment %d", &dd60);  // Amendment 1
  int mm61 = sscanf(text,"Amendment %[IXVL]", ww60);  // Amendment IX
  int mm62 = sscanf(text,"Amendment %c%c", ww60, &cc60);  // Amendment B.

  int MM160 = sscanf(text,"AMENDMENT %d", &dd60);  // Amendment 1
  int MM161 = sscanf(text,"AMENDMENT %[IXVL]", ww60);  // Amendment IX
  int MM162 = sscanf(text,"AMENDMENT %c%c", ww60, &cc60);  // Amendment B.

  int mm70 = sscanf(text,"Exhibit %d", &dd70);  // Exhibit 1
  int mm71 = sscanf(text,"Exhibit %[IXVL]", ww70);  // Exhibit IX
  int mm72 = sscanf(text,"Exhibit %c%c", ww70, &cc70);  // Exhibit B.

  int MM170 = sscanf(text,"EXHIBIT %d", &dd70);  // Exhibit 1
  int MM171 = sscanf(text,"EXHIBIT %[IXVL]", ww70);  // Exhibit IX
  int MM172 = sscanf(text,"EXHIBIT %c%c", ww70, &cc70);  // Exhibit B.

  int mm80 = sscanf(text,"Attachment %d", &dd80);  // Attachment 1
  int mm81 = sscanf(text,"Attachment %[IXVL]", ww80);  // Attachment IX
  int mm82 = sscanf(text,"Attachment %c%c", ww80, &cc80);  // Attachment B.

  int MM180 = sscanf(text,"ATTACHMENT %d", &dd80);  // Attachment 1
  int MM181 = sscanf(text,"ATTACHMENT %[IXVL]", ww80);  // Attachment IX
  int MM182 = sscanf(text,"ATTACHMENT %c%c", ww80, &cc80);  // Attachment B.

  int mm90 = sscanf(text,"Paragraph %d", &dd90);  // Paragraph 1
  int mm91 = sscanf(text,"Paragraph %[IXVL]", ww90);  // Paragraph IX
  int mm92 = sscanf(text,"Paragraph %c%c", ww90, &cc90);  // Paragraph B.

  int MM190 = sscanf(text,"PARAGRAPH %d", &dd90);  // Paragraph 1
  int MM191 = sscanf(text,"PARAGRAPH %[IXVL]", ww90);  // Paragraph IX
  int MM192 = sscanf(text,"PARAGRAPH %c%c", ww90, &cc90);  // Paragraph B.

  
  int XX = (strncmp(text,"EXHIBIT",7) == 0 || strncmp(text,"Exhibit",7) == 0 || strncmp(text,"LR",2) == 0 || strncmp(text,"RF",2) == 0);
  
  if (nn0 == 2 && dd0 > 0 && dd0 < 100 && cc0 == '.') ret = 1;
  if (nn1 == 2 && cc1 == '.') ret = 1;
  if (nn2 == 2 && cc2 == '.') ret = 1;
  if ((nn3 == 2 && (cc3 == '.' || cc3 == '-'  || cc3 == ')'))) /*|| cc3 == ' '  || cc3 == '\0'))  || (nn3 == 1 && strlen(text) == 1))*/ ret = 1;
  if (nn4 == 2 && cc4 == ')') ret = 1;    
  if (nn5 == 2 && dd5 > 0 && dd5 < 100 && cc5 == ')') ret = 1;
  if (nn51 == 2 && strlen(ww51) > 0 && cc51 == ')') ret = 1;  
  /*********************************************************/  
  if (mm20 == 1 && dd20 > 0 && dd20 < 100) ret = 1;
  if (mm21 == 2) ret = 1;
  if (mm22 == 2 && cc20 == '.') ret = 1;          

  if (MM120 == 1 && dd20 > 0 && dd20 < 100) ret = 1;
  if (MM121 == 2) ret = 1;
  if (MM122 == 2 && cc20 == '.') ret = 1;          
  /*********************************************************/  
  if (mm30 == 1 && dd30 > 0 && dd30 < 100) ret = 1;
  if (mm31 == 2) ret = 1;
  if (mm32 == 2 && cc30 == '.') ret = 1;          

  if (MM130 == 1 && dd30 > 0 && dd30 < 100) ret = 1;
  if (MM131 == 2) ret = 1;
  if (MM132 == 2 && cc30 == '.') ret = 1;          


    /*********************************************************/  
  if (mm40 == 1 && dd40 > 0 && dd40 < 100) ret = 1;
  if (mm41 == 2) ret = 1;
  if (mm42 == 2 && cc40 == '.') ret = 1;          

  if (MM140 == 1 && dd40 > 0 && dd40 < 100) ret = 1;
  if (MM141 == 2) ret = 1;
  if (MM142 == 2 && cc40 == '.') ret = 1;          

  /*********************************************************/  
  if (mm50 == 1 && dd50 > 0 && dd50 < 100) ret = 1;
  if (mm51 == 2) ret = 1;
  if (mm52 == 2 && cc50 == '.') ret = 1;          

  if (MM150 == 1 && dd50 > 0 && dd50 < 100) ret = 1;
  if (MM151 == 2) ret = 1;
  if (MM152 == 2 && cc50 == '.') ret = 1;          

  /*********************************************************/  
  if (mm60 == 1 && dd60 > 0 && dd60 < 100) ret = 1;
  if (mm61 == 2) ret = 1;
  if (mm62 == 2 && cc60 == '.') ret = 1;          

  if (MM160 == 1 && dd60 > 0 && dd60 < 100) ret = 1;
  if (MM161 == 2) ret = 1;
  if (MM162 == 2 && cc60 == '.') ret = 1;          

  /*********************************************************/  

  if (mm70 == 1 && dd70 > 0 && dd70 < 100) ret = 1;
  if (mm71 == 2) ret = 1;
  if (mm72 == 2 && cc70 == '.') ret = 1;          

  if (MM170 == 1 && dd70 > 0 && dd70 < 100) ret = 1;
  if (MM171 == 2) ret = 1;
  if (MM172 == 2 && cc70 == '.') ret = 1;          

  /*********************************************************/  

  if (mm80 == 1 && dd80 > 0 && dd80 < 100) ret = 1;
  if (mm81 == 2) ret = 1;
  if (mm82 == 2 && cc80 == '.') ret = 1;          

  if (MM180 == 1 && dd80 > 0 && dd80 < 100) ret = 1;
  if (MM181 == 2) ret = 1;
  if (MM182 == 2 && cc80 == '.') ret = 1;          

  /*********************************************************/  

  if (mm90 == 1 && dd90 > 0 && dd90 < 100) ret = 1;
  if (mm91 == 2) ret = 1;
  if (mm92 == 2 && cc90 == '.') ret = 1;          

  if (MM190 == 1 && dd90 > 0 && dd90 < 100) ret = 1;
  if (MM191 == 2) ret = 1;
  if (MM192 == 2 && cc90 == '.') ret = 1;          
  
  
  if (XX == 1) ret = 1; // exhibit

  if (0) fprintf(stderr,"TTTNEW: ret=%d:  XX=%d: 0:%d: 1:%d: 2:%d: 3:%d:%c: 4:%d: 5:%d: 20:%d: 21:%d: 22:%d: 120:%d : 121:%d: 122:%d:     30:%d: 31:%d: 32:%d: 130:%d : 131:%d: 132:%d:       t=%s:\n"
		 , ret, XX, nn0, nn1, nn2,  nn3,cc3,  nn4, nn5
		 ,        mm20, mm21, mm22,   MM120, MM121, MM122
		 ,        mm30, mm31, mm32,   MM130, MM131, MM132
		 , text);  
  return ret; 
} // detect_toc_section()



int insert_spaces(int last_block_no) {
  int bk, ii;
  for (bk = 0, ii = 0; bk <= last_block_no; bk++, ii++) {
    int y2_prev = (bk > 0) ? block_array[bk-1].my_y2 : -1;
    int y2_curr = block_array[bk].my_y2;
    int y1_curr = block_array[bk].my_y1;
    int page = block_array[bk].page;
    char *text = block_array[bk].text;
    int delta = y1_curr - y2_prev;
    int fw =  block_array[bk].first_word;
    int line =  block_array[bk].my_line_in_page_renumbered;    

    if (delta > DELTA_VERT_FOR_PARA) {
      space_array[space_no].token_no = fw;
      space_array[space_no].page_no = page;
      space_array[space_no].line_no = line;
      space_array[space_no].space_size = delta;      
      space_no++;
    }
  }
  return space_no;
} // insert_spaces(int last_block_no) 

int found_herein(int bk) {
  int ret = 0;
  return ret;
}


// found that the end of hatstack matches needle!!!
char *strrstr(char haystack[], char needle[]) {
  char *pp;
  int ii;
  if (!haystack || strlen(haystack) < strlen(needle)) {
    pp = NULL;
  } else {
    pp = haystack + ((haystack) ? strlen(haystack) : 0);
    for (ii = 0; ii < strlen(needle) && ii < strlen(haystack) && pp != NULL; ii++) {
      if (pp[0] == needle[strlen(needle) - ii]) {
	pp--;
      } else {
	pp = NULL;
      }
    }
  }
  //fprintf(stderr,"   SSD: :%s: %d:%s:  %d:%d:\n", pp,  strlen(haystack),haystack,  strlen(needle), needle);
  return pp;
}

// in_section [\n\ \,\.\;](In|in|This|this|under|Under|of|Of|with|With|to|To|On|on|The|the|an|attached|any|all|for|thereto|See|see|and|or|as)
int found_in_section(int bk) { // ends with a herein word
  int ret0 = 0;
  int ret = 0;  

  if (bk > 0) {
    char *pp = NULL;
    if (1) ret0 = (
	   (pp = strrstr(block_array[bk].text, " in")) == NULL
	   && (pp = strrstr(block_array[bk].text, " In")) == NULL
	   && (pp = strrstr(block_array[bk].text, " On")) == NULL
	   && (pp = strrstr(block_array[bk].text, " on")) == NULL
	   && (pp = strrstr(block_array[bk].text, " This")) == NULL                  
	   && (pp = strrstr(block_array[bk].text, " this")) == NULL
	   && (pp = strrstr(block_array[bk].text, " under")) == NULL
	   && (pp = strrstr(block_array[bk].text, " Under")) == NULL
	   && (pp = strrstr(block_array[bk].text, " of")) == NULL                  
	   && (pp = strrstr(block_array[bk].text, " Of")) == NULL
	   && (pp = strrstr(block_array[bk].text, " with")) == NULL
	   && (pp = strrstr(block_array[bk].text, " With")) == NULL
	   && (pp = strrstr(block_array[bk].text, " to")) == NULL                  
	   && (pp = strrstr(block_array[bk].text, " To")) == NULL
	   && (pp = strrstr(block_array[bk].text, " The")) == NULL
	   && (pp = strrstr(block_array[bk].text, " the")) == NULL
	   && (pp = strrstr(block_array[bk].text, " an")) == NULL                  
	   && (pp = strrstr(block_array[bk].text, " attached")) == NULL
	   && (pp = strrstr(block_array[bk].text, " Attached")) == NULL
	   && (pp = strrstr(block_array[bk].text, " any")) == NULL
	   && (pp = strrstr(block_array[bk].text, " Any")) == NULL      
	   && (pp = strrstr(block_array[bk].text, " all")) == NULL
	   && (pp = strrstr(block_array[bk].text, " All")) == NULL      
	   && (pp = strrstr(block_array[bk].text, " for")) == NULL
	   && (pp = strrstr(block_array[bk].text, " For")) == NULL                        
	   && (pp = strrstr(block_array[bk].text, " thereto")) == NULL
	   && (pp = strrstr(block_array[bk].text, " Thereto")) == NULL      
	   && (pp = strrstr(block_array[bk].text, " see")) == NULL
	   && (pp = strrstr(block_array[bk].text, " See")) == NULL      
	   && (pp = strrstr(block_array[bk].text, " and")) == NULL
	   && (pp = strrstr(block_array[bk].text, " or")) == NULL      
	   && (pp = strrstr(block_array[bk].text, " as")) == NULL
	   ) ? 0 : 1;
    if (1) fprintf(stderr,"SSS:%d:     :%s:     :%s:\n", ret0,  pp,   block_array[bk].text);
  }
  return ret0;
}


int insert_new_paras(int last_block_no) {
  /* this is based:
  ** 1. distance to next line
  ** 4. section (1.3, (b), etc)
  ** 2. start_para indentation  -- not yet
  ** 3. short-line before
  ** 5. centered header
  ** 6. short lines, indented
  ** 7. charts??
   */
  
  int bk;
  fprintf(stderr,"*** DOING VERT_DIST total_blocks=%d:\n", last_block_no);
  int last_page = -1;
  curr_para = -1;
  for (bk = 0; bk <= last_block_no; bk++) {
    block_array[bk].para = -1;
  }
  int ii;
  // do all the blocks that are at the start of the new para
  for (bk = 0, ii = 0; bk <= last_block_no; bk++, ii++) {
    int y1_prev = (bk > 0) ? block_array[bk-1].my_y1 : -1;
    int y2_prev = (bk > 0) ? block_array[bk-1].my_y2 : -1;
    int y2_curr = block_array[bk].my_y2;
    int y1_curr = block_array[bk].my_y1;
    int page = block_array[bk].page;
    int my_line_in_page = block_array[bk].my_line_in_page; 
    char *text = block_array[bk].text;
    int delta = y1_curr - y2_prev;
    int found_toc_section = detect_toc_section(bk);

    fprintf(stderr,"TRAV VERT_DIST ii=%d: para=%d: pp=%d:%d: ln=%d: bk=%d: renum=%d:   y12_curr=%d:%d:%d: y12_prev=%d:%d:%d: fl=%d:%d: ftoc=%d:%d:%d: 2-1=%d: D=%d:%d: t=%s: last_t=%s:\n\n"
	    , ii, curr_para, page, last_page,   my_line_in_page, bk, block_array[bk-1].renumbered
	    , y1_curr, y2_curr,  y2_curr - y1_curr, y1_prev, y2_prev,  y2_prev - y1_prev
	    , block_array[bk].first_word, block_array[bk].last_word
	    , found_toc_section, found_herein(bk), found_in_section(bk-1)
	    , y1_curr - y2_prev, delta, DELTA_VERT_FOR_PARA
	    , text, block_array[bk-1].text);

    if (block_array[bk].renumbered >= 0
	&& (delta > DELTA_VERT_FOR_PARA
	    || last_page < page
	    || ((delta > 0 || delta < -400)
		&& ( // make sure (1) either a new line or (2) a first line in an RHS block
		    found_toc_section == 1
		    && found_herein(bk) == 0
		    && found_in_section(bk-1) == 0
		     )
	    	)
	    )
	) {
      fprintf(stderr,"****LLL0:%d:\n", curr_para);
      curr_para++;      
      int fw = block_array[bk].first_word;
      int lw = block_array[bk].last_word;      

      para_array[curr_para].token = fw;
      para_array[curr_para].page = page;
      para_array[curr_para].line = my_line_in_page;      
      para_array[curr_para].doc = doc_id;
      para_array[curr_para].source_program = "AWS";
      para_array[curr_para-1].num_tokens = para_array[curr_para].token - para_array[curr_para-1].token;

      fprintf(stderr,"IIIIIIIIII:  pid=%d: tid=%d: pg=%d: text=%s:\n", curr_para, fw, page, word_array[fw].text);
      block_array[bk].para = curr_para;      
      int bb;
      for (bb = bk-1; block_array[bb].para == -1 && bb > 0; bb--) {
	block_array[bb].para = curr_para-1;
      }
    } // bigger than VERT_DIST
    if (block_array[bk].renumbered >= 0) last_page = page;
  } // for bk
  int bb;
  for (bb = bk-1; block_array[bb].para == -1 && bb > 0; bb--) {
    block_array[bb].para = curr_para;
  }
  para_array[curr_para+1].token = block_array[bk-1].last_word+1;
  para_array[curr_para].num_tokens = para_array[curr_para+1].token - para_array[curr_para].token;
  fprintf(stderr,"LLLLLLL:cp=%d: tok=%d:\n",curr_para, para_array[curr_para].token);
  for (bk = 0; bk <= last_block_no; bk++) {
    int fw = block_array[bk].first_word;
    int lw = block_array[bk].last_word;      
    int ww;
    for (ww = fw; ww <= lw; ww++) {
      if (word_array[ww].renumbered >= 0) word_array[ww].para = block_array[bk].para;
    }
  }  
  return curr_para;
} // insert_new_paras(int last_page_no)



int get_org_id(int doc_id) {
  int ret = 0;
  static char query[300];
  sprintf(query,"select organization_id \n\
                     from deals_document \n\
                     where id = %d "
	  , doc_id);
  if (debug) fprintf(stderr,"QUERY19a (%s): query=%s:\n",prog,query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY19a (%s): query=%s:\n",prog,query);
    exit(1);
  }
  sql_res = mysql_store_result(conn);
  if ((sql_row = mysql_fetch_row(sql_res))) {
    ret = atoi(sql_row[0]);
  }

  return ret;
}

int is_all_upper(int para) {
  int ii;
  int is_upper = 1;
  char all_text[500000];
  strcpy(all_text,"");
  for (ii = para_array[para].token; ii <= para_array[para].token + para_array[para].num_tokens; ii++) {
    char *text = (word_array[ii].text) ? word_array[ii].text : "";
    fprintf(stderr,"   ISSI:%d:%s:\n",ii, text);
    strcat(all_text," ");
    strcat(all_text,text);
    int cc;
    for (cc = 0; cc < strlen(text); cc++) {
      if (isalpha(text[cc]) != 0 && islower(text[cc]) != 0) {
	is_upper = 0;
      }
    }
  }
  fprintf(stderr,"ISU:%d:%d:  :%d:%d:  %s:\n",is_upper, para, para_array[para].token, para_array[para].num_tokens, all_text);
  return is_upper;
}

int insert_paras_into_sql(int last_para, int space_no, int doc_id, int org_id) {
  int pp;
  char query[1000000];

  /****************************************** paragraphtoken ************************************************/

  
  sprintf(query,"delete from deals_paragraphtoken \n\
                 where doc_id = %d /* and source_program = 'aws' */ "
	 , doc_id);
    
  if (debug) fprintf(stderr,"QUERY92A: query=%s:\n", query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY92A: query=%s: \n", query);
  }

  sprintf(query,"insert into deals_paragraphtoken \n\
                (para_no, token_id, num_tokens, doc_id, organization_id, page_no, line_no, source_program) \n\
                values \n");

  for (pp = 0; pp <= last_para; pp++) {
    char buff[5000];
    sprintf(buff,"(%d, %d, %d, %d, %d, %d, %d, '%s'), \n"
	    , pp
	    , para_array[pp].token
	    , para_array[pp].num_tokens
	    , doc_id
	    , org_id
	    , para_array[pp].page
	    , para_array[pp].line	    
	    , "AWS"
	    );
    strcat(query,buff);
  }

  remove_last_comma(query);    
  if (debug) fprintf(stderr,"QUERY93A: query=%s:\n", query);
  if (mysql_query(conn, query)) {
    fprintf(stderr,"%s\n",mysql_error(conn));
    fprintf(stderr,"QUERY93A: query=%s: \n", query);
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
} // insert_paras_into_sql(int last_para, int doc_id, int org_id) 

int update_word_para_sql_per_page(int last_page_no, int total_renumbered_words) {
  char query[500000];
  char buff[500];
  int pp;
  int mm = 0;
  int last_ii = -1;
  for (pp = 1; pp <= last_page_no; pp+=DELTA_PP) { // we update sql in groups of DELTA_PP pages
    strcpy(query,"update deals_ocrtoken set para = (case id \n ");
    /*case id when 344807 then -7 when 344808 then -8 when 344809 then -9 \n\ end) where id in (344807, 344808, 344809) \n"); */
    int ii;
    int nn;
    for (ii = last_ii+1, nn=0; ii < total_renumbered_words; ii++) {
      if (word_array[ii].page >= pp && word_array[ii].page < pp+DELTA_PP) {
	sprintf(buff,"when %d then %d \n ", word_array[ii].obj_id, word_array[ii].para);
	strcat(query,buff);
	mm++;
	nn++;
	last_ii = ii;
      }
    }
    strcat(query," end)\n " );

    sprintf(buff, "     where doc_id = %d and page >=%d and page < %d ", doc_id, pp, pp+DELTA_PP);
    strcat(query, buff);    

    if (debug) fprintf(stderr,"QUERY89 pp=%d: (%s): query=%s:\n",pp,prog,query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY89 (%s): query=%s:\n",prog,query);
    }
    int affected = mysql_affected_rows(conn);
    fprintf(stderr,"AFFECTED WORD PARA pp=%d: ii=%d: total_words_done=%d: done_in_this_batch=%d: affected=%d: trw=%d:\n", pp ,ii, mm, nn, affected, total_renumbered_words);
  }
  return 0;
} // update_word_para_sql_per_page()


int update_word_para_sql_per_page_rf(int last_page_no, int rf_word_count) {
  char query[500000];
  char buff[500];
  int pp;
  int mm = 0;
  int last_ii = -1;
  for (pp = 1; pp <= last_page_no; pp+=DELTA_PP) { // we update sql in groups of DELTA_PP pages
    strcpy(query,"update deals_ocrtoken set para = (case id \n ");
    /*case id when 344807 then -7 when 344808 then -8 when 344809 then -9 \n\ end) where id in (344807, 344808, 344809) \n"); */
    int ii;
    int nn;
    for (ii = last_ii+1, nn=0; ii < rf_word_count; ii++) {
      if (rf_word_array[ii].page >= pp && rf_word_array[ii].page < pp+DELTA_PP) {
	sprintf(buff,"when %d then %d \n ", rf_word_array[ii].obj_id, rf_word_array[ii].para);
	strcat(query,buff);
	mm++;
	nn++;
	last_ii = ii;
      }
    }
    strcat(query," end)\n " );

    sprintf(buff, "     where doc_id = %d and page >=%d and page < %d ", doc_id, pp, pp+DELTA_PP);
    strcat(query, buff);    

    if (debug) fprintf(stderr,"QUERY89 pp=%d: (%s): query=%s:\n",pp,prog,query);
    if (mysql_query(conn, query)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      fprintf(stderr,"QUERY89 (%s): query=%s:\n",prog,query);
    }
    int affected = mysql_affected_rows(conn);
    fprintf(stderr,"AFFECTED WORD PARA pp=%d: ii=%d: total_words_done=%d: done_in_this_batch=%d: affected=%d: trw=%d:\n", pp ,ii, mm, nn, affected, rf_word_count);
  }
  return 0;
} // update_word_para_sql_per_page_rf()
