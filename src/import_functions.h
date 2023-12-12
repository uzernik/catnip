// for DB config
static char *db_IP;
static char *db_name;
static char *db_pwd;
static char *db_user_name;
MYSQL *conn;

int doc_id;
char* source_program;
/*tokens processing related stuff*/
enum category {NONE,DATE,YEAR,HOUR,INCH,FOOT,FOOTINCH,SF,DECNUM,MODEL,PRICE,ZIPCODE,ORDINAL,NUMERATION};

struct Instance{
    int id, page, sn_in_doc;
    char* source;
    char* text;
    char* normalized_text;
    double x1;
    double x2;
    double y1;
    double y2;
    int my_x1;
    int my_x2;
    int my_y1;
    int my_y2;
    int confidence;
    enum category junk_category;
};

#define MAX_ARRAY 100000
struct Instance instances[MAX_ARRAY];
static int curr_ind=0;
static int sn=0;

char *edit_special_chars(char *text);
int normalize_text_and_insert_into_token_array(int page,char *text, double x1,double x2,double y1,double y2,int confidence,enum category junk_catg);
int insert_into_token_array(int page,char *text, char *normalized_text, double x1,double x2,double y1,double y2,int confidence,enum category junk_catg,int sn_in_doc);
int print_array();
int enter_into_sql(int doc_id,char* table_name,char* source);
int delete_old_from_sql(MYSQL* conn,int doc_id, char* table_name,char* source);
int multiple_factor(double i);
char* get_path_to_source_file(MYSQL* conn, int doc_id, char* media_path, char* source_file);
int enter_into_OcrPageSize(MYSQL* conn, int doc_id,int size);
int print_pageInfo_array(int size);

struct PageSize{
    int page;
    int xx;
    int yy;
};

struct PageSize pageInfo[1000];
static int page_ind=0;


/*alignment related stuff*/
FILE *dictionary;

// ***hash table implement for dictionary***
struct nlist { /* table entry: */
    struct nlist *next; /* next entry in chain */
    char *name; /* defined name */
    char *defn; /* replacement text */
};

#define HASHSIZE 94660
static struct nlist *hashtab_dict[HASHSIZE]; /* pointer table */
unsigned hash_dict(char *s);
struct nlist *lookup_in_dictionary(char *s);
int ignore_word_for_dictionary(char* s);
struct nlist *install_in_dictionary(char *name, char *defn);

struct Source_data{
    int page;
    int num_of_instances;
    struct Instance *instances;
};

struct Source_data* aws_data;
struct Source_data* gcs_data;

int size_of_source1, size_of_source2;

enum discrepancy {SAME,SIMILAR,NONALNUM,CAPITAL,DIFFERENT,AWSMISS,GCSMISS, JUXTA};
enum decision {AWS,GCS,NEITHER};

struct Couple{
    struct Instance instance1;
    struct Instance instance2;
    int overlap;
    enum discrepancy disc;
    enum decision desc;
    int split;
    int juxta;
};

struct Couples_data{
    int page;
    int num_of_couples;
    struct Couple *couples;
};

struct Couple *non_aligned_array;
struct Couples_data *aligned_array;
int non_aligned_ind, aligned_ind;


struct edit_distance{
    int alnum;
    int non_alnum;
    int capital;
};
int isAlnum(char c);
struct edit_distance levenshtein_edit_distance (const char * word1, const char * word2);
int delete_old_from_aligned_token_tables(MYSQL* conn, int doc_id, char* table_name);
int parse_dictionary(char* dict_path);
