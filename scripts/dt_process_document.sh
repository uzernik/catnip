#!/bin/bash
TEST=1
## CALLING: date; (bash ../scripts/dt_process_document.sh -Pmysql -Ndealthing -Uroot -Wimaof333 -D1 -M media -x ../../.. -O 'Comcast License Agreement' -c 'hsgatlin' -m 'comcast') >& ttt63; date

## doc_id is NOT an input;  it's calculated by set_doc_entry!!!
## Input: (c) Client (owner of contracts) (m) deal (group of contracts) (O) name of PDF in webapp/Dropbox folder
## Output: (1) in sql: deals_document entry; (2) in media folder: htmlfile, dsafile; (3) in sql: deals_toc_item, deals_token


usage() { echo "Usage $0: cd ~/dealthing/webapp ; (bash ../mytcstuff/scripts/dt_process_document.sh -d 12345 -Pudev1-from-prod-feb26.cu0erabygff3.us-west-2.rds.amazonaws.com  -Ndealthing -Uroot -Wimaof333 -D1      -M ~/efs/media -x ../../ ) >& ttt"; exit 1;}


while getopts "p:P:N:U:W:S:M:D:d:x:X:Y:9:L:O:F:c:m:" o; do
    case "${o}" in
        d)
	    doc_id=${OPTARG}
	    ;;

        P)
	    db_IP=${OPTARG}
	    ;;
        N)
	    db_Name=${OPTARG}
	    ;;

        U)
	    db_User=${OPTARG}
	    ;;

        W)
	    db_Pwd=${OPTARG}
	    ;;

        S)
	    do_sanitize=${OPTARG}
	    ;;

        M)
	    media=${OPTARG}
	    ;;
	

        D)
            debug=${OPTARG}
            ;;

	x)
	    DT_path=${OPTARG}
	    ;;

	B)  batch_id=${OPTARG}
	    ;;

	X)
	    X_back_to_aby=${OPTARG}
	    ;;

	Y)  Y_back_to_aby=${OPTARG}
	    ;;

	F)
	    path_in_media=${OPTARG} 
	    ;;
	
	O)
	    origPDF=${OPTARG} ## located in folder Dropbox
	    ;;
	c)
	    client=${OPTARG} 
	    ;;
	m)
	    deal=${OPTARG} 
	    ;;
        *)
            usage
            ;;
    esac
    ##echo "LLLL0 ${OPTARG} "    
done

shift $((OPTIND-1))

echo "pim=$path_in_media: opdf=$origPDF: med=$media: dbn=$db_Name: client=${client}: deal=${deal}: dtp={$DT_path}:"

##if  [ -z "${origPDF}" ] || [ -z "${path_in_media}" ] || [ -z "${media}" ] || [ -z "${DT_path}" ] || [ -z "${db_IP}" ] || [ -z "${db_Name}" ] || [ -z "${db_User}" ] || [ -z "${db_Pwd}" ]  ; then
if  [ -z "${origPDF}" ] || [ -z "${client}" ] || [ -z "${deal}" ] || [ -z "${media}" ]  ; then    
    usage
fi

timestamp() {
  date +"%T.%2N"
}



