#include <stdio.h>
#include <string.h>
#include <mysql/mysql.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include "import_functions.h"

#define MAX(a,b) ((a>b)?a:b)
// for DB config 
static char *db_IP;
static char *db_name;
static char *db_pwd;
static char *db_user_name; 
MYSQL *conn;
MYSQL_RES *sql_res;
MYSQL_RES *sql_res2;
MYSQL_RES *sql_res3;
MYSQL_ROW sql_row;
MYSQL_ROW sql_row2;

struct timeval start, end;

double get_time () {
    gettimeofday(&end, NULL);
    return end.tv_sec + end.tv_usec / 1e6 -
                        start.tv_sec - start.tv_usec / 1e6;
}

struct Instance empty = {-1,-1,-1,"","","",0,0,0,0,-1,-1,-1,-1,-1,0};

// ***hash table implement for aligned_array***
struct DataItem {
   int data; // index in aligned_array
   int key; // id in table 'deals_alma_non_aligned_token'
};

static int size_of_hashtab_aligned;
struct DataItem** hashtab_aligned;
struct DataItem* dummyItem;

int hashCode(int key) {
   return key % size_of_hashtab_aligned;
}

struct DataItem *search_aligned(int key) {
   //get the hash
   int hashIndex = hashCode(key);
   //move in array until an empty
   while(hashtab_aligned[hashIndex] != NULL) {
      if(hashtab_aligned[hashIndex]->key == key){
        return hashtab_aligned[hashIndex];
      }
      //go to next cell
      ++hashIndex;
      //wrap around the table
      hashIndex %= size_of_hashtab_aligned;
   }
   return NULL;
}

void insert_into_aligned(int key,int data) {
   struct DataItem *item = (struct DataItem*) malloc(sizeof(struct DataItem));
   item->data = data;
   item->key = key;
   //get the hash
   int hashIndex = hashCode(key);
   //move in array until an empty or deleted cell
   while(hashtab_aligned[hashIndex] != NULL && hashtab_aligned[hashIndex]->key != -1) {
      //go to next cell
      ++hashIndex;
      //wrap around the table
      hashIndex %= size_of_hashtab_aligned;
   }
   hashtab_aligned[hashIndex] = item;
}

struct DataItem* delete_from_aligned(struct DataItem* item) {
   int key = item->key;
   //get the hash
   int hashIndex = hashCode(key);
   //move in array until an empty
   while(hashtab_aligned[hashIndex] != NULL) {
      if(hashtab_aligned[hashIndex]->key == key) {
         struct DataItem* temp = hashtab_aligned[hashIndex];
         //assign a dummy item at deleted position
         hashtab_aligned[hashIndex] = dummyItem;
         return temp;
      }
      //go to next cell
      ++hashIndex;
      //wrap around the table
      hashIndex %= size_of_hashtab_aligned;
   }
   return NULL;
}

void set_up_hashtab_aligned(int size){
    if ((hashtab_aligned= calloc(2*size, sizeof(struct DataItem*)))==NULL) {
        fprintf(stderr, "Fatal: failed to allocate %zu bytes.\n", 2*size * sizeof(struct DataItem*));
        abort();
    }
    size_of_hashtab_aligned= 2*size;
}

//editing text before inserting to mysql server
char *escape_edit(char *text) {
    static char bext[5000];
    int j, i;
    for (i = 0, j = 0; i < strlen(text); i++) {
        if (text[i] == '\"' || text[i] == '\'' || text[i] == '\\') {
            bext[j++] = '\\';
        }
        bext[j++] = text[i];

    }
    bext[j++] = '\0';
    return strdup(bext);
}

//query to find number of pages in [doc_id]
int find_num_of_pages(int doc_id){
    static char query[5000];
    sprintf(query,"SELECT MAX(page) FROM `deals_alma_non_aligned_token` where doc_id=%d",doc_id);
    fprintf(stderr,"query0=%s:\n",query);
    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
    }
    fprintf(stderr,"query1=%s:\n",query);
    sql_res = mysql_store_result(conn);
    fprintf(stderr,"query2=%s:\n",query);    
    if(NULL != (sql_row = mysql_fetch_row(sql_res))){
        return atoi(sql_row[0]);
    }
    return 0;

}

int size_of_source(char* source,int doc_id,int page){
    static char query[5000];
    sprintf(query,"SELECT count(id)\n\
    FROM `deals_alma_non_aligned_token` WHERE doc_id=%d and page=%d and source_program='%s'",
	doc_id,page,source);
    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
    }
    sql_res = mysql_store_result(conn);
    if (NULL != (sql_row = mysql_fetch_row(sql_res))) {
        return atoi(sql_row[0]);
    }
    return -1;
}

int max(int num1, int num2)
{
    return (num1 > num2 ) ? num1 : num2;
}

int min(int num1, int num2)
{
    return (num1 < num2 ) ? num1 : num2;
}

//decide weather 2 Instances are overlapping and calculate overlap accordingly
int overlap(struct Instance ins1, struct Instance ins2){
    int areaI=0;
    if (ins1.page!=ins2.page){
        return areaI;
    }
    if(ins1.my_x1>=ins2.my_x1 && ins2.my_x2>ins1.my_x1){
        if((ins1.my_y1<ins2.my_y2 && ins1.my_y1>=ins2.my_y1) ||( ins1.my_y1<=ins2.my_y1 && ins1.my_y2>ins2.my_y1)){
            areaI = (min(ins1.my_x2,ins2.my_x2) -
                 max(ins1.my_x1,ins2.my_x1)) *
                (min(ins1.my_y2, ins2.my_y2) -
                 max(ins1.my_y1,ins2.my_y1));
        }
    }
    else if (ins2.my_x1>=ins1.my_x1 && ins1.my_x2>ins2.my_x1){
        if((ins2.my_y1<ins1.my_y2 && ins2.my_y1>=ins1.my_y1) ||( ins2.my_y1<=ins1.my_y1 && ins2.my_y2>ins1.my_y1)){
            areaI = (min(ins1.my_x2,ins2.my_x2) -
                 max(ins1.my_x1,ins2.my_x1)) *
                (min(ins1.my_y2, ins2.my_y2) -
                 max(ins1.my_y1,ins2.my_y1));
        }
    }

    return areaI;
}

//deep copy for struct Instance
struct Instance copy_instance(struct Instance i){
    return (struct Instance) {i.id,i.page,i.sn_in_doc, strdup(i.source),strdup(i.text),strdup(i.normalized_text),0,0,0,0,i.my_x1,i.my_x2,i.my_y1,i.my_y2,i.confidence,i.junk_category};
}

//extract information from 'deals_alma_non_aligned_token' into aws_data array according to [doc_id]
int read_from_token_array_aws(int doc_id){
    static char query[5000];
    static char query2[5000];
    sprintf(query,"SELECT page,text,normalized_text,confidence,my_x1,my_y1,my_x2,my_y2,id,junk_category,sn_in_doc\n\
    FROM `deals_alma_non_aligned_token` WHERE doc_id=%d and source_program='aws'\n\
    order by sn_in_doc",
	doc_id);

    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
    }

    sql_res = mysql_store_result(conn);

    sprintf(query2,"SELECT page, count(id)\n\
    FROM `deals_alma_non_aligned_token` WHERE doc_id=%d and source_program='aws'\n\
    group by page",
	doc_id);
    if (mysql_query(conn, query2)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
    }

    sql_res2 = mysql_store_result(conn);
    int instance_num;
    int page=-1;
    int total_instances=0;
    printf("\t4.1. reading tokens of aws into aws_data array\n");
    while (NULL != (sql_row = mysql_fetch_row(sql_res))) {
        int curr_page= atoi(sql_row[0]);
        if(curr_page!=page){
            page=curr_page;
            aws_data[page-1].page=page;
            if(NULL != (sql_row2 = mysql_fetch_row(sql_res2))){
                aws_data[page-1].num_of_instances= atoi(sql_row2[1]);
                aws_data[page-1].instances= malloc(aws_data[page-1].num_of_instances * sizeof(struct Instance));
                total_instances= total_instances+ aws_data[page-1].num_of_instances;
                //printf("total:%d\n",total_instances);
                instance_num=0;
            }
            else{
               fprintf(stderr,"couldn't find num of tokens in page\n");
               return 1;
            }
        }
        aws_data[page-1].instances[instance_num].page= atoi(sql_row[0]);
        aws_data[page-1].instances[instance_num].source= strdup("AWS");
        aws_data[page-1].instances[instance_num].text= escape_edit(sql_row[1]);
        aws_data[page-1].instances[instance_num].normalized_text= escape_edit(sql_row[2]);
        aws_data[page-1].instances[instance_num].confidence= atoi(sql_row[3]);
        aws_data[page-1].instances[instance_num].my_x1= atoi(sql_row[4]);
        aws_data[page-1].instances[instance_num].my_y1= atoi(sql_row[5]);
        aws_data[page-1].instances[instance_num].my_x2= atoi(sql_row[6]);
        aws_data[page-1].instances[instance_num].my_y2= atoi(sql_row[7]);
        aws_data[page-1].instances[instance_num].id= atoi(sql_row[8]);
        aws_data[page-1].instances[instance_num].junk_category= atoi(sql_row[9]);
        aws_data[page-1].instances[instance_num].sn_in_doc= atoi(sql_row[10]);
        instance_num++;
    }
    return total_instances;
}
//extract information from 'deals_alma_non_aligned_token' into gcs_data array according to [doc_id]
int read_from_token_array_gcs(int doc_id){
    static char query[5000];
    static char query2[5000];
    sprintf(query,"SELECT page,text,normalized_text,confidence,my_x1,my_y1,my_x2,my_y2,id,junk_category,sn_in_doc\n\
    FROM `deals_alma_non_aligned_token` WHERE doc_id=%d and source_program='gcs'\n\
    order by sn_in_doc",
	doc_id);

    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
    }

    sql_res = mysql_store_result(conn);

    sprintf(query2,"SELECT page, count(id)\n\
    FROM `deals_alma_non_aligned_token` WHERE doc_id=%d and source_program='gcs'\n\
    group by page",
	doc_id);
    if (mysql_query(conn, query2)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
    }

    sql_res2 = mysql_store_result(conn);
    int instance_num;
    int page=-1;
    int total_instances=0;
     printf("\t4.2. reading tokens of gcs into gcs_data array\n");
    while (NULL != (sql_row = mysql_fetch_row(sql_res))) {
        int curr_page= atoi(sql_row[0]);
        if(curr_page!=page){

            page=curr_page;
            gcs_data[page-1].page=page;
            if(NULL != (sql_row2 = mysql_fetch_row(sql_res2))){
                gcs_data[page-1].num_of_instances= atoi(sql_row2[1]);
                gcs_data[page-1].instances= malloc(gcs_data[page-1].num_of_instances * sizeof(struct Instance));
                total_instances= total_instances+ gcs_data[page-1].num_of_instances;
                instance_num=0;
            }
            else{
               fprintf(stderr,"couldn't find num of tokens in page\n");
               return 1;
            }
        }
        gcs_data[page-1].instances[instance_num].page= atoi(sql_row[0]);
        gcs_data[page-1].instances[instance_num].source= strdup("GCS");
        gcs_data[page-1].instances[instance_num].text= escape_edit(sql_row[1]);
        gcs_data[page-1].instances[instance_num].normalized_text= escape_edit(sql_row[2]);
        gcs_data[page-1].instances[instance_num].confidence= atoi(sql_row[3]);
        gcs_data[page-1].instances[instance_num].my_x1= atoi(sql_row[4]);
        gcs_data[page-1].instances[instance_num].my_y1= atoi(sql_row[5]);
        gcs_data[page-1].instances[instance_num].my_x2= atoi(sql_row[6]);
        gcs_data[page-1].instances[instance_num].my_y2= atoi(sql_row[7]);
        gcs_data[page-1].instances[instance_num].id= atoi(sql_row[8]);
        gcs_data[page-1].instances[instance_num].junk_category= atoi(sql_row[9]);
        gcs_data[page-1].instances[instance_num].sn_in_doc= atoi(sql_row[10]);
        instance_num++;
    }
    return total_instances;
}


