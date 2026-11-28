/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 2

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 2 "yacc_sql.y"


#include "sql/parser/parse_defs.h"
#include "sql/parser/yacc_sql.tab.h"
#include "sql/parser/lex.yy.h"
// #include "common/log/log.h" // 包含C++中的头文件

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<strings.h>

typedef struct ParserContext {
  Query * ssql;
  size_t select_length;
  size_t condition_length;
  size_t from_length;
  size_t value_length;
  Value values[MAX_NUM];
  size_t row_num;
  size_t row_value_num[MAX_NUM];
  size_t row_start;
  int nullable;
  int type_len;
  Condition conditions[MAX_NUM];
  CompOp comp;
	char id[MAX_NUM];
} ParserContext;

//获取子串
char *substr(const char *s,int n1,int n2)/*从s中提取下标为n1~n2的字符组成一个新字符串，然后返回这个新串的首地址*/
{
  char *sp = malloc(sizeof(char) * (n2 - n1 + 2));
  int i, j = 0;
  for (i = n1; i <= n2; i++) {
    sp[j++] = s[i];
  }
  sp[j] = 0;
  return sp;
}

int agg_type_of_name(const char *name)
{
	if (0 == strcasecmp(name, "count")) return AGG_COUNT;
	if (0 == strcasecmp(name, "min")) return AGG_MIN;
	if (0 == strcasecmp(name, "max")) return AGG_MAX;
	if (0 == strcasecmp(name, "sum")) return AGG_SUM;
	if (0 == strcasecmp(name, "avg")) return AGG_AVG;
	return AGG_NONE;
}

// 词法器没有 ORDER/BY 关键字（不能重新生成 lex.yy.c），所以把 "order by"
// 替换成已有的 VALUES token，再由 order_clause 规则识别。
// 另外词法器会把 "-2" 直接吞成一个 NUMBER（[\-]?DIGIT+），使得 col1-2 无法解析成减法，
// 所以在引号外把 '-' 与后续数字之间补一个空格，让 '-' 变成独立的字符 token。
char *rewrite_order_by(const char *s)
{
	char *out = (char *)malloc(strlen(s) * 2 + 2);
	if (out == NULL) {
		return NULL;
	}
	size_t i = 0, j = 0;
	int in_quote = 0;
	while (s[i] != '\0') {
		char c = s[i];
		if (c == '\'') {
			in_quote = !in_quote;
			out[j++] = c;
			i++;
			continue;
		}
		if (!in_quote && c == '-' && s[i + 1] >= '0' && s[i + 1] <= '9') {
			out[j++] = '-';
			out[j++] = ' ';
			i++;
			continue;
		}
		if (!in_quote && (c == 'g' || c == 'G') &&
		    strncasecmp(s + i, "group", 5) == 0 &&
		    (s[i + 5] == ' ' || s[i + 5] == '\t')) {
			size_t k = i + 5;
			while (s[k] == ' ' || s[k] == '\t') {
				k++;
			}
			if ((s[k] == 'b' || s[k] == 'B') && strncasecmp(s + k, "by", 2) == 0 &&
			    (s[k + 2] == ' ' || s[k + 2] == '\t' || s[k + 2] == '\0')) {
				out[j++] = 'd';
				out[j++] = 'a';
				out[j++] = 't';
				out[j++] = 'a';
				i = k + 2;
				continue;
			}
		}
		if (!in_quote && (c == 'o' || c == 'O') &&
		    strncasecmp(s + i, "order", 5) == 0 &&
		    (s[i + 5] == ' ' || s[i + 5] == '\t')) {
			size_t k = i + 5;
			while (s[k] == ' ' || s[k] == '\t') {
				k++;
			}
			if ((s[k] == 'b' || s[k] == 'B') && strncasecmp(s + k, "by", 2) == 0 &&
			    (s[k + 2] == ' ' || s[k + 2] == '\t' || s[k + 2] == '\0')) {
				out[j++] = 'v';
				out[j++] = 'a';
				out[j++] = 'l';
				out[j++] = 'u';
				out[j++] = 'e';
				out[j++] = 's';
				i = k + 2;
				continue;
			}
		}
		out[j++] = c;
		i++;
	}
	out[j] = '\0';
	return out;
}

// ---- 表达式语法树 ----
static ExpressionNode *expr_node_new(ExprNodeType type)
{
	ExpressionNode *node = (ExpressionNode *)calloc(1, sizeof(ExpressionNode));
	if (node != NULL) {
		node->type = type;
	}
	return node;
}

static ExpressionNode *expr_node_from_value(Value *value)
{
	ExpressionNode *node = expr_node_new(EXPR_VALUE_NODE);
	if (node == NULL) {
		return NULL;
	}
	if (value->is_null) {
		node->value.type = UNDEFINED;
		node->value.data = NULL;
		node->value.is_null = 1;
		return node;
	}
	node->value.type = value->type;
	switch (value->type) {
		case INTS: {
			int *p = (int *)malloc(sizeof(int));
			*p = *(int *)value->data;
			node->value.data = p;
		} break;
		case FLOATS: {
			float *p = (float *)malloc(sizeof(float));
			*p = *(float *)value->data;
			node->value.data = p;
		} break;
		default: {
			node->value.data = strdup((const char *)value->data);
		} break;
	}
	return node;
}

static ExpressionNode *expr_node_from_attr(const char *relation_name, const char *attribute_name)
{
	ExpressionNode *node = expr_node_new(EXPR_ATTR_NODE);
	if (node == NULL) {
		return NULL;
	}
	node->relation_name = relation_name == NULL ? NULL : strdup(relation_name);
	node->attribute_name = strdup(attribute_name);
	return node;
}

static ExpressionNode *expr_node_binary(ExprOp op, ExpressionNode *left, ExpressionNode *right)
{
	ExpressionNode *node = expr_node_new(EXPR_BINARY_NODE);
	if (node == NULL) {
		return NULL;
	}
	node->op = op;
	node->left = left;
	node->right = right;
	return node;
}

static ExpressionNode *expr_node_unary(ExprOp op, ExpressionNode *operand)
{
	ExpressionNode *node = expr_node_new(EXPR_UNARY_NODE);
	if (node == NULL) {
		return NULL;
	}
	node->op = op;
	node->left = operand;
	return node;
}

static ExpressionNode *expr_node_paren(ExpressionNode *operand)
{
	ExpressionNode *node = expr_node_new(EXPR_PAREN_NODE);
	if (node == NULL) {
		return NULL;
	}
	node->left = operand;
	return node;
}

void yyerror(yyscan_t scanner, const char *str)
{
  ParserContext *context = (ParserContext *)(yyget_extra(scanner));
  query_reset(context->ssql);
  context->ssql->flag = SCF_ERROR;
  context->condition_length = 0;
  context->from_length = 0;
  context->select_length = 0;
  context->value_length = 0;
  context->ssql->sstr.insertion.value_num = 0;
  printf("parse sql failed. error=%s", str);
}

ParserContext *get_context(yyscan_t scanner)
{
  return (ParserContext *)yyget_extra(scanner);
}

#define CONTEXT get_context(scanner)


