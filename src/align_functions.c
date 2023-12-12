  /********* this program takes two sequences and aligns them (into a unified output sequence)  
   ** each token in the sequence is given as a structure which contains:
   ** IN:  my_char -- the character 
   ** IN:  the sn -- serial number
   ** OUT: pair_no -- the sn of the partner
   ** SUPPRESS_SECONDARY: if suppress_secondary is 1 then we don't output extraneous tokens on the secondary sequence.
   ** EQUIVALENT SET: per each application (in SIGMA)
   ** D is equal with B and I
   ** G is equal with A and a
   ******/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "align_functions.h"
#define MAX(a,b) (a>b)?a:b
  struct DB_Pair {
    int val;
    char dir;
  };
  struct DB_Pair dp_table[ALIGN_MAX_COL+1][ALIGN_MAX_COL+1];
   int match = 2;
   int mismatch = -1;
   int special_match = 0;

int init_pair_of_triples(struct Align_Triple triple1[], struct Align_Triple triple2[]) {				      
  int ii;
  for (ii = 0; ii < ALIGN_MAX_COL; ii++) {
    triple1[ii].pair_no = -1;
    triple1[ii].sn = -1;
    triple2[ii].pair_no = -1;
    triple2[ii].sn = -1;
  }
  return 0;
}


int init_one_triples(struct Align_Triple triple_array[]) {				      
  int ii,jj;
  for (ii = 0; ii < ALIGN_MAX_COL; ii++) {
    triple_array[ii].pair_no = -1;
    triple_array[ii].sn = -1;
  }
  return 0;
}


int len_triple(struct Align_Triple triple[]) {
  int ii;
  for (ii = 0; (triple[ii].my_char != '|' && ii < ALIGN_MAX_COL); ii++) {
    if (triple[ii].my_char == '|') break;
  }
  return ii;
}

int text_triple(struct Align_Triple triple[], char text_array[120]) {
  int ii;
  static char bext[100];
  for (ii = 0
	 ; (triple[ii].my_char != '|'
	    && triple[ii].my_char != '\0'
	    && ii < ALIGN_MAX_COL
	    && ii < len_triple(triple))
	 ; ii++) {
    bext[ii] = triple[ii].my_char;
  }
  bext[ii++] = '\0';
  strcpy(text_array, bext);
  return 0;
}

int pair_no_triple(struct Align_Triple triple[], char pair_no_array[120]) {
  int ii;
  static char bext[100];
  bext[0] = '\0';
  for (ii = 0
	 ; (triple[ii].my_char != '|'
	    && triple[ii].my_char != '\0'
	    && ii < ALIGN_MAX_COL
	    && ii < len_triple(triple))
	 ; ii++) {
    static char buff[500];
    sprintf(buff,"%d:",triple[ii].pair_no);
    strcat(bext,buff);
  }
  strcpy(pair_no_array, bext);
  return 0;
}


int old_sigma(char C1, char C2) {
  if (C1 == C2 
      || (C1 == 'D' && (C2 == 'B' || C2 == 'I')) // date before after
      || (C2 == 'D' && (C1 == 'B' || C1 == 'I'))
      || (C1 == 'G' && (C2 == 'A' || C2 == 'a')) // amt general, month, year
      || (C2 == 'G' && (C1 == 'A' || C1 == 'a'))
      || (C1 == 'a' && C2 == 'A')
      || (C1 == 'A' && C2 == 'A')
      || (C1 == 'm' && C2 == 'y')
      || (C1 == 'y' && C2 == 'm') // year_period, month_period
      || (C1 == 'F' && (C2 == 'C' || C2 == 'c' || C2 == 'f'))
      || (C1 == 'C' && (C2 == 'F' || C2 == 'c' || C2 == 'f'))
      || (C1 == 'f' && (C2 == 'C' || C2 == 'c' || C2 == 'F'))
      || (C1 == 'c' && (C2 == 'C' || C2 == 'f' || C2 == 'F'))
      || ((C1 == 'L' || C1 == 'l') && C2 == 'D')
      || (C1 == 'D' && (C2 == 'L' || C2 == 'l'))
      ) {
    return match;
  } else if (! isalnum(C1)!=0 &&  isalnum(C2)==0) {
    return special_match; // don't give credit for non-alnum
  } else {
    return mismatch;
  }
} // sigma