//calculate for each 2 instances from different sources their overlap and insert into analogous array
int fill_non_aligned_array_and_calculate_overlap(int page){
  fprintf(stderr,"\t\t6.14  s1=%d: s2=%d:\n", size_of_source1, size_of_source2);
  non_aligned_ind=0;
  for (int i=0; i<size_of_source1 ;i++){
    if (size_of_source2 > 0) { // UZ NEW
      for (int j=0;j < size_of_source2 ;j++){
	int curr_overlap= overlap(aws_data[page-1].instances[i],gcs_data[page-1].instances[j]);
	if(curr_overlap>0 || i==0 || j==0){
	  non_aligned_array[non_aligned_ind].overlap=curr_overlap;
	  non_aligned_array[non_aligned_ind].instance1=copy_instance(aws_data[page-1].instances[i]);
	  non_aligned_array[non_aligned_ind].instance2=copy_instance(gcs_data[page-1].instances[j]);
	  non_aligned_ind++;
	}
      } // for j
    } else {  // UZ NEW size_of_source2 == 0
      non_aligned_array[non_aligned_ind].overlap=0;
      non_aligned_array[non_aligned_ind].instance1=copy_instance(aws_data[page-1].instances[i]);
      //non_aligned_array[non_aligned_ind].instance2=copy_instance(gcs_data[page-1].instances[j]);
      non_aligned_ind++;
    }      
  } // for i
  fprintf(stderr,"\t\t6.15  naa=%d:\n", non_aligned_ind);
  return non_aligned_ind;
}


//deep copy for struct Couple
struct Couple copy_non_aligned(struct Couple a){
    return (struct Couple) {copy_instance(a.instance1),copy_instance(a.instance2),a.overlap, a.disc};
}

/* Function to merge the two haves arr[l..m] and arr[m+1..r] of array arr[] */
void merge(struct Couple arr[], int l, int m, int r)
{
    int i, j, k;
    int n1 = m - l + 1;
    int n2 =  r - m;

    /* create temp arrays */
    struct Couple *L,*R;
    if((L= malloc(n1*sizeof(struct Couple)))==NULL){
        fprintf(stderr, "Fatal: failed to allocate %zu bytes.\n", n1*sizeof(struct Couple));
        abort();
    }
    if((R= malloc(n2*sizeof(struct Couple)))==NULL){
        fprintf(stderr, "Fatal: failed to allocate %zu bytes.\n", n2*sizeof(struct Couple));
        abort();
    }
    /* Copy data to temp arrays L[] and R[] */
    for (i = 0; i < n1; i++)
        L[i] = copy_non_aligned(arr[l + i]);
    for (j = 0; j < n2; j++)
        R[j] = copy_non_aligned(arr[m + 1+ j]);

    /* Merge the temp arrays back into arr[l..r]*/
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2)
    {
        free(arr[k].instance1.source);
        free(arr[k].instance1.text);
        free(arr[k].instance1.normalized_text);
        free(arr[k].instance2.source);
        free(arr[k].instance2.text);
        free(arr[k].instance2.normalized_text);
        if (L[i].overlap <= R[j].overlap)
        {
            arr[k] = copy_non_aligned(L[i]);
            free(L[i].instance1.source);
            free(L[i].instance1.text);
            free(L[i].instance1.normalized_text);
            free(L[i].instance2.source);
            free(L[i].instance2.text);
            free(L[i].instance2.normalized_text);
            i++;
        }
        else
        {
            arr[k] = copy_non_aligned(R[j]);
            free(R[j].instance1.source);
            free(R[j].instance1.text);
            free(R[j].instance1.normalized_text);
            free(R[j].instance2.source);
            free(R[j].instance2.text);
            free(R[j].instance2.normalized_text);
            j++;
        }
        k++;
    }

    /* Copy the remaining elements of L[], if there are any */
    while (i < n1)
    {
        free(arr[k].instance1.source);
        free(arr[k].instance1.text);
        free(arr[k].instance1.normalized_text);
        free(arr[k].instance2.source);
        free(arr[k].instance2.text);
        free(arr[k].instance2.normalized_text);
        arr[k] = copy_non_aligned(L[i]);
        free(L[i].instance1.source);
        free(L[i].instance1.text);
        free(L[i].instance1.normalized_text);
        free(L[i].instance2.source);
        free(L[i].instance2.text);
        free(L[i].instance2.normalized_text);
        i++;
        k++;
    }

    /* Copy the remaining elements of R[], if there are any */
    while (j < n2)
    {
        free(arr[k].instance1.source);
        free(arr[k].instance1.text);
        free(arr[k].instance1.normalized_text);
        free(arr[k].instance2.source);
        free(arr[k].instance2.text);
        free(arr[k].instance2.normalized_text);
        arr[k] = copy_non_aligned(R[j]);
        free(R[j].instance1.source);
        free(R[j].instance1.text);
        free(R[j].instance1.normalized_text);
        free(R[j].instance2.source);
        free(R[j].instance2.text);
        free(R[j].instance2.normalized_text);
        j++;
        k++;
    }
    free(L);
    free(R);
}

/* Iterative mergesort function to sort arr[0...n-1] */
void mergeSort(struct Couple arr[], int n)
{
   int curr_size;  // For current size of subarrays to be merged
                   // curr_size varies from 1 to n/2
   int left_start; // For picking starting index of left subarray
                   // to be merged

   // Merge subarrays in bottom up manner.  First merge subarrays of
   // size 1 to create sorted subarrays of size 2, then merge subarrays
   // of size 2 to create sorted subarrays of size 4, and so on.
   for (curr_size=1; curr_size<=n-1; curr_size = 2*curr_size)  {
       // Pick starting point of different subarrays of current size
       for (left_start=0; left_start<n-1; left_start += 2*curr_size)  {
           // Find ending point of left subarray. mid+1 is starting
           // point of right
           int mid = min(left_start + curr_size - 1, n-1);

           int right_end = min(left_start + 2*curr_size - 1, n-1);

           // Merge Subarrays arr[left_start...mid] & arr[mid+1...right_end]
           merge(arr, left_start, mid, right_end);
       }
   }
}

//calculate present of similarity between two words
float calculateSimilarity(char* str1,char* str2){
    int difference= ((struct edit_distance) levenshtein_edit_distance(str1,str2)).alnum;
    return (float)difference/ max(strlen(str1),strlen(str2));
}

//calculate present of similarity between two words in case where one word was splitted
float calculateSimilaritySplit(char* str1,char* str2){
    int difference= ((struct edit_distance) levenshtein_edit_distance(str1,str2)).alnum- (max(strlen(str1),strlen(str2))-min(strlen(str1),strlen(str2)));
    return (float)difference/min(strlen(str1),strlen(str2));
}

int contain_gibrish_or_numbers(char* s){
    for(int i=0;i<strlen(s);i++){
        if(s[i]<0)
            return 1;
    }
    return 0;
}

enum decision decide_by_confidence(int conf1, int conf2){
    if(conf1>=conf2)
        return AWS;
    else
        return GCS;
}

