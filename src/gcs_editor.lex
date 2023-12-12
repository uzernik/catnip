%{
    #include<stdio.h>
    #include <string.h>
    #include <stdlib.h>
    #include <ctype.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
    #include <sys/wait.h>

    int num_of_indent=0;
%}
%x  WORDS COORDINATES TEXT CONF WORD_TEXT LAST_CHAR LAST_CHAR_TEXT
%%

<COORDINATES>[\{]    {
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"%s\t",yytext);
}

<COORDINATES>[\}][,][\ ]  {
    fprintf(yyout,"\t},\n");
}

\"pages\":[\ ][\[] {
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"%s\n",yytext);
    num_of_indent++;
}

\"width\":[\ ][0-9]+,[\ ]\"height\":[\ ][0-9]+  {
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"%s\n",yytext);
}


\"paragraphs\":[\ ][\[] {
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"%s\n",yytext);
    num_of_indent++;
}

\"words\":[\ ][\[]  {
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"%s\n",yytext);
    num_of_indent++;
    BEGIN WORDS;
}

<WORDS>\{\"normalizedVertices\":[\ ]\[\{    {
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"\{\t");
    num_of_indent++;
    fprintf(yyout,"\[\t");
    num_of_indent++;
    fprintf(yyout,"\{\t");
    BEGIN COORDINATES;
}

<COORDINATES>.  {
    fprintf(yyout,"%s",yytext);
}

<COORDINATES>\}\]\},    {
    fprintf(yyout,"\t}\n");
    num_of_indent--;
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"]\n");
    BEGIN WORDS;
}

<WORDS>\"symbols\":[\ ][\[] {
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"%s\n",yytext);
    num_of_indent++;
    BEGIN WORD_TEXT;
}

<WORD_TEXT>\"text\":[\ ]\"    {
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"%s",yytext);
    BEGIN TEXT;
}

<WORD_TEXT>\"detectedBreak\":  {
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"%s\n",yytext);
    BEGIN LAST_CHAR;
}

<LAST_CHAR>\"text\":[\ ]\"  {
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"%s",yytext);
    BEGIN LAST_CHAR_TEXT;
}

<TEXT>.  {
    fprintf(yyout,"%s",yytext);
}

<TEXT>\",   {
    fprintf(yyout,"%s\n",yytext);
    BEGIN WORD_TEXT;
}

<TEXT>\"/\},  {
    fprintf(yyout,"%s,\n",yytext);
    BEGIN WORD_TEXT;
}

<LAST_CHAR_TEXT>.  {
    fprintf(yyout,"%s",yytext);

}

<LAST_CHAR_TEXT>\",[\ ]\"confidence\":[\ ][0-9.]+ {
    fprintf(yyout,"\"\n");
    num_of_indent--;
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"]\n");
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    BEGIN CONF;
}

<LAST_CHAR_TEXT>\"\}\],[\ ] {
    fprintf(yyout,"\"\n");
    num_of_indent--;
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"]\n");
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    BEGIN CONF;
}

<CONF>\"confidence\":[\ \t]*[0-9.]+/"\},"   {
    fprintf(yyout,"%s\n",yytext);
    num_of_indent--;
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"}\n");
    BEGIN WORDS;
}

<CONF>\"confidence\":[\ \t]*[0-9.]+/"\}\]"   {
    fprintf(yyout,"%s\n",yytext);
    num_of_indent--;
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"}\n");
    num_of_indent--;
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"]\n");
    BEGIN 0;
}

\"blockType\":[\ ]\"TEXT\",[\ ]\"confidence\":[\ ][0-9.]+[\}],   {
    num_of_indent--;
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"]\n");
}

\"blockType\":[\ ]\"TEXT\",[\ ]\"confidence\":[\ ][0-9.]+[\}][\]]   {
    num_of_indent--;
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"]\n");
    num_of_indent--;
    for(int i=0;i<num_of_indent;i++){
        fprintf(yyout,"\t");
    }
    fprintf(yyout,"]\n");
    BEGIN 0;
}

\"pageNumber\":[\ ][0-9]+    {
    fprintf(yyout,"%s\n",yytext);
}

<WORDS>.|\n ;

<CONF>.|\n ;

<WORD_TEXT>.|\n ;

<LAST_CHAR>.|\n ;

.|\n ;

%%

int yywrap() 
{ 
    return 1; 
}

int main(int argc, char **argv){
    if(argc<2){
        printf("Error: Program format is [prog_name] [gcpjson_path]\n");
        exit(1);
    }
    char* gcpjson_path= argv[1];
    extern FILE *yyin, *yyout;
    yyin= fopen(gcpjson_path, "r");
    if(yyin==NULL){
        fprintf(stderr, "Error opening gcsjson file %s: %s\n", gcpjson_path, strerror(errno));
        exit(1);
    }
    printf("\t- succeeded opening gcsjson file\n");
    char output_file[strlen(gcpjson_path)+8];
    sprintf(output_file,"%s_edited",gcpjson_path);
    yyout= fopen(output_file,"w");
    if(yyout==NULL){
        fprintf(stderr, "Error opnning new edited gcsjson file %s: %s\n", output_file, strerror(errno));
        exit(1);
    }
    printf("\t- succeeded opening new edited gcsjson file\n");
    yylex();
    fclose(yyin);
    fclose(yyout);
    if(num_of_indent!=0){
        printf("Not a valid gcsjson format, exit program\n");
        return 1;
    }
    else
        return 0;
}    