#line 298 "yacc_sql.tab.c"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "yacc_sql.tab.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_SEMICOLON = 3,                  /* SEMICOLON  */
  YYSYMBOL_CREATE = 4,                     /* CREATE  */
  YYSYMBOL_DROP = 5,                       /* DROP  */
  YYSYMBOL_TABLE = 6,                      /* TABLE  */
  YYSYMBOL_TABLES = 7,                     /* TABLES  */
  YYSYMBOL_INDEX = 8,                      /* INDEX  */
  YYSYMBOL_SELECT = 9,                     /* SELECT  */
  YYSYMBOL_DESC = 10,                      /* DESC  */
  YYSYMBOL_SHOW = 11,                      /* SHOW  */
  YYSYMBOL_SYNC = 12,                      /* SYNC  */
  YYSYMBOL_INSERT = 13,                    /* INSERT  */
  YYSYMBOL_DELETE = 14,                    /* DELETE  */
  YYSYMBOL_UPDATE = 15,                    /* UPDATE  */
  YYSYMBOL_LBRACE = 16,                    /* LBRACE  */
  YYSYMBOL_RBRACE = 17,                    /* RBRACE  */
  YYSYMBOL_COMMA = 18,                     /* COMMA  */
  YYSYMBOL_TRX_BEGIN = 19,                 /* TRX_BEGIN  */
  YYSYMBOL_TRX_COMMIT = 20,                /* TRX_COMMIT  */
  YYSYMBOL_TRX_ROLLBACK = 21,              /* TRX_ROLLBACK  */
  YYSYMBOL_INT_T = 22,                     /* INT_T  */
  YYSYMBOL_STRING_T = 23,                  /* STRING_T  */
  YYSYMBOL_FLOAT_T = 24,                   /* FLOAT_T  */
  YYSYMBOL_HELP = 25,                      /* HELP  */
  YYSYMBOL_EXIT = 26,                      /* EXIT  */
  YYSYMBOL_DOT = 27,                       /* DOT  */
  YYSYMBOL_INTO = 28,                      /* INTO  */
  YYSYMBOL_VALUES = 29,                    /* VALUES  */
  YYSYMBOL_FROM = 30,                      /* FROM  */
  YYSYMBOL_WHERE = 31,                     /* WHERE  */
  YYSYMBOL_AND = 32,                       /* AND  */
  YYSYMBOL_SET = 33,                       /* SET  */
  YYSYMBOL_ON = 34,                        /* ON  */
  YYSYMBOL_LOAD = 35,                      /* LOAD  */
  YYSYMBOL_DATA = 36,                      /* DATA  */
  YYSYMBOL_INFILE = 37,                    /* INFILE  */
  YYSYMBOL_EQ = 38,                        /* EQ  */
  YYSYMBOL_LT = 39,                        /* LT  */
  YYSYMBOL_GT = 40,                        /* GT  */
  YYSYMBOL_LE = 41,                        /* LE  */
  YYSYMBOL_GE = 42,                        /* GE  */
  YYSYMBOL_NE = 43,                        /* NE  */
  YYSYMBOL_NUMBER = 44,                    /* NUMBER  */
  YYSYMBOL_FLOAT = 45,                     /* FLOAT  */
  YYSYMBOL_ID = 46,                        /* ID  */
  YYSYMBOL_PATH = 47,                      /* PATH  */
  YYSYMBOL_SSS = 48,                       /* SSS  */
  YYSYMBOL_STAR = 49,                      /* STAR  */
  YYSYMBOL_STRING_V = 50,                  /* STRING_V  */
  YYSYMBOL_51_ = 51,                       /* '+'  */
  YYSYMBOL_52_ = 52,                       /* '-'  */
  YYSYMBOL_53_ = 53,                       /* '/'  */
  YYSYMBOL_NEG = 54,                       /* NEG  */
  YYSYMBOL_YYACCEPT = 55,                  /* $accept  */
  YYSYMBOL_commands = 56,                  /* commands  */
  YYSYMBOL_command = 57,                   /* command  */
  YYSYMBOL_exit = 58,                      /* exit  */
  YYSYMBOL_help = 59,                      /* help  */
  YYSYMBOL_sync = 60,                      /* sync  */
  YYSYMBOL_begin = 61,                     /* begin  */
  YYSYMBOL_commit = 62,                    /* commit  */
  YYSYMBOL_rollback = 63,                  /* rollback  */
  YYSYMBOL_drop_table = 64,                /* drop_table  */
  YYSYMBOL_show_tables = 65,               /* show_tables  */
  YYSYMBOL_desc_table = 66,                /* desc_table  */
  YYSYMBOL_create_index = 67,              /* create_index  */
  YYSYMBOL_index_attr_list = 68,           /* index_attr_list  */
  YYSYMBOL_drop_index = 69,                /* drop_index  */
  YYSYMBOL_create_table = 70,              /* create_table  */
  YYSYMBOL_attr_def_list = 71,             /* attr_def_list  */
  YYSYMBOL_attr_def = 72,                  /* attr_def  */
  YYSYMBOL_attr_constraint = 73,           /* attr_constraint  */
  YYSYMBOL_number = 74,                    /* number  */
  YYSYMBOL_type = 75,                      /* type  */
  YYSYMBOL_ID_get = 76,                    /* ID_get  */
  YYSYMBOL_insert = 77,                    /* insert  */
  YYSYMBOL_row_list = 78,                  /* row_list  */
  YYSYMBOL_row = 79,                       /* row  */
  YYSYMBOL_80_1 = 80,                      /* $@1  */
  YYSYMBOL_value_list = 81,                /* value_list  */
  YYSYMBOL_value = 82,                     /* value  */
  YYSYMBOL_signed_value = 83,              /* signed_value  */
  YYSYMBOL_expr = 84,                      /* expr  */
  YYSYMBOL_delete = 85,                    /* delete  */
  YYSYMBOL_update = 86,                    /* update  */
  YYSYMBOL_select = 87,                    /* select  */
  YYSYMBOL_group_clause = 88,              /* group_clause  */
  YYSYMBOL_group_list = 89,                /* group_list  */
  YYSYMBOL_group_item = 90,                /* group_item  */
  YYSYMBOL_order_clause = 91,              /* order_clause  */
  YYSYMBOL_order_list = 92,                /* order_list  */
  YYSYMBOL_order_item = 93,                /* order_item  */
  YYSYMBOL_agg_arg_tail = 94,              /* agg_arg_tail  */
  YYSYMBOL_select_attr = 95,               /* select_attr  */
  YYSYMBOL_select_item_list = 96,          /* select_item_list  */
  YYSYMBOL_select_item = 97,               /* select_item  */
  YYSYMBOL_from_list = 98,                 /* from_list  */
  YYSYMBOL_from_item = 99,                 /* from_item  */
  YYSYMBOL_where = 100,                    /* where  */
  YYSYMBOL_condition_list = 101,           /* condition_list  */
  YYSYMBOL_condition = 102,                /* condition  */
  YYSYMBOL_comOp = 103,                    /* comOp  */
  YYSYMBOL_load_data = 104                 /* load_data  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   228

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  55
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  50
/* YYNRULES -- Number of rules.  */
#define YYNRULES  121
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  239

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   306


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    51,     2,    52,     2,    53,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    54
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   310,   310,   312,   316,   317,   318,   319,   320,   321,
     322,   323,   324,   325,   326,   327,   328,   329,   330,   331,
     332,   336,   341,   346,   352,   358,   364,   370,   376,   382,
     389,   401,   420,   424,   436,   443,   452,   454,   458,   470,
     485,   486,   495,   508,   511,   512,   513,   514,   529,   538,
     550,   551,   555,   555,   561,   563,   568,   571,   574,   583,
     584,   596,   610,   615,   625,   630,   633,   636,   639,   642,
     645,   651,   661,   671,   687,   689,   693,   694,   698,   704,
     713,   715,   719,   720,   724,   730,   736,   743,   757,   764,
     782,   783,   787,   790,   792,   796,   801,   811,   819,   827,
     835,   843,   852,   883,   884,   888,   891,   894,   898,   900,
     904,   906,   911,   920,   938,   958,   959,   960,   961,   962,
     963,   967
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "SEMICOLON", "CREATE",
  "DROP", "TABLE", "TABLES", "INDEX", "SELECT", "DESC", "SHOW", "SYNC",
  "INSERT", "DELETE", "UPDATE", "LBRACE", "RBRACE", "COMMA", "TRX_BEGIN",
  "TRX_COMMIT", "TRX_ROLLBACK", "INT_T", "STRING_T", "FLOAT_T", "HELP",
  "EXIT", "DOT", "INTO", "VALUES", "FROM", "WHERE", "AND", "SET", "ON",
  "LOAD", "DATA", "INFILE", "EQ", "LT", "GT", "LE", "GE", "NE", "NUMBER",
  "FLOAT", "ID", "PATH", "SSS", "STAR", "STRING_V", "'+'", "'-'", "'/'",
  "NEG", "$accept", "commands", "command", "exit", "help", "sync", "begin",
  "commit", "rollback", "drop_table", "show_tables", "desc_table",
  "create_index", "index_attr_list", "drop_index", "create_table",
  "attr_def_list", "attr_def", "attr_constraint", "number", "type",
  "ID_get", "insert", "row_list", "row", "$@1", "value_list", "value",
  "signed_value", "expr", "delete", "update", "select", "group_clause",
  "group_list", "group_item", "order_clause", "order_list", "order_item",
  "agg_arg_tail", "select_attr", "select_item_list", "select_item",
  "from_list", "from_item", "where", "condition_list", "condition",
  "comOp", "load_data", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-185)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -185,    79,  -185,    -1,    11,     2,   -32,    19,    26,     7,
       8,    -3,    65,    84,    93,    94,    98,    77,  -185,  -185,
    -185,  -185,  -185,  -185,  -185,  -185,  -185,  -185,  -185,  -185,
    -185,  -185,  -185,  -185,  -185,  -185,    73,    81,   115,    85,
      89,    12,  -185,  -185,    51,  -185,  -185,    12,  -185,    10,
     106,   119,   135,   136,  -185,    95,    96,   107,  -185,  -185,
    -185,  -185,  -185,   108,   127,   110,   100,   144,   145,   122,
      24,    -9,   104,  -185,    12,    12,    12,    12,   105,     2,
    -185,  -185,  -185,   123,   124,   111,   113,   114,   117,   125,
    -185,  -185,  -185,  -185,    68,     4,  -185,  -185,   -38,   -38,
    -185,  -185,    -8,   118,   119,   137,    12,   153,   120,   134,
    -185,   147,     9,   150,   121,  -185,   126,   128,  -185,   126,
     105,   132,   129,  -185,  -185,   166,   152,    69,   139,  -185,
      80,   167,   114,   159,  -185,  -185,  -185,  -185,    -7,   131,
     162,  -185,    63,   163,    99,   118,   133,   154,   -22,    80,
    -185,   137,  -185,  -185,  -185,  -185,  -185,  -185,   138,    12,
      12,  -185,  -185,    58,  -185,   124,   140,   147,   178,   141,
     142,  -185,  -185,   112,   131,  -185,   143,  -185,  -185,   155,
     169,  -185,   146,   187,    12,   157,   175,  -185,   148,    10,
     139,  -185,   192,   193,  -185,  -185,  -185,   180,  -185,   195,
     156,   116,  -185,   158,   133,     3,   181,  -185,  -185,   139,
      12,    80,   183,  -185,  -185,  -185,  -185,   160,  -185,  -185,
     198,  -185,  -185,  -185,   161,  -185,   146,  -185,   139,   175,
    -185,  -185,  -185,     6,  -185,  -185,  -185,  -185,  -185
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int8 yydefact[] =
{
       2,     0,     1,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     3,    20,
      19,    14,    15,    16,    17,     9,    10,    11,    12,    13,
       8,     5,     7,     6,     4,    18,     0,     0,     0,     0,
       0,     0,    56,    57,    63,    58,    95,     0,    62,    96,
       0,    93,     0,     0,    23,     0,     0,     0,    24,    25,
      26,    22,    21,     0,     0,     0,     0,     0,     0,    63,
       0,     0,     0,    66,     0,     0,     0,     0,     0,     0,
      92,    29,    28,     0,   108,     0,     0,     0,     0,     0,
      27,    34,    65,    98,     0,     0,    64,    69,    67,    68,
      70,   105,   108,   103,    93,     0,     0,     0,     0,     0,
      48,    36,     0,     0,     0,   101,     0,     0,    97,     0,
       0,    74,     0,    94,    52,     0,    50,     0,   110,    71,
       0,     0,     0,     0,    44,    45,    46,    47,    40,     0,
       0,    90,     0,     0,     0,   104,     0,    80,     0,     0,
      49,     0,   115,   116,   117,   118,   119,   120,     0,     0,
       0,   109,    61,     0,    59,   108,     0,    36,     0,     0,
      41,    39,    32,     0,     0,    99,     0,   102,   100,    78,
      75,    76,     0,     0,     0,     0,    54,    51,   113,   112,
     110,    60,     0,     0,    37,    35,    43,     0,    42,     0,
       0,     0,    91,     0,     0,    84,    81,    82,    73,   110,
       0,     0,     0,   114,   111,    72,   121,    40,    30,    33,
       0,    79,    77,    85,     0,    87,     0,   106,   110,    54,
      53,    38,    31,    88,    83,   107,    55,    86,    89
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -185,  -185,  -185,  -185,  -185,  -185,  -185,  -185,  -185,  -185,
    -185,  -185,  -185,    29,  -185,  -185,    38,    76,    -6,  -185,
    -185,  -185,  -185,    59,  -185,  -185,   -20,  -129,  -145,    -5,
    -185,  -185,  -185,  -185,  -185,    13,  -185,  -185,   -14,    97,
    -185,   109,   149,  -185,   101,  -100,  -184,  -157,  -185,  -185
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_uint8 yydefgoto[] =
{
       0,     1,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    28,   173,    29,    30,   133,   111,   171,   197,
     138,   112,    31,   125,   126,   149,   212,    48,   165,   127,
      32,    33,    34,   147,   180,   181,   183,   206,   207,   142,
      50,    80,    51,   102,   103,   107,   161,   128,   159,    35
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      49,   164,   121,   190,   186,    36,   214,    37,    93,   169,
     120,    74,   184,   223,    52,    77,   237,    39,    41,    40,
     164,   118,   119,   106,   185,   227,    53,   209,    41,    54,
     224,   134,   135,   136,   191,    55,    70,    94,    56,   170,
      95,    92,    73,    57,   235,    38,    42,    43,    44,   225,
      45,    46,   238,   228,    47,   137,    42,    43,    69,    74,
      45,    75,    76,    77,    47,   192,   229,    71,    58,    97,
      98,    99,   100,    74,    49,    75,    76,    77,    72,     2,
     175,   176,   164,     3,     4,   115,   116,    59,     5,     6,
       7,     8,     9,    10,    11,   117,    60,    61,    12,    13,
      14,    62,    42,    43,    15,    16,    45,   152,   153,   154,
     155,   156,   157,    63,    17,   158,   178,   176,    74,    64,
      75,    76,    77,    66,    42,    43,   162,    65,    45,   199,
     200,    67,   163,   220,   200,    68,    78,    79,    81,    82,
      85,    83,    84,    87,    88,    86,    89,    90,    91,    72,
      96,   101,   105,   124,   189,   106,   129,   108,   130,   114,
     110,   109,   131,   113,   122,   132,   139,   140,   146,   150,
     151,   160,   141,   166,   143,   148,   168,   172,   174,   179,
     177,   195,   203,   182,   188,   196,   193,   204,   198,   202,
     208,   210,   205,   211,   213,   215,   216,   217,   218,   226,
     230,   232,   219,   201,   221,   194,   170,   233,   167,   236,
     187,   231,   234,   123,     0,     0,   144,   222,     0,     0,
       0,   145,     0,     0,     0,     0,     0,     0,   104
};

static const yytype_int16 yycheck[] =
{
       5,   130,   102,   160,   149,     6,   190,     8,    17,    16,
      18,    49,    34,    10,    46,    53,    10,     6,    16,     8,
     149,    17,    18,    31,    46,   209,     7,   184,    16,     3,
      27,    22,    23,    24,   163,    28,    41,    46,    30,    46,
      49,    17,    47,    46,   228,    46,    44,    45,    46,    46,
      48,    49,    46,   210,    52,    46,    44,    45,    46,    49,
      48,    51,    52,    53,    52,   165,   211,    16,     3,    74,
      75,    76,    77,    49,    79,    51,    52,    53,    27,     0,
      17,    18,   211,     4,     5,    17,    18,     3,     9,    10,
      11,    12,    13,    14,    15,    27,     3,     3,    19,    20,
      21,     3,    44,    45,    25,    26,    48,    38,    39,    40,
      41,    42,    43,    36,    35,    46,    17,    18,    49,    46,
      51,    52,    53,     8,    44,    45,    46,    46,    48,    17,
      18,    46,    52,    17,    18,    46,    30,    18,     3,     3,
      33,    46,    46,    16,    34,    37,    46,     3,     3,    27,
      46,    46,    29,    16,   159,    31,     3,    46,    38,    34,
      46,    48,    28,    46,    46,    18,    16,    46,    36,     3,
      18,    32,    46,     6,    46,    46,    17,    46,    16,    46,
      17,     3,    27,    29,    46,    44,    46,    18,    46,    46,
       3,    34,    46,    18,    46,     3,     3,    17,     3,    18,
      17,     3,    46,   174,    46,   167,    46,    46,   132,   229,
     151,   217,   226,   104,    -1,    -1,   119,   204,    -1,    -1,
      -1,   120,    -1,    -1,    -1,    -1,    -1,    -1,    79
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    56,     0,     4,     5,     9,    10,    11,    12,    13,
      14,    15,    19,    20,    21,    25,    26,    35,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    69,
      70,    77,    85,    86,    87,   104,     6,     8,    46,     6,
       8,    16,    44,    45,    46,    48,    49,    52,    82,    84,
      95,    97,    46,     7,     3,    28,    30,    46,     3,     3,
       3,     3,     3,    36,    46,    46,     8,    46,    46,    46,
      84,    16,    27,    84,    49,    51,    52,    53,    30,    18,
      96,     3,     3,    46,    46,    33,    37,    16,    34,    46,
       3,     3,    17,    17,    46,    49,    46,    84,    84,    84,
      84,    46,    98,    99,    97,    29,    31,   100,    46,    48,
      46,    72,    76,    46,    34,    17,    18,    27,    17,    18,
      18,   100,    46,    96,    16,    78,    79,    84,   102,     3,
      38,    28,    18,    71,    22,    23,    24,    46,    75,    16,
      46,    46,    94,    46,    94,    99,    36,    88,    46,    80,
       3,    18,    38,    39,    40,    41,    42,    43,    46,   103,
      32,   101,    46,    52,    82,    83,     6,    72,    17,    16,
      46,    73,    46,    68,    16,    17,    18,    17,    17,    46,
      89,    90,    29,    91,    34,    46,    83,    78,    46,    84,
     102,    82,   100,    46,    71,     3,    44,    74,    46,    17,
      18,    68,    46,    27,    18,    46,    92,    93,     3,   102,
      34,    18,    81,    46,   101,     3,     3,    17,     3,    46,
      17,    46,    90,    10,    27,    46,    18,   101,   102,    83,
      17,    73,     3,    46,    93,   101,    81,    10,    46
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    55,    56,    56,    57,    57,    57,    57,    57,    57,
      57,    57,    57,    57,    57,    57,    57,    57,    57,    57,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    67,    68,    68,    69,    70,    71,    71,    72,    72,
      73,    73,    73,    74,    75,    75,    75,    75,    76,    77,
      78,    78,    80,    79,    81,    81,    82,    82,    82,    83,
      83,    83,    84,    84,    84,    84,    84,    84,    84,    84,
      84,    85,    86,    87,    88,    88,    89,    89,    90,    90,
      91,    91,    92,    92,    93,    93,    93,    93,    93,    93,
      94,    94,    95,    96,    96,    97,    97,    97,    97,    97,
      97,    97,    97,    98,    98,    99,    99,    99,   100,   100,
     101,   101,   102,   102,   102,   103,   103,   103,   103,   103,
     103,   104
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     2,     2,     2,     2,     2,     2,     4,     3,     3,
       9,    10,     1,     3,     4,     8,     0,     3,     6,     3,
       0,     1,     2,     1,     1,     1,     1,     1,     1,     6,
       1,     3,     0,     5,     0,     3,     1,     1,     1,     1,
       2,     1,     1,     1,     3,     3,     2,     3,     3,     3,
       3,     5,     8,     8,     0,     2,     1,     3,     1,     3,
       0,     2,     1,     3,     1,     2,     4,     2,     3,     4,
       1,     3,     2,     0,     3,     1,     1,     4,     3,     6,
       6,     4,     6,     1,     3,     1,     6,     7,     0,     3,
       0,     3,     3,     3,     4,     1,     1,     1,     1,     1,
       1,     8
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (scanner, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, scanner); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, void *scanner)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (scanner);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, void *scanner)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, scanner);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule, void *scanner)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)], scanner);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, scanner); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, void *scanner)
{
  YY_USE (yyvaluep);
  YY_USE (scanner);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (void *scanner)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, scanner);
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 21: /* exit: EXIT SEMICOLON  */
#line 336 "yacc_sql.y"
                   {
        CONTEXT->ssql->flag=SCF_EXIT;//"exit";
    }