/*
rules for voting:
0. If there is no discrepancy between the tokens, or only non-alnum discrepancy, no voting is needed- choose amazon.
1. If only one of the tokens was found in the dictionary or alternatively has a junk category, choose him.
2. If both were found in the dictionary, choose the token with the highest frequency.
    2.1. If it’s equal or both have a junk category, use confidence to decide.
    2.2 If both cannot be classified, use confidence to decide.
    2.3 If both one characters tokens, use confidence to decide.
    2.4. In case of a discrepancy where aws token ends with a ‘-’ and gcs token was found in the dictionary, choose gcs.
*/
enum decision
make_decision(struct Instance ins1,struct Instance ins2){
    struct nlist *ins1_dict = lookup_in_dictionary(ins1.normalized_text);
    struct nlist *ins2_dict = lookup_in_dictionary(ins2.normalized_text);
    if((ins2.junk_category>0 && ins1.junk_category>0)){
            return decide_by_confidence(ins1.confidence,ins2.confidence);
    }
    if((ins1_dict==NULL && ins2_dict!=NULL && ins1.junk_category==0) || (ins2.junk_category>0 && ins1.junk_category==0 && ins1_dict==NULL)){
        return GCS;
    }
    if((ins2_dict==NULL && ins1_dict!=NULL && ins2.junk_category==0) || (ins1.junk_category>0 && ins2.junk_category==0 && ins2_dict==NULL)){
        return AWS;
    }
    if(ins1_dict==NULL && ins2_dict==NULL){
        //when deciding between c. and C., prefer the non capital
        if(strlen(ins1.text)==2 && strlen(ins2.text)==2 && ins1.text[1]=='.' && ins2.text[1]=='.'){
            if(ins1.text[0]+32== ins2.text[0])
                return GCS;
              if(ins2.text[0]+32== ins1.text[0])
                return AWS;
        }
        if(contain_gibrish_or_numbers(ins1.text) && !contain_gibrish_or_numbers(ins2.text)){
            return GCS;
        }
        else if(!contain_gibrish_or_numbers(ins1.text) && contain_gibrish_or_numbers(ins2.text)){
            return AWS;
        }
        else{
            return decide_by_confidence(ins1.confidence,ins2.confidence);
        }
    }

    if(ins1_dict!=NULL && ins2_dict!=NULL){
        if(atoi(ins1_dict->defn)>atoi(ins2_dict->defn)){
            return AWS;
        }
        else if(atoi(ins2_dict->defn)>atoi(ins1_dict->defn)){
            return GCS;
        }
        else{
            if(strcmp(ins1.text,"a's")==0 && strcmp(ins2.text,"as")==0){
                return GCS;
            }
            return decide_by_confidence(ins1.confidence,ins2.confidence);
        }
    }

    if((ins1_dict!=NULL && ins2.junk_category>0) || (ins2_dict!=NULL && ins1.junk_category>0)){
        return decide_by_confidence(ins1.confidence,ins2.confidence);
    }
}

//Criteria for splitted token are:
//1. aws token ends with '-', length>1
//2. gcs token (the complete token) can be found in dictionary and don't have '-' where the aws token has
//3. The aws token is a substring of the gcs token
int is_splitted_token(struct Instance instance1, struct Instance instance2){
    char editedstr[strlen(instance1.text)-1];
    memmove(editedstr,instance1.text,strlen(instance1.text)-1);
    if(strcmp(instance2.text,editedstr)!=0){
        return 0;
    }
    if(instance1.text[strlen(instance1.text)-1]!='-' || strlen(instance1.text)==1)
        return 0;
    if(instance2.text[strlen(instance1.text)-1]=='-')
        return 0;
    /*for (int i=0;i<strlen(instance1.text)-1; i++){
        if(instance1.text[i]!=instance2.text[i])
            return 0;
    }
    */
    return 1;
}

/*
Picking from non_aligned_array pairs with maximum overlap size witougt repeats
*/
void insert_distinct_couples_with_max_overlap(int page){
  for(int i=non_aligned_ind-1;i>=0 && non_aligned_array[i].overlap>0;i--) {
    if (search_aligned(non_aligned_array[i].instance1.id)==NULL && search_aligned(non_aligned_array[i].instance2.id)==NULL){
      insert_into_aligned(non_aligned_array[i].instance1.id,aligned_ind);
      insert_into_aligned(non_aligned_array[i].instance2.id,aligned_ind);
      aligned_array[page-1].couples[aligned_ind].instance1= copy_instance(non_aligned_array[i].instance1);
      aligned_array[page-1].couples[aligned_ind].instance2= copy_instance(non_aligned_array[i].instance2);
      aligned_array[page-1].couples[aligned_ind].overlap= non_aligned_array[i].overlap;
      aligned_array[page-1].couples[aligned_ind].desc= make_decision(non_aligned_array[i].instance1,non_aligned_array[i].instance2);
      aligned_array[page-1].couples[aligned_ind].split=0;
      if(is_splitted_token(aligned_array[page-1].couples[aligned_ind].instance1,aligned_array[page-1].couples[aligned_ind].instance2)){ //case where one word was splitted between two lines
	aligned_array[page-1].couples[aligned_ind].split=1;
	aligned_array[page-1].couples[aligned_ind].desc= AWS;
      }
      // giving the aligned tokens discrepancy code
      if(strcmp(aligned_array[page-1].couples[aligned_ind].instance1.text,aligned_array[page-1].couples[aligned_ind].instance2.text)!=0){
	double similarity= calculateSimilarity(non_aligned_array[i].instance1.text,non_aligned_array[i].instance2.text);
	if(similarity==(double)0 && levenshtein_edit_distance(aligned_array[page-1].couples[aligned_ind].instance1.text,aligned_array[page-1].couples[aligned_ind].instance2.text).non_alnum>0){
	  aligned_array[page-1].couples[aligned_ind++].disc=NONALNUM;
	  continue;
	}
	if(similarity==(double)1 && strcmp(aligned_array[page-1].couples[aligned_ind].instance1.normalized_text,aligned_array[page-1].couples[aligned_ind].instance2.normalized_text)==0){
	  aligned_array[page-1].couples[aligned_ind++].disc=NONALNUM;
	  continue;
	}
	if(similarity==(double)0 && levenshtein_edit_distance(aligned_array[page-1].couples[aligned_ind].instance1.text,aligned_array[page-1].couples[aligned_ind].instance2.text).capital>0){
	  aligned_array[page-1].couples[aligned_ind++].disc=CAPITAL;
	  continue;
	}
	if(similarity<=0.5){
	  aligned_array[page-1].couples[aligned_ind++].disc=SIMILAR;
	  continue;
	}
	if(similarity>0.5){
	  aligned_array[page-1].couples[aligned_ind++].disc=DIFFERENT;
	  continue;
	}
      }
      else
	aligned_array[page-1].couples[aligned_ind++].disc=SAME;
    }
  }
}
/*
Juxta token- one token in one engine fits two or more tokens in the second engine
Voting rules for juxta token:
If the single token can be classified, choose him.
Otherwise, do voting separately for each token from the second engine.
*/
void insert_juxta_token(int page, int i){
    struct DataItem* source1_val= search_aligned(non_aligned_array[i].instance1.id);
    struct DataItem* source2_val= search_aligned(non_aligned_array[i].instance2.id);
    if(source1_val==NULL || source2_val== NULL){
        double similarity= calculateSimilaritySplit(non_aligned_array[i].instance1.text,non_aligned_array[i].instance2.text);
        if(similarity<=0.5){
           insert_into_aligned(non_aligned_array[i].instance1.id,aligned_ind);
           insert_into_aligned(non_aligned_array[i].instance2.id,aligned_ind);
           aligned_array[page-1].couples[aligned_ind].instance1= copy_instance(non_aligned_array[i].instance1);
           aligned_array[page-1].couples[aligned_ind].instance2= copy_instance(non_aligned_array[i].instance2);
           aligned_array[page-1].couples[aligned_ind].overlap= non_aligned_array[i].overlap;
           aligned_array[page-1].couples[aligned_ind].disc=JUXTA;
           aligned_array[page-1].couples[aligned_ind].split=0;

            //update previously inserted aligned tokens instance to be juxta
            if(source2_val!=NULL){
                int source2_ind = source2_val->data;
                aligned_array[page-1].couples[source2_ind].disc=JUXTA;
                if(aligned_array[page-1].couples[source2_ind].split)
                    aligned_array[page-1].couples[source2_ind].split=0;
                //voting
                if((lookup_in_dictionary(non_aligned_array[i].instance1.normalized_text)!=NULL || non_aligned_array[i].instance1.junk_category>0) &&
                aligned_array[page-1].couples[source2_ind].desc==AWS){
                    aligned_array[page-1].couples[aligned_ind++].desc=AWS;
                }
                else if(lookup_in_dictionary(non_aligned_array[i].instance1.normalized_text)==NULL && non_aligned_array[i].instance1.junk_category==0 &&
                aligned_array[page-1].couples[source2_ind].desc==GCS){
                        aligned_array[page-1].couples[aligned_ind++].desc=NEITHER; //gcs token was already chosen
                }
                else{
                   aligned_array[page-1].couples[aligned_ind].desc=make_decision(non_aligned_array[i].instance1,non_aligned_array[i].instance2);
                    if (aligned_array[page-1].couples[aligned_ind].desc==GCS && aligned_array[page-1].couples[source2_ind].desc==AWS){
                            //previous aws token has category but gcs token don't => choose aws
                        if((lookup_in_dictionary(aligned_array[page-1].couples[source2_ind].instance1.normalized_text)!=NULL || aligned_array[page-1].couples[source2_ind].instance1.junk_category>0) &&
                            (lookup_in_dictionary(aligned_array[page-1].couples[aligned_ind].instance2.normalized_text)==NULL && aligned_array[page-1].couples[aligned_ind].instance2.junk_category==0)){
                           aligned_array[page-1].couples[aligned_ind].desc=AWS;
                        }
                        else{
                            //previous aws token has higher confidence than gcs token => choose aws.
                            if(aligned_array[page-1].couples[source2_ind].instance1.confidence>aligned_array[page-1].couples[aligned_ind].instance2.confidence){
                               aligned_array[page-1].couples[aligned_ind].desc=AWS;
                            }
                            else{
                                // otherwise prefer gcs, but mark this one as NEITHER for not choosing it twice.
                                aligned_array[page-1].couples[source2_ind].desc=GCS;
                                aligned_array[page-1].couples[aligned_ind].desc=NEITHER;
                            }
                        }
                    }
                    else if (aligned_array[page-1].couples[aligned_ind].desc==GCS && aligned_array[page-1].couples[source2_ind].desc==GCS){
                       aligned_array[page-1].couples[aligned_ind].desc=NEITHER; //gcs token was already choosen
                    }
                    else if (aligned_array[page-1].couples[aligned_ind].desc==AWS && aligned_array[page-1].couples[source2_ind].desc==GCS){
                        if((lookup_in_dictionary(aligned_array[page-1].couples[source2_ind].instance2.normalized_text)==NULL && aligned_array[page-1].couples[source2_ind].instance2.junk_category==0) &&
                        (lookup_in_dictionary(aligned_array[page-1].couples[aligned_ind].instance1.normalized_text)!=NULL ||aligned_array[page-1].couples[aligned_ind].instance1.junk_category>0)){
                            aligned_array[page-1].couples[source2_ind].desc=AWS;
                        }
                        else{
                            //previous gcss token has higher confidence than current aws token => choose gcs (mark this one as NEITHER for not choosing it twice).
                            if(aligned_array[page-1].couples[source2_ind].instance2.confidence>aligned_array[page-1].couples[aligned_ind].instance1.confidence){
                               aligned_array[page-1].couples[aligned_ind].desc=NEITHER;
                            }
                            else{
                                aligned_array[page-1].couples[source2_ind].desc=AWS; // otherwise prefer aws.
                            }
                        }
                    }
                    aligned_ind++;
                }
            }
            if(source1_val!=NULL){
                int source1_ind = source1_val->data;
                aligned_array[page-1].couples[source1_ind].disc=JUXTA;
                if(aligned_array[page-1].couples[source1_ind].split)
                    aligned_array[page-1].couples[source1_ind].split=0;
                //voting

                if((lookup_in_dictionary(non_aligned_array[i].instance2.normalized_text)!=NULL || non_aligned_array[i].instance2.junk_category>0) &&
                aligned_array[page-1].couples[source1_ind].desc==GCS){
                    aligned_array[page-1].couples[aligned_ind++].desc=GCS;
                }

                else if(lookup_in_dictionary(non_aligned_array[i].instance2.normalized_text)==NULL && non_aligned_array[i].instance2.junk_category==0 &&
                aligned_array[page-1].couples[source1_ind].desc==AWS)
                    aligned_array[page-1].couples[aligned_ind++].desc=NEITHER; //aws token was already chosen
                else{
                   aligned_array[page-1].couples[aligned_ind].desc=make_decision(non_aligned_array[i].instance1,non_aligned_array[i].instance2);
                    if (aligned_array[page-1].couples[aligned_ind].desc==AWS && aligned_array[page-1].couples[source1_ind].desc==GCS){ // this time aws was chosen but for the previous token gcs was chosen
                        //previous gcs token has category but aws token don't => choose gcs
                        if((lookup_in_dictionary(aligned_array[page-1].couples[source1_ind].instance2.normalized_text)!=NULL || aligned_array[page-1].couples[source1_ind].instance2.junk_category>0) &&
                            (lookup_in_dictionary(aligned_array[page-1].couples[aligned_ind].instance1.normalized_text)==NULL && aligned_array[page-1].couples[aligned_ind].instance1.junk_category==0)){
                           aligned_array[page-1].couples[aligned_ind].desc=GCS;
                        }
                        else{
                            //previous gcs token has higher confidence than aws token => choose gcs.
                            if(aligned_array[page-1].couples[source1_ind].instance2.confidence>aligned_array[page-1].couples[aligned_ind].instance1.confidence){
                                aligned_array[page-1].couples[aligned_ind].desc=GCS;
                            }
                            else{
                                // otherwise prefer aws, but mark this one as NEITHER for not choosing it twice.
                                aligned_array[page-1].couples[source1_ind].desc=AWS;
                                aligned_array[page-1].couples[aligned_ind].desc=NEITHER;
                            }
                        }
                    }
                    else if (aligned_array[page-1].couples[aligned_ind].desc==AWS && aligned_array[page-1].couples[source1_ind].desc==AWS){
                       aligned_array[page-1].couples[aligned_ind].desc=NEITHER; //aws token was already choosen
                    }
                    else if (aligned_array[page-1].couples[aligned_ind].desc==GCS && aligned_array[page-1].couples[source1_ind].desc==AWS){
                        if((lookup_in_dictionary(aligned_array[page-1].couples[source1_ind].instance1.normalized_text)==NULL && aligned_array[page-1].couples[source1_ind].instance1.junk_category==0) &&
                        (lookup_in_dictionary(aligned_array[page-1].couples[aligned_ind].instance2.normalized_text)!=NULL ||aligned_array[page-1].couples[aligned_ind].instance2.junk_category>0)){
                            aligned_array[page-1].couples[source1_ind].desc=GCS;
                        }
                        else{
                            //previous aws token has higher confidence than current gcss token => choose aws (mark this one as NEITHER for not choosing it twice).
                            if(aligned_array[page-1].couples[source1_ind].instance1.confidence>aligned_array[page-1].couples[aligned_ind].instance2.confidence){
                               aligned_array[page-1].couples[aligned_ind].desc=NEITHER;
                            }
                            else{
                                aligned_array[page-1].couples[source1_ind].desc=GCS; // otherwise prefer gcs.
                            }
                        }
                    }
                    aligned_ind++;
                }
            }
        }
    }
}

