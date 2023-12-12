%{
    #include<stdio.h>
    #include <string.h>
    #include <mysql/mysql.h>
    #include <stdlib.h>
    #include <ctype.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include "import_functions.h"
    
    
    int page=0;
    #define MAX_TXT 1000
    #define MAX_CORD 100
    static char text[MAX_TXT];
    static char x1[MAX_CORD];
    static char x2[MAX_CORD];
    static char y1[MAX_CORD];
    static char y2[MAX_CORD];
    int confidence;
    enum category junk_catg;
    /*
    // for DB config
    static char *db_IP;
    static char *db_name;
    static char *db_pwd;
    static char *db_user_name; 
    MYSQL *conn;
    */
    
    int normalize_text_and_insert_into_token_array(int page,char *text, double x1,double x2,double y1,double y2,int confidence, enum category junk_catg);

    //runs the program 'gcs_validation' on the file at [gcp_path]
    int edit_json_file( char* gcp_path, char* gcs_editor_path){
        int status;
        pid_t child = fork();
        if(child==0){
            if (execlp(gcs_editor_path, gcs_editor_path, gcp_path, NULL) < 0) {
                perror("Unable to execute gcs_editor program");
                exit(1);
            }
            exit(1);
        }
        else{
            wait(&status);
            if(status){
                return 1;
            }
        }
        return 0;
    }


    // try to find junk_category for the given text
    enum category tokenization(char* text){
        junk_catg=NONE;
        int num_of_slashs=0;
        int non_digit=0;
        for (int i = 0; i < strlen(text); i++) {
            if(!isdigit(text[i])){
                non_digit++;
                if(i==0 && isalpha(text[i]) && strlen(text)>2)
                    junk_catg=MODEL;
            }
            if(junk_catg==MODEL && i==1){
                if(text[i]=='-')
                    non_digit=0;
                else
                    junk_catg=NONE;
            }
            // ‘(‘ and ‘)’ at the beginning or at the end of a token is acceptable
            if((i==0 || i==strlen(text)-1) && (text[i]=='(' || text[i]==')'))
                non_digit--;

            if(i!=0 && i!=strlen(text)-1 && text[i]==':' && non_digit==1 && isdigit(text[i+1]) && isdigit(text[i-1])){
                if(((i-3>0 && !isdigit(text[i-3]))|| i-3<0) && ((i+3<strlen(text) && !isdigit(text[i+3]))|| i+3==strlen(text))){
                    junk_catg= HOUR;
                    non_digit=0;
                }
            }
            if(i!=0 && i!=strlen(text)-1 && text[i]=='/' && non_digit==1 && isdigit(text[i+1]) && isdigit(text[i-1])){
                num_of_slashs++;
                non_digit=0;
            }
            if(i>0 && (text[i]==27 || text[i]=='\'') && (non_digit==1 ||non_digit==2) && num_of_slashs==0){
                junk_catg=FOOTINCH;
                non_digit=0;
            }
            if(i>0 && i==strlen(text)-1 && (non_digit==1 || (non_digit==2 && text[i-1]=='\\')) && isdigit(text[i-1]) && num_of_slashs==0){
                if(text[i]==27 || text[i]=='\''){
                    junk_catg=FOOT;
                    non_digit=0;
                }
                if((text[i]=='"' || text[i]=='\"') && junk_catg!=FOOTINCH){
                    junk_catg=INCH;
                    non_digit=0;
                }
                if((text[i]=='"' || text[i]=='\"') && junk_catg==FOOTINCH){
                    non_digit=0;
                }
                if(text[i]!='"' && text[i]!='\"' && junk_catg==FOOTINCH){
                    junk_catg=NONE;
                }
                if(text[i]=='.' && non_digit==1 && ((junk_catg==DECNUM) || (junk_catg!=DECNUM && strlen(text)<4))){
                    junk_catg=NUMERATION;
                    non_digit=0;
                }
            }
            if(i==strlen(text)-2 && non_digit==1 && ((text[i]=='s' && text[i+1]=='t') || (text[i]=='n' && text[i+1]=='d') || (text[i]=='r' && text[i+1]=='d') || (text[i]=='t' && text[i+1]=='h'))){
                junk_catg=ORDINAL;
                non_digit=0;
                break;
            }
            if(i==0 && text[i]=='$' && non_digit==1){
                junk_catg=PRICE;
                non_digit=0;
            }
            if(i==strlen(text)-4 && text[i]==',' && non_digit==1 && isdigit(text[i-1]) && isdigit(text[i+1]) && isdigit(text[i+2]) && isdigit(text[i+3])){
                if(junk_catg==NONE){
                    non_digit=0;
                    junk_catg=SF;
                }
            }
            if(i!=0 && i!=strlen(text)-1 && text[i]=='.' && non_digit==1 && isdigit(text[i+1]) && isdigit(text[i-1])){
                non_digit=0;
                if(junk_catg!=PRICE)
                    junk_catg=DECNUM;
            }       
        }
        if(!non_digit && junk_catg==NONE){
            if(num_of_slashs==2)
                junk_catg= DATE;
            else if(atoi(text)>=1980 && atoi(text)<=2030)
                junk_catg= YEAR;
            else if(strlen(text)==5 && atoi(text)!=0)
                junk_catg= ZIPCODE;
        }
        if(!non_digit && junk_catg!=NONE && junk_catg!=DATE && num_of_slashs!=0){
            junk_catg=NONE;
        }
        if(non_digit>1 || (non_digit==1 && junk_catg!=NONE && text[strlen(text)-1]!='.' && text[strlen(text)-1]!=',')){
            junk_catg=NONE;
        }
        
        return junk_catg; 
    }
     
%}
%x  WORDS COORDINATES COOR_X1 COOR_Y1 COOR_X2 COOR_Y2 TEXT CONF WORD_TEXT LAST_CHAR LAST_CHAR_TEXT PAGE_WIDTH PAGE_HEIGHT COOR1 COOR2 PAGENUM
%%