int sigma(char C1, char C2) { // UZ 01715
  if (! isalnum(C1)!=0 &&  isalnum(C2)==0) {
    return special_match; // don't give credit for non-alnum
  } else if (C1 == C2 
	     || (C1 == 'd' && (C2 == 'J' || C2 == 'j')) // per_period/per_month/per_year
	     || (C2 == 'J' && (C1 == 'd' || C1 == 'j'))
	     || (C2 == 'j' && (C1 == 'd' || C1 == 'J'))

	     || (C1 == 'G' && (C2 == 'A' || C2 == 'a')) // amt general, month, year
	     || (C2 == 'G' && (C1 == 'A' || C1 == 'a'))

	     || (C1 == 'a' && (C2 == 'A' || C2 == 'z')) // the various amounts: amt, and z
	     || (C1 == 'A' && (C2 == 'a'  || C2 == 'z'))
	     || (C1 == 'z' && (C2 == 'a'  || C2 == 'A'))

	     || (C1 == 'm' && C2 == 'y')
	     || (C1 == 'd' && C2 == 'm') // month_count, year_count
      || (C1 == 'y' && C2 == 'm') // year_period, month_period
      || (C1 == 'F' && (C2 == 'C' || C2 == 'c' || C2 == 'f'))
      || (C1 == 'C' && (C2 == 'F' || C2 == 'c' || C2 == 'f'))
      || (C1 == 'f' && (C2 == 'C' || C2 == 'c' || C2 == 'F'))
      || (C1 == 'c' && (C2 == 'C' || C2 == 'f' || C2 == 'F'))

      || ((C1 == 'L' || C1 == 'l') && C2 == 'D')
      || (C1 == 'D' && (C2 == 'L' || C2 == 'l'))
	     
      ) {
    return match;
  } else {
    return mismatch;
  }
} // sigma

float wrap_align(struct Align_Triple triple1[], struct Align_Triple triple2[]) {
  int l1 = len_triple(triple1);
  int l2 = len_triple(triple2);
  if (debug) fprintf(stderr,"L12=%d:%d:\n",l1,l2);
  make_dp_table(l1, l2, triple1, triple2);
  float relative_dist = align(triple1, triple2);
  if (debug) fprintf(stderr,"RD=%6.2f:\n",relative_dist);
  return relative_dist;
}

int make_dp_table(int l1, int l2, struct Align_Triple triple1[], struct Align_Triple triple2[]) {
  int ii;
  for (ii = 0; ii < l2+1; ii++) {
    dp_table[0][ii].val = -1 * ii;
    dp_table[0][ii].dir = 'l';
    //if (debug) fprintf(stderr,"DP:%d:%d: v=%d:d=%c\n",0,ii,-1 * ii,'u');
  }
  int jj;
  for (jj = 1; jj < l1+1; jj++) {
    char c1 = triple1[jj-1].my_char;
    dp_table[jj][0].val = -1 * jj;
    dp_table[jj][0].dir = 'u';
    //if (debug) fprintf(stderr,"DP:%d:%d: v=%d:d=%c\n",0,jj,-1 * jj,'l');
    for (ii = 1; ii < l2+1; ii++) {
      char c2 = triple2[ii-1].my_char;
      int lvalue = dp_table[jj-1][ii].val + sigma( '$',c2);
      int dvalue = dp_table[jj-1][ii-1].val + sigma(c1,c2);
      int uvalue = dp_table[jj][ii-1].val + sigma(c1,'$');
      if (lvalue >= dvalue && lvalue >= uvalue) {
	dp_table[jj][ii].val = lvalue;
	dp_table[jj][ii].dir = 'u';
	//if (debug) fprintf(stderr,"LP:%d:%d: v=%d:d=%c",jj,ii,lvalue,'u');
      }	
      if (dvalue >= lvalue && dvalue >= uvalue) {
	dp_table[jj][ii].val = dvalue;
	dp_table[jj][ii].dir = 'd';
	//if (debug) fprintf(stderr,"DP:%d:%d: v=%d:d=%c",jj,ii,dvalue,'d');
      }	
      if (uvalue >= lvalue && uvalue >= dvalue) {
	dp_table[jj][ii].val = uvalue;
	dp_table[jj][ii].dir = 'l';
	//if (debug) fprintf(stderr,"UP:%d:%d: v=%d:d=%c",jj,ii,uvalue,'l');
      }	
    } // for
    //if (debug) fprintf(stderr,"\n");
  } // for
  dp_table[0][0].val = 0;
  dp_table[0][0].dir = 's';
  return 0;
} // make_dp_table()