/*
Voting rules for pairless token:
Use it only if it can be found in the dictionary or alternatively has a junk category or confidence value>=80.
*/
int insert_pairless_tokens(int page, int i, int aligned_ind) {
  //fprintf(stderr,"INSERTING NON_PAIRED :%d:\n",non_aligned_array[i].instance1.id);
  if(search_aligned(non_aligned_array[i].instance1.id)==NULL){
    //fprintf(stderr,"  NOT FOUND IN ALIGNED\n");    
    insert_into_aligned(non_aligned_array[i].instance1.id, aligned_ind);
    aligned_array[page-1].couples[aligned_ind].instance1= copy_instance(non_aligned_array[i].instance1);
    aligned_array[page-1].couples[aligned_ind].instance2= empty;
    aligned_array[page-1].couples[aligned_ind].overlap= 0;
    aligned_array[page-1].couples[aligned_ind].disc= GCSMISS;
    aligned_array[page-1].couples[aligned_ind].split=0;
    if(/*size_of_source2 == 0 || */lookup_in_dictionary(non_aligned_array[i].instance1.normalized_text)!=NULL || non_aligned_array[i].instance1.junk_category>0 || non_aligned_array[i].instance1.confidence>80) {
      //(strlen(non_aligned_array[i].instance1.normalized_text)==1 && non_aligned_array[i].instance1.confidence>70  && ((non_aligned_array[i].instance1.normalized_text[0]>=65 && non_aligned_array[i].instance1.normalized_text[0]<=90) || (non_aligned_array[i].instance1.normalized_text[0]>=97 && non_aligned_array[i].instance1.normalized_text[0]<=122))))
      aligned_array[page-1].couples[aligned_ind++].desc=AWS;
      //fprintf(stderr,"  AWS :%d:\n",aligned_ind);    
    } else {
      //fprintf(stderr,"  NEI\n");          
      aligned_array[page-1].couples[aligned_ind++].desc=NEITHER;
    }
  }
  if(size_of_source2 > 0 && search_aligned(non_aligned_array[i].instance2.id)==NULL){
    insert_into_aligned(non_aligned_array[i].instance2.id,aligned_ind);
    aligned_array[page-1].couples[aligned_ind].instance2= copy_instance(non_aligned_array[i].instance2);
    aligned_array[page-1].couples[aligned_ind].instance1= empty;
    aligned_array[page-1].couples[aligned_ind].overlap= 0;
    aligned_array[page-1].couples[aligned_ind].disc= AWSMISS;
    aligned_array[page-1].couples[aligned_ind].split=0;
    if(lookup_in_dictionary(non_aligned_array[i].instance2.normalized_text)!=NULL || non_aligned_array[i].instance2.junk_category>0 || atoi(non_aligned_array[i].instance2.normalized_text)!=0 || non_aligned_array[i].instance2.confidence>=80) {
      //(strlen(non_aligned_array[i].instance2.normalized_text)==1 && non_aligned_array[i].instance2.confidence>=70 && ((non_aligned_array[i].instance2.normalized_text[0]>=65 && non_aligned_array[i].instance2.normalized_text[0]<=90) || (non_aligned_array[i].instance2.normalized_text[0]>=97 && non_aligned_array[i].instance2.normalized_text[0]<=122))))
      aligned_array[page-1].couples[aligned_ind++].desc=GCS;
    } else {
      aligned_array[page-1].couples[aligned_ind++].desc=NEITHER;
    }
  }
  //fprintf(stderr,"INSERTED NON_PAIRED :%d:\n", aligned_ind);
  return aligned_ind;
}