#line 1562 "yacc_sql.tab.c"
    break;

  case 22: /* help: HELP SEMICOLON  */
#line 341 "yacc_sql.y"
                   {
        CONTEXT->ssql->flag=SCF_HELP;//"help";
    }
#line 1570 "yacc_sql.tab.c"
    break;

  case 23: /* sync: SYNC SEMICOLON  */
#line 346 "yacc_sql.y"
                   {
      CONTEXT->ssql->flag = SCF_SYNC;
    }
#line 1578 "yacc_sql.tab.c"
    break;

  case 24: /* begin: TRX_BEGIN SEMICOLON  */
#line 352 "yacc_sql.y"
                        {
      CONTEXT->ssql->flag = SCF_BEGIN;
    }
#line 1586 "yacc_sql.tab.c"
    break;

  case 25: /* commit: TRX_COMMIT SEMICOLON  */
#line 358 "yacc_sql.y"
                         {
      CONTEXT->ssql->flag = SCF_COMMIT;
    }
#line 1594 "yacc_sql.tab.c"
    break;

  case 26: /* rollback: TRX_ROLLBACK SEMICOLON  */
#line 364 "yacc_sql.y"
                           {
      CONTEXT->ssql->flag = SCF_ROLLBACK;
    }
#line 1602 "yacc_sql.tab.c"
    break;

  case 27: /* drop_table: DROP TABLE ID SEMICOLON  */