int prepare_one_triple(struct Align_Triple triple1[], char buff[]) { // prepare a triple based on the buff
  int ll = strlen(buff);
  int ii;
  if (debug) fprintf(stderr,"PREO:%d:%s:\n",ll,buff);
  for (ii = 0; ii < ll; ii++) { 
    triple1[ii].sn = ii;
    triple1[ii].my_char = buff[ii];
  }
  triple1[ii].sn = -1;
  triple1[ii].my_char = '|';
  ii++;
  return ll;
}

int prepare_one_line_triple(struct Align_Triple triple1[], char buff[], int ref_point) {
  int ll = strlen(buff);
  int ii;
  if (debug) fprintf(stderr,"PREO:%d:%s:\n",ll,buff);
  for (ii = 0; ii < ll; ii++) { 
    triple1[ii].sn = ii;
    triple1[ii].my_char = buff[ii];
    triple1[ii].ref_point = ref_point+ii;
  }
  triple1[ii].sn = -1;
  triple1[ii].my_char = '|';
  ii++;
  return ll;
}

// input are two triples, output is distance (relative) and pair_no on each
float align(struct Align_Triple triple1[], struct Align_Triple triple2[]) {
  char res1[ALIGN_MAX_COL];
  char res2[ALIGN_MAX_COL];

  strcpy(res1,"");
  strcpy(res2,"");
  int l11 = len_triple(triple1);
  int l21 = len_triple(triple2);
  int ii = l11;
  int jj = l21;
  //if (debug) fprintf(stderr,"starting:%d:%d:\n",ii,jj);
  while (dp_table[ii][jj].dir != 's') {
    static char ttt[ALIGN_MAX_COL];
    int my_dir = dp_table[ii][jj].dir;
    if (my_dir == 'l') {
      sprintf(ttt,"%c%s",'_',res1);
      strcpy(res1,ttt);
      sprintf(ttt,"%c%s",triple2[jj-1].my_char,res2);
      strcpy(res2,ttt);
      if ( suppress_secondary == 1) {
	triple2[jj-1].pair_no = -1;
      } else {
	triple2[jj-1].pair_no = -1;
      }
      if (debug) fprintf(stderr,"LL:%d:%d--%s:%s:-%2d:\n",ii,jj,res1,res2,triple2[jj-1].pair_no);
      jj--;
    } else if (my_dir == 'd') {
      sprintf(ttt,"%c%s",triple1[ii-1].my_char,res1);
      strcpy(res1,ttt);
      sprintf(ttt,"%c%s",triple2[jj-1].my_char,res2);
      strcpy(res2,ttt);
      triple1[ii-1].pair_no = jj-1;
      triple2[jj-1].pair_no = ii-1;
      if (debug) fprintf(stderr,"DD:%d:%d--:%s:%s:-%2d:\n",ii,jj,res1,res2,triple2[jj-1].pair_no);
      jj--;
      ii--;
    } else if (my_dir == 'u') {
      sprintf(ttt,"%c%s",triple1[ii-1].my_char,res1);
      strcpy(res1,ttt);
      sprintf(ttt,"%c%s",'_',res2);
      strcpy(res2,ttt);
      triple1[ii-1].pair_no = -1;
      if (debug) fprintf(stderr,"RR:%d:%d--:%s:%s:-%2d:\n",ii,jj,res1,res2,triple2[jj-1].pair_no);
      ii--;
    } else if (my_dir == 's') {
      if (debug) fprintf(stderr,"ZZ:%d:%d--:%s:%s:-%2d:\n",ii,jj,res1,res2,triple2[jj-1].pair_no);
    }
  } // while

  int kk = 0;
  while (kk < l11) {
    int pair_no = triple1[kk].pair_no;
    if (debug) fprintf(stderr,"MM:%d:%c:%c:%d:\n", kk, triple1[kk].my_char
	    , ((pair_no >=0) ? triple2[pair_no].my_char : '_')
	    , ((pair_no >=0) ? triple2[pair_no].sn : -1)
	    );
    kk++;
  } 
  int dist = strlen(res1)+strlen(res2)-(l11+l21);
  float rel_dist = (float)(dist) / (float)(MAX(l11,l21));
  if (debug) fprintf(stderr,"ALIGN_RES: rl1=%d:%d, rl2=%d:%d, ii=%d: dist=%d: rel_dist=%4.2f: dpt=%d: :%s:||:%s:\n"
				,(int)strlen(res1),l11,   (int)strlen(res2),jj,   ii, dist, rel_dist, dp_table[0][0].val, res1,res2);
  return rel_dist;
} // align()