## not working
  ##DT_path="~"

  Lexicons=$DT_path/dealthing/lexicons
  DT_tmp=$DT_path/dealthing/mytcstuff/tmp
  if [ TEST = 1 ]; then
      DT_bin=../bin
  else
      DT_bin=$DT_path/dealthing/mytcstuff/bin
  fi
  DT_bin=../bin  
  echo "TEST=$TEST: DT_BIN=$DT_bin"
  
  DT_python=$DT_path/dealthing/mytcstuff/python
  DT_python=../python
  echo "TEST=$TEST: DT_PYTHON=$DT_python"
  
  ##DT_voting=../votingstuff  ##UZ
  DT_src=$DT_path/dealthing/mytcstuff/src
  DT_toc=$DT_path/dealthing/mytcstuff/tmp/Toc
  DT_index=$DT_path/dealthing/mytcstuff/tmp/Index
  DT_logs=$DT_path/dealthing/mytcstuff/logs
  dict_path=$DT_path/dealthing/mytcstuff/data/WordCountDir/words_file_for_bigrams
  ##tmp_dsafile=$DT_path/dealthing/webapp/tmp/dsatmp_$doc_id.html
  my_toc_file=$DT_toc/toc.html

  mkdir -p $DT_tmp
  mkdir -p $DT_tmp/Xxx
  mkdir -p $DT_tmp/Index  
  mkdir -p $DT_toc
  mkdir -p $DT_index
  mkdir -p $DT_logs

  printf "\n\n0.0  CALLING: ($DT_bin/clean_text $client)\n"
  clean_client=`$DT_bin/clean_text "$client"`
  echo "CLEAN=$clean_client CLIENT=$client"

  printf "\n\n0.1  CALLING: ($DT_bin/clean_text $deal)\n"
  clean_deal=`$DT_bin/clean_text "$deal"`
  echo "CLEAN=$clean_deal CLIENT=$deal"

  printf "\n\n0.2  CALLING: ($DT_bin/clean_text $origPDF)\n"
  clean_pdf=`$DT_bin/clean_text "$origPDF"`
  echo "CLEAN=$clean_pdf PDF=$origPDF"

  path_in_media="$clean_client"/"$clean_deal"
  echo "PATH_IN_MEDIA=$path_in_media"
  mkdir -p $media
  mkdir -p $media/$clean_client
  mkdir -p $media/$clean_client/$clean_deal #$path_in_media

  ########### OCR ###################
  textract_output_json=media/$path_in_media/${clean_pdf}_AWS.json

  Dropbox='.'
  if [ -f "$Dropbox/$origPDF" ]; then

      # Check if the file exists and has more than 100 bytes
      if [ -f "$textract_output_json" ] && [ $(stat -c%s "$textract_output_json") -gt 100 ]; then
	  no_of_pages=`$DT_bin/get_no_of_pages_from_json < $textract_output_json`      
	  printf "\n\nOUTPUT file $textract_output_json ALREADY exists and has $no_of_pages pages BYPASSING OCR\n"
	  printf "\n\n1.  NOT CALLING: (python $DT_python/textract/textract_test.py $Dropbox/$origPDF $path_in_media/$clean_pdf)\n"      
      else
	  printf "\n\nFile $textract_output_json does not exist or is less than 100 bytes NOW DOING OCR\n"
	  printf "\n\n1.  CALLING: (python $DT_python/textract_test.py /Dropbox/$origPDF $path_in_media/$clean_pdf)\n"
	  python $DT_python/textract_test.py   "/Dropbox/$origPDF" "media/$path_in_media/$clean_pdf"
	  date
      fi
  else
      printf "\n\nINPUT file:$Dropbox/$origPDF: does not exist  ABORTING!!\n"
      exit
  fi
  
  ## get no_pf_pages from JSON above
  printf "\n\n2.  CALLING: (no_of_pages=$DT_bin/get_no_of_pages_from_json < $textract_output_json)\n"
  no_of_pages=`$DT_bin/get_no_of_pages_from_json < $textract_output_json`
  date
  echo "NO_OF_PAGES=$no_of_pages"
  
  org_id=1 ## fictitious for now
  folder_id=8 ## fictitious for now
  ## inserting an entry in SQL deals_document, and getting doc_id
  printf "\n\n3.  CALLING: ($DT_bin/set_doc_entry -P $db_IP -N $db_Name -U $db_User -W $db_Pwd -D0 -O $origPDF -F $path_in_media -n $no_of_pages -o $org_id -f $folder_id)\n"
  doc_id=`$DT_bin/set_doc_entry -P $db_IP -N $db_Name -U $db_User -W $db_Pwd -D0 -O $origPDF -F $path_in_media -n $no_of_pages -o $org_id -f $folder_id`
  padded_doc_id=`$DT_bin/zero_padding_to_8 $doc_id`
  echo "DOC ID=$doc_id PADDED_DOC ID=$padded_doc_id"
  ## we now have did
  date


  #adding the column tenant_path_in_media
  printf "\n\n4.  CALLING: ($DT_bin/does_field_exist  -D 1 -P mysql -N dealthing -U root -W imaof333 -F tenant_path_in_media)\n"
  $DT_bin/does_field_exist  -D 1 -P mysql -N dealthing -U root -W imaof333 -F tenant_path_in_media
  
  date
  printf "\n\n5.  CALLING: ($DT_bin/json_convert < $textract_output_json > media/$path_in_media/jsonfile_${padded_doc_id})\n"
  $DT_bin/json_convert < $textract_output_json > media/$path_in_media/jsonfile_${padded_doc_id}



  date
  printf "\n\n6. CALLING: $DT_bin/get_folder_path $doc_id $db_IP $db_Name $db_User $db_Pwd $media tenant_path_in_media 1 \n "
  tenant_dir=`$DT_bin/get_folder_path $doc_id $db_IP $db_Name $db_User $db_Pwd $media tenant_path_in_media 1`

  echo "TENANT_DIR=$tenant_dir"
  date

  dsafile=$tenant_dir/dsafile_${padded_doc_id}
  displayfile=$tenant_dir/displayfile_${padded_doc_id}
  echo "CALLING: ls -t $tenant_dir/jsonfile_${doc_id}"
  json_file=$tenant_dir/jsonfile_${padded_doc_id}
  spdf_file=$tenant_dir/spdffile_${padded_doc_id}
  textfile=$tenant_dir/textfile_${padded_doc_id}
  index_file=$DT_index/index_file_${padded_doc_id}

  echo "JSON_FILE=$json_file DSAFILE=$dsafile DISPLAYFILE=$displayfile SPDFFILE=$spdf_file"

  doc_country=`$DT_bin/get_country -d $doc_id -P $db_IP -N $db_Name -U $db_User -W $db_Pwd -D0 `   ## get GBR / USA / AUS, etc
  printf "\nCALLING:   $DT_bin/get_country -d $doc_id -P $db_IP -N $db_Name -U $db_User -W $db_Pwd -D0 \n\n"

  doc_language=`$DT_bin/get_language -d $doc_id -P $db_IP -N $db_Name -U $db_User -W $db_Pwd -D0 `   ## get ENG / FRE / GER, etc
  printf "\nCALLING:   $DT_bin/get_language -d $doc_id -P $db_IP -N $db_Name -U $db_User -W $db_Pwd -D0 \n\n" 
  echo "DOC_COUNTRY=$doc_country"
  echo "DOC_LANGUAGE=$doc_language"