#line 370 "yacc_sql.y"
                            {
        CONTEXT->ssql->flag = SCF_DROP_TABLE;//"drop_table";
        drop_table_init(&CONTEXT->ssql->sstr.drop_table, (yyvsp[-1].string));
    }
#line 1611 "yacc_sql.tab.c"
    break;

  case 28: /* show_tables: SHOW TABLES SEMICOLON  */
#line 376 "yacc_sql.y"
                          {
      CONTEXT->ssql->flag = SCF_SHOW_TABLES;
    }
#line 1619 "yacc_sql.tab.c"
    break;

  case 29: /* desc_table: DESC ID SEMICOLON  */
#line 382 "yacc_sql.y"
                      {
      CONTEXT->ssql->flag = SCF_DESC_TABLE;
      desc_table_init(&CONTEXT->ssql->sstr.desc_table, (yyvsp[-1].string));
    }
#line 1628 "yacc_sql.tab.c"
    break;

  case 30: /* create_index: CREATE INDEX ID ON ID LBRACE index_attr_list RBRACE SEMICOLON  */
#line 390 "yacc_sql.y"
                {
			CONTEXT->ssql->flag = SCF_CREATE_INDEX;//"create_index";
			CreateIndex *ci = &CONTEXT->ssql->sstr.create_index;
			ci->index_name = strdup((yyvsp[-6].string));
			ci->relation_name = strdup((yyvsp[-4].string));
			ci->unique = 0;
			ci->attribute_name = ci->attribute_num > 0 ? strdup(ci->attributes[0]) : NULL;
			free((yyvsp[-6].string));
			free((yyvsp[-4].string));
		}