// count the real chars of a string (i.e., don't count '_')
int count_real(char res[]) {
  int ii;
  int ret = 0;
  for (ii = 0; ii < strlen(res); ii++) {
    if (res[ii] != '_') {
      ret++;
    }
  }
  return ret;
} // count_real()


// input are two triples, output is distance (relative) and pair_no on each, we want to keep going with the results to the next item in the cluster
// we continue with the output that has more letters (not counting '_')
float align_for_clustering(struct Align_Triple triple1[], struct Align_Triple triple2[], int *real_res1, int *real_res2, int debug) {
  char res1[ALIGN_MAX_COL];
  char res2[ALIGN_MAX_COL];

  strcpy(res1,"");
  strcpy(res2,"");
  int l11 = len_triple(triple1);
  int l21 = len_triple(triple2);
  int ii = l11;
  int jj = l21;
  //if (debug) fprintf(stderr,"starting:%d:%d:\n",ii,jj);
  while (dp_table[ii][jj].dir != 's') {
    static char ttt[ALIGN_MAX_COL];
    int my_dir = dp_table[ii][jj].dir;
    if (my_dir == 'l') {
      sprintf(ttt,"%c%s",'_',res1);
      strcpy(res1,ttt);
      sprintf(ttt,"%c%s",triple2[jj-1].my_char,res2);
      strcpy(res2,ttt);
      if ( suppress_secondary == 1) {
	triple2[jj-1].pair_no = -1;
      } else {
	triple2[jj-1].pair_no = -1;
      }
      if (debug) fprintf(stderr,"LL:%d:%d--%s:%s:-%2d:\n",ii,jj,res1,res2,triple2[jj-1].pair_no);
      jj--;
    } else if (my_dir == 'd') {
      sprintf(ttt,"%c%s",triple1[ii-1].my_char,res1);
      strcpy(res1,ttt);
      sprintf(ttt,"%c%s",triple2[jj-1].my_char,res2);
      strcpy(res2,ttt);
      triple1[ii-1].pair_no = jj-1;
      triple2[jj-1].pair_no = ii-1;
      if (debug) fprintf(stderr,"DD:%d:%d--:%s:%s:-%2d:\n",ii,jj,res1,res2,triple2[jj-1].pair_no);
      jj--;
      ii--;
    } else if (my_dir == 'u') {
      sprintf(ttt,"%c%s",triple1[ii-1].my_char,res1);
      strcpy(res1,ttt);
      sprintf(ttt,"%c%s",'_',res2);
      strcpy(res2,ttt);
      triple1[ii-1].pair_no = -1;
      if (debug) fprintf(stderr,"RR:%d:%d--:%s:%s:-%2d:\n",ii,jj,res1,res2,triple2[jj-1].pair_no);
      ii--;
    } else if (my_dir == 's') {
      if (debug) fprintf(stderr,"ZZ:%d:%d--:%s:%s:-%2d:\n",ii,jj,res1,res2,triple2[jj-1].pair_no);
    }
  } // while

  int kk = 0;
  while (kk < l11) {
    int pair_no = triple1[kk].pair_no;
    if (debug) fprintf(stderr,"MM:%d:%c:%c:%d:\n", kk, triple1[kk].my_char
	    , ((pair_no >=0) ? triple2[pair_no].my_char : '_')
	    , ((pair_no >=0) ? triple2[pair_no].sn : -1)
	    );
    kk++;
  } 
  int dist = strlen(res1)+strlen(res2)-(l11+l21);
  float rel_dist = (float)(dist) / (float)(MAX(l11,l21));
  
  if (debug) fprintf(stderr,"ALIGN_RES: rl1=%d:%d, rl2=%d:%d, ii=%d: dist=%d: rel_dist=%4.2f: dpt=%d: :%s:||:%s:\n"
				,(int)strlen(res1),l11,   (int)strlen(res2),jj,   ii, dist,  rel_dist,  dp_table[0][0].val,   res1, res2);
  *real_res1 = count_real(res1);
  *real_res2 = count_real(res2);  
  return rel_dist;
} // align_for_clustering()