####################################  ORDERING and VOTING ####################################################
  date
  ### takes AWS JSON  writes DEALS_OCRBLOCK and DEALS_ORIG_OCRTOKEN
  printf "\n\n7.  CALLING: ($DT_bin/parse_aws_json -d $doc_id -P$db_IP -N $db_Name -U $db_User -W $db_Pwd -D 1 -sAWS -n1  -F1  -L $doc_language  < $json_file > $DT_src/bbb111) >& $DT_logs/www200 \n"
  ($DT_bin/parse_aws_json -d $doc_id -P$db_IP -N $db_Name -U $db_User -W $db_Pwd -D 1 -sAWS -n1 -F1 -L $doc_language < $json_file > $DT_src/bbb111) >& $DT_logs/www200 

  printf "\n\n8.  CALLING:     date; (python $DT_python/run2.py -d $doc_id -2$DT_path -3$dict_path -4 $media -P$db_IP -N $db_Name -U $db_User -W $db_Pwd) >& $DT_logs/www01; date \n"
  date; python $DT_python/run2.py -d $doc_id -2$DT_path -3$dict_path -4 $media -P$db_IP -N $db_Name -U $db_User -W $db_Pwd >& $DT_logs/www01; date 


  ### takes DEALS_OCRBLOCK and DEALS_ORIG_OCRTOKEN and ALMA_VOTED; writes DEALS_OCRTOKEN with real tokens and reorder
  printf "\n\n9.  CALLING: ($DT_bin/parse_aws_json -d $doc_id -P$db_IP -N $db_Name -U $db_User -W $db_Pwd -D 1 -sAWS -n1 -F0 -S$DT_path  -L $doc_language ) >& $DT_logs/www20 \n"
  ($DT_bin/parse_aws_json -d $doc_id -P$db_IP -N $db_Name -U $db_User -W $db_Pwd -D 1 -sAWS -n1 -F0 -S$DT_path  -L $doc_language ) >& $DT_logs/www20
  date



    VERT_THRESH=120 ## above this gap we call it a new para
    ## doc_country -- we don't do enumerated lines in GBR, it catches TOC items w/o periods
    printf "\n\n10.  CALLING: ($DT_bin/calc_page_params -d $doc_id -P$db_IP -N $db_Name -U $db_User -W $db_Pwd -D1 -scalc_page_params -2$displayfile -M$media -v$VERT_THRESH -T$dict_path -A$dt_path'/..' -c$doc_country > $DT_tmp/aaa61.$doc_id) >& $DT_logs/www21 \n"
    ($DT_bin/calc_page_params -d $doc_id -P$db_IP -N $db_Name -U $db_User -W $db_Pwd -D 1 -scalc_page_params -2$displayfile -M$media -v$VERT_THRESH -T$dict_path  -c$doc_country > $DT_tmp/aaa61.$doc_id) >& $DT_logs/www21
    date

    ### for transfer
    printf "\n\n11.  CALLING: cp -v $DT_tmp/aaa61.$doc_id $tenant_dir/aaa5.$doc_id "
    cp -v $DT_tmp/aaa61.$doc_id $tenant_dir/aaa5.$doc_id


