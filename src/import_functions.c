#include <stdio.h>
#include <string.h>
#include <mysql/mysql.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <errno.h>
#include "import_functions.h"

// query to get gcp_json path
char* get_path_to_source_file(MYSQL* conn, int doc_id, char* media_path, char* source_file){
    static char query[5000];
    static char sub_folder[5000];
    MYSQL_RES *sql_res;
    MYSQL_ROW sql_row;
    sprintf(query,"SELECT %s FROM deals_document\n\
    WHERE id=%d",
    source_file,
    doc_id);

    if (mysql_query(conn, query)) {
        fprintf(stderr,"%s\n",mysql_error(conn));
    }

    sql_res = mysql_store_result(conn);
    if (NULL != (sql_row = mysql_fetch_row(sql_res))) {
        char* sub_folder= sql_row[0];
        char program_path[1000];
        sprintf(program_path,"%s/%s",media_path,sub_folder);
	fprintf(stderr,"ProgPath:%s:\n", program_path);    
        return strdup(program_path);
    }

    return NULL;
}

//edit the text variable to ignore special charactars
char *edit_special_chars(char *text) {
    static char bext[5000];
    int j, i;
    for (i = 0, j = 0; i < strlen(text); i++) {
        if (text[i] == '\"' || text[i] == '\'' || text[i] == '\\') {
            if(i!=strlen(text)-1 && text[i]== '\\' && text[i+1] == '\"')
                continue;
            else
                bext[j++] = '\\';
        }
        bext[j++] = text[i];
            
    }
    bext[j++] = '\0';
    return strdup(bext);
}

int normalize_text_and_insert_into_token_array(int page,char *text, double x1,double x2,double y1,double y2,int confidence,enum category junk_catg){

    if(strcmp(text,"Ist")==0){
        strcpy(text,"1st");
    }
    static char bext[5000];
    int split=0;
    int j, i;
    for (i = 0, j = 0; i < strlen(text); i++) {
        if(i<strlen(text)-1 && text[i]=='\'' && (text[i+1]=='s' || text[i+1]=='S')){
            break;
        }
        if(text[i]>=65 && text[i]<=90){
            bext[j++] = text[i]+32;
            continue;
        }
        if (isalpha(text[i]) || isdigit(text[i])) {
            bext[j++] = text[i];
        }
        //for juxta checking later on
        if(i==strlen(text)-1 && text[i]=='-' && isalpha(text[i-1])){
            bext[j++]=text[i];
        }
        //split tokens in normalization that connected with '-', '\' or '/'
        if (i!=0 && (text[i]=='-' || text[i]=='\\' || text[i]=='/') && i!=strlen(text)-1 && isalpha(text[i+1]) && isalpha(text[i-1]) ){
            bext[j] = '\0';
            if(split==0){
                split=sn;
            }
            insert_into_token_array(page,text,strdup(bext),x1,x2,y1,y2,confidence,NONE,split);
            j=0;
            continue;
        }
        if (i!=0 && (text[i]=='-' || text[i]==',') && i<strlen(text)-2 && text[i+1]=='-' && isalpha(text[i+2]) && isalpha(text[i-1]) ){
            bext[j] = '\0';
            if(split==0){
                split=sn;
            }
            insert_into_token_array(page,text,strdup(bext),x1,x2,y1,y2,confidence,NONE,split);
            j=0;
            i++;
            continue;
        }
    }
    bext[j++] = '\0';
    sn++;
    if(split) {
      //fprintf(stderr,"    TOK :%d:%s:",sn-1,text);
      return insert_into_token_array(page,text,strdup(bext),x1,x2,y1,y2,confidence,NONE,split);
    }
    //fprintf(stderr,"      TOK1 :%d:%s:",sn-1,text);    
    return insert_into_token_array(page,text,strdup(bext),x1,x2,y1,y2,confidence,junk_catg,sn-1);
}
        