#line 1643 "yacc_sql.tab.c"
    break;

  case 31: /* create_index: CREATE ID INDEX ID ON ID LBRACE index_attr_list RBRACE SEMICOLON  */
#line 402 "yacc_sql.y"
                {
			if (0 != strcasecmp((yyvsp[-8].string), "unique")) {
				CONTEXT->ssql->flag = SCF_ERROR;
				yyerror(scanner, "expect unique");
			}
			CONTEXT->ssql->flag = SCF_CREATE_INDEX;
			CreateIndex *ci = &CONTEXT->ssql->sstr.create_index;
			ci->index_name = strdup((yyvsp[-6].string));
			ci->relation_name = strdup((yyvsp[-4].string));
			ci->unique = 1;
			ci->attribute_name = ci->attribute_num > 0 ? strdup(ci->attributes[0]) : NULL;
			free((yyvsp[-8].string));
			free((yyvsp[-6].string));
			free((yyvsp[-4].string));
		}
#line 1663 "yacc_sql.tab.c"
    break;

  case 32: /* index_attr_list: ID  */
#line 420 "yacc_sql.y"
       {
        CONTEXT->ssql->sstr.create_index.attributes[0] = (yyvsp[0].string);
        CONTEXT->ssql->sstr.create_index.attribute_num = 1;
      }
#line 1672 "yacc_sql.tab.c"
    break;

  case 33: /* index_attr_list: index_attr_list COMMA ID  */
#line 424 "yacc_sql.y"
                               {
        size_t n = CONTEXT->ssql->sstr.create_index.attribute_num;
        if (n < MAX_NUM) {
          CONTEXT->ssql->sstr.create_index.attributes[n] = (yyvsp[0].string);
          CONTEXT->ssql->sstr.create_index.attribute_num = n + 1;
        } else {
          free((yyvsp[0].string));
        }
      }
#line 1686 "yacc_sql.tab.c"
    break;

  case 34: /* drop_index: DROP INDEX ID SEMICOLON  */
#line 437 "yacc_sql.y"
                {
			CONTEXT->ssql->flag=SCF_DROP_INDEX;//"drop_index";
			drop_index_init(&CONTEXT->ssql->sstr.drop_index, (yyvsp[-1].string));
		}
#line 1695 "yacc_sql.tab.c"
    break;

  case 35: /* create_table: CREATE TABLE ID LBRACE attr_def attr_def_list RBRACE SEMICOLON  */
#line 444 "yacc_sql.y"
                {
			CONTEXT->ssql->flag=SCF_CREATE_TABLE;//"create_table";
			// CONTEXT->ssql->sstr.create_table.attribute_count = CONTEXT->value_length;
			create_table_init_name(&CONTEXT->ssql->sstr.create_table, (yyvsp[-5].string));
			//临时变量清零	
			CONTEXT->value_length = 0;
		}
#line 1707 "yacc_sql.tab.c"
    break;

  case 37: /* attr_def_list: COMMA attr_def attr_def_list  */
#line 454 "yacc_sql.y"
                                   {    }
#line 1713 "yacc_sql.tab.c"
    break;

  case 38: /* attr_def: ID_get type LBRACE number RBRACE attr_constraint  */
#line 459 "yacc_sql.y"
                {
			AttrInfo attribute;
			CONTEXT->type_len = 0;
			attr_info_init(&attribute, CONTEXT->id, (yyvsp[-4].number), (yyvsp[-2].number), CONTEXT->nullable);
			create_table_append_attribute(&CONTEXT->ssql->sstr.create_table, &attribute);
			// CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].name =(char*)malloc(sizeof(char));
			// strcpy(CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].name, CONTEXT->id); 
			// CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].type = $2;  
			// CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].length = $4;
			CONTEXT->value_length++;
		}
#line 1729 "yacc_sql.tab.c"
    break;

  case 39: /* attr_def: ID_get type attr_constraint  */
#line 471 "yacc_sql.y"
                {
			AttrInfo attribute;
			attr_info_init(&attribute, CONTEXT->id, (yyvsp[-1].number), CONTEXT->type_len > 0 ? CONTEXT->type_len : 4, CONTEXT->nullable);
			CONTEXT->type_len = 0;
			create_table_append_attribute(&CONTEXT->ssql->sstr.create_table, &attribute);
			// CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].name=(char*)malloc(sizeof(char));
			// strcpy(CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].name, CONTEXT->id); 
			// CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].type=$2;  
			// CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].length=4; // default attribute length
			CONTEXT->value_length++;
		}
#line 1745 "yacc_sql.tab.c"
    break;

  case 40: /* attr_constraint: %empty  */
#line 485 "yacc_sql.y"
                { CONTEXT->nullable = 0; }
#line 1751 "yacc_sql.tab.c"
    break;

  case 41: /* attr_constraint: ID  */
#line 486 "yacc_sql.y"
         {
        if (0 == strcasecmp((yyvsp[0].string), "nullable")) {
          CONTEXT->nullable = 1;
        } else {
          CONTEXT->ssql->flag = SCF_ERROR;
          yyerror(scanner, "expect nullable or not null");
        }
        free((yyvsp[0].string));
      }
#line 1765 "yacc_sql.tab.c"
    break;

  case 42: /* attr_constraint: ID ID  */
#line 495 "yacc_sql.y"
            {
        if (0 == strcasecmp((yyvsp[-1].string), "not") && 0 == strcasecmp((yyvsp[0].string), "null")) {
          CONTEXT->nullable = 0;
        } else {
          CONTEXT->ssql->flag = SCF_ERROR;
          yyerror(scanner, "expect nullable or not null");
        }
        free((yyvsp[-1].string));
        free((yyvsp[0].string));
      }
#line 1780 "yacc_sql.tab.c"
    break;

  case 43: /* number: NUMBER  */
#line 508 "yacc_sql.y"
                       {(yyval.number) = (yyvsp[0].number);}
#line 1786 "yacc_sql.tab.c"
    break;

  case 44: /* type: INT_T  */
#line 511 "yacc_sql.y"
              { (yyval.number)=INTS; }
#line 1792 "yacc_sql.tab.c"
    break;

  case 45: /* type: STRING_T  */
#line 512 "yacc_sql.y"
                  { (yyval.number)=CHARS; }
#line 1798 "yacc_sql.tab.c"
    break;

  case 46: /* type: FLOAT_T  */
#line 513 "yacc_sql.y"
                 { (yyval.number)=FLOATS; }
#line 1804 "yacc_sql.tab.c"
    break;

  case 47: /* type: ID  */
#line 514 "yacc_sql.y"
            {
		CONTEXT->type_len = 0;
		if (0 == strcasecmp((yyvsp[0].string), "date")) {
			(yyval.number) = DATES;
		} else if (0 == strcasecmp((yyvsp[0].string), "text")) {
			// text 是不限长的字符串，按测试要求一个字段最大 4096 字节
			(yyval.number) = CHARS;
			CONTEXT->type_len = 4096;
		} else {
			(yyval.number) = UNDEFINED;
		}
		free((yyvsp[0].string));
       }