####################################  DSA ####################################################

## creates SQL DEALS_SUMMARYPOINTS
printf "\n\n6.  CALLING: ($DT_bin/detect_rental_form_ocr -d $doc_id  -P$db_IP -N $db_Name -U $db_User -W $db_Pwd -D1 -Oaws < $displayfile > /dev/null) >& $DT_logs/www39 \n"
date
($DT_bin/detect_rental_form_ocr -d $doc_id  -P$db_IP -N $db_Name -U $db_User -W $db_Pwd -D1 -Oaws < $displayfile > /dev/null)  >& $DT_logs/www39

## creates SQL DEALS_PAGE2SUMMARY_POINTS, DEALS_PAGE_LINE_PROPERTIES

printf "\n\n7.  CALLING: ( $DT_bin/new_table_of_contents_ocr -d $doc_id  -P$db_IP -N $db_Name -U $db_User -W $db_Pwd -D1 -I$index_file -X $Lexicons/Word_Counts/word_total_count -Oaws < $DT_tmp/aaa61.$doc_id > /dev/null ) >& $DT_logs/www24 \n"
date
( $DT_bin/new_table_of_contents_ocr -d $doc_id  -P$db_IP -N $db_Name -U $db_User -W $db_Pwd -D1 -I$index_file -X $Lexicons/Word_Counts/word_total_count -Oaws < $DT_tmp/aaa61.$doc_id > /dev/null ) >& $DT_logs/www24

## creates iii and aaa62
printf "\n\n8. CALLING: ($DT_bin/create_toc_ocr -D1 -d $doc_id -f Document -t $DT_toc/toc.html -u URL -i $index_file -n DEAL -r 1 -P$db_IP -N $db_Name -U $db_User -W $db_Pwd -C BLA -A ~/dealthing/webapp -X1 -Y0 -F1 -m1 -o aaa62 -G$doc_country < $DT_tmp/aaa61.$doc_id) >& $DT_logs/www25 \n"
date
($DT_bin/create_toc_ocr -D1 -d $doc_id -f Document -t $DT_toc/toc.html -u URL -i $index_file -n DEAL -r 1 -P$db_IP -N $db_Name -U $db_User -W $db_Pwd -C $DT_tmp/XXX_CCC_$doc_id -A ~/dealthing/webapp -X1 -Y0 -F1 -m1 -o aaa62 -G$doc_country < $DT_tmp/aaa61.$doc_id) >& $DT_logs/www25


##  NOT USING AAA62!!!!
# reads aaa62 and iii, creates iii.out  ## output is $index_file.out
printf "\n\n9.  CALLING: ($DT_bin/get_preamble_ocr 0 $doc_id $index_file $db_IP $db_Name $db_User $db_Pwd "0" < $DT_tmp/aaa61.$doc_id > /dev/null) >& $DT_logs/www26 \n"
date
($DT_bin/get_preamble_ocr 0 $doc_id $index_file $db_IP $db_Name $db_User $db_Pwd "0" < $DT_tmp/aaa61.$doc_id > /dev/null) >& $DT_logs/www26