int insert_into_token_array(int page,char *text, char *normalized_text, double x1,double x2,double y1,double y2,int confidence,enum category junk_catg,int sn_in_doc){
    if(curr_ind==100000){
        enter_into_sql(doc_id,"deals_alma_non_aligned_token",source_program);
    }
    instances[curr_ind++]= (struct Instance) {0,
        page,
        sn_in_doc,
        "",
        edit_special_chars(text),
        normalized_text,
        x1,
        x2,
        y1,
        y2,
        multiple_factor(x1),
        multiple_factor(x2),
        multiple_factor(y1),
        multiple_factor(y2),
        confidence,
        junk_catg};
    return 0;
}
    

int print_array(){
    int i;
    for(i=0;i<curr_ind;i++){
            printf("page num: %d\ntext is: %s\nnormalized text is: %s\nx0: %.17lf\ny0: %.17lf\nx1: %.17lf\ny1: %.17lf\nmy_x0: %d\nmy_y0: %d\nmy_x1: %d\nmy_y1: %d\nconfidence: %d\njunk catg: %d\n",      
            instances[i].page,
            instances[i].text,
            instances[i].normalized_text,
            instances[i].x1,
            instances[i].y1,
            instances[i].x2,
            instances[i].y2,
            instances[i].my_x1,
            instances[i].my_y1,
            instances[i].my_x2,
            instances[i].my_y2,
            instances[i].confidence,
            instances[i].junk_category);
    }
    return i;
}

//mysql query for enter instance into [table_name]
int enter_into_sql(int doc_id,char* table_name,char* source){
    static char query[50000];
    unsigned long total_rows_affected=0;
    printf("\t- REALLY entering into alma_non_aligned_token table:%d:\n",curr_ind);    
    sprintf(query,"INSERT INTO %s(id,text,normalized_text,junk_category,x1,y1,x2,y2,my_x1,my_y1,my_x2,my_y2,page,source_program,confidence,doc_id,sn_in_doc) VALUES",table_name);
    for(int i=0;i<curr_ind;i++){
        if (strlen(query)>49000){
            query[strlen(query)-1]=';';
            if(mysql_query(conn, query)){
                fprintf(stderr,"%s\n",mysql_error(conn));
                return 1;
            }
            else{
                total_rows_affected+=mysql_affected_rows(conn);
                sprintf(query,"INSERT INTO %s(id,text,normalized_text,junk_category,x1,y1,x2,y2,my_x1,my_y1,my_x2,my_y2,page,source_program,confidence,doc_id,sn_in_doc) VALUES",table_name);
            }
        }
        sprintf(query+ strlen(query),"(0,'%s','%s',%d,%.17lf,%.17lf,%.17lf,%.17lf,%d,%d,%d,%d,%d,'%s',%d,%d,%d),",
            instances[i].text,
            instances[i].normalized_text,
            instances[i].junk_category,
            instances[i].x1,
            instances[i].y1,
            instances[i].x2,
            instances[i].y2,
            instances[i].my_x1,
            instances[i].my_y1,
            instances[i].my_x2,
            instances[i].my_y2,
            instances[i].page,
            source,
            instances[i].confidence,
            doc_id,
            instances[i].sn_in_doc);
    }
    if(query[strlen(query)-1]==','){
        query[strlen(query)-1]=';';
	fprintf(stderr,"PRINTING NON-ALIGNED QUERY:%s:\n",query);
        if(mysql_query(conn, query)){
                fprintf(stderr,"%s\n",mysql_error(conn));
                return 1;
        }
        else{
            printf("\t- Insert query successed for %s source program, doc_id: %d\n\t%lu row/s was/were added\n",
            source,
            doc_id,
            total_rows_affected+mysql_affected_rows(conn)
            );
            curr_ind=0;
        }
    }
    return 0;
}

//mysql query for delete all data of [doc_id] from the [table_name]
int delete_old_from_sql(MYSQL* conn,int doc_id, char* table_name,char* source){
    static char query[50000];
    sprintf(query,"delete from %s \n\
                     where doc_id = %d and source_program='%s'"
      ,table_name, doc_id,source);
    //char * reset_id= "alter table Block_table auto_increment =1"; //reset block_id after deleting
    if(mysql_query(conn, query)){ //|| mysql_query(conn,reset_id)){
        fprintf(stderr,"%s\n",mysql_error(conn));
        return 1;
    }
    else{
        printf("\t- Delete query from %s successed for source: %s, doc_id: %d, %lu rows were deleted\n",
        table_name, source, doc_id, mysql_affected_rows(conn));
    }
    return 0;
}