//fill up aligned_array after array is sorted by overlap, therfore choose greedly each time the pairs with the maximun overlap.
void fill_aligned_tokens_array(int page, int non_aligned_ind, int aligned_ind){
    aligned_ind=0;
    //first iteration- choose only distinct instances with maximun overlap value
    printf("\t\t4.1. insert pairs with max overlap without repeats\n");
    insert_distinct_couples_with_max_overlap(page);
    //sencond iteration- trying to match pairless instances with instances that were already choosen as juxta tokens, decision is made by levenshtein_edit_distance
    printf("\t\t4.2. insert juxta and pairless tokens :%d:\n",non_aligned_ind);
    for(int i=non_aligned_ind-1;i>=0;i--){
      //printf("\t\t4.21 JUX :%d:\n",non_aligned_ind);      
      if(non_aligned_array[i].overlap!=0){
	insert_juxta_token(page, i);
        // instances with overlap=0 finally get picked if they're not already in the array
      } else {
	//printf("\t\t4.22 PAIRLESS aligned=%d:\n",aligned_ind);      	
	aligned_ind = insert_pairless_tokens(page, i, aligned_ind);
	//printf("\t\t4.23 PAIRLESS aligned=%d:\n",aligned_ind);      		
      }
    }
    aligned_array[page-1].num_of_couples=aligned_ind;
    printf("   4.3  page=%d: num_of_couples=%d:\n", page-1,aligned_ind);                
}

void print_couple_array(struct Couple A[], int size){
    for(int ind=0;ind<size;ind++){
        printf("overlap num. %d of size %d, discrepancy: %d, decision: %d\nsplit: %d\ninstance1:\nsource: %s\npage num: %d\ntext is: %s\nnormalized text is: %s\nconfidence: %d\nsn: %d\nmy_x0: %d\nmy_y0: %d\nmy_x1: %d\nmy_y1: %d\ninstance2:\nsource: %s\npage num: %d\ntext is: %s\nnormalized text is: %s\nconfidence: %d\nsn: %d\nmy_x0: %d\nmy_y0: %d\nmy_x1: %d\nmy_y1: %d\n",
            ind,A[ind].overlap,A[ind].disc,A[ind].desc,A[ind].split,
            A[ind].instance1.source,
            A[ind].instance1.page,
            A[ind].instance1.text,
            A[ind].instance1.normalized_text,
            A[ind].instance1.confidence,
            A[ind].instance1.sn_in_doc,
            A[ind].instance1.my_x1,
            A[ind].instance1.my_y1,
            A[ind].instance1.my_x2,
            A[ind].instance1.my_y2,
            A[ind].instance2.source,
            A[ind].instance2.page,
            A[ind].instance2.text,
            A[ind].instance2.normalized_text,
            A[ind].instance2.confidence,
            A[ind].instance2.sn_in_doc,
            A[ind].instance2.my_x1,
            A[ind].instance2.my_y1,
            A[ind].instance2.my_x2,
            A[ind].instance2.my_y2);
        }
}

// query to check whether this juxta token is the first to be added or not
int query_juxta_order(int doc_id,int aws_sn,char* table_name){
    static char query[5000];
    sprintf(query,"SELECT count(sn_in_doc_aws) FROM %s\n\
    WHERE doc_id=%d and sn_in_doc_aws=%d and text_gcs is not null",
    table_name,
    doc_id,
    aws_sn);

    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
    }

    sql_res = mysql_store_result(conn);
    if(NULL != (sql_row = mysql_fetch_row(sql_res))){
        return atoi(sql_row[0]);
    }
    return -1;
}

int is_the_gcs_token_already_inserted(int doc_id, int sn_in_doc_gcs,char* table_name){
    static char query[5000];
    sprintf(query,"SELECT count(id) FROM %s\n\
    WHERE doc_id=%d and sn_in_doc_gcs=%d",
    table_name,
    doc_id,
    sn_in_doc_gcs);

    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
    }

    sql_res = mysql_store_result(conn);
    if(NULL != (sql_row = mysql_fetch_row(sql_res))){
        return atoi(sql_row[0]);
    }
    return -1;
}

char* is_voted_text_in_already_inserted_gcs_token(int doc_id, char* text,int x1, int x2, int y1, int y2,char* table_name){
    static char query[5000];
    sprintf(query,"SELECT my_voted_text FROM %s\n\
    WHERE doc_id=%d and text_gcs='%s' and my_x1_gcs=%d and my_x2_gcs=%d and my_y1_gcs=%d and my_y2_gcs=%d",
    table_name,
    doc_id,
    text,
    x1,
    x2,
    y1,
    y2);

    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
    }

    sql_res = mysql_store_result(conn);
    if(NULL != (sql_row = mysql_fetch_row(sql_res))){
        return sql_row[0];
    }
    return "";
}

// query for pairless gcs token "sf" to check whether the previous aws token includes the word "sf"
int query_is_previous_aws_sf(int doc_id,int aws_sn,char* table_name){
    static char query[5000];
    sprintf(query,"SELECT my_voted_text FROM %s\n\
    WHERE doc_id=%d and sn_in_doc_aws=%d",
    table_name,
    doc_id,
    aws_sn);

    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
    }

    sql_res = mysql_store_result(conn);
    if(NULL != (sql_row = mysql_fetch_row(sql_res))){
        char s= sql_row[0][strlen(sql_row[0])-2];
        char f= sql_row[0][strlen(sql_row[0])-1];
        if(f=='f' && s=='s'){
            return 1;
        }
        else
            return 0;
    }
    return -1;
}

int find_inserted_after_sn(int doc_id, int gcs_sn, int curr_page){
    //printf("find inserted for sn_in_doc_gcs: %d\n",gcs_sn);
    static char query[5000];
    sprintf(query,"SELECT id,page FROM deals_alma_non_aligned_token\n\
        where doc_id=%d  and source_program='GCS' and sn_in_doc<%d order by sn_in_doc DESC",
        doc_id,
        gcs_sn);

    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
    }

    sql_res = mysql_store_result(conn);
    while(NULL != (sql_row = mysql_fetch_row(sql_res))){
        int id= atoi(sql_row[0]);
        if (search_aligned(id)!=NULL){
            int index= search_aligned(id)->data;
            int page= atoi(sql_row[1]);
            if (page!=curr_page){
                return 1000000;
            }
            if(aligned_array[page-1].couples[index].disc!=AWSMISS){
                //printf("sn_in_doc_aws: %d\n",aligned_array[page-1].couples[index].instance1.sn_in_doc);
                return aligned_array[page-1].couples[index].instance1.sn_in_doc;
            }
        }
    }
    return 1000000;
}

// query for finding duplicates and delete them
int check_if_duplicate_tokens (int doc_id,char* table_name){
    static char query[5000];
    static char query2[5000];
    static char query3[5000];
    // query for tokens of type GCSMISS- check if any AWSMISS token duplicate of them and if so delete it
    sprintf(query,"SELECT sn_in_doc_aws,normalized_text_aws FROM %s\n\
        where doc_id=%d and discrepancy=6",
        table_name,
        doc_id);

    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
    }

    sql_res = mysql_store_result(conn);
    while(NULL != (sql_row = mysql_fetch_row(sql_res))){
        int sn_in_doc_aws= atoi(sql_row[0]);
        char* normalized_text_aws= sql_row[1];
        sprintf(query2,"SELECT normalized_text_gcs, id FROM %s\n\
        where doc_id=%d and inserted_after_sn_in_doc=%d and order_after_sn_in_doc=1",
        table_name,
        doc_id,
        sn_in_doc_aws-1);
        if (mysql_query(conn, query2)) {
            fprintf(stderr,"%s\n",mysql_error(conn));
        }

        sql_res2 = mysql_store_result(conn);
        if(NULL != (sql_row = mysql_fetch_row(sql_res2))){
            if(strcmp(sql_row[0],normalized_text_aws)==0){
                int id_to_delete= atoi(sql_row[1]);
                sprintf(query3,"delete from %s\n\
                where doc_id=%d and id=%d",
                table_name,
                doc_id,
                id_to_delete);
                if (mysql_query(conn, query3)) {
                    fprintf(stderr,"%s\n",mysql_error(conn));
                }
            }
        }
    }
    // query for duplicate tokens of type AWSMISS with the same sn_in_doc_gcs- leave only one of them
    sprintf(query,"SELECT sn_in_doc_gcs,id FROM %s\n\
        where doc_id=%d and discrepancy=5 group by sn_in_doc_gcs having count(id)>1",
        table_name,
        doc_id);

    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
    }

    sql_res = mysql_store_result(conn);
    while(NULL != (sql_row = mysql_fetch_row(sql_res))){
        int sn_in_doc_gcs= atoi(sql_row[0]);
        int primary_duplicate_sn_id= atoi(sql_row[1]);
        sprintf(query2,"delete from %s where doc_id=%d and sn_in_doc_gcs=%d and id!=%d",
        table_name,
        doc_id,
        sn_in_doc_gcs,
        primary_duplicate_sn_id);
        if (mysql_query(conn, query2)) {
            fprintf(stderr,"%s\nquery: %s",mysql_error(conn),query2);
        }
    }
    return 0;
}