printf "\n\n10.  CALLING:  ($DT_bin/new_index_ocr $doc_id $index_file.out "0" "0" $db_IP $db_Name $db_User $db_Pwd  < $DT_tmp/aaa61.$doc_id > $DT_tmp/aaa9.$doc_id ) >& $DT_logs/www27 \n"
($DT_bin/new_index_ocr $doc_id $index_file.out "0" "0" $db_IP $db_Name $db_User $db_Pwd  < $DT_tmp/aaa61.$doc_id >  $DT_tmp/aaa9.$doc_id) >& $DT_logs/www27 
date


if [ 0 = 0 ]; then
    printf "\n\n14. CALLING: REMOVE TEMP FILES: $DT_tmp/aaa*.$doc_id \n"
    ls -lt $DT_tmp/aaa*.$doc_id  ### show the files before remove
    for pathname in $DT_tmp/aaa*.$doc_id; do  ### copy them into aaa*
	filename=$(basename "$pathname")  ## lose the path
	root="${filename%.*}"  ## lose the extension
	echo "ROOT=$root"
	cp ${pathname} ${DT_tmp}/${root}
    done

    rm $DT_tmp/aaa*.$doc_id  ### remove the aaaN.$doc_id files
    ls -lt $DT_tmp/aaa* ### show the aaaN files
fi


printf "\n\n16.  DONE\n"

exit
EXIT!!!!!
#################################################################### EPILOG #######################################################################
## For Transfer
##    get prior docs from DEALS_FOLDER_LOCK_LIST
##    insert one new doc into DEALS_FOLDER_LOCK_LIST
##    insert similarity with all the prior into DEALS_PROPERTY_SIMILARITY_MATRIX 
printf "\n\n11.  CALLING:  ($DT_bin/incremental_match_doc_pair -D1 -d $doc_id -P$db_IP  -N$db_Name  -U$db_User -W$db_Pwd  -T ttt -R rrr -A . -b $DT_tmp/Sim -I $media)  >& $DT_logs/www28 \n"
$DT_bin/incremental_match_doc_pair -D1 -d $doc_id     -P$db_IP  -N$db_Name  -U$db_User -W$db_Pwd   -T ttt -R rrr -A . -b ~/dealthing/mytcstuff/tmp/Sim  -I $media >& $DT_logs/www28
date


##printf "\n\n12.  CALLING: mv $coords_file $DT_tmp/XXX_CCC \n" ## for reference only
##mv -vf $coords_file $DT_tmp/XXX_CCC_$doc_id #for reference only


printf "\n\n13. CALLING: cp -f $DT_tmp/aaa9.$doc_id $dsafile \n"
cp -f $DT_tmp/aaa9.$doc_id $dsafile
printf "\n\n13.0 CALLING: cp -f $dsafile $tmp_dsafile \n"
cp -f $dsafile $tmp_dsafile
printf "\n\n13.1. CALLING: cp -f $index_file.out $tenant_dir/index_file_$doc_id \n"
cp -f "$index_file.out" $tenant_dir/index_file_$doc_id
ls -lt $DT_tmp/aaa9.$doc_id $dsafile

printf "\n\n13.2. CALLING: $DT_bin/rem_bra < $dsafile > $textfile \n"
$DT_bin/rem_bra < $dsafile > $textfile

date
##fi
##fi


printf "\n\n15.  CALLING:  $DT_bin/insert_preamble_into_TOC_ocr -d $doc_id -P $db_IP -N '$db_Name' -U '$db_User' -W '$db_Pwd' -D1 -F1 -G$doc_country >& $DT_logs/www29\n"
$DT_bin/insert_preamble_into_TOC_ocr -d $doc_id -P $db_IP -N "$db_Name" -U "$db_User" -W "$db_Pwd" -D1 -F1 -G$doc_country >& $DT_logs/www29
date


is_form=` $DT_bin/is_form_org -D1 -d $doc_id -P$db_IP  -N$db_Name  -U$db_User -W$db_Pwd ` ## for now is_form is set to 1 for Alma forms
if [ $is_form = 1 ]; then
    printf "\n\n2.  CALLING: date; python $DT_path/dealthing/webapp/deals/formdetect/form_detect.py -d $doc_id  -P$db_IP -N $db_Name -U $db_User -W $db_Pwd -F $json_file >& $DT_logs/www30;  date; \n"
    date; python $DT_path/dealthing/webapp/deals/formdetect/form_detect.py -d $doc_id  -P$db_IP -N $db_Name -U $db_User -W $db_Pwd -F $json_file >& $DT_logs/www30; date
fi