int print_table(int l1, int l2, struct Align_Triple triple1[], struct Align_Triple triple2[]) {
  if (l1 > ALIGN_MAX_COL -1 || l2 > ALIGN_MAX_COL -1) {
    fprintf(stderr,"Error (align in base_table): ALIGN_MAX_COL (%d) exceeded (:%d:%d:)!\n",ALIGN_MAX_COL,l1,l2);
    l1 = l2 = ALIGN_MAX_COL -2;
  }
  if (0) {
  if (debug) fprintf(stderr,"TABLE SIZE=%d:%d:\n",l1,l2);
  int ii,jj;
  for (ii = 0; ii < l1+1; ii++) {
    for (jj = 0; jj < l2+1; jj++) {
      if (debug) fprintf(stderr,":%2d:%2d--%3d:%c:\t\t",ii,jj,dp_table[ii][jj].val,dp_table[ii][jj].dir);
    }
    if (debug) fprintf(stderr,"\n");
  }
  if (debug) fprintf(stderr,"\n");
  }
  return 0;
}


// input are two triples, output is distance (relative) and pair_no on each AND a new triple with the resulting string
float sig_align(struct Align_Triple triple1[], struct Align_Triple triple2[], struct Align_Triple triple_res[]) {
  char res1[ALIGN_MAX_COL];
  char res2[ALIGN_MAX_COL];

  strcpy(res1,"");
  strcpy(res2,"");
  int l11 = len_triple(triple1);
  int l21 = len_triple(triple2);
  int ii = l11;
  int jj = l21;
  if (debug) fprintf(stderr,"starting:%d:%d:\n",ii,jj);
  while (dp_table[ii][jj].dir != 's') {
    static char ttt[ALIGN_MAX_COL];
    int my_dir = dp_table[ii][jj].dir;
    if (my_dir == 'l') {
      sprintf(ttt,"%c%s",'_',res1);
      strcpy(res1,ttt);
      sprintf(ttt,"%c%s",triple2[jj-1].my_char,res2);
      strcpy(res2,ttt);
      if ( suppress_secondary == 1) {
	triple2[jj-1].pair_no = -1;
      } else {
	triple2[jj-1].pair_no = -1;
      }
      if (debug) fprintf(stderr,"LL:%d:%d--%s:%s:-%2d:%2d:\n",ii,jj,res1,res2,triple2[jj-1].pair_no, triple2[ii-1].pair_no);
      jj--;
    } else if (my_dir == 'd') {
      sprintf(ttt,"%c%s",triple1[ii-1].my_char,res1);
      strcpy(res1,ttt);
      sprintf(ttt,"%c%s",triple2[jj-1].my_char,res2);
      strcpy(res2,ttt);
      triple1[ii-1].pair_no = jj-1;
      triple2[jj-1].pair_no = ii-1;

      if (debug) fprintf(stderr,"DD:%d:%d--:%s:%s:-%2d:%2d:\n", ii, jj,res1,res2,triple2[jj-1].pair_no, triple2[ii-1].pair_no);
      jj--;
      ii--;
    } else if (my_dir == 'u') {
      sprintf(ttt,"%c%s",triple1[ii-1].my_char,res1);
      strcpy(res1,ttt);
      sprintf(ttt,"%c%s",'_',res2);
      strcpy(res2,ttt);
      triple1[ii-1].pair_no = -1;
      if (debug) fprintf(stderr,"RR:%d:%d--:%s:%s:-%2d:%2d:\n",ii,jj,res1,res2,triple2[jj-1].pair_no, triple2[ii-1].pair_no);
      ii--;
    } else if (my_dir == 's') {
      if (debug) fprintf(stderr,"ZZ:%d:%d--:%s:%s:-%2d:%2d:\n",ii,jj,res1,res2,triple2[jj-1].pair_no, triple2[ii-1].pair_no);
    }
  } // while

  int kk = 0;
  while (kk < l11) {
    int pair_no = triple1[kk].pair_no;
    if (debug) fprintf(stderr,"MM:%d:%c:%c:%d:\n", kk, triple1[kk].my_char
	    , ((pair_no >=0) ? triple2[pair_no].my_char : '_')
	    , ((pair_no >=0) ? triple2[pair_no].sn : -1)
	    );
    kk++;
  } 
  int dist = strlen(res1)+strlen(res2)-(l11+l21);
  float rel_dist = (float)(dist) / (float)(MAX(l11,l21));
  if (debug) fprintf(stderr,"ALIGN_RES: rl1=%d:%d, rl2=%d:%d, ii=%d: dist=%d: rel_dist=%4.2f: dpt=%d: :%s:||:%s:\n"
		     ,(int)strlen(res1),l11,   (int)strlen(res2),jj,   ii, dist, rel_dist, dp_table[0][0].val, res1,res2);
  if (triple_res) {
    int mm;
    for (mm = 0; mm < strlen(res2); mm++) {
      triple_res[mm].my_char = (res1[mm] == '_') ? res2[mm] : res1[mm];
      triple_res[mm].sn = mm;
    }
    triple_res[mm].my_char = '|';
    triple_res[mm].sn = -1;
    mm++;
  }
  return rel_dist;
} // sig_align()



