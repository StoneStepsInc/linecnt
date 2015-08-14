/*
    linecnt - a source line counting utility

    Copyright (c) 2003-2015, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information
*/
%option noyywrap
%option batch
%option never-interactive
%option 8bit

WS                   [\x09\x0B\x0C\x0E-\x20]
CODE                 [^\x09\x0B\x0C\x0E-\x20\r\n]
CPP_COMMENT          \/\/
C_COMMENT_START      \/\*
C_COMMENT_END        \*\/
EOL                  \r\n|\r|\n

/*
 * Use start conditions to match pattern groups selectively and also
 * handle end-of-file based on the current parsing state, which is 
 * necessary because <<EOF>> does not allow patterns.
 *
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

%x lws
%x lbr

%x str
%x str_c_comment

%{
#include "cpplexer_decl.h"
%}

%%

  /* check for leading whitespace */
^{WS}+                                    BEGIN(lws);

  /* check for leading single brace surrounded by optional whitespace */
^{WS}*[\{\}]{WS}*                         BEGIN(lbr);

  /* a string is considered as code, so we only need to track if we saw C comments */
<INITIAL,lws,lbr,code>\"                  BEGIN(str);
<c_comment,code_c_comment>\"              BEGIN(str_c_comment);

  /* ignore the '\' character followed by new-line */
<str,str_c_comment>\\{EOL}

  /* ignore all escape sequences */
<str,str_c_comment>\\.

  /* count lines with unterminiated strings (shouldn't happen in compilable code) */
<str>{EOL}                                BEGIN(INITIAL); return TOKEN_CODE_EOL;
<str_c_comment>{EOL}                      BEGIN(INITIAL); return TOKEN_CODE_C_COMMENT_EOL;

  /* ignore all other string characters (must be after all the patterns above) */
<str,str_c_comment>[^\"]

  /* end the string and enter the code state based on whether we saw any C comments */
<str>\"                                   BEGIN(code);
<str_c_comment>\"                         BEGIN(code_c_comment);

  /* count an empty line */
^{EOL}                                    BEGIN(INITIAL); return TOKEN_EMPTY_LINE;
<lws>{EOL}                                BEGIN(INITIAL); return TOKEN_EMPTY_LINE;

  /* count a brace-only line */
<lbr>{EOL}                                BEGIN(INITIAL); return TOKEN_BRACE_LINE;

  /* enter code state if we matched any non-whitespace characters */
<INITIAL,lws,lbr>{CODE}                   BEGIN(code);

  /* enter the C++ comment state */
<INITIAL,lws,lbr>{CPP_COMMENT}            BEGIN(cpp_comment);
<code>{CPP_COMMENT}                       BEGIN(code_cpp_comment);
<code_c_comment>{CPP_COMMENT}             BEGIN(code_c_cpp_comment);
<c_comment>{CPP_COMMENT}                  BEGIN(c_cpp_comment);

  /* enter code/C-comment state if we matched any non-whitespace characters after seeing a C comment */
<c_comment>{CODE}                         BEGIN(code_c_comment);

  /* count the code line and reset the state */
<code>{EOL}                               BEGIN(INITIAL); return TOKEN_CODE_EOL;

  /* count a line that contains a closed C comment and reset the state */
<c_comment>{EOL}                          BEGIN(INITIAL); return TOKEN_C_COMMENT_EOL;

  /* count a line with an open C comment */
<c_comment_open>{EOL}                     return TOKEN_C_COMMENT_EOL;

  /* count a line that contains some code and a closed C comment and reset the state */
<code_c_comment>{EOL}                     BEGIN(INITIAL); return TOKEN_CODE_C_COMMENT_EOL;

  /* count a line with some code and an open C comment */
<code_c_comment_open>{EOL}                return TOKEN_CODE_C_COMMENT_EOL;

  /* count a C++ comment line and reset the state because C++ comments always end lines */
<cpp_comment>.*{EOL}                      BEGIN(INITIAL); return TOKEN_CPP_COMMENT_EOL;
<code_cpp_comment>.*{EOL}                 BEGIN(INITIAL); return TOKEN_CODE_CPP_COMMENT_EOL;
<code_c_cpp_comment>.*{EOL}               BEGIN(INITIAL); return TOKEN_CODE_C_CPP_COMMENT_EOL;
<c_cpp_comment>.*{EOL}                    BEGIN(INITIAL); return TOKEN_C_CPP_COMMENT_EOL;

  /* enter an open C comment state that reflects whether we saw some code or not */
<INITIAL,lws,lbr,c_comment>{C_COMMENT_START}      BEGIN(c_comment_open);
<code,code_c_comment>{C_COMMENT_START}    BEGIN(code_c_comment_open);

  /* close an open C comment and enter the state that reflects whether we saw some code or not */
<c_comment_open>{C_COMMENT_END}           BEGIN(c_comment);
<code_c_comment_open>{C_COMMENT_END}      BEGIN(code_c_comment);

 /* 
  * We need to match trailing lines at the end of the file, but the <<EOF>> rule 
  * does not accept patterns, so we need to maintain a state per pattern we want 
  * to recohnize. Flex v2.5.37 on Linux matches {EOL} rules above at the end of 
  * the file, so some of the rules below will not fire on Linux.
  *
  * The state should be reset back to INITIAL at the end of the file because Flex 
  * does not do it automatically when opening a new file.
  *
  * Note that unqualified <<EOF>> applies to all start conditions.
  */

<INITIAL,lws><<EOF>>                      BEGIN(INITIAL); return TOKEN_EMPTY_LINE + TOKEN_EOF;

<lbr><<EOF>>                              BEGIN(INITIAL); return TOKEN_BRACE_LINE + TOKEN_EOF;

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

  /* count lines with unterminiated strings (shouldn't happen in compilable code) */
<str><<EOF>>                              BEGIN(INITIAL); return TOKEN_CODE_EOL;
<str_c_comment><<EOF>>                    BEGIN(INITIAL); return TOKEN_CODE_C_COMMENT_EOL;

  /* discard any other characters in any state */
<*>.

%%