int remove_last_comma(char *text) {
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
    return 0;
}


int enter_into_OcrPageSize(MYSQL* conn, int doc_id, int size){
    // AS 06/24/21:
    // Disabling this function for now because Google OCR doesn't work and is disabled.
    // Instead this data is now inserted to the DB in python during Textract.
    // TODO: fix google
    return 0;

    static char query[50000];
    sprintf(query,"delete from deals_ocrpagesizeinpixel where doc_id = %d"
      ,doc_id);
    if(mysql_query(conn, query)){
        fprintf(stderr,"%s\n",mysql_error(conn));
        return 1;
    }
    else{
        printf("\t- Delete query from OcrPageSize successed for doc_id: %d, %lu rows were deleted\n",
        doc_id, mysql_affected_rows(conn));
    }
    sprintf(query,"INSERT INTO deals_ocrpagesizeinpixel (doc_id,page,xx,yy) VALUES \n");
    for(int i=0;i<size;i++){
        sprintf(query+ strlen(query),"(%d,%d,%d,%d), \n ",doc_id,pageInfo[i].page,pageInfo[i].xx,pageInfo[i].yy);
    }
    //query[strlen(query)-1]=';';
    remove_last_comma(query);
    if(mysql_query(conn, query)){
        fprintf(stderr,"%s\n",mysql_error(conn));
        printf("query: %s\n",query);
        return 1;
    }
    else{
        printf("\t- Insert query successed into deals_ocrpagesizeinpixel, doc_id: %d\n\t%lu row/s was/were added\n",
        doc_id,
        mysql_affected_rows(conn)
        );
    }
}

//mysql query that delete all data where [doc_id] from [table_name]
int delete_old_from_aligned_token_tables(MYSQL* conn,int doc_id, char* table_name){
    static char query[50000];
    sprintf(query,"delete from %s \n\
                     where doc_id =%d",
      table_name,doc_id);
    if(mysql_query(conn, query)){
        fprintf(stderr,"%s\n",mysql_error(conn));
        return 1;
    }
    /*else{
        printf("Delete query successed for doc_id: %d,page: %d %lld rows were deleted\n",
        doc_id, page, mysql_affected_rows(conn));
    }*/
    return 0;
}

int print_pageInfo_array(int size){
    int i;
    for(i=0;i<size;i++){
            printf("page: %d\nxx: %d\nyy: %d\n",
            pageInfo[i].page,pageInfo[i].xx,pageInfo[i].yy);
    }
    return i;
}

int multiple_factor(double i){
    return (int)(i*10000);
}

// ***hash table implement for dictionary***
/* hash: form hash value for string s */
unsigned hash_dict(char *s)
{
    unsigned hashval;
    for (hashval = 0; *s != '\0'; s++)
      hashval = *s + 31 * hashval;
    return hashval % HASHSIZE;
}

/* lookup: look for s in hashtab_dict */
struct nlist *lookup_in_dictionary(char *s)
{
    struct nlist *np;
    for (np = hashtab_dict[hash_dict(s)]; np != NULL; np = np->next)
        if (strcmp(s, np->name) == 0){
            return np; /* found */
        }
    return NULL; /* not found */
}

int ignore_word_for_dictionary(char* s){
    if(strlen(s)==1){
        return 1;
    }
    for(int i=0;i<strlen(s);i++){
        if(isdigit(s[i])){
            return 1;
        }
    }
    return 0;
}

