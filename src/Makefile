
##
##
## UZ: 5/5/15: now created ../inter file to collect intermediate files (usually .c files)
## ran the program fix_makefile to convert from Makefile_old to Makefile
## !!! .c files that are source stay in ./src
## !!! added else to ifeq clauses to cp .c to ./inter
##


TOP=  ../..

## include $(TOP)/SysDefs.mk

CXX = g++ -std=c++11 -g -Wall
CC = gcc -g -Wall
CFLAGS =
LDFLAGS = -lmysqlclient
PY_INCLUDE = /usr/include/python3.4

## This flex can handle more than fricken 32000 states
LEX = ~/flex/flex-2.5.37/flex
LEX = /etc/flex
LEX = ../../lib/flex ## UZ 05/07/15: we now pass flex binary via git

FLEX = ~/Downloads/flex-2.5.39/flex
LIBRARIES = -lm

BIN     = ../bin
BUILD	= ../build
INTER	= ../my_inter
DJANGO_DLLS = ../../webapp/dlls 

MYSQL_LIBS = -lmysqlclient

INSTALL = cp -auv

DSINCDIR = .
DSINTER = ../src
INCLUDE_FLAGS = -I${MYSQL} -I${DSINCDIR} -I${DSINTER}

SRC_PATTERN = ^*.o

## check the architecture: 64 or 32 bit. 
LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
## FOR 64 BIT (Shahar)
	SQL_flags = -I. -I/usr/include/mysql  -g -pipe -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -m64 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -fno-strict-aliasing -fwrapv
	SQL_libs = -rdynamic -L/usr/lib64/mysql -lmysqlclient -lz -lcrypt -lnsl -lm -L/usr/lib64 -lssl -lcrypto -ljansson 
else
## FOR 32 BIT (Uri, Amazon)
	SQL_flags = -I.  -I/usr/include/mysql  -g -pipe -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -m32 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=32 -D_LARGEFILE_SOURCE -fno-strict-aliasing -fwrapv
	SQL_libs = -rdynamic -L/usr/lib32/mysql -lmysqlclient -lz -lcrypt -lnsl -lm -L/usr/lib32 -lssl -lcrypto -ljson 
endif

PRE = \
${BUILD}/flex_include \
${BUILD}/flex_included

#################  PROGRAM LIST ###############################
ALL = \
${BUILD}/clean_text \
${BUILD}/rm_bra \
${BUILD}/get_no_of_pages_from_json \
${BUILD}/does_field_exist \
${BUILD}/set_doc_entry \
${BUILD}/get_folder_path \
${BUILD}/get_country \
${BUILD}/get_language \
${BUILD}/parse_aws_json \
${BUILD}/parse_aws_jsond \
${BUILD}/json_convert \
${BUILD}/calc_page_params \
${BUILD}/new_index_ocrd \
${BUILD}/new_index_ocr \
${BUILD}/get_preamble_ocr \
${BUILD}/get_preamble_ocrd \
${BUILD}/create_toc_ocr \
${BUILD}/create_toc_ocrd \
${BUILD}/zero_padding_to_8  \
${BUILD}/new_table_of_contents_ocr \
${BUILD}/new_table_of_contents_ocrd  \
\
${BUILD}/gcs_editor \
${BUILD}/tokens_GCS4 \
${BUILD}/tokens_AWS2 \
${BUILD}/tokens_AWS2d \
${BUILD}/alignment3 \

###############################################################

all: build_pre install_pre build install

build_pre: ${BUILD} ${BIN} ${INTER} ${DJANGO_DLLS} $(PRE)

build: ${BUILD} ${BIN} ${INTER} ${DJANGO_DLLS} $(ALL)

install: build
	$(INSTALL) $(ALL) $(BIN)

install_pre: build_pre
	$(INSTALL) $(PRE) $(BIN)

.c.o:
	${CC} ${INCLUDE_FLAGS} -c -g -Wall $<

${BUILD}:
	mkdir -p ${BUILD}

${BIN}:
	mkdir -p ${BIN}

${INTER}:
	mkdir -p ${INTER}

${DJANGO_DLLS}:
	mkdir -p ${DJANGO_DLLS}

############ START RULES ##############

${BUILD}/clean_text: clean_text.c 
	gcc -o ${BUILD}/clean_text clean_text.c 

${BUILD}/rm_bra:	rm_bra.lex
	${LEX} -t -i rm_bra.lex > ${INTER}/rm_bra.c
	cc -o ${BUILD}/rm_bra ${INTER}/rm_bra.c -lfl

${BUILD}/rm_brad:	rm_bra.lex
	${LEX} -t -i -d rm_bra.lex > ${INTER}/rm_brad.c
	cc -o ${BUILD}/rm_brad ${INTER}/rm_brad.c -lfl

