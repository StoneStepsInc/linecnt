/*
    linecnt - a source line counting utility

    Copyright (c) 2003-2020, Stone Steps Inc. (www.stonesteps.ca)

    See COPYING and Copyright files for additional licensing and copyright information
*/

/*
 * noyywrap                never call yywrap() to check if there's more input
 * batch                   optimize for a batch run
 * never-interactive       never read one character at a time (as if from TTY)
 * 8bit                    all eight bits are significant in all characters
 * nounistd                do not include unistd.h
 */
%option noyywrap
%option batch
%option never-interactive
%option 8bit
%option nounistd

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

%x BOL

/* leading whitespace */
%x lws

/* brace-only line */
%x brl

/*
 * Define scanner states for single- and double-quote strings, so we can skip 
 * string characters that would be interpreted otherwise. For example, this C++ 
 * string literal could be counted as a C++ comment unless string characters are 
 * ignored:
 *
 *   const char *url = "http://localhost/"
 *
 * Both types of strings are considered code, which means that we only need to 
 * track whether we saw any C comments prior to encountering the opening quote 
 * of a string.
 */
%x dqstr
%x dqstr_c_comment

%x sqstr
%x sqstr_c_comment

%{
#include "cpplexer_scanner.h"
%}

%%

  /* check for leading whitespace */
<INITIAL,BOL>^{WS}+                       BEGIN(lws);

  /* check for leading single brace surrounded by optional whitespace */
<INITIAL,BOL>^{WS}*[\{\}]{WS}*            BEGIN(brl);

  /* a string is considered code, so we only need to track if we saw C comments */
<INITIAL,BOL,lws,brl,code>\"              BEGIN(dqstr);
<c_comment,code_c_comment>\"              BEGIN(dqstr_c_comment);
<INITIAL,BOL,lws,brl,code>\'              BEGIN(sqstr);
<c_comment,code_c_comment>\'              BEGIN(sqstr_c_comment);

  /* ignore the '\' character followed by new-line */
<dqstr,dqstr_c_comment>\\{EOL}
<sqstr,sqstr_c_comment>\\{EOL}

  /* ignore all escape sequences */
<dqstr,dqstr_c_comment>\\.
<sqstr,sqstr_c_comment>\\.

  /* count lines with unterminiated strings (shouldn't happen in compilable code) */
<dqstr>{EOL}                              BEGIN(BOL); return TOKEN_CODE_EOL;
<dqstr_c_comment>{EOL}                    BEGIN(BOL); return TOKEN_CODE_C_COMMENT_EOL;
<sqstr>{EOL}                              BEGIN(BOL); return TOKEN_CODE_EOL;
<sqstr_c_comment>{EOL}                    BEGIN(BOL); return TOKEN_CODE_C_COMMENT_EOL;

  /* ignore all other string characters (must be after all the patterns above) */