#line 1822 "yacc_sql.tab.c"
    break;

  case 48: /* ID_get: ID  */
#line 530 "yacc_sql.y"
        {
		char *temp=(yyvsp[0].string); 
		snprintf(CONTEXT->id, sizeof(CONTEXT->id), "%s", temp);
	}
#line 1831 "yacc_sql.tab.c"
    break;

  case 49: /* insert: INSERT INTO ID VALUES row_list SEMICOLON  */
#line 539 "yacc_sql.y"
                {
			CONTEXT->ssql->flag=SCF_INSERT;//"insert";
			inserts_init(&CONTEXT->ssql->sstr.insertion, (yyvsp[-3].string), CONTEXT->values, CONTEXT->value_length,
          CONTEXT->row_num, CONTEXT->row_value_num);

      //临时变量清零
      CONTEXT->value_length=0;
      CONTEXT->row_num=0;
    }
#line 1845 "yacc_sql.tab.c"
    break;

  case 52: /* $@1: %empty  */
#line 555 "yacc_sql.y"
           { CONTEXT->row_start = CONTEXT->value_length; }
#line 1851 "yacc_sql.tab.c"
    break;

  case 53: /* row: LBRACE $@1 signed_value value_list RBRACE  */
#line 556 "yacc_sql.y"
    {
      CONTEXT->row_value_num[CONTEXT->row_num++] = CONTEXT->value_length - CONTEXT->row_start;
    }
#line 1859 "yacc_sql.tab.c"
    break;

  case 55: /* value_list: COMMA signed_value value_list  */
#line 563 "yacc_sql.y"
                                     { 
  		// CONTEXT->values[CONTEXT->value_length++] = *$2;
	  }
#line 1867 "yacc_sql.tab.c"
    break;

  case 56: /* value: NUMBER  */
#line 568 "yacc_sql.y"
          {	
  		value_init_integer(&CONTEXT->values[CONTEXT->value_length++], (yyvsp[0].number));
		}
#line 1875 "yacc_sql.tab.c"
    break;

  case 57: /* value: FLOAT  */
#line 571 "yacc_sql.y"
          {
  		value_init_float(&CONTEXT->values[CONTEXT->value_length++], (yyvsp[0].floats));
		}
#line 1883 "yacc_sql.tab.c"
    break;

  case 58: /* value: SSS  */
#line 574 "yacc_sql.y"
         {
			(yyvsp[0].string) = substr((yyvsp[0].string),1,strlen((yyvsp[0].string))-2);
  		value_init_string(&CONTEXT->values[CONTEXT->value_length++], (yyvsp[0].string));
		}
#line 1892 "yacc_sql.tab.c"
    break;

  case 59: /* signed_value: value  */
#line 583 "yacc_sql.y"
          { (yyval.value1) = &CONTEXT->values[CONTEXT->value_length - 1]; }
#line 1898 "yacc_sql.tab.c"
    break;

  case 60: /* signed_value: '-' value  */
#line 584 "yacc_sql.y"
                {
        Value *v = &CONTEXT->values[CONTEXT->value_length - 1];
        if (v->type == INTS) {
          *(int *)v->data = -*(int *)v->data;
        } else if (v->type == FLOATS) {
          *(float *)v->data = -*(float *)v->data;
        } else {
          CONTEXT->ssql->flag = SCF_ERROR;
          yyerror(scanner, "minus only applies to number");
        }
        (yyval.value1) = v;
      }
#line 1915 "yacc_sql.tab.c"
    break;

  case 61: /* signed_value: ID  */
#line 596 "yacc_sql.y"
         {
        if (0 != strcasecmp((yyvsp[0].string), "null")) {
          CONTEXT->ssql->flag = SCF_ERROR;
          yyerror(scanner, "expect null");
        }
        free((yyvsp[0].string));
        Value *v = &CONTEXT->values[CONTEXT->value_length++];
        value_init_null(v);
        (yyval.value1) = v;
      }
#line 1930 "yacc_sql.tab.c"
    break;

  case 62: /* expr: value  */
#line 610 "yacc_sql.y"
          {
        Value *v = &CONTEXT->values[CONTEXT->value_length - 1];
        (yyval.expr) = expr_node_from_value(v);
        if ((yyval.expr) == NULL) { CONTEXT->ssql->flag = SCF_ERROR; }
      }
#line 1940 "yacc_sql.tab.c"
    break;

  case 63: /* expr: ID  */
#line 615 "yacc_sql.y"
         {
        if (0 == strcasecmp((yyvsp[0].string), "null")) {
          Value null_value;
          value_init_null(&null_value);
          (yyval.expr) = expr_node_from_value(&null_value);
        } else {
          (yyval.expr) = expr_node_from_attr(NULL, (yyvsp[0].string));
        }
        free((yyvsp[0].string));
      }
#line 1955 "yacc_sql.tab.c"
    break;

  case 64: /* expr: ID DOT ID  */
#line 625 "yacc_sql.y"
                {
        (yyval.expr) = expr_node_from_attr((yyvsp[-2].string), (yyvsp[0].string));
        free((yyvsp[-2].string));
        free((yyvsp[0].string));
      }
#line 1965 "yacc_sql.tab.c"
    break;

  case 65: /* expr: LBRACE expr RBRACE  */
#line 630 "yacc_sql.y"
                         {
        (yyval.expr) = expr_node_paren((yyvsp[-1].expr));
      }
#line 1973 "yacc_sql.tab.c"
    break;

  case 66: /* expr: '-' expr  */
#line 633 "yacc_sql.y"
                         {
        (yyval.expr) = expr_node_unary(EXPR_OP_SUB, (yyvsp[0].expr));
      }
#line 1981 "yacc_sql.tab.c"
    break;

  case 67: /* expr: expr '+' expr  */
#line 636 "yacc_sql.y"
                    {
        (yyval.expr) = expr_node_binary(EXPR_OP_ADD, (yyvsp[-2].expr), (yyvsp[0].expr));
      }
#line 1989 "yacc_sql.tab.c"
    break;

  case 68: /* expr: expr '-' expr  */
#line 639 "yacc_sql.y"
                    {
        (yyval.expr) = expr_node_binary(EXPR_OP_SUB, (yyvsp[-2].expr), (yyvsp[0].expr));
      }
#line 1997 "yacc_sql.tab.c"
    break;

  case 69: /* expr: expr STAR expr  */
#line 642 "yacc_sql.y"
                     {
        (yyval.expr) = expr_node_binary(EXPR_OP_MUL, (yyvsp[-2].expr), (yyvsp[0].expr));
      }
#line 2005 "yacc_sql.tab.c"
    break;

  case 70: /* expr: expr '/' expr  */
#line 645 "yacc_sql.y"
                    {
        (yyval.expr) = expr_node_binary(EXPR_OP_DIV, (yyvsp[-2].expr), (yyvsp[0].expr));
      }
#line 2013 "yacc_sql.tab.c"
    break;

  case 71: /* delete: DELETE FROM ID where SEMICOLON  */
#line 652 "yacc_sql.y"
                {
			CONTEXT->ssql->flag = SCF_DELETE;//"delete";
			deletes_init_relation(&CONTEXT->ssql->sstr.deletion, (yyvsp[-2].string));
			deletes_set_conditions(&CONTEXT->ssql->sstr.deletion, 
					CONTEXT->conditions, CONTEXT->condition_length);
			CONTEXT->condition_length = 0;	
    }
#line 2025 "yacc_sql.tab.c"
    break;

  case 72: /* update: UPDATE ID SET ID EQ signed_value where SEMICOLON  */
#line 662 "yacc_sql.y"
                {
			CONTEXT->ssql->flag = SCF_UPDATE;//"update";
			Value *value = &CONTEXT->values[0];
			updates_init(&CONTEXT->ssql->sstr.update, (yyvsp[-6].string), (yyvsp[-4].string), value, 
					CONTEXT->conditions, CONTEXT->condition_length);
			CONTEXT->condition_length = 0;
		}
