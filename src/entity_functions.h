int delete_complex_field(MYSQL *conn, int doc_id, char *complex_name);
int insert_complex_field(MYSQL *conn, int doc_id, int deal_id, int org_id, int para_id, char *name, int complex_id_name[], int score, int order_no, char *hd, int show_sub_hd);
int get_did_oid(MYSQL *conn, int doc_id, int oid[]);
int insert_any_field(MYSQL *conn, char *table_type, int doc_id, int deal_id, int org_id, int para_id, int complex_id, int complex_name_id, char *name, char orig_text[], char val[], int order_no, int score, char *hd);
int delete_any_field(MYSQL *conn, char *table_type, int doc_id, char *field_name);
int delete_complex_field_by_id(MYSQL *conn, int doc_id, int field_id);
int delete_any_field_by_id(MYSQL *conn, char *table_type, int doc_id, int field_id);
int delete_any_field_by_complex(MYSQL *conn, char *field_type, int doc_id, int complex_id);
int chart_membership(MYSQL *conn, int doc_id, int chart_id, int entity_id, int row_id);
int delete_charts(MYSQL *conn, int doc_id);
int insert_chart_header(MYSQL *conn, int doc_id, int chart_id,int col_no,char *hdr);
int delete_existing_pid(MYSQL *conn, int doc_id);
int insert_recitals_feature(MYSQL *conn,char *recitals_feature, int fid, int doc_id, int deal_id, int org_id);
int delete_recitals_feature(MYSQL *conn, int doc_id);
int get_my_dataset_id(MYSQL *conn);
int delete_any_field_by_doc_id(MYSQL *conn, char *field_type, int doc_id);
int delete_complex_field_by_doc_id(MYSQL *conn, int doc_id);
int delete_all_complex_fields_by_doc_id(MYSQL *conn, int doc_id);

int total_query; // how many queries?
int debug_entity;
