%{
    #include<stdio.h>
    #include <string.h>
    #include <mysql/mysql.h>
    #include <stdlib.h>
    #include <ctype.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
    #include "import_functions.h"


    int page=0;
    #define MAX_TXT 1000
    #define MAX_CORD 100
    static char text[MAX_TXT];
    static char x0[MAX_CORD];
    static char x1[MAX_CORD];
    static char y0[MAX_CORD];
    static char y1[MAX_CORD];
    static char confidence[MAX_CORD];
    enum category junk_catg=NONE;

    /*
    // for DB config 
    static char *db_IP;
    static char *db_name;
    static char *db_pwd;
    static char *db_user_name; 
    MYSQL *conn;
    */
    
    int normalize_text_and_insert_into_token_array(int page,char *text, double x1,double x2,double y1,double y2,int confidence,enum category junk_catg);
    
%}
%x WORD TXT PAGE GEO GEOX0 GEOX1 GEOY0 GEOY1 CONF
%%

"\"BlockType\":"[\ \t]*\"WORD\",[\n] {
  BEGIN CONF;
}
 
<WORD>[\ \t]*\""Text"\"[\:][\ ]\" {
    BEGIN TXT;
}

<TXT>. {
    strcat(text,yytext);
}

<TXT>[0-9]{1,2}[:][0-9][0-9]/[\"][,][\n] {
    strcpy(text,yytext);
    junk_catg=HOUR;
}

<TXT>((0(1|2|3|4|5|6|7|8|9))|(10|11|12))[/]([0-2][0-9]|[3][0-1])[/][0-9][0-9]/[\"][,][\n] {
    strcpy(text,yytext);
    junk_catg=DATE;
}

<TXT>((0(1|2|3|4|5|6|7|8|9))|(10|11|12))[/]([0-2][0-9]|[3][0-1])[/][1-2][0-9][0-9][0-9]/[\"][,][\n] {
    strcpy(text,yytext);
    junk_catg=DATE;
}

<TXT>[(]*[$][0-9]+[.,)]*/[\"][,][\n] {
    strcpy(text,yytext);
    junk_catg=PRICE;
}

<TXT>[(]*[$][0-9]+[,]*[0-9]+[.,]*[0-9]+[,.)]*/[\"][,][\n] {
    strcpy(text,yytext);
    junk_catg=PRICE;
}

<TXT>[(]*[$][0-9]+[,]*[0-9]+[,.)]*/[\"][,][\n] {
    strcpy(text,yytext);
    junk_catg=PRICE;
}

<TXT>[(]*[$][0-9]+[.]*[0-9]+[,.)]*/[\"][,][\n] {
    strcpy(text,yytext);
    junk_catg=PRICE;
}

<TXT>[0-9]+["]/[\"][,][\n] {
    strcpy(text,yytext);
    junk_catg=INCH;
}

<TXT>[0-9]+[']/[\"][,][\n] {
    strcpy(text,yytext);
    junk_catg=FOOT;
}

<TXT>[0-9]+['][0-9]+["]/[\"][,][\n] {
    strcpy(text,yytext);
    junk_catg=FOOTINCH;
}

<TXT>[(]*[0-9]+[,][0-9][0-9][0-9][)]*/[\"][,][\n] {
    strcpy(text,yytext);
    junk_catg=SF;
}

<TXT>[(]*[0-9]+[,][0-9][0-9][0-9][)]*[\ ][Ss][Ff]/[\"][,][\n] {
    strcpy(text,yytext);
    junk_catg=SF;
}

<TXT>[0-9][0-9][0-9][0-9][0-9]/[\"][,][\n] {
    strcpy(text,yytext);
    junk_catg=ZIPCODE;
}

<TXT>[(]*[0-9]+[.][0-9][0-9]+[)]*/[\"][,][\n] {
    strcpy(text,yytext);
    junk_catg=DECNUM;
}

<TXT>[A-Za-z][-][0-9]+/[\"][,][\n] {
    strcpy(text,yytext);
    junk_catg=MODEL;
}

<TXT>[(]*[0-9]+(st|nd|rd|th)[)]*/[\"][,][\n] {
    strcpy(text,yytext);
    junk_catg=ORDINAL;
}

<TXT>[0-9]+[.]*[0-9]*[.]/[\"][,][\n] {
    strcpy(text,yytext);
    junk_catg=NUMERATION;
}