\"pages\":[\ ][\[]  {
    page++;
}

\"pageNumber\":[\ ]   {
    BEGIN PAGENUM;
}

<PAGENUM>[0-9]+ {
    if (atoi(yytext)==page+1){
        page++;
    }
}

<PAGENUM>\n {
    BEGIN 0;
}

\"width\":[\ ]  {
    BEGIN PAGE_WIDTH;
}

<PAGE_WIDTH>[0-9]+ {
    pageInfo[page_ind].page=page;
    pageInfo[page_ind].xx=atoi(yytext);
}

<PAGE_WIDTH>,[\ ]\"height\":[\ ]    {
    BEGIN PAGE_HEIGHT;
}

<PAGE_HEIGHT>[0-9]+ {
    pageInfo[page_ind].yy=atoi(yytext);
    page_ind++;
    BEGIN 0;
}

\"words\":[\ ][\[]  {
    BEGIN WORDS;
}

<WORDS>[\{][\t][\[][\t][\{][\t]  {
    BEGIN COOR1;
}

<COOR1>\"x\":[\ ]   {
    BEGIN COOR_X1;
}

<COOR1>\"y\":[\ ]   {
    BEGIN COOR_Y1;
}

<COOR_X1>[0-9.]+   {
    strcpy(x1,yytext);
}

<COOR_Y1>[0-9.]+ {
    strcpy(y1,yytext);
}

<COOR_X1>,[\ ] {
    BEGIN COOR1;
}

<COOR_Y1>,[\ ]  {
    BEGIN COOR1;
}

<COOR_X1>[\t]\},\n[\t]+\{[\t] {
    if(strcmp(y1,"")==0){
        BEGIN COOR1;
    }
    else
        BEGIN COOR2;
}

<COOR_Y1>[\t]\},\n[\t]+\{[\t]+  {
    if(strcmp(x1,"")==0){
        BEGIN COOR1;
    }
    else
        BEGIN COOR2;
}

<COOR2>[\{][\t]\"x\":[\ ]  {
    BEGIN COOR_X2;
}

<COOR2>[\{][\t]\"y\":[\ ]  {
    BEGIN COOR_Y2;
}

<COOR_X2>[0-9.]+ {
    strcpy(x2,yytext);
}

<COOR_X2>,[\ ]\"y\":[\ ]     {
    BEGIN COOR_Y2;
}

<COOR_Y2>,[\ ]\"x\":[\ ]     {
    BEGIN COOR_X2;
}

<COOR_Y2>[0-9.]+ {
    strcpy(y2,yytext);
}

<COOR_X1>[\t][\}]\n  {
    BEGIN WORDS;
}

<COOR_Y1>[\t][\}]\n  {
    BEGIN WORDS;
}

<COOR_X2>[\t][\}]  {
    BEGIN WORDS;
}

<COOR_Y2>[\t][\}]  {
    BEGIN WORDS;
}

<WORDS>\"symbols\":[\ ][\[] {
    BEGIN WORD_TEXT;
}

<WORD_TEXT>\"text\":[\ ]\"    {
    BEGIN TEXT;
}

<WORD_TEXT>\"detectedBreak\":  {
    BEGIN LAST_CHAR;
}

<LAST_CHAR>\"text\":[\ ]\"  {
    BEGIN LAST_CHAR_TEXT;
}

<TEXT>.  {
    strcat(text,yytext);
}

