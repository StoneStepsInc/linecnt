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
 * All states are exclusive. Combined states indicate that all types
 * of source have been seen on the line (e.g. code followed by C-style
 * comments or vice versa).
 */
%x code
%x c_comment
%x cpp_comment

%x code_c_comment
%x code_cpp_comment
%x code_c_cpp_comment

%x c_cpp_comment

%x c_comment_open
%x code_c_comment_open

%{
#include "cpplexer_decl.h"
%}

%%

  /* count an empty line */
^{WS}*{EOL}					               return TOKEN_EMPTY_LINE;

  /* count a brace-only line */
^{WS}*[\{\}]{WS}*{EOL}				      return TOKEN_BRACE_LINE;

  /* enter code state if we matched any non-whitespace characters */
{CODE}						BEGIN(code);

  /* */
{CPP_COMMENT}                          BEGIN(cpp_comment);
<code>{CPP_COMMENT}                    BEGIN(code_cpp_comment);
<code_c_comment>{CPP_COMMENT}          BEGIN(code_c_cpp_comment);
<c_comment>{CPP_COMMENT}               BEGIN(c_cpp_comment);

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
<cpp_comment>.*{EOL}                BEGIN(INITIAL); return TOKEN_CPP_COMMENT_EOL;
<code_cpp_comment>.*{EOL}           BEGIN(INITIAL); return TOKEN_CODE_CPP_COMMENT_EOL;
<code_c_cpp_comment>.*{EOL}         BEGIN(INITIAL); return TOKEN_CODE_C_CPP_COMMENT_EOL;
<c_cpp_comment>.*{EOL}              BEGIN(INITIAL); return TOKEN_C_CPP_COMMENT_EOL;

  /* enter an open C comment state that reflects whether we saw some code or not */
<INITIAL,c_comment>{C_COMMENT_START}		BEGIN(c_comment_open);
<code,code_c_comment>{C_COMMENT_START}		BEGIN(code_c_comment_open);

  /* close an open C comment and enter the state that reflects whether we saw some code or not */
<c_comment_open>{C_COMMENT_END}			BEGIN(c_comment);
<code_c_comment_open>{C_COMMENT_END}		BEGIN(code_c_comment);

 /* 
  * We need to match trailing lines at the end of the file, but the <<EOF>> rule 
  * does not accept patterns, so we need to maintain a state per pattern we want 
  * to recohnize. Flex v2.5.37 on Linux matches {EOL} rules above at the end of 
  * the file, so some of the rules below will not fire.
  *
  * Note that we need to reset the state at the end of the file because Flex does 
  * not do it automatically when opening a new file.
  */

  /* unqualified <<EOF>> applies to all states */ 
<INITIAL><<EOF>>                          BEGIN(INITIAL); return TOKEN_EOF;

<code><<EOF>>                             BEGIN(INITIAL); return TOKEN_CODE_EOL + TOKEN_EOF;
<c_comment><<EOF>>                        BEGIN(INITIAL); return TOKEN_C_COMMENT_EOL + TOKEN_EOF;
<cpp_comment><<EOF>>                      BEGIN(INITIAL); return TOKEN_CPP_COMMENT_EOL + TOKEN_EOF;
<c_cpp_comment><<EOF>>                    BEGIN(INITIAL); return TOKEN_C_CPP_COMMENT_EOL + TOKEN_EOF;
<code_c_comment><<EOF>>                   BEGIN(INITIAL); return TOKEN_CODE_C_COMMENT_EOL + TOKEN_EOF;
<code_cpp_comment><<EOF>>                 BEGIN(INITIAL); return TOKEN_CODE_CPP_COMMENT_EOL + TOKEN_EOF;
<code_c_cpp_comment><<EOF>>               BEGIN(INITIAL); return TOKEN_CODE_C_CPP_COMMENT_EOL + TOKEN_EOF;

  /* count an unterminated C comment at the end of the file as a complete comment */
<c_comment_open><<EOF>>                   BEGIN(INITIAL); return TOKEN_C_COMMENT_EOL + TOKEN_EOF;
<code_c_comment_open><<EOF>>              BEGIN(INITIAL); return TOKEN_CODE_C_COMMENT_EOL + TOKEN_EOF;

  /* discard any other characters in any state */
<*>.

%%