// query for tokens of type AWSMISS- find out what is the internal order between the gcs's tokens after the aws's token's sn
int update_order_after_sn_in_doc(int doc_id,char* table_name){
    static char query[5000];
    static char query2[5000];
    static char query3[49500];
    static char query4[5000];
    int run_query3=0;
    sprintf(query3,"update %s set order_after_sn_in_doc = (case id \n ",table_name);
    sprintf(query,"SELECT inserted_after_sn_in_doc, COUNT(id) AS count FROM %s\n\
        where doc_id=%d and inserted_after_sn_in_doc is not null and inserted_after_sn_in_doc!=1000000 GROUP BY inserted_after_sn_in_doc HAVING count>=1 order by count",
        table_name,
        doc_id);

    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
    }

    sql_res = mysql_store_result(conn);
    //order for inserted_after_Sn_in_doc!=1000000
    while(NULL != (sql_row = mysql_fetch_row(sql_res))){
        int inserted_after_sn= atoi(sql_row[0]);
        //printf("inserted_after_sn: %d\n",inserted_after_sn);
        sprintf(query2,"select id,normalized_text_gcs from %s\n\
        where doc_id=%d and inserted_after_sn_in_doc=%d order by sn_in_doc_gcs",
        table_name,
        doc_id,
        inserted_after_sn);
        if (mysql_query(conn, query2)) {
            fprintf(stderr,"%s\n",mysql_error(conn));
        }
        sql_res2 = mysql_store_result(conn);
        int order=1;
        while(NULL != (sql_row = mysql_fetch_row(sql_res2))){
            int token_id= atoi(sql_row[0]);
            if(order==1){
                char* normalized_text_gcs= sql_row[1];
                sprintf(query4,"select normalized_text from deals_alma_non_aligned_token\n\
                where doc_id=%d and source_program='AWS' and sn_in_doc=%d",
                doc_id,
                inserted_after_sn);
                if (mysql_query(conn, query4)) {
                    fprintf(stderr,"%s\n",mysql_error(conn));
                }
                sql_res3 = mysql_store_result(conn);
                if(NULL != (sql_row = mysql_fetch_row(sql_res3))){
                    char* normalized_text_aws= sql_row[0];

                    if(strcmp(normalized_text_gcs,normalized_text_aws)==0){
                        continue;
                    }
                }
            }
            //printf("id: %d\n",token_id);
            sprintf(query3+strlen(query3),"when %d then %d\n",
                token_id,order);
            run_query3=1;
            order++;
        }
    }
    sprintf(query,"select page_gcs from %s where doc_id=%d and inserted_after_sn_in_doc=1000000 group by page_gcs",
        table_name,
        doc_id);
    if (mysql_query(conn, query)) {
            fprintf(stderr,"%s\n",mysql_error(conn));
    }
    sql_res = mysql_store_result(conn);
    while(NULL != (sql_row = mysql_fetch_row(sql_res))){
        int page= atoi(sql_row[0]);
        sprintf(query2,"select id from %s\n\
        where doc_id=%d and inserted_after_sn_in_doc=1000000 and page_gcs=%d order by sn_in_doc_gcs",
        table_name,
        doc_id,
        page);
        if (mysql_query(conn, query2)) {
            fprintf(stderr,"%s\n",mysql_error(conn));
        }
        sql_res2 = mysql_store_result(conn);
        int order=1;
        while(NULL != (sql_row = mysql_fetch_row(sql_res2))){
            int token_id= atoi(sql_row[0]);
            //printf("id: %d\n",token_id);
            sprintf(query3+strlen(query3),"when %d then %d\n",
                token_id,order);
            run_query3=1;
            order++;
        }
    }
    if(run_query3){
        sprintf(query3+strlen(query3),"end)\n where doc_id=%d",doc_id);
        //printf("query: %s\n",query3);
        if (mysql_query(conn, query3)) {
            fprintf(stderr,"%s\n",mysql_error(conn));
        }
    }
    return 0;
}

int handle_juxta_tokens(int doc_id,int ind, char* table_name, int page){
    static char query[50000];
    char* voted_text ="";
    char* voted_ocr="";
    switch (aligned_array[page-1].couples[ind].desc)
    {
    case AWS:
        voted_text= aligned_array[page-1].couples[ind].instance1.text;
        voted_ocr= "AWS";
        break;
    case GCS:
        voted_text= aligned_array[page-1].couples[ind].instance2.text;
        voted_ocr= "GCS";
        break;
    default:
        break;
    }
    int not_first_juxta;
    if((not_first_juxta= query_juxta_order(doc_id,aligned_array[page-1].couples[ind].instance1.sn_in_doc,table_name))==-1)
                printf("error in query_juxta_order\n");
    if(!not_first_juxta){
        sprintf(query,"INSERT INTO %s\n\
                    (id,doc_id,overlap,page_AWS,text_AWS,normalized_text_AWS,confidence_AWS,my_x1_AWS,my_y1_AWS,my_x2_AWS,my_y2_AWS,sn_in_doc_AWS,\n\
                    page_GCS,text_GCS,normalized_text_GCS,confidence_GCS,my_x1_GCS,my_y1_GCS,my_x2_GCS,my_y2_GCS,sn_in_doc_GCS,discrepancy,split_token,voted_ocr,my_voted_text)\n\
                    values(0,%d,%d,%d,'%s','%s',%d,%d,%d,%d,%d,%d,%d,'%s','%s',%d,%d,%d,%d,%d,%d,%d,%d,'%s','%s')",
                table_name,
                doc_id,
                aligned_array[page-1].couples[ind].overlap,
                aligned_array[page-1].couples[ind].instance1.page,
                aligned_array[page-1].couples[ind].instance1.text,
                aligned_array[page-1].couples[ind].instance1.normalized_text,
                aligned_array[page-1].couples[ind].instance1.confidence,
                aligned_array[page-1].couples[ind].instance1.my_x1,
                aligned_array[page-1].couples[ind].instance1.my_y1,
                aligned_array[page-1].couples[ind].instance1.my_x2,
                aligned_array[page-1].couples[ind].instance1.my_y2,
                aligned_array[page-1].couples[ind].instance1.sn_in_doc,
                aligned_array[page-1].couples[ind].instance2.page,
                aligned_array[page-1].couples[ind].instance2.text,
                aligned_array[page-1].couples[ind].instance2.normalized_text,
                aligned_array[page-1].couples[ind].instance2.confidence,
                aligned_array[page-1].couples[ind].instance2.my_x1,
                aligned_array[page-1].couples[ind].instance2.my_y1,
                aligned_array[page-1].couples[ind].instance2.my_x2,
                aligned_array[page-1].couples[ind].instance2.my_y2,
                aligned_array[page-1].couples[ind].instance2.sn_in_doc,
                aligned_array[page-1].couples[ind].disc,
                aligned_array[page-1].couples[ind].split,
                voted_ocr,
                voted_text);
    }
    else if (not_first_juxta){
        if(!is_the_gcs_token_already_inserted(
        doc_id,
        aligned_array[page-1].couples[ind].instance2.sn_in_doc,
        table_name)){
            int inserted_after_sn_in_doc= find_inserted_after_sn(doc_id,aligned_array[page-1].couples[ind].instance2.sn_in_doc,page);
            if(strcmp(aligned_array[page-1].couples[ind].instance2.normalized_text,"sf")==0 && query_is_previous_aws_sf(doc_id,inserted_after_sn_in_doc,table_name)==1){
                voted_text="";
            }
            sprintf(query,"insert into %s\n\
            (doc_id,overlap,page_GCS,text_GCS,normalized_text_GCS,confidence_GCS,my_x1_GCS,my_y1_GCS,my_x2_GCS,my_y2_GCS,discrepancy,split_token,sn_in_doc_GCS,voted_ocr,my_voted_text,inserted_after_sn_in_doc)\n\
            values(%d,%d,%d,'%s','%s',%d,%d,%d,%d,%d,%d,%d,%d,'%s','%s',%d)",
                table_name,
                doc_id,
                aligned_array[page-1].couples[ind].overlap,
                aligned_array[page-1].couples[ind].instance2.page,
                aligned_array[page-1].couples[ind].instance2.text,
                aligned_array[page-1].couples[ind].instance2.normalized_text,
                aligned_array[page-1].couples[ind].instance2.confidence,
                aligned_array[page-1].couples[ind].instance2.my_x1,
                aligned_array[page-1].couples[ind].instance2.my_y1,
                aligned_array[page-1].couples[ind].instance2.my_x2,
                aligned_array[page-1].couples[ind].instance2.my_y2,
                aligned_array[page-1].couples[ind].disc,
                aligned_array[page-1].couples[ind].split,
                aligned_array[page-1].couples[ind].instance2.sn_in_doc,
                voted_ocr,
                voted_text,
                inserted_after_sn_in_doc);
        }
        else{
            if(strcmp(is_voted_text_in_already_inserted_gcs_token(doc_id,
            aligned_array[page-1].couples[ind].instance2.text,
            aligned_array[page-1].couples[ind].instance2.my_x1,
            aligned_array[page-1].couples[ind].instance2.my_x2,
            aligned_array[page-1].couples[ind].instance2.my_y1,
            aligned_array[page-1].couples[ind].instance2.my_y2,
            table_name),"")!=0 && aligned_array[page-1].couples[ind].desc==NEITHER){
                sprintf(query,"update %s set\n\
                my_voted_text=''\n\
                where doc_id=%d and text_gcs='%s' and my_x1_gcs=%d and my_x2_gcs=%d and my_y1_gcs=%d and my_y2_gcs=%d",
                table_name,
                doc_id,
                aligned_array[page-1].couples[ind].instance2.text,
                aligned_array[page-1].couples[ind].instance2.my_x1,
                aligned_array[page-1].couples[ind].instance2.my_x2,
                aligned_array[page-1].couples[ind].instance2.my_y1,
                aligned_array[page-1].couples[ind].instance2.my_y2);
            }
            else{
                return 0;
            }
        }
    }
    if(mysql_query(conn, query)){
        fprintf(stderr,"%s\n",mysql_error(conn));
        return 1;
    }
    return 0;
}


