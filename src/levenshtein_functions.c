#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* used to be in remove_footers */
#define MIN(a,b,c) ((a<b&&a<c)?a:((b<a&&b<c)?b:c))
#define MIN2(a,b) ((a<b)?a:b)
#define MAX2(a,b) ((a>b)?a:b)
#define MEMO_MAX_LEN 20
#define MEMO_MAX_DISP 200
#define MAX_FOOTER_LEN 150
/* used to be in remove_footers */

int memo[MEMO_MAX_DISP][MEMO_MAX_LEN][MEMO_MAX_LEN];

/* 
  ** Levenshtein functions
  ** init_memo(), calc_diff(), wrap_LD(), LD()  
*/

int init_memo() {
  int i1, i2, i3;
  for (i1 = 0; i1 < MEMO_MAX_DISP; i1++) {
    for (i2 = 0; i2 < MEMO_MAX_LEN; i2++) {
      for (i3 = 0; i3 < MEMO_MAX_LEN; i3++) {
	memo[i1][i2][i3] = -1;
      }
    }
  }
  return 0;
}

int calc_diff(char c1, char c2) {
  int ret;
  if (c1 == c2) ret = 0;
  else ret = 1;
  return ret;
}

int LD(char *str1, int i, int len1, char *str2, int j, int len2) {
  int dist = memo[j][len1][len2];

  if (dist != -1) {
    //if (debug) fprintf(stderr,"GET MEMO %d--%d:%d:%d:\n",dist,j,len1,len2);<
    return dist;
  } else {

    if (len1 == 0) return len2;
    if (len2 == 0) return len1;
    int cost = 0;
    int my_diff = calc_diff(str1[i],str2[j]);
    if (my_diff) cost = 1;

    int dist = MIN(
		   LD(str1, i+1,len1-1, str2,j,len2)+1,
		   LD(str1,i,len1,str2,j+1,len2-1)+1,
		   LD(str1,i+1,len1-1,str2,j+1,len2-1)+cost);
    memo[j][len1][len2] = dist;
    //if (debug) fprintf(stderr,"SET MEMO %d--%d:%d:%d:\n",dist,j,len1,len2);

    return dist;
  }
}
    
int wrap_LD(char *str1, char *str2) {
  int len1 = (int)strlen(str1);
  int len2 = (int)strlen(str2);
  int i = 0;
  int j = 0;
  int dist;
    
  init_memo();
  //fprintf(stderr, "init_memo();\n");
  //fprintf(stderr, "len1=%d,len2=%d\n", len1, len2);
  if (len1 > MAX_FOOTER_LEN || len2 > MAX_FOOTER_LEN) {
    dist = len1 + len2;
  } else {  // footer OK
    dist = LD(str1, i, len1, str2, j, len2); 
  }
  return dist;
}

int wrap_LD_old(char *str1, int i, int len1, char *str2, int j, int len2) {
  init_memo();
  int dist = LD(str1, i, len1, str2, j, len2);
  return dist;
}