<dqstr,dqstr_c_comment>[^\"]
<sqstr,sqstr_c_comment>[^\']

  /* end the string and enter the code state based on whether we saw any C comments */
<dqstr>\"                                 BEGIN(code);
<dqstr_c_comment>\"                       BEGIN(code_c_comment);
<sqstr>\'                                 BEGIN(code);
<sqstr_c_comment>\'                       BEGIN(code_c_comment);

  /* count an empty line */
<INITIAL,BOL>^{EOL}                       BEGIN(BOL); return TOKEN_EMPTY_LINE;
<lws>{EOL}                                BEGIN(BOL); return TOKEN_EMPTY_LINE;

  /* count a brace-only line */
<brl>{EOL}                                BEGIN(BOL); return TOKEN_BRACE_LINE;

  /* enter code state if we matched any non-whitespace characters */
<INITIAL,BOL,lws,brl>{CODE}               BEGIN(code);

  /* enter the C++ comment state */
<INITIAL,BOL,lws,brl>{CPP_COMMENT}        BEGIN(cpp_comment);
<code>{CPP_COMMENT}                       BEGIN(code_cpp_comment);
<code_c_comment>{CPP_COMMENT}             BEGIN(code_c_cpp_comment);
<c_comment>{CPP_COMMENT}                  BEGIN(c_cpp_comment);

  /* enter code/C-comment state if we matched any non-whitespace characters after seeing a C comment */
<c_comment>{CODE}                         BEGIN(code_c_comment);

  /* count the code line and reset the state */
<code>{EOL}                               BEGIN(BOL); return TOKEN_CODE_EOL;

  /* count a line that contains a closed C comment and reset the state */
<c_comment>{EOL}                          BEGIN(BOL); return TOKEN_C_COMMENT_EOL;

  /* count a line with an open C comment */
<c_comment_open>{EOL}                     return TOKEN_C_COMMENT_EOL;

  /* count a line that contains some code and a closed C comment and reset the state */
<code_c_comment>{EOL}                     BEGIN(BOL); return TOKEN_CODE_C_COMMENT_EOL;

  /* count a line with some code and an open C comment */
<code_c_comment_open>{EOL}                return TOKEN_CODE_C_COMMENT_EOL;

  /* count a C++ comment line and reset the state because C++ comments always end lines */
<cpp_comment>.*{EOL}                      BEGIN(BOL); return TOKEN_CPP_COMMENT_EOL;
<code_cpp_comment>.*{EOL}                 BEGIN(BOL); return TOKEN_CODE_CPP_COMMENT_EOL;
<code_c_cpp_comment>.*{EOL}               BEGIN(BOL); return TOKEN_CODE_C_CPP_COMMENT_EOL;
<c_cpp_comment>.*{EOL}                    BEGIN(BOL); return TOKEN_C_CPP_COMMENT_EOL;

  /* enter an open C comment state that reflects whether we saw some code or not */
<INITIAL,BOL,lws,brl,c_comment>{C_COMMENT_START}      BEGIN(c_comment_open);
<code,code_c_comment>{C_COMMENT_START}    BEGIN(code_c_comment_open);

  /* close an open C comment and enter the state that reflects whether we saw some code or not */
<c_comment_open>{C_COMMENT_END}           BEGIN(c_comment);
<code_c_comment_open>{C_COMMENT_END}      BEGIN(code_c_comment);

 /* 
  * We need to match trailing lines at the end of the file, but the <<EOF>> rule 
  * does not accept patterns, so we need to maintain a state per pattern we want 
  * to recohnize.
  * 
  * vi adds an extra LF character at the end of any text file it saves, but does
  * not count this empty line as such. When a file edited in vi is parsed by this 
  * lexer, the total number of lines for this file will be reported one more than 
  * vi shows as a line count. This is not a bug.
  * 
  * The state should be reset back to INITIAL at the end of the file because Flex 
  * does not do it automatically when opening a new file.
  *
  * Note that unqualified <<EOF>> applies to all states.
  */

<INITIAL><<EOF>>                          BEGIN(INITIAL); return TOKEN_EOF;
<BOL,lws><<EOF>>                          BEGIN(INITIAL); return TOKEN_EMPTY_LINE + TOKEN_EOF;

<brl><<EOF>>                              BEGIN(INITIAL); return TOKEN_BRACE_LINE + TOKEN_EOF;

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
<dqstr><<EOF>>                            BEGIN(INITIAL); return TOKEN_CODE_EOL;
<dqstr_c_comment><<EOF>>                  BEGIN(INITIAL); return TOKEN_CODE_C_COMMENT_EOL;
<sqstr><<EOF>>                            BEGIN(INITIAL); return TOKEN_CODE_EOL;
<sqstr_c_comment><<EOF>>                  BEGIN(INITIAL); return TOKEN_CODE_C_COMMENT_EOL;

  /* discard any other characters in any state */
<*>.

%%
