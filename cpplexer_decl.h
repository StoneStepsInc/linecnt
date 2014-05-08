#ifndef __CPPLEXER_DECL_H
#define __CPPLEXER_DECL_H

#include <stdio.h>

#ifndef __CPPLEXER_IMP_CPP
#include "cpplexer_decl.h"
#endif
//
//
//

class CppFlexLexer {
	public:
		CppFlexLexer(FILE* arg_yyin = 0, FILE* arg_yyout = 0 );

		~CppFlexLexer(void);
		
		const char *YYText(void);

		int yylex(void);
};

#ifndef __CPPLEXER_IMP_CPP
#undef yyFlexLexer
#endif // __CPPLEXER_IMP_CPP

//
//
//

#define TOKEN_CPP_END				0
#define TOKEN_CPP_COMMENT_START	1
#define TOKEN_CPP_COMMENT			3
#define TOKEN_C_COMMENT_START		4
#define TOKEN_C_COMMENT_END		5
#define TOKEN_EOL						6
#define TOKEN_C_COMMENT_EOL		7
#define TOKEN_CPP_COMMENT_EOL		8

#endif // __CPPLEXER_DECL_H