//
//mysql query to insert all pairs from aligned_array to 'deals_alma_aligned_token'
int enter_instances_into_aligned_token(int doc_id,int num_of_pages, char* table_name){
    static char query_two_sources[1000000];
    static char query_aws_source[1000000];
    static char query_gcs_source[1000000];
    sprintf(query_two_sources,"INSERT INTO %s\n\
                    (id,doc_id,overlap,page_AWS,text_AWS,normalized_text_AWS,confidence_AWS,my_x1_AWS,my_y1_AWS,my_x2_AWS,my_y2_AWS,sn_in_doc_AWS,\n\
                    page_GCS,text_GCS,normalized_text_GCS,confidence_GCS,my_x1_GCS,my_y1_GCS,my_x2_GCS,my_y2_GCS,sn_in_doc_GCS,discrepancy,split_token,voted_ocr,my_voted_text)\n\
                    values",table_name);
    sprintf(query_aws_source,"INSERT INTO %s\n\
                    (id,doc_id,overlap,page_AWS,text_AWS,normalized_text_AWS,confidence_AWS,my_x1_AWS,my_y1_AWS,my_x2_AWS,my_y2_AWS,discrepancy,split_token,sn_in_doc_AWS,voted_ocr,my_voted_text)\n\
                    values", table_name);
    sprintf(query_gcs_source,"INSERT INTO %s\n\
                    (id,doc_id,overlap,page_GCS,text_GCS,normalized_text_GCS,confidence_GCS,my_x1_GCS,my_y1_GCS,my_x2_GCS,my_y2_GCS,discrepancy,split_token,sn_in_doc_GCS,voted_ocr,my_voted_text,inserted_after_sn_in_doc)\n\
                    values", table_name);
    fprintf(stderr, "6.4: inserting %d pages\n", num_of_pages); 
    for(int page=1;page<=num_of_pages;page++){
        fprintf(stderr, "  6.5: page: %d\n", page); 	  
        for(int ind=aligned_array[page-1].num_of_couples-1;ind>=0;ind--) {
            if(aligned_array[page-1].couples[ind].disc==SAME || aligned_array[page-1].couples[ind].disc==NONALNUM) {
                continue;
            }
            char* voted_text ="";
            char* voted_ocr="";
            switch (aligned_array[page-1].couples[ind].desc)
            {
            case AWS:
                voted_text= aligned_array[page-1].couples[ind].instance1.text;
                voted_ocr= "AWS";
                break;
            case GCS:
                voted_text= aligned_array[page-1].couples[ind].instance2.text;
                voted_ocr= "GCS";
                break;
            default:
                break;
            }
            if(aligned_array[page-1].couples[ind].instance1.source!="" && aligned_array[page-1].couples[ind].instance2.source!=""){
                if(aligned_array[page-1].couples[ind].disc==JUXTA){
                    handle_juxta_tokens(doc_id,ind,table_name,page);
                    continue;
                }
                if (strlen(query_two_sources)>999500){
                    query_two_sources[strlen(query_two_sources)-1]=';';
                    if(mysql_query(conn, query_two_sources)){
                        fprintf(stderr,"%s\n",mysql_error(conn));
                        return 1;
                    }
                    else{
                        printf("\t- Insert query succeeded for query_two_sources into '%s', doc_id: %d\n\t\t%lu row/s was/were added\n",
                            table_name,
                            doc_id,
                            mysql_affected_rows(conn)
                            );
                        sprintf(query_two_sources,"INSERT INTO %s\n\
                        (id,doc_id,overlap,page_AWS,text_AWS,normalized_text_AWS,confidence_AWS,my_x1_AWS,my_y1_AWS,my_x2_AWS,my_y2_AWS,sn_in_doc_AWS,\n\
                        page_GCS,text_GCS,normalized_text_GCS,confidence_GCS,my_x1_GCS,my_y1_GCS,my_x2_GCS,my_y2_GCS,sn_in_doc_GCS,discrepancy,split_token,voted_ocr,my_voted_text)\n\
                        values",table_name);
                    }
                }
                sprintf(query_two_sources+strlen(query_two_sources),"(0,%d,%d,%d,'%s','%s',%d,%d,%d,%d,%d,%d,%d,'%s','%s',%d,%d,%d,%d,%d,%d,%d,%d,'%s','%s'),",
                    doc_id,
                    aligned_array[page-1].couples[ind].overlap,
                    aligned_array[page-1].couples[ind].instance1.page,
                    aligned_array[page-1].couples[ind].instance1.text,
                    aligned_array[page-1].couples[ind].instance1.normalized_text,
                    aligned_array[page-1].couples[ind].instance1.confidence,
                    aligned_array[page-1].couples[ind].instance1.my_x1,
                    aligned_array[page-1].couples[ind].instance1.my_y1,
                    aligned_array[page-1].couples[ind].instance1.my_x2,
                    aligned_array[page-1].couples[ind].instance1.my_y2,
                    aligned_array[page-1].couples[ind].instance1.sn_in_doc,
                    aligned_array[page-1].couples[ind].instance2.page,
                    aligned_array[page-1].couples[ind].instance2.text,
                    aligned_array[page-1].couples[ind].instance2.normalized_text,
                    aligned_array[page-1].couples[ind].instance2.confidence,
                    aligned_array[page-1].couples[ind].instance2.my_x1,
                    aligned_array[page-1].couples[ind].instance2.my_y1,
                    aligned_array[page-1].couples[ind].instance2.my_x2,
                    aligned_array[page-1].couples[ind].instance2.my_y2,
                    aligned_array[page-1].couples[ind].instance2.sn_in_doc,
                    aligned_array[page-1].couples[ind].disc,
                    aligned_array[page-1].couples[ind].split,
                    voted_ocr,
                    voted_text);
            }
            if(aligned_array[page-1].couples[ind].disc==AWSMISS && strcmp(voted_text,"")!=0){
                if (strlen(query_gcs_source)>999500){
                    query_gcs_source[strlen(query_gcs_source)-1]=';';
                    if(mysql_query(conn, query_gcs_source)){
                        fprintf(stderr,"%s\n",mysql_error(conn));
                        return 1;
                    }
                    else{
                            printf("\t- Insert query succeeded for query_gcs_source into '%s', doc_id: %d,\n\t\t%lu row/s was/were added\n",
                            table_name,
                            doc_id,
                            mysql_affected_rows(conn)
                            );
                        sprintf(query_gcs_source,"INSERT INTO %s\n\
                        (id,doc_id,overlap,page_GCS,text_GCS,normalized_text_GCS,confidence_GCS,my_x1_GCS,my_y1_GCS,my_x2_GCS,my_y2_GCS,discrepancy,split_token,sn_in_doc_GCS,voted_ocr,my_voted_text,inserted_after_sn_in_doc)\n\
                        values", table_name);
                    }
                }
                int inserted_after_sn_in_doc= find_inserted_after_sn(doc_id,aligned_array[page-1].couples[ind].instance2.sn_in_doc,page);
                sprintf(query_gcs_source+strlen(query_gcs_source),"(0,%d,%d,%d,'%s','%s',%d,%d,%d,%d,%d,%d,%d,%d,'%s','%s',%d),",
                    doc_id,
                    aligned_array[page-1].couples[ind].overlap,
                    aligned_array[page-1].couples[ind].instance2.page,
                    aligned_array[page-1].couples[ind].instance2.text,
                    aligned_array[page-1].couples[ind].instance2.normalized_text,
                    aligned_array[page-1].couples[ind].instance2.confidence,
                    aligned_array[page-1].couples[ind].instance2.my_x1,
                    aligned_array[page-1].couples[ind].instance2.my_y1,
                    aligned_array[page-1].couples[ind].instance2.my_x2,
                    aligned_array[page-1].couples[ind].instance2.my_y2,
                    aligned_array[page-1].couples[ind].disc,
                    aligned_array[page-1].couples[ind].split,
                    aligned_array[page-1].couples[ind].instance2.sn_in_doc,
                    voted_ocr,
                    voted_text,
                    inserted_after_sn_in_doc);
            }
            if(aligned_array[page-1].couples[ind].disc==GCSMISS){
                if (strlen(query_aws_source)>999500){
                    query_aws_source[strlen(query_aws_source)-1]=';';
                    if(mysql_query(conn, query_aws_source)){
                        fprintf(stderr,"%s\n",mysql_error(conn));
                        return 1;
                    }
                    else{
                            printf("\t- Insert query succeeded for query_aws_source into '%s', doc_id: %d\n\t\t%lu row/s was/were added\n",
                            table_name,
                            doc_id,
                            mysql_affected_rows(conn)
                            );
                        sprintf(query_aws_source,"INSERT INTO %s\n\
                        (id,doc_id,overlap,page_AWS,text_AWS,normalized_text_AWS,confidence_AWS,my_x1_AWS,my_y1_AWS,my_x2_AWS,my_y2_AWS,discrepancy,split_token,sn_in_doc_AWS,voted_ocr,my_voted_text)\n\
                        values", table_name);
                    }
                }
                sprintf(query_aws_source+strlen(query_aws_source),"(0,%d,%d,%d,'%s','%s',%d,%d,%d,%d,%d,%d,%d,%d,'%s','%s'),",
                    doc_id,
                    aligned_array[page-1].couples[ind].overlap,
                    aligned_array[page-1].couples[ind].instance1.page,
                    aligned_array[page-1].couples[ind].instance1.text,
                    aligned_array[page-1].couples[ind].instance1.normalized_text,
                    aligned_array[page-1].couples[ind].instance1.confidence,
                    aligned_array[page-1].couples[ind].instance1.my_x1,
                    aligned_array[page-1].couples[ind].instance1.my_y1,
                    aligned_array[page-1].couples[ind].instance1.my_x2,
                    aligned_array[page-1].couples[ind].instance1.my_y2,
                    aligned_array[page-1].couples[ind].disc,
                    aligned_array[page-1].couples[ind].split,
                    aligned_array[page-1].couples[ind].instance1.sn_in_doc,
                    voted_ocr,
                    voted_text);
            }
        }
        fprintf(stderr, "  6.6: DONE page: %d\n", page); 	  
    } // for
    if(query_gcs_source[strlen(query_gcs_source)-1]==','){
        query_gcs_source[strlen(query_gcs_source)-1]=';';
        if(mysql_query(conn, query_gcs_source)){
            fprintf(stderr,"%s, \nquery: %s\n",mysql_error(conn),query_gcs_source);
            return 1;
        }
        printf("\t- Insert query succeeded for query_gcs_source into '%s', doc_id: %d\n\t\t%lu row/s was/were added\n",
        table_name,
        doc_id,
        mysql_affected_rows(conn)
        );
    }
    if(query_aws_source[strlen(query_aws_source)-1]==','){
        query_aws_source[strlen(query_aws_source)-1]=';';
        if(mysql_query(conn, query_aws_source)){
            fprintf(stderr,"%s, \nquery: %s\n",mysql_error(conn),query_aws_source);
            return 1;
        }
        printf("\t- Insert query succeeded for query_aws_source into '%s', doc_id: %d\n\t\t%lu row/s was/were added\n",
            table_name,
            doc_id,
            mysql_affected_rows(conn)
            );
    }
    if(query_two_sources[strlen(query_two_sources)-1]==','){
        query_two_sources[strlen(query_two_sources)-1]=';';
        if(mysql_query(conn, query_two_sources)){
            fprintf(stderr,"%s, \nquery: %s\n",mysql_error(conn),query_two_sources);
            return 1;
        }
        printf("\t- Insert query succeeded for query_two_sources into '%s', doc_id: %d\n\t\t%lu row/s was/were added\n",
                table_name,
                doc_id,
                mysql_affected_rows(conn)
                );
    }
    printf("\ttime passed: %fs\n",get_time());
    printf("\t6.11. update order after sn in doc\n");
    update_order_after_sn_in_doc(doc_id, table_name);
    printf("\t6.12. delete duplicate tokens\n");
    check_if_duplicate_tokens(doc_id, table_name);
    return 0;
}