/* install: put (name, defn) in hashtab */
struct nlist *install_in_dictionary(char *name, char *defn)
{
    struct nlist *np;
    unsigned hashval;
    if ((np = lookup_in_dictionary(name)) == NULL) { /* not found and a standard word*/
        np = (struct nlist *) malloc(sizeof(*np));
        if (np == NULL || (np->name = strdup(name)) == NULL)
          return NULL;
        hashval = hash_dict(name);
        np->next = hashtab_dict[hashval];
        hashtab_dict[hashval] = np;
    } else /* already there */
        free((void *) np->defn); /*free previous defn */
    if ((np->defn = strdup(defn)) == NULL)
       return NULL;
    return np;
}

//go through dictionary file and add {word,frequency} into hash_tab
int
parse_dictionary(char* dict_path){
    //char *dictionary_path= "/home/ubuntu/dealthing/votingstuff/words_file_for_bigrams";
    dictionary= fopen(dict_path,"r");
    if(dictionary==NULL){
        fprintf(stderr, "Error opening dictionary file %s: %s\n", dict_path, strerror(errno));
        exit(1);
    }
    char line[1024];
    int ii = 0;
    while (fgets(line, 1024, dictionary))  {
        char* token,*save;
        token = strtok_r(line, "\t", &save);
        if (token == NULL)
                printf("error in parsing dictionary\n");
        char copy_of_save[strlen(save)];
        strcpy(copy_of_save,save);
        char* value= strtok(copy_of_save,"\t");
        token = strtok_r(save, "\t", &save);
        if (token == NULL)
                printf("error in parsing dictionary\n");
        char key[strlen(save)];
        strcpy(key,save);
        key[strlen(save)-1]='\0';
        if(!ignore_word_for_dictionary(key)){
            if(install_in_dictionary(key,value)==NULL)
                printf("couldn't install key: %s with value: %s\n",key,value);
        }
	ii++;
    }
    fprintf(stderr,"Found :%d: dictionary items\n", ii);
   return 0;
}

int isAlnum(char c){
    if((c>=32 && c<48) || (c>=58 && c<65) || (c>=91 && c<97) || (c>=123 && c<127))
        return 0;
    else
        return 1;
}

//calculate edit distance between two words
struct edit_distance levenshtein_edit_distance (const char * word1, const char * word2)
{
    struct edit_distance edit= {0,0,0};
    int len1= strlen(word1);
    int len2= strlen(word2);
    int matrix[len1 + 1][len2 + 1];
    int i;
    for (i = 0; i <= len1; i++) {
        matrix[i][0] = i;
    }
    for (i = 0; i <= len2; i++) {
        matrix[0][i] = i;
    }
    for (i = 1; i <= len1; i++) {
        int j;
        char c1;

        c1 = word1[i-1];
        for (j = 1; j <= len2; j++) {
            char c2;

            c2 = word2[j-1];
            if (c1 == c2) {
                matrix[i][j] = matrix[i-1][j-1];
            }
            else {
                int delete;
                int insert;
                int substitute;
                int minimum;

                delete = matrix[i-1][j] + 1;
                insert = matrix[i][j-1] + 1;
                substitute = matrix[i-1][j-1] + 1;
                minimum = delete;
                if (insert < minimum) {
                    minimum = insert;
                }
                // TOC difference where gcs adss '.' and aws don't
                if(j>len1 && c2=='.' && i==len1){
                    edit.non_alnum++;
                }
                if (substitute < minimum) {
                    minimum = substitute;
                    // tokens only differ
                    if((!isAlnum(c1) && len1>len2 && (i==0 || i==len1)) || (!isAlnum(c2) && len2>len1 && (j==0 || j==len2))){
                        edit.non_alnum++;
                    }
                    // one engine got nonalnum char differently than the second engine
                    if(!isAlnum(c1) && !isAlnum(c2))
                        edit.non_alnum++;
                    // TOC difference where gcs adss '.' and aws don't
                    if(j>=len1 && c2=='.')
                        edit.non_alnum++;
                    // capital letters differnces
                    if(c1+32==c2 || c2+32==c1)
                        edit.capital++;
                }
                matrix[i][j] = minimum;
            }
        }
    }
    edit.alnum= matrix[len1][len2]-(edit.non_alnum+edit.capital);
    return edit;
}