${BUILD}/flex_include:	flex_include.lex
	${LEX} -t -i flex_include.lex > ${INTER}/flex_include.c
	cc -o ${BUILD}/flex_include ${INTER}/flex_include.c -lfl

${BUILD}/flex_included:	flex_include.lex
	${LEX} -t -d -i flex_include.lex > ${INTER}/flex_included.c
	cc -o ${BUILD}/flex_included ${INTER}/flex_included.c -lfl

${BUILD}/set_doc_entry: set_doc_entry.c entity_functions.c
	gcc -o ${BUILD}/set_doc_entry set_doc_entry.c $(SQL_flags) $(SQL_libs) 

${BUILD}/does_field_exist: does_field_exist.c entity_functions.c
	gcc -o ${BUILD}/does_field_exist does_field_exist.c $(SQL_flags) $(SQL_libs) 

${BUILD}/get_folder_path:	${INTER}/get_folder_path.c
	${CC} -o $@  $(SQL_flags) $+ $(SQL_libs) -lfl
${INTER}/get_folder_path.c: get_folder_path.lex
	${LEX} -t get_folder_path.lex > ${INTER}/get_folder_path.c



${BUILD}/get_country: get_country.c entity_functions.c
	gcc -o ${BUILD}/get_country get_country.c $(SQL_flags) $(SQL_libs) 

${BUILD}/get_language: get_language.c entity_functions.c
	gcc -o ${BUILD}/get_language get_language.c $(SQL_flags) $(SQL_libs) 

${BUILD}/json_convert:	json_convert.lex
	${LEX} -t -i json_convert.lex > ${INTER}/json_convert.c
	cc -o ${BUILD}/json_convert ${INTER}/json_convert.c -lfl

${BUILD}/parse_aws_json:	parse_aws_json.lex str_replace_code.c
	${LEX} -t parse_aws_json.lex > ${INTER}/parse_aws_json.c
	cc -o ${BUILD}/parse_aws_json ${INTER}/parse_aws_json.c str_replace_code.c $(SQL_flags) $(SQL_libs) -lfl

${BUILD}/parse_aws_jsond:	parse_aws_json.lex str_replace_code.c
	${LEX} -t -d parse_aws_json.lex > ${INTER}/parse_aws_jsond.c
	cc -o ${BUILD}/parse_aws_jsond ${INTER}/parse_aws_jsond.c str_replace_code.c $(SQL_flags) $(SQL_libs) -lfl

${BUILD}/calc_page_params:	calc_page_params.c ${INTER}/calc_get_summary_page.c calc_rental_form.c calc_enumerated.c calc_footers.c calc_double_column.c calc_toc_block_clusters.c calc_new_para.c levenshtein_functions.c calc_page_params.h
	cc -o ${BUILD}/calc_page_params ${INTER}/calc_get_summary_page.c calc_page_params.c calc_rental_form.c calc_enumerated.c calc_footers.c calc_double_column.c calc_toc_block_clusters.c calc_new_para.c levenshtein_functions.c $(SQL_flags)  $(SQL_libs) -lfl

${BUILD}/get_no_of_pages_from_json:	get_no_of_pages_from_json.lex
	${LEX} -t -i get_no_of_pages_from_json.lex > ${INTER}/get_no_of_pages_from_json.c
	cc -o ${BUILD}/get_no_of_pages_from_json ${INTER}/get_no_of_pages_from_json.c -lfl

${INTER}/calc_get_summary_page.c:	calc_get_summary_page.lex
	${LEX} -t -i calc_get_summary_page.lex > ${INTER}/calc_get_summary_page.c


${BUILD}/new_index_ocr:	${INTER}/new_index_ocr.c entity_functions.c
	${CC} -o $@  $(SQL_flags) $+ $(SQL_libs) -lfl
${INTER}/new_index_ocr.c: new_index_ocr.lex
	${LEX} -t new_index_ocr.lex > ${INTER}/new_index_ocr.c

${BUILD}/new_index_ocrd:	${INTER}/new_index_ocrd.c  entity_functions.c
	${CC} -o $@  $(SQL_flags) $+ $(SQL_libs) -lfl
${INTER}/new_index_ocrd.c: new_index_ocr.lex
	${LEX} -t -d new_index_ocr.lex > ${INTER}/new_index_ocrd.c


${BUILD}/get_preamble_ocrd:	${INTER}/get_preamble_ocrd.c entity_functions.c  recitals_functions.c   recitals_functions.h
	${CC} -o $@  $(SQL_flags) $+ $(SQL_libs) -lfl
${INTER}/get_preamble_ocrd.c: get_preamble_ocr.lex
	${LEX} -t -d get_preamble_ocr.lex > ${INTER}/get_preamble_ocrd.c


