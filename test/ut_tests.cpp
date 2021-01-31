#include <gtest/gtest.h>

#include "../cpplexer.h"

#include <string_view>

using namespace std::string_view_literals;

namespace test {
//
// A few top tests are ported from test case files used for manual
// testing and are more complex than unit tests would normally have.
// New tests should be added at the bottom and should focus on
// specific functionality.
//
// All tests must check all counts, not only those that are being
// verified and are named to list expected count categories, for
// the lack of a better naming convention. Count suffixes are used
// to simplify identification and are as follows, left-to-right:
//
//    * (L) total lines
//    * (d) code
//    * (C) lines with comments
//    * (p) lines with C++ comments
//    * (c) lines with C-style comments
//    * (e) empty lines
//    * (b) brace lines
//
TEST(FlexLexerTest, TC_0L_0d_0C_0p_0c_0e_0b)
{
   CppFlexLexer lex("");

   CppFlexLexer::Result counts = lex.CountLines();

   ASSERT_EQ(0, counts.linecnt);
   ASSERT_EQ(0, counts.codecnt);
   ASSERT_EQ(0, counts.cppcnt);
   ASSERT_EQ(0, counts.cmntcnt);
   ASSERT_EQ(0, counts.emptycnt);
   ASSERT_EQ(0, counts.bracecnt);
}

TEST(FlexLexerTest, TC_11L_10d_3C_0p_3c_1e_0b)
{
   CppFlexLexer lex(R"==("string"
   "indented string with a /* comment */"
	"unterminated string
{ "string"
   { "string"
code "string" code
code /* comment */ "string"
/* comment */ "//string"
/* comment */ "string" /* comment */
"unterminated string
)==");

   CppFlexLexer::Result counts = lex.CountLines();

   ASSERT_EQ(11, counts.linecnt);
   ASSERT_EQ(10, counts.codecnt);
   ASSERT_EQ(3, counts.cmntcnt);
   ASSERT_EQ(0, counts.cppcnt);
   ASSERT_EQ(3, counts.ccnt);
   ASSERT_EQ(1, counts.emptycnt);
   ASSERT_EQ(0, counts.bracecnt);
}


TEST(FlexLexerTest, TC_14L_4d_4C_2p_3c_4e_4b)
{
   CppFlexLexer lex(R"==(
code
{
        {
        }
}

        // C++ comment
        /* C comment */

 code /* C comment */ code /* C comment */
 code /* C comment */ code // C++ comment

trailing code line)==");

   CppFlexLexer::Result counts = lex.CountLines();

   ASSERT_EQ(14, counts.linecnt);
   ASSERT_EQ(4, counts.codecnt);
   ASSERT_EQ(4, counts.cmntcnt);
   ASSERT_EQ(2, counts.cppcnt);
   ASSERT_EQ(3, counts.ccnt);
   ASSERT_EQ(4, counts.emptycnt);
   ASSERT_EQ(4, counts.bracecnt);
}

TEST(FlexLexerTest, TC_20L_15d_0C_0p_0c_5e_0b)
{
   CppFlexLexer lex(R"==("text // text"
    "text // text"
code "text // text"

'text // text'
    'text // text'
code 'text // text'

"text ' text"
    "text ' text"
code "text ' text"

"text /* text */ text"
    "text /* text */ text"
code "text /* text */ text"

'text /* text */ text'
    'text /* text */ text'
code 'text /* text */ text'
)==");

   CppFlexLexer::Result counts = lex.CountLines();

   ASSERT_EQ(20, counts.linecnt);
   ASSERT_EQ(15, counts.codecnt);
   ASSERT_EQ(0, counts.cmntcnt);
   ASSERT_EQ(0, counts.cppcnt);
   ASSERT_EQ(0, counts.ccnt);
   ASSERT_EQ(5, counts.emptycnt);
   ASSERT_EQ(0, counts.bracecnt);
}

TEST(FlexLexerTest, TC_2L_0d_1C_1p_0c_1e_0b)
{
   CppFlexLexer lex(R"==(
// trailing C++ comment line)==");

   CppFlexLexer::Result counts = lex.CountLines();

   ASSERT_EQ(2, counts.linecnt);
   ASSERT_EQ(0, counts.codecnt);
   ASSERT_EQ(1, counts.cmntcnt);
   ASSERT_EQ(1, counts.cppcnt);
   ASSERT_EQ(0, counts.ccnt);
   ASSERT_EQ(1, counts.emptycnt);
   ASSERT_EQ(0, counts.bracecnt);
}

TEST(FlexLexerTest, TC_9L_3d_5C_2p_3c_1e_1b)
{
   CppFlexLexer lex(R"==(/* 
	trailing brace test 
*/
code 

// C++ comment 
if(test) {
   code     // C++ comment
})==");

   CppFlexLexer::Result counts = lex.CountLines();

   ASSERT_EQ(9, counts.linecnt);
   ASSERT_EQ(3, counts.codecnt);
   ASSERT_EQ(5, counts.cmntcnt);
   ASSERT_EQ(2, counts.cppcnt);
   ASSERT_EQ(3, counts.ccnt);
   ASSERT_EQ(1, counts.emptycnt);
   ASSERT_EQ(1, counts.bracecnt);
}

TEST(FlexLexerTest, TC_0L_0d_0C_0p_0c_5e_0b)
{
   // 5 empty lines (including one in the next source line)
   CppFlexLexer lex(R"==(



)==");

   CppFlexLexer::Result counts = lex.CountLines();

   ASSERT_EQ(5, counts.linecnt);
   ASSERT_EQ(0, counts.codecnt);
   ASSERT_EQ(0, counts.cmntcnt);
   ASSERT_EQ(0, counts.cppcnt);
   ASSERT_EQ(0, counts.ccnt);
   ASSERT_EQ(5, counts.emptycnt);
   ASSERT_EQ(0, counts.bracecnt);
}

TEST(FlexLexerTest, TC_5L_5d_0C_0p_0c_0e_0b)
{
   // 5 code lines
   CppFlexLexer lex(R"==(code 1
code 2
code 3
code 4
code 5)==");

   CppFlexLexer::Result counts = lex.CountLines();

   ASSERT_EQ(5, counts.linecnt);
   ASSERT_EQ(5, counts.codecnt);
   ASSERT_EQ(0, counts.cmntcnt);
   ASSERT_EQ(0, counts.cppcnt);
   ASSERT_EQ(0, counts.ccnt);
   ASSERT_EQ(0, counts.emptycnt);
   ASSERT_EQ(0, counts.bracecnt);
}

TEST(FlexLexerTest, TC_6L_5d_0C_0p_0c_1e_0b)
{
   // 5 code lines, one empty line
   CppFlexLexer lex(R"==(code 1
code 2
code 3
code 4
code 5
)==");

   CppFlexLexer::Result counts = lex.CountLines();

   ASSERT_EQ(6, counts.linecnt);
   ASSERT_EQ(5, counts.codecnt);
   ASSERT_EQ(0, counts.cmntcnt);
   ASSERT_EQ(0, counts.cppcnt);
   ASSERT_EQ(0, counts.ccnt);
   ASSERT_EQ(1, counts.emptycnt);
   ASSERT_EQ(0, counts.bracecnt);
}

}