void clean(int num_of_pages){
    for(int j=0;j<num_of_pages;j++){
        for (int i=0;i<aligned_array[j].num_of_couples;i++){
            if(aligned_array[j].couples[i].instance1.source!=""){
                free(aligned_array[j].couples[i].instance1.source);
                free(aligned_array[j].couples[i].instance1.text);
                free(aligned_array[j].couples[i].instance1.normalized_text);
                free(search_aligned(aligned_array[j].couples[i].instance1.id));
           }
            if(aligned_array[j].couples[i].instance2.source!=""){
                free(aligned_array[j].couples[i].instance2.source);
                free(aligned_array[j].couples[i].instance2.text);
                free(aligned_array[j].couples[i].instance2.normalized_text);
                free(search_aligned(aligned_array[j].couples[i].instance2.id));
           }
        }
        free(aligned_array[j].couples);
    }
    free(aligned_array);
    free(hashtab_aligned);
    free(aws_data);
    free(gcs_data);
}

//delete allocated memory
void clean_per_page(int page){
    printf("\t6.4. cleaning memory for page: %d\n", page);
    for (int i=0;i<size_of_source1;i++){
        free(aws_data[page-1].instances[i].source);
        free(aws_data[page-1].instances[i].text);
        free(aws_data[page-1].instances[i].normalized_text);
    }
    for (int i=0;i<size_of_source2;i++){
        free(gcs_data[page-1].instances[i].source);
        free(gcs_data[page-1].instances[i].text);
        free(gcs_data[page-1].instances[i].normalized_text);
    }

    free(aws_data[page-1].instances);
    free(gcs_data[page-1].instances);
    for (int i=0;i<non_aligned_ind;i++){
        free(non_aligned_array[i].instance1.source);
        free(non_aligned_array[i].instance1.text);
        free(non_aligned_array[i].instance1.normalized_text);
        free(non_aligned_array[i].instance2.source);
        free(non_aligned_array[i].instance2.text);
        free(non_aligned_array[i].instance2.normalized_text);
    }

    free(non_aligned_array);
}

/*
1. reading data from table 'alma_non_aligned_token' and stores it to the array 'data_sources'
2. creating pairs of aws and gcs tokens while screening out irrelevant pairs and calculating overlap between them, into array 'non_aligned_token'
3. sorting the array by overlap size
4. packing into aligned_token array by unique
5. insert the aligned tokens into table 'deals_alma_aligned_token' and 'deals_ocr_discrepancy'
*/
void align_per_page(int doc_id, int num_of_pages){
    for(int page=1;page<=num_of_pages;page++){
        printf("\n*aligning page %d/%d*\n",page,num_of_pages);
        size_of_source1= aws_data[page-1].num_of_instances;
        size_of_source2= gcs_data[page-1].num_of_instances;
        if ((non_aligned_array= malloc(size_of_source1* MAX(size_of_source2,2) * sizeof(struct Couple)))==NULL) {
            fprintf(stderr, "Fatal: failed to allocate %zu bytes.\n", size_of_source1*size_of_source2 * sizeof(struct Couple));
            abort();
        }
        printf("\t6.1. creating pairs of aws and gcs tokens while screening out irrelevant pairs and calculating overlap\n\
        \ttime: %fs\n",get_time());
        non_aligned_ind = fill_non_aligned_array_and_calculate_overlap(page);
        printf("\ttime passed: %fs, non_aligned_ind=%d:\n",get_time(),non_aligned_ind);

        printf("\t6.2. sorting the pairs array by overlapping size\n\
        \ttime: %fs\n",get_time());
        if (size_of_source2 > 0) mergeSort(non_aligned_array, non_aligned_ind);
        printf("\tDONE MERGE time passed: %fs\n",get_time());
        aligned_array[page-1].page=page;

        if ((aligned_array[page-1].couples= malloc(100*non_aligned_ind * sizeof(struct Couple)))==NULL) {
            fprintf(stderr, "Fatal: failed to allocate %zu bytes.\n", 2*aligned_ind * sizeof(struct Couple));
            abort();
        }
        //print_couple_array(non_aligned_array,non_aligned_ind);
	if (size_of_source2 == 0) aligned_ind = 0; 
        printf("\t6.3. picking in a greedy way and without repeats pairs with maximum overlap size, juxta and pairless tokens\n\
        \tfille_aligned %d:%d: time: %fs\n", non_aligned_ind, aligned_ind, get_time());
        fill_aligned_tokens_array(page, non_aligned_ind, aligned_ind);
        //print_couple_array(aligned_array[page-1].couples,aligned_array[page-1].num_of_couples);
        if (size_of_source2 > 0) clean_per_page(page);
        printf("\tDONE PAGE %d: time passed: %fs\n",page, get_time());
    }
    printf("\tDONE ALIGN PER PAGE time passed:  %fs\n",get_time());
}

/*Expects:
    1. dictionary
    2. doc_id
    3. token table ('deals_alma_non_aligned_token') from 2 sources: aws and gcs
    4. aws stream token ('deals_ocrtoken')

output: tokens in table 'deals_alma_aligned_token'
*/
int main(int argc, char **argv){
    int opt,doc_id;
    char* dict_path;

    while ((opt = getopt(argc, argv, "d:2:3:4:5:6:")) != -1) {
        switch (opt) {
        case 'd':
            doc_id= atoi(optarg);
            break;
        case '2':
            dict_path= optarg;
            break;
        case '3':
            db_IP= optarg;
            break;
        case '4':
            db_name= optarg;
            break;
        case '5':
            db_user_name= optarg;
            break;
        case '6':
            db_pwd= optarg;
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-d doc_id] [-2 dict_path]  [-3 db_ip] [-4 db_name] [-5 db_user_name] [-6 db_pwd]\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn,db_IP,db_user_name,db_pwd,db_name,0,NULL,0)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      exit(1);
    }
    gettimeofday(&start, NULL);
    printf("4. parsing the dictionary\n");
    //format: id[\t]frequancy[\t]word
    parse_dictionary(dict_path);
    printf("time passed: %fs\n",get_time());
    delete_old_from_aligned_token_tables(conn,doc_id, "deals_alma_aligned_token");
    printf("time passed1: %fs\n",get_time());    
    delete_old_from_aligned_token_tables(conn,doc_id, "deals_ocr_discrepancy");
    printf("time passed2: %fs\n",get_time());    
    int num_of_pages= find_num_of_pages(doc_id);
    printf("no of pages=%d:\n",num_of_pages);        
    if ((aws_data= malloc(num_of_pages * sizeof(struct Source_data)))==NULL) {
        fprintf(stderr, "Fatal: failed to allocate %zu bytes.\n", num_of_pages * sizeof(struct Source_data));
        abort();
    }

    printf("4.5 DONE parsing the dictionary\n");    
     if ((gcs_data= malloc(num_of_pages * sizeof(struct Source_data)))==NULL) {
        fprintf(stderr, "Fatal: failed to allocate %zu bytes.\n", num_of_pages * sizeof(struct Source_data));
        abort();
     }

    printf("5. reading data from deals_alma_non_aligned_token\n\
    time: %fs\n",get_time());
    int size_of_aws= read_from_token_array_aws(doc_id);
    int size_of_gcs= read_from_token_array_gcs(doc_id);
    printf("size of hashtab aligned: %d\n",size_of_aws+size_of_gcs);
    set_up_hashtab_aligned(size_of_aws+size_of_gcs);
    if ((aligned_array= malloc(num_of_pages * sizeof(struct Couples_data)))==NULL) {
        fprintf(stderr, "Fatal: failed to allocate %zu bytes.\n", num_of_pages * sizeof(struct Couples_data));
        abort();
    }
    printf("6. Align per page, number of pages: %d\n",num_of_pages);
    align_per_page(doc_id,num_of_pages);
    printf("7. insert tokens into deals_alma_aligned_token and into deals_ocr_discrepancy\n\
    \ttime: %fs\n",get_time());
    //printf("entering data into deals_alma_aligned_token\n");
    enter_instances_into_aligned_token(doc_id, num_of_pages,"deals_alma_aligned_token");
    //enter_instances_into_aligned_token(doc_id,num_of_pages,"deals_ocr_discrepancy");
    printf("\ttime passed: %fs\n",get_time());
    printf("Finished aligning %d pages\ntime passed: %fs\n",num_of_pages,get_time());
    //clean(num_of_pages);
    mysql_close(conn);
    fclose(dictionary);
    printf("Out of Align\n");
    return 0;
}