${BUILD}/get_preamble_ocr:	${INTER}/get_preamble_ocr.c entity_functions.c  recitals_functions.c   recitals_functions.h
	${CC} -o $@  $(SQL_flags) $+ $(SQL_libs) -lfl
${INTER}/get_preamble_ocr.c: get_preamble_ocr.lex
	${LEX} -t get_preamble_ocr.lex > ${INTER}/get_preamble_ocr.c

${BUILD}/create_toc_ocr:	${INTER}/create_toc_ocr.c create_toc_ocr.h seq_create_toc_ocr.c lev_create_toc_ocr.c entity_functions.c create_toc_ocr.h align_toc_functions.c align_toc_functions.h align_functions.c levenshtein_functions.c
	${CC} -o $@ $(SQL_flags) $+ $(SQL_libs) -lfl
${INTER}/create_toc_ocr.c: create_toc_ocr.lex create_toc_ocr.h
	${LEX} -t create_toc_ocr.lex > ${INTER}/create_toc_ocr.c


${BUILD}/create_toc_ocrd:	${INTER}/create_toc_ocrd.c create_toc_ocr.h seq_create_toc_ocr.c lev_create_toc_ocr.c entity_functions.c create_toc_ocr.h align_toc_functions.c align_toc_functions.h align_functions.c levenshtein_functions.c
	${CC} -o $@ $(SQL_flags) $+ $(SQL_libs) -lfl
${INTER}/create_toc_ocrd.c: create_toc_ocr.lex create_toc_ocr.h
	${LEX} -t -d create_toc_ocr.lex > ${INTER}/create_toc_ocrd.c

## given a doc_id, produce the padding for the media eight digit number (dt_new_ocr.sh)
${BUILD}/zero_padding_to_8:	zero_padding_to_8.c
	cc -o ${BUILD}/zero_padding_to_8 zero_padding_to_8.c -lfl

${BUILD}/new_table_of_contents_ocr:	new_table_of_contents_ocr.lex  line_token_ocr.h line_token_ocr.c
	${LEX} -t new_table_of_contents_ocr.lex > ${INTER}/new_table_of_contents_ocr.c
	cc -o ${BUILD}/new_table_of_contents_ocr ${INTER}/new_table_of_contents_ocr.c  line_token_ocr.c -lfl $(SQL_flags) $(SQL_libs)

${BUILD}/new_table_of_contents_ocrd:	new_table_of_contents_ocr.lex line_token_ocr.h line_token_ocr.c
	${LEX} -t -d new_table_of_contents_ocr.lex > ${INTER}/new_table_of_contents_ocrd.c
	cc -o ${BUILD}/new_table_of_contents_ocrd ${INTER}/new_table_of_contents_ocrd.c line_token_ocr.c -lfl $(SQL_flags) $(SQL_libs)



################## VOTING #######################

${BUILD}/gcs_editor:   gcs_editor.lex
	${LEX} -t gcs_editor.lex > gcs_editor.c
	cc -o ${BUILD}/gcs_editor gcs_editor.c
${BUILD}/tokens_GCS4:	tokens_GCS4.lex import_functions.c import_functions.h
	${LEX} -t tokens_GCS4.lex > ${INTER}/tokens_GCS4.c
	cc -o ${BUILD}/tokens_GCS4 ${INTER}/tokens_GCS4.c import_functions.c  $(SQL_flags) -lfl  $(SQL_libs)  
${BUILD}/tokens_GCS4d:	tokens_GCS4.lex import_functions.c
	${LEX} -t -d tokens_GCS4.lex > ${INTER}/tokens_GCS4d.c import_functions.h
	cc -o ${BUILD}/tokens_GCS4d ${INTER}/tokens_GCS4d.c import_functions.c  $(SQL_flags) -lfl  $(SQL_libs)
${BUILD}/tokens_AWS2:   tokens_AWS2.lex import_functions.c import_functions.h
	${LEX} -t tokens_AWS2.lex > ${INTER}/tokens_AWS2.c
	cc -o ${BUILD}/tokens_AWS2 ${INTER}/tokens_AWS2.c import_functions.c  $(SQL_flags) -lfl  $(SQL_libs) 
${BUILD}/tokens_AWS2d:  tokens_AWS2.lex import_functions.c import_functions.h
	${LEX} -t -d tokens_AWS2.lex > ${INTER}/tokens_AWS2d.c 
	cc -o ${BUILD}/tokens_AWS2d ${INTER}/tokens_AWS2d.c import_functions.c  $(SQL_flags) -lfl  $(SQL_libs)

${BUILD}/alignment3:   alignment3.c import_functions.c import_functions.h
	cc -o ${BUILD}/alignment3 alignment3.c import_functions.c  $(SQL_flags) -lfl  $(SQL_libs)

#######################################
