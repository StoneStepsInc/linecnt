%option noyywrap
%option batch
%option never-interactive
%option 8bit

DIGIT				[0-9]
DASHCHAR			"-"
WS				[\x09\x0B\x0C\x0E-\x20]
EOL				\r|\r\n|\n|\0

%x c_comment
%x cpp_comment

%{
#include "cpplexer_decl.h"
%}

%%

\/\/			BEGIN(cpp_comment); return TOKEN_CPP_COMMENT_START;
\/\*			BEGIN(c_comment); return TOKEN_C_COMMENT_START;
{EOL}			return TOKEN_EOL;

<c_comment>\*\/		BEGIN(INITIAL); return TOKEN_C_COMMENT_END;
<c_comment>{EOL}	return TOKEN_C_COMMENT_EOL;


<cpp_comment>{EOL}	BEGIN(INITIAL); return TOKEN_CPP_COMMENT_EOL;


<<EOF>> BEGIN(INITIAL); return TOKEN_CPP_END;
<*>.	

%%