#line 2037 "yacc_sql.tab.c"
    break;

  case 73: /* select: SELECT select_attr FROM from_list where group_clause order_clause SEMICOLON  */
#line 672 "yacc_sql.y"
                {

			selects_append_conditions(&CONTEXT->ssql->sstr.selection, CONTEXT->conditions, CONTEXT->condition_length);

			CONTEXT->ssql->flag=SCF_SELECT;//"select";
			// CONTEXT->ssql->sstr.selection.attr_num = CONTEXT->select_length;

			//临时变量清零
			CONTEXT->condition_length=0;
			CONTEXT->from_length=0;
			CONTEXT->select_length=0;
			CONTEXT->value_length = 0;
	}
#line 2055 "yacc_sql.tab.c"
    break;

  case 78: /* group_item: ID  */
#line 698 "yacc_sql.y"
       {
		RelAttr attr;
		relation_attr_init(&attr, NULL, (yyvsp[0].string));
		selects_append_group_by(&CONTEXT->ssql->sstr.selection, &attr);
		free((yyvsp[0].string));
	}
#line 2066 "yacc_sql.tab.c"
    break;

  case 79: /* group_item: ID DOT ID  */
#line 704 "yacc_sql.y"
                {
		RelAttr attr;
		relation_attr_init(&attr, (yyvsp[-2].string), (yyvsp[0].string));
		selects_append_group_by(&CONTEXT->ssql->sstr.selection, &attr);
		free((yyvsp[-2].string));
		free((yyvsp[0].string));
	}
#line 2078 "yacc_sql.tab.c"
    break;

  case 84: /* order_item: ID  */
#line 724 "yacc_sql.y"
       {
		RelAttr attr;
		relation_attr_init(&attr, NULL, (yyvsp[0].string));
		selects_append_order_by(&CONTEXT->ssql->sstr.selection, &attr, 1);
		free((yyvsp[0].string));
	}
#line 2089 "yacc_sql.tab.c"
    break;

  case 85: /* order_item: ID DESC  */
#line 730 "yacc_sql.y"
              {
		RelAttr attr;
		relation_attr_init(&attr, NULL, (yyvsp[-1].string));
		selects_append_order_by(&CONTEXT->ssql->sstr.selection, &attr, 0);
		free((yyvsp[-1].string));
	}
#line 2100 "yacc_sql.tab.c"
    break;

  case 86: /* order_item: ID DOT ID DESC  */
#line 736 "yacc_sql.y"
                     {
		RelAttr attr;
		relation_attr_init(&attr, (yyvsp[-3].string), (yyvsp[-1].string));
		selects_append_order_by(&CONTEXT->ssql->sstr.selection, &attr, 0);
		free((yyvsp[-3].string));
		free((yyvsp[-1].string));
	}
#line 2112 "yacc_sql.tab.c"
    break;

  case 87: /* order_item: ID ID  */
#line 743 "yacc_sql.y"
            {
		int asc = 1;
		if (0 == strcasecmp((yyvsp[0].string), "desc")) {
			asc = 0;
		} else if (0 != strcasecmp((yyvsp[0].string), "asc")) {
			CONTEXT->ssql->flag = SCF_ERROR;
			yyerror(scanner, "expect ASC or DESC");
		}
		RelAttr attr;
		relation_attr_init(&attr, NULL, (yyvsp[-1].string));
		selects_append_order_by(&CONTEXT->ssql->sstr.selection, &attr, asc);
		free((yyvsp[-1].string));
		free((yyvsp[0].string));
	}
#line 2131 "yacc_sql.tab.c"
    break;

  case 88: /* order_item: ID DOT ID  */
#line 757 "yacc_sql.y"
                {
		RelAttr attr;
		relation_attr_init(&attr, (yyvsp[-2].string), (yyvsp[0].string));
		selects_append_order_by(&CONTEXT->ssql->sstr.selection, &attr, 1);
		free((yyvsp[-2].string));
		free((yyvsp[0].string));
	}
#line 2143 "yacc_sql.tab.c"
    break;

  case 89: /* order_item: ID DOT ID ID  */
#line 764 "yacc_sql.y"
                   {
		int asc = 1;
		if (0 == strcasecmp((yyvsp[0].string), "desc")) {
			asc = 0;
		} else if (0 != strcasecmp((yyvsp[0].string), "asc")) {
			CONTEXT->ssql->flag = SCF_ERROR;
			yyerror(scanner, "expect ASC or DESC");
		}
		RelAttr attr;
		relation_attr_init(&attr, (yyvsp[-3].string), (yyvsp[-1].string));
		selects_append_order_by(&CONTEXT->ssql->sstr.selection, &attr, asc);
		free((yyvsp[-3].string));
		free((yyvsp[-1].string));
		free((yyvsp[0].string));
	}
#line 2163 "yacc_sql.tab.c"
    break;

  case 90: /* agg_arg_tail: ID  */
#line 782 "yacc_sql.y"
       { free((yyvsp[0].string)); }
#line 2169 "yacc_sql.tab.c"
    break;

  case 91: /* agg_arg_tail: agg_arg_tail COMMA ID  */
#line 783 "yacc_sql.y"
                            { free((yyvsp[0].string)); }
#line 2175 "yacc_sql.tab.c"
    break;

  case 95: /* select_item: STAR  */
#line 796 "yacc_sql.y"
         {
			RelAttr attr;
			relation_attr_init(&attr, NULL, "*");
			selects_append_attribute(&CONTEXT->ssql->sstr.selection, &attr);
		}
#line 2185 "yacc_sql.tab.c"
    break;

  case 96: /* select_item: expr  */
#line 801 "yacc_sql.y"
           {
			RelAttr attr;
			if ((yyvsp[0].expr) != NULL && (yyvsp[0].expr)->type == EXPR_ATTR_NODE) {
				relation_attr_init(&attr, (yyvsp[0].expr)->relation_name, (yyvsp[0].expr)->attribute_name);
			} else {
				relation_attr_init(&attr, NULL, "");
				attr.expr = (yyvsp[0].expr);
			}
			selects_append_attribute(&CONTEXT->ssql->sstr.selection, &attr);
		}
#line 2200 "yacc_sql.tab.c"
    break;

  case 97: /* select_item: ID LBRACE STAR RBRACE  */
#line 811 "yacc_sql.y"
                                {
			int agg = agg_type_of_name((yyvsp[-3].string));
			free((yyvsp[-3].string));
			RelAttr attr;
			relation_attr_init(&attr, NULL, "*");
			attr.agg_type = agg;
			selects_append_attribute(&CONTEXT->ssql->sstr.selection, &attr);
		}
#line 2213 "yacc_sql.tab.c"
    break;

  case 98: /* select_item: ID LBRACE RBRACE  */
#line 819 "yacc_sql.y"
                           {
			int agg = agg_type_of_name((yyvsp[-2].string));
			free((yyvsp[-2].string));
			RelAttr attr;
			relation_attr_init(&attr, NULL, "");
			attr.agg_type = agg;
			selects_append_attribute(&CONTEXT->ssql->sstr.selection, &attr);
		}
#line 2226 "yacc_sql.tab.c"
    break;

  case 99: /* select_item: ID LBRACE ID COMMA agg_arg_tail RBRACE  */
#line 827 "yacc_sql.y"
                                                 {
			int agg = agg_type_of_name((yyvsp[-5].string));
			free((yyvsp[-5].string));
			RelAttr attr;
			relation_attr_init(&attr, NULL, "");
			attr.agg_type = agg == AGG_NONE ? AGG_COUNT : agg;
			selects_append_attribute(&CONTEXT->ssql->sstr.selection, &attr);
		}