int print_triples(int triple_len, struct Align_Triple triple[]) {
  int ii;

    fprintf(stderr,"LEN=%d: ",triple_len);
    fprintf(stderr,"CHAR=");
    for (ii = 0; ii < triple_len; ii++) {
      fprintf(stderr,"%c:", triple[ii].my_char);
    }
    fprintf(stderr,"  ");	  
    fprintf(stderr,"SN=");
    for (ii = 0; ii < triple_len; ii++) {
      fprintf(stderr,"%d:", triple[ii].sn);
    }
    fprintf(stderr,"\n");

  return 0;
} // print_triples()

int copy_shrink_triples(struct Align_Triple triple1[], struct Align_Triple triple2[]) {
  int ii,jj;
  int len2;
  int len1 = len_triple(triple1);
  for (jj=0,ii = 0; ii < len1; ii++) {
    if (triple1[ii].my_char != '_') {
      triple2[jj].my_char = triple1[ii].my_char;
      triple2[jj].sn = triple1[ii].sn;
      triple2[jj].ref_point = triple1[ii].ref_point;
      triple2[jj].pair_no = triple1[ii].pair_no;
      jj++;
    }
  }
  triple2[jj].my_char = '|';
  triple2[jj].sn = -1;
  return len2;
} // print_triples()