<TEXT>”|“|"\u201d"|"\u201c" {
    strcat(text,"\"");
}

<TEXT>"\u00B0"  {
    strcat(text,"°");
}

<TEXT>"\u2019" {
    strcat(text,"\'");
}

<TEXT>"\u2013" {
    strcat(text,"-");
}

<TEXT>"\u0434" {
    strcat(text,"д");
}

<TEXT>"\u043f" {
    strcat(text,"п");
}

<TEXT>\",   {
    BEGIN WORD_TEXT;
}

<LAST_CHAR_TEXT>.  {
    strcat(text,yytext);
}

<LAST_CHAR_TEXT>”|“|"\u201d"|"\u201c" {
    strcat(text,"\"");
}

<LAST_CHAR_TEXT>"\u2019" {
    strcat(text,"\'");
}

<LAST_CHAR_TEXT>"\u2013" {
    strcat(text,"-");
}

<LAST_CHAR_TEXT>"\u0434" {
    strcat(text,"д");
}

<LAST_CHAR_TEXT>"\u043f" {
    strcat(text,"п");
}

<LAST_CHAR_TEXT>\"\n[\t]+[\]]\n[\t]+\"confidence\":[\ ] {
    BEGIN CONF;
}

<CONF>[0-9.]+   {
    confidence= (int)(atof(yytext)*100);;
    tokenization(text);
    normalize_text_and_insert_into_token_array(page,text, atof(x1),atof(x2),atof(y1),atof(y2),confidence,junk_catg);
    memset(text,0,strlen(text));
    memset(x1,0,strlen(x1));
    memset(y1,0,strlen(y1));
}

<CONF>\n[\t]+[\}]\n[\t]+/[\{]    {
    BEGIN WORDS;
}

<CONF>\n[\t]+[\}]\n[\t]+[\]]    {
    BEGIN 0;
}

<COOR2>.|\n ;

<COORDINATES>.|\n ;

<WORDS>.|\n ;

<WORD_TEXT>.|\n ;

<LAST_CHAR>.|\n ;

.|\n ;
%%

int yywrap()
{
    return 1;
}

//program input: -1 doc_id -2 program_path -3 media_path -4 gcs_editor_path -5 db_ip -6 db_name -7 db_user -8 db_pwd
int main(int argc, char **argv){
    int opt;
    source_program= "GCS";
    char *media_path, *gcs_editor_path;

    while ((opt = getopt(argc, argv, "d:2:3:4:5:6:7:")) != -1) {
        switch (opt) {
        case 'd':
            doc_id= atoi(optarg);
            break;
        case '2':
            media_path= optarg;
            break;
        case '3':
            gcs_editor_path= optarg;
            break;
        case '4':
            db_IP= optarg;
            break;
        case '5':
            db_name= optarg;
            break;
        case '6':
            db_user_name= optarg;
            break;
        case '7':
            db_pwd= optarg;
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-d doc_id] [-2 media_path]  [-3 gcs_editor_path] [-4 db_ip] [-5 db_name] [-6 db_user_name] [-7 db_pwd]\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    fprintf(stderr,"LLLI\n");
    conn = mysql_init(NULL);
    mysql_options(conn, MYSQL_SET_CHARSET_NAME, "latin1");
    mysql_options(conn, MYSQL_INIT_COMMAND, "SET NAMES latin1");
    if (!mysql_real_connect(conn,db_IP,db_user_name,db_pwd,db_name,0,NULL,0)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      exit(1);
    }

    fprintf(stderr,"LLL0\n");
    //gets the gcp_json path according to [doc_id]
    char* gcpjson_path= get_path_to_source_file(conn,doc_id,media_path,"gcpjsonfile");
    //check format validation of the file before processing
    if(edit_json_file(gcpjson_path, gcs_editor_path))
        exit(1);
    char edited_json[strlen(gcpjson_path)+8];
    sprintf(edited_json,"%s_edited",gcpjson_path);
    fprintf(stderr,"LLL1\n");
    extern FILE *yyin;
    yyin= fopen(edited_json, "r");
    if(yyin==NULL){
        fprintf(stderr, "\tError opening edited gcsjson file %s: %s\n", edited_json, strerror(errno));
        exit(1);
    }
    printf("\t- succeeded opening edited gcsjson file\n");
    delete_old_from_sql(conn,doc_id,"deals_alma_non_aligned_token","GCS");
    yylex();
    //print_array();
    //print_my_array(page_ind);
    enter_into_sql(doc_id,"deals_alma_non_aligned_token","GCS");
    enter_into_OcrPageSize(conn,doc_id,page_ind);
    mysql_close(conn);
    fclose(yyin);
    return 0;
}