<TXT>[(][0-9a-zA-Z][)]/[\"][,][\n] {
    strcpy(text,yytext);
    junk_catg=NUMERATION;
}

<TXT>[0-9][0-9][0-9][0-9][.,]*/[\"][,][\n] {
    strcpy(text,yytext);
    if(atoi(text)>=1980 && atoi(text)<=2030)
        junk_catg=YEAR;
}

<TXT>[\"],[\n] {
    BEGIN GEO;
}

<CONF>[\ \t]*"\"Confidence\": " {
    ;
}

<CONF>[0-9/.]+ {
    strcpy(confidence,yytext);
}

<CONF>,[\n] {
    BEGIN WORD;
}

"\"BlockType\": \"PAGE\","[\n]  {
  page++;
}

<GEO>"\"Polygon\": ["[\n][\ \t]*[{][\n][\ \t]*"\"X\": " {
    BEGIN GEOX0;
}

<GEOX0>[0-9/.]+ {
    strcpy(x0,yytext);
}

<GEOX0>,[\n][\ \t]*"\"Y\": " {
    BEGIN GEOY0;
}

<GEOY0>[0-9/.]+ {
    strcpy(y0,yytext);
}

<GEOY0>[\n][\ \t]*[}],[\n] {
    BEGIN GEO;
}

<GEO>[}],[\n][\ \t]*[{][\n][\ \t]*"\"X\": " {
    BEGIN GEOX1;
}

<GEOX1>[0-9/.]+ {
    strcpy(x1,yytext);
}

<GEOX1>,[\n][\ \t]*"\"Y\": " {
    BEGIN GEOY1;
}

<GEOY1>[0-9/.]+ {
    strcpy(y1,yytext);
}

<GEOY1>[\n][\ \t]*[}], {
    normalize_text_and_insert_into_token_array(page,text,atof(x0),atof(x1),atof(y0),atof(y1),atoi(confidence),junk_catg);
    memset(text,0,strlen(text));
    junk_catg=NONE;
    BEGIN 0;
}

<GEO>.|\n ;
    
.|\n ;
%%

int yywrap() 
{ 
    return 1; 
}

//program input: -1 doc_id -2 program_path -3 media_path -4 db_ip -5 db_name -6 db_user -7 db_pwd
int main(int argc, char **argv){
    int opt;
    char *media_path;
    source_program= "AWS";

    while ((opt = getopt(argc, argv, "d:2:3:4:5:6:")) != -1) {
        switch (opt) {
        case 'd':
            doc_id= atoi(optarg);
            break;
        case '2':
            media_path= optarg;
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
            fprintf(stderr, "Usage: %s [-d doc_id] [-2 media_path]  [-3 db_ip] [-4 db_name] [-5 db_user_name] [-6 db_pwd]\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    /*
    if(argc<3){
        printf("Error: Program format is [prog_name] [doc_id] [media_path] [db_ip] [db_name] [db_user] [db_pwd]\n");
        exit(1);
    }
    int doc_id= atoi(argv[1]);
    char *media_path= argv[2];
    db_IP= argv[3];
    db_name= argv[4];
    db_user_name= argv[5];
    db_pwd= argv[6];
    */
    fprintf(stderr,"LLL0\n");
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn,db_IP,db_user_name,db_pwd,db_name,0,NULL,0)) {
      fprintf(stderr,"%s\n",mysql_error(conn));
      exit(1);
    } 
    fprintf(stderr,"LLL1\n");
    char* awsjson_path= get_path_to_source_file(conn,doc_id,media_path,"awsjsonfile");
    extern FILE *yyin;
    yyin= fopen(awsjson_path, "r");
    fprintf(stderr, "Opening awsjson file :%s:\n", awsjson_path);
    if(yyin==NULL){
        fprintf(stderr, "Error opening awsjson file %s: %s\n", awsjson_path,strerror(errno));
        exit(1);
    }
    printf("\t- succeeded opening awsjson file\n");
    delete_old_from_sql(conn,doc_id,"deals_alma_non_aligned_token","AWS");
    yylex();
    printf("\t- entering into alma_non_aligned_token table\n");
    enter_into_sql(doc_id,"deals_alma_non_aligned_token","AWS");
    printf("\t- entered into alma_non_aligned_token table\n");    
    mysql_close(conn);
    fclose(yyin);
    return 0;
}
