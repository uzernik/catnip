%{
  // this program runs before the various extract programs, cleaning up tags
  // <X[0-9]+ replaced by <MARK
#include <ctype.h>
%}
hh  \<(\/)?[Hh][0-9][^\>]*\>
xx  \<(\/)?(([Xx][0-9])|(MARK))[^\>]*\>
epp \<\/[Pp][^\>]*\>
pp \<[Pp][^\>]*\>
utable \<[\/]?utable\>
tag \<[^\>]*\>
ttag (\<(\/)?table[^\>]*\>)|(\<(\/)?td[^\>]*\>)|(\<(\/)?tr[^\>]*\>)|(\<(\/)?th[^\>]*\>)
%% 
   /****** leave major markers alone **********/
"<body"[^\>]*\> ECHO;
"</body"[^\>]*\> ECHO;
"<head"[^\>]*\> ECHO;
"</head"[^\>]*\> ECHO;
"<html"[^\>]*\> ECHO;
"</html"[^\>]*\> ECHO;
"<title"[^\>]*\> ECHO;
"</title"[^\>]*\> ECHO;
"<HR"[^\>]*\> ECHO;
   /****** leave major markers alone **********/

{pp} ECHO;
{utable} ECHO;
{hh} ECHO;
{xx} ECHO; // for Chaim
{epp} ECHO;
{ttag} ECHO;
{tag} ;

XXXXXXX"&quot;" printf("\"");

%%
