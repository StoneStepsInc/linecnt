%option noyywrap
%option batch
%option never-interactive
%option 8bit

WS				[\x09\x0B\x0C\x0E-\x20]
CODE				[^\x09\x0B\x0C\x0E-\x20\r\n]
CPP_COMMENT			\/\/
C_COMMENT_START			\/\*
C_COMMENT_END			\*\/
EOL				\r|\r\n|\n|\x00

/*
 * All states are exclusive. Combined states indicate that both types
 * of source have been seen on the line (e.g. code followed by C-style
 * comments).
 */
%x code
%x code_c_comment
%x code_c_comment_open
%x c_comment
%x c_comment_open

%{
#include "cpplexer_decl.h"
%}

%%

  /* count an empty line */
^{WS}*{EOL}					return TOKEN_EMPTY_LINE;

  /* count a brace-only line */
^{WS}*[\{\}]{WS}*{EOL}				return TOKEN_BRACE_LINE;

  /* enter code state if we matched any non-whitespace characters */
{CODE}						BEGIN(code);

  /* enter code/C-comment state if we matched any non-whitespace characters after seeing a C comment */
<c_comment>{CODE}				BEGIN(code_c_comment);

  /* count the code line and reset the state */
<code>{EOL}					BEGIN(INITIAL); return TOKEN_CODE_EOL;

  /* count a line that contains a closed C comment and reset the state */
<c_comment>{EOL}				BEGIN(INITIAL); return TOKEN_C_COMMENT_EOL;

  /* count a line with an open C comment */
<c_comment_open>{EOL}				return TOKEN_C_COMMENT_EOL;

  /* count a line that contains some code and a closed C comment and reset the state */
<code_c_comment>{EOL}				BEGIN(INITIAL); return TOKEN_CODE_C_COMMENT_EOL;

  /* count a line with some code and an open C comment */
<code_c_comment_open>{EOL}			return TOKEN_CODE_C_COMMENT_EOL;

  /* count a C++ comment line and reset the state because C++ comments always end lines */
{CPP_COMMENT}.*{EOL}				return TOKEN_CPP_COMMENT_EOL;
<code>{CPP_COMMENT}.*{EOL}			BEGIN(INITIAL); return TOKEN_CODE_CPP_COMMENT_EOL;
<code_c_comment>{CPP_COMMENT}.*{EOL}		BEGIN(INITIAL); return TOKEN_CODE_C_CPP_COMMENT_EOL;

  /* enter an open C comment state that reflects whether we saw some code or not */
<INITIAL,c_comment>{C_COMMENT_START}		BEGIN(c_comment_open);
<code,code_c_comment>{C_COMMENT_START}		BEGIN(code_c_comment_open);

  /* close an open C comment and enter the state that reflects whether we saw some code or not */
<c_comment_open>{C_COMMENT_END}			BEGIN(c_comment);
<code_c_comment_open>{C_COMMENT_END}		BEGIN(code_c_comment);

 /* 
  * In general, we need to match trailing code or code/C-comment lines
  * in a file with a pattern similar to that below, but current Flex 
  * matches these trailing lines as if they has {EOL} at the end, so 
  * there's no need for these rules.
  * 
  * <code><<EOF>> 				return TOKEN_TRAILING_CODE_EOL;
  * <code_c_comment><<EOF>> 			return TOKEN_TRAILING_CODE_C_COMMENT_EOL;
  */
<<EOF>>						BEGIN(INITIAL); return TOKEN_CPP_END;

 /*
  * Note that we do not handle the open C comment state at the end of 
  * the file because the expectation is that the file is syntactically
  * correct. Otherwise we would need to add a special return code to 
  * handle the end of file with an open C comment, which is messy.
  *
  */

  /* discard non-leading whitespace in any state */
<*>{WS}

  /* discard any other characters in any state */
<*>.

%%