#line 2239 "yacc_sql.tab.c"
    break;

  case 100: /* select_item: ID LBRACE STAR COMMA agg_arg_tail RBRACE  */
#line 835 "yacc_sql.y"
                                                   {
			int agg = agg_type_of_name((yyvsp[-5].string));
			free((yyvsp[-5].string));
			RelAttr attr;
			relation_attr_init(&attr, NULL, "");
			attr.agg_type = agg == AGG_NONE ? AGG_COUNT : agg;
			selects_append_attribute(&CONTEXT->ssql->sstr.selection, &attr);
		}
#line 2252 "yacc_sql.tab.c"
    break;

  case 101: /* select_item: ID LBRACE ID RBRACE  */
#line 843 "yacc_sql.y"
                              {
			int agg = agg_type_of_name((yyvsp[-3].string));
			free((yyvsp[-3].string));
			RelAttr attr;
			relation_attr_init(&attr, NULL, (yyvsp[-1].string));
			attr.agg_type = agg;
			selects_append_attribute(&CONTEXT->ssql->sstr.selection, &attr);
			free((yyvsp[-1].string));
		}
#line 2266 "yacc_sql.tab.c"
    break;

  case 102: /* select_item: ID LBRACE ID DOT ID RBRACE  */
#line 852 "yacc_sql.y"
                                     {
			int agg = agg_type_of_name((yyvsp[-5].string));
			free((yyvsp[-5].string));
			RelAttr attr;
			relation_attr_init(&attr, (yyvsp[-3].string), (yyvsp[-1].string));
			attr.agg_type = agg;
			selects_append_attribute(&CONTEXT->ssql->sstr.selection, &attr);
			free((yyvsp[-3].string));
			free((yyvsp[-1].string));
		}
#line 2281 "yacc_sql.tab.c"
    break;

  case 105: /* from_item: ID  */
#line 888 "yacc_sql.y"
       {
        selects_append_relation(&CONTEXT->ssql->sstr.selection, (yyvsp[0].string));
      }
#line 2289 "yacc_sql.tab.c"
    break;

  case 106: /* from_item: from_item ID ID ON condition condition_list  */
#line 891 "yacc_sql.y"
                                                  {  /* inner join t on ... */
        selects_append_relation(&CONTEXT->ssql->sstr.selection, (yyvsp[-2].string));
      }
#line 2297 "yacc_sql.tab.c"
    break;

  case 107: /* from_item: from_item ID ID ID ON condition condition_list  */
#line 894 "yacc_sql.y"
                                                     {  /* inner/left/right join t on ... */
        selects_append_relation(&CONTEXT->ssql->sstr.selection, (yyvsp[-2].string));
      }
#line 2305 "yacc_sql.tab.c"
    break;

  case 109: /* where: WHERE condition condition_list  */
#line 900 "yacc_sql.y"
                                     {	
				// CONTEXT->conditions[CONTEXT->condition_length++]=*$2;
			}
#line 2313 "yacc_sql.tab.c"
    break;

  case 111: /* condition_list: AND condition condition_list  */
#line 906 "yacc_sql.y"
                                   {
				// CONTEXT->conditions[CONTEXT->condition_length++]=*$2;
			}
#line 2321 "yacc_sql.tab.c"
    break;

  case 112: /* condition: expr comOp expr  */
#line 912 "yacc_sql.y"
                {
			Condition condition;
			memset(&condition, 0, sizeof(condition));
			condition.comp = CONTEXT->comp;
			condition.left_expr = (yyvsp[-2].expr);
			condition.right_expr = (yyvsp[0].expr);
			CONTEXT->conditions[CONTEXT->condition_length++] = condition;
		}
#line 2334 "yacc_sql.tab.c"
    break;

  case 113: /* condition: expr ID ID  */
#line 921 "yacc_sql.y"
                {
			Condition condition;
			memset(&condition, 0, sizeof(condition));
			if (0 == strcasecmp((yyvsp[-1].string), "is") && 0 == strcasecmp((yyvsp[0].string), "null")) {
				condition.comp = IS_NULL;
			} else if (0 == strcasecmp((yyvsp[-1].string), "not") && 0 == strcasecmp((yyvsp[0].string), "null")) {
				condition.comp = IS_NOT_NULL;
			} else {
				CONTEXT->ssql->flag = SCF_ERROR;
				yyerror(scanner, "expect is null or is not null");
			}
			condition.left_expr = (yyvsp[-2].expr);
			condition.right_expr = NULL;
			CONTEXT->conditions[CONTEXT->condition_length++] = condition;
			free((yyvsp[-1].string));
			free((yyvsp[0].string));
		}
#line 2356 "yacc_sql.tab.c"
    break;

  case 114: /* condition: expr ID ID ID  */
#line 939 "yacc_sql.y"
                {
			Condition condition;
			memset(&condition, 0, sizeof(condition));
			if (0 == strcasecmp((yyvsp[-2].string), "is") && 0 == strcasecmp((yyvsp[-1].string), "not") && 0 == strcasecmp((yyvsp[0].string), "null")) {
				condition.comp = IS_NOT_NULL;
			} else {
				CONTEXT->ssql->flag = SCF_ERROR;
				yyerror(scanner, "expect is not null");
			}
			condition.left_expr = (yyvsp[-3].expr);
			condition.right_expr = NULL;
			CONTEXT->conditions[CONTEXT->condition_length++] = condition;
			free((yyvsp[-2].string));
			free((yyvsp[-1].string));
			free((yyvsp[0].string));
		}
#line 2377 "yacc_sql.tab.c"
    break;

  case 115: /* comOp: EQ  */
#line 958 "yacc_sql.y"
             { CONTEXT->comp = EQUAL_TO; }
#line 2383 "yacc_sql.tab.c"
    break;

  case 116: /* comOp: LT  */
#line 959 "yacc_sql.y"
         { CONTEXT->comp = LESS_THAN; }
#line 2389 "yacc_sql.tab.c"
    break;

  case 117: /* comOp: GT  */
#line 960 "yacc_sql.y"
         { CONTEXT->comp = GREAT_THAN; }
#line 2395 "yacc_sql.tab.c"
    break;

  case 118: /* comOp: LE  */
#line 961 "yacc_sql.y"
         { CONTEXT->comp = LESS_EQUAL; }
#line 2401 "yacc_sql.tab.c"
    break;

  case 119: /* comOp: GE  */
#line 962 "yacc_sql.y"
         { CONTEXT->comp = GREAT_EQUAL; }
#line 2407 "yacc_sql.tab.c"
    break;

  case 120: /* comOp: NE  */
#line 963 "yacc_sql.y"
         { CONTEXT->comp = NOT_EQUAL; }
#line 2413 "yacc_sql.tab.c"
    break;

  case 121: /* load_data: LOAD DATA INFILE SSS INTO TABLE ID SEMICOLON  */
#line 968 "yacc_sql.y"
                {
		  CONTEXT->ssql->flag = SCF_LOAD_DATA;
			load_data_init(&CONTEXT->ssql->sstr.load_data, (yyvsp[-1].string), (yyvsp[-4].string));
		}
#line 2422 "yacc_sql.tab.c"
    break;


#line 2426 "yacc_sql.tab.c"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (scanner, YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, scanner);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, scanner);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (scanner, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, scanner);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, scanner);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 973 "yacc_sql.y"

//_____________________________________________________________________
extern void scan_string(const char *str, yyscan_t scanner);

int sql_parse(const char *s, Query *sqls){
	ParserContext context;
	memset(&context, 0, sizeof(context));

	yyscan_t scanner;
	yylex_init_extra(&context, &scanner);
	context.ssql = sqls;
	char *rewritten = rewrite_order_by(s);
	scan_string(rewritten != NULL ? rewritten : s, scanner);
	int result = yyparse(scanner);
	yylex_destroy(scanner);
	free(rewritten);
	return result;
}
