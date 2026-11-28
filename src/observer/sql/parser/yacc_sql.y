
%{

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

%}

%define api.pure full
%lex-param { yyscan_t scanner }
%parse-param { void *scanner }

//标识tokens
%token  SEMICOLON
        CREATE
        DROP
        TABLE
        TABLES
        INDEX
        SELECT
        DESC
        SHOW
        SYNC
        INSERT
        DELETE
        UPDATE
        LBRACE
        RBRACE
        COMMA
        TRX_BEGIN
        TRX_COMMIT
        TRX_ROLLBACK
        INT_T
        STRING_T
        FLOAT_T
        HELP
        EXIT
        DOT //QUOTE
        INTO
        VALUES
        FROM
        WHERE
        AND
        SET
        ON
        LOAD
        DATA
        INFILE
        EQ
        LT
        GT
        LE
        GE
        NE

%union {
  struct _Attr *attr;
  struct _Condition *condition1;
  struct _Value *value1;
  char *string;
  int number;
  float floats;
	char *position;
	struct _ExpressionNode *expr;
}

%token <number> NUMBER
%token <floats> FLOAT 
%token <string> ID
%token <string> PATH
%token <string> SSS
%token <string> STAR
%token <string> STRING_V
//非终结符

%type <number> type;
%type <condition1> condition;
%type <value1> value;
%type <number> number;
%type <expr> expr;
%type <value1> signed_value;

// 算术运算优先级：+ - 最低，* / 次之，一元负号最高
%left '+' '-'
%left STAR '/'
%right NEG

%%

commands:		//commands or sqls. parser starts here.
    /* empty */
    | commands command
    ;

command:
	  select  
	| insert
	| update
	| delete
	| create_table
	| drop_table
	| show_tables
	| desc_table
	| create_index	
	| drop_index
	| sync
	| begin
	| commit
	| rollback
	| load_data
	| help
	| exit
    ;

exit:			
    EXIT SEMICOLON {
        CONTEXT->ssql->flag=SCF_EXIT;//"exit";
    };

help:
    HELP SEMICOLON {
        CONTEXT->ssql->flag=SCF_HELP;//"help";
    };

sync:
    SYNC SEMICOLON {
      CONTEXT->ssql->flag = SCF_SYNC;
    }
    ;

begin:
    TRX_BEGIN SEMICOLON {
      CONTEXT->ssql->flag = SCF_BEGIN;
    }
    ;

commit:
    TRX_COMMIT SEMICOLON {
      CONTEXT->ssql->flag = SCF_COMMIT;
    }
    ;

rollback:
    TRX_ROLLBACK SEMICOLON {
      CONTEXT->ssql->flag = SCF_ROLLBACK;
    }
    ;

drop_table:		/*drop table 语句的语法解析树*/
    DROP TABLE ID SEMICOLON {
        CONTEXT->ssql->flag = SCF_DROP_TABLE;//"drop_table";
        drop_table_init(&CONTEXT->ssql->sstr.drop_table, $3);
    };

show_tables:
    SHOW TABLES SEMICOLON {
      CONTEXT->ssql->flag = SCF_SHOW_TABLES;
    }
    ;

desc_table:
    DESC ID SEMICOLON {
      CONTEXT->ssql->flag = SCF_DESC_TABLE;
      desc_table_init(&CONTEXT->ssql->sstr.desc_table, $2);
    }
    ;

create_index:		/*create index 语句的语法解析树*/
    CREATE INDEX ID ON ID LBRACE ID RBRACE SEMICOLON 
		{
			CONTEXT->ssql->flag = SCF_CREATE_INDEX;//"create_index";
			create_index_init(&CONTEXT->ssql->sstr.create_index, $3, $5, $7);
		}
    ;

drop_index:			/*drop index 语句的语法解析树*/
    DROP INDEX ID  SEMICOLON 
		{
			CONTEXT->ssql->flag=SCF_DROP_INDEX;//"drop_index";
			drop_index_init(&CONTEXT->ssql->sstr.drop_index, $3);
		}
    ;
create_table:		/*create table 语句的语法解析树*/
    CREATE TABLE ID LBRACE attr_def attr_def_list RBRACE SEMICOLON 
		{
			CONTEXT->ssql->flag=SCF_CREATE_TABLE;//"create_table";
			// CONTEXT->ssql->sstr.create_table.attribute_count = CONTEXT->value_length;
			create_table_init_name(&CONTEXT->ssql->sstr.create_table, $3);
			//临时变量清零	
			CONTEXT->value_length = 0;
		}
    ;
attr_def_list:
    /* empty */
    | COMMA attr_def attr_def_list {    }
    ;
    
attr_def:
    ID_get type LBRACE number RBRACE attr_constraint
		{
			AttrInfo attribute;
			attr_info_init(&attribute, CONTEXT->id, $2, $4, CONTEXT->nullable);
			create_table_append_attribute(&CONTEXT->ssql->sstr.create_table, &attribute);
			// CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].name =(char*)malloc(sizeof(char));
			// strcpy(CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].name, CONTEXT->id); 
			// CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].type = $2;  
			// CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].length = $4;
			CONTEXT->value_length++;
		}
    |ID_get type attr_constraint
		{
			AttrInfo attribute;
			attr_info_init(&attribute, CONTEXT->id, $2, 4, CONTEXT->nullable);
			create_table_append_attribute(&CONTEXT->ssql->sstr.create_table, &attribute);
			// CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].name=(char*)malloc(sizeof(char));
			// strcpy(CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].name, CONTEXT->id); 
			// CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].type=$2;  
			// CONTEXT->ssql->sstr.create_table.attributes[CONTEXT->value_length].length=4; // default attribute length
			CONTEXT->value_length++;
		}
    ;
/* 字段约束：默认可以存 null，"not null" 表示不允许 */
attr_constraint:
    /* empty */ { CONTEXT->nullable = 0; }
    | ID {
        if (0 == strcasecmp($1, "nullable")) {
          CONTEXT->nullable = 1;
        } else {
          CONTEXT->ssql->flag = SCF_ERROR;
          yyerror(scanner, "expect nullable or not null");
        }
        free($1);
      }
    | ID ID {
        if (0 == strcasecmp($1, "not") && 0 == strcasecmp($2, "null")) {
          CONTEXT->nullable = 0;
        } else {
          CONTEXT->ssql->flag = SCF_ERROR;
          yyerror(scanner, "expect nullable or not null");
        }
        free($1);
        free($2);
      }
    ;

number:
		NUMBER {$$ = $1;}
		;
type:
	INT_T { $$=INTS; }
       | STRING_T { $$=CHARS; }
       | FLOAT_T { $$=FLOATS; }
       | ID {
		if (0 == strcasecmp($1, "date")) {
			$$ = DATES;
		} else {
			$$ = UNDEFINED;
		}
		free($1);
       }
       ;
ID_get:
	ID 
	{
		char *temp=$1; 
		snprintf(CONTEXT->id, sizeof(CONTEXT->id), "%s", temp);
	}
	;

	
insert:				/*insert   语句的语法解析树*/
    INSERT INTO ID VALUES row_list SEMICOLON 
		{
			CONTEXT->ssql->flag=SCF_INSERT;//"insert";
			inserts_init(&CONTEXT->ssql->sstr.insertion, $3, CONTEXT->values, CONTEXT->value_length,
          CONTEXT->row_num, CONTEXT->row_value_num);

      //临时变量清零
      CONTEXT->value_length=0;
      CONTEXT->row_num=0;
    }

row_list:
    row
    | row COMMA row_list
    ;

row:
    LBRACE { CONTEXT->row_start = CONTEXT->value_length; } signed_value value_list RBRACE
    {
      CONTEXT->row_value_num[CONTEXT->row_num++] = CONTEXT->value_length - CONTEXT->row_start;
    }
    ;

value_list:
    /* empty */
    | COMMA signed_value value_list  { 
  		// CONTEXT->values[CONTEXT->value_length++] = *$2;
	  }
    ;
value:
    NUMBER{	
  		value_init_integer(&CONTEXT->values[CONTEXT->value_length++], $1);
		}
    |FLOAT{
  		value_init_float(&CONTEXT->values[CONTEXT->value_length++], $1);
		}
    |SSS {
			$1 = substr($1,1,strlen($1)-2);
  		value_init_string(&CONTEXT->values[CONTEXT->value_length++], $1);
		}
    ;

/* insert/update 里的常量可以是负数：词法器已经不允许把 "-2" 吞成一个 NUMBER，
   所以这里用独立的 '-' token 处理 */
signed_value:
    value { $$ = &CONTEXT->values[CONTEXT->value_length - 1]; }
    | '-' value {
        Value *v = &CONTEXT->values[CONTEXT->value_length - 1];
        if (v->type == INTS) {
          *(int *)v->data = -*(int *)v->data;
        } else if (v->type == FLOATS) {
          *(float *)v->data = -*(float *)v->data;
        } else {
          CONTEXT->ssql->flag = SCF_ERROR;
          yyerror(scanner, "minus only applies to number");
        }
        $$ = v;
      }
    | ID {
        if (0 != strcasecmp($1, "null")) {
          CONTEXT->ssql->flag = SCF_ERROR;
          yyerror(scanner, "expect null");
        }
        free($1);
        Value *v = &CONTEXT->values[CONTEXT->value_length++];
        value_init_null(v);
        $$ = v;
      }
    ;

/* 算术表达式。词法器里 '+' '/' 是字符 token，'*' 是 STAR token */
expr:
    value {
        Value *v = &CONTEXT->values[CONTEXT->value_length - 1];
        $$ = expr_node_from_value(v);
        if ($$ == NULL) { CONTEXT->ssql->flag = SCF_ERROR; }
      }
    | ID {
        if (0 == strcasecmp($1, "null")) {
          Value null_value;
          value_init_null(&null_value);
          $$ = expr_node_from_value(&null_value);
        } else {
          $$ = expr_node_from_attr(NULL, $1);
        }
        free($1);
      }
    | ID DOT ID {
        $$ = expr_node_from_attr($1, $3);
        free($1);
        free($3);
      }
    | LBRACE expr RBRACE {
        $$ = expr_node_paren($2);
      }
    | '-' expr %prec NEG {
        $$ = expr_node_unary(EXPR_OP_SUB, $2);
      }
    | expr '+' expr {
        $$ = expr_node_binary(EXPR_OP_ADD, $1, $3);
      }
    | expr '-' expr {
        $$ = expr_node_binary(EXPR_OP_SUB, $1, $3);
      }
    | expr STAR expr {
        $$ = expr_node_binary(EXPR_OP_MUL, $1, $3);
      }
    | expr '/' expr {
        $$ = expr_node_binary(EXPR_OP_DIV, $1, $3);
      }
    ;

delete:		/*  delete 语句的语法解析树*/
    DELETE FROM ID where SEMICOLON 
		{
			CONTEXT->ssql->flag = SCF_DELETE;//"delete";
			deletes_init_relation(&CONTEXT->ssql->sstr.deletion, $3);
			deletes_set_conditions(&CONTEXT->ssql->sstr.deletion, 
					CONTEXT->conditions, CONTEXT->condition_length);
			CONTEXT->condition_length = 0;	
    }
    ;
update:			/*  update 语句的语法解析树*/
    UPDATE ID SET ID EQ signed_value where SEMICOLON
		{
			CONTEXT->ssql->flag = SCF_UPDATE;//"update";
			Value *value = &CONTEXT->values[0];
			updates_init(&CONTEXT->ssql->sstr.update, $2, $4, value, 
					CONTEXT->conditions, CONTEXT->condition_length);
			CONTEXT->condition_length = 0;
		}
    ;
select:				/*  select 语句的语法解析树*/
    SELECT select_attr FROM from_list where group_clause order_clause SEMICOLON
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
	;

group_clause:
    /* empty */
    | DATA group_list
    ;

group_list:
    group_item
    | group_list COMMA group_item
    ;

group_item:
    ID {
		RelAttr attr;
		relation_attr_init(&attr, NULL, $1);
		selects_append_group_by(&CONTEXT->ssql->sstr.selection, &attr);
		free($1);
	}
    | ID DOT ID {
		RelAttr attr;
		relation_attr_init(&attr, $1, $3);
		selects_append_group_by(&CONTEXT->ssql->sstr.selection, &attr);
		free($1);
		free($3);
	}
    ;

order_clause:
    /* empty */
    | VALUES order_list
    ;

order_list:
    order_item
    | order_list COMMA order_item
    ;

order_item:
    ID {
		RelAttr attr;
		relation_attr_init(&attr, NULL, $1);
		selects_append_order_by(&CONTEXT->ssql->sstr.selection, &attr, 1);
		free($1);
	}
    | ID DESC {
		RelAttr attr;
		relation_attr_init(&attr, NULL, $1);
		selects_append_order_by(&CONTEXT->ssql->sstr.selection, &attr, 0);
		free($1);
	}
    | ID DOT ID DESC {
		RelAttr attr;
		relation_attr_init(&attr, $1, $3);
		selects_append_order_by(&CONTEXT->ssql->sstr.selection, &attr, 0);
		free($1);
		free($3);
	}
    | ID ID {
		int asc = 1;
		if (0 == strcasecmp($2, "desc")) {
			asc = 0;
		} else if (0 != strcasecmp($2, "asc")) {
			CONTEXT->ssql->flag = SCF_ERROR;
			yyerror(scanner, "expect ASC or DESC");
		}
		RelAttr attr;
		relation_attr_init(&attr, NULL, $1);
		selects_append_order_by(&CONTEXT->ssql->sstr.selection, &attr, asc);
		free($1);
		free($2);
	}
    | ID DOT ID {
		RelAttr attr;
		relation_attr_init(&attr, $1, $3);
		selects_append_order_by(&CONTEXT->ssql->sstr.selection, &attr, 1);
		free($1);
		free($3);
	}
    | ID DOT ID ID {
		int asc = 1;
		if (0 == strcasecmp($4, "desc")) {
			asc = 0;
		} else if (0 != strcasecmp($4, "asc")) {
			CONTEXT->ssql->flag = SCF_ERROR;
			yyerror(scanner, "expect ASC or DESC");
		}
		RelAttr attr;
		relation_attr_init(&attr, $1, $3);
		selects_append_order_by(&CONTEXT->ssql->sstr.selection, &attr, asc);
		free($1);
		free($3);
		free($4);
	}
    ;

agg_arg_tail:
    ID { free($1); }
    | agg_arg_tail COMMA ID { free($3); }
    ;

select_attr:
    select_item select_item_list
    ;

select_item_list:
    /* empty */
    | COMMA select_item select_item_list
    ;

select_item:
    STAR {
			RelAttr attr;
			relation_attr_init(&attr, NULL, "*");
			selects_append_attribute(&CONTEXT->ssql->sstr.selection, &attr);
		}
    | expr {
			RelAttr attr;
			if ($1 != NULL && $1->type == EXPR_ATTR_NODE) {
				relation_attr_init(&attr, $1->relation_name, $1->attribute_name);
			} else {
				relation_attr_init(&attr, NULL, "");
				attr.expr = $1;
			}
			selects_append_attribute(&CONTEXT->ssql->sstr.selection, &attr);
		}
	| ID LBRACE STAR RBRACE {
			int agg = agg_type_of_name($1);
			free($1);
			RelAttr attr;
			relation_attr_init(&attr, NULL, "*");
			attr.agg_type = agg;
			selects_append_attribute(&CONTEXT->ssql->sstr.selection, &attr);
		}
	| ID LBRACE RBRACE {
			int agg = agg_type_of_name($1);
			free($1);
			RelAttr attr;
			relation_attr_init(&attr, NULL, "");
			attr.agg_type = agg;
			selects_append_attribute(&CONTEXT->ssql->sstr.selection, &attr);
		}
	| ID LBRACE ID COMMA agg_arg_tail RBRACE {
			int agg = agg_type_of_name($1);
			free($1);
			RelAttr attr;
			relation_attr_init(&attr, NULL, "");
			attr.agg_type = agg == AGG_NONE ? AGG_COUNT : agg;
			selects_append_attribute(&CONTEXT->ssql->sstr.selection, &attr);
		}
	| ID LBRACE STAR COMMA agg_arg_tail RBRACE {
			int agg = agg_type_of_name($1);
			free($1);
			RelAttr attr;
			relation_attr_init(&attr, NULL, "");
			attr.agg_type = agg == AGG_NONE ? AGG_COUNT : agg;
			selects_append_attribute(&CONTEXT->ssql->sstr.selection, &attr);
		}
	| ID LBRACE ID RBRACE {
			int agg = agg_type_of_name($1);
			free($1);
			RelAttr attr;
			relation_attr_init(&attr, NULL, $3);
			attr.agg_type = agg;
			selects_append_attribute(&CONTEXT->ssql->sstr.selection, &attr);
			free($3);
		}
	| ID LBRACE ID DOT ID RBRACE {
			int agg = agg_type_of_name($1);
			free($1);
			RelAttr attr;
			relation_attr_init(&attr, $3, $5);
			attr.agg_type = agg;
			selects_append_attribute(&CONTEXT->ssql->sstr.selection, &attr);
			free($3);
			free($5);
		}
    ;

attr_list:
    /* empty */
    | COMMA ID attr_list {
			RelAttr attr;
			relation_attr_init(&attr, NULL, $2);
			selects_append_attribute(&CONTEXT->ssql->sstr.selection, &attr);
     	  // CONTEXT->ssql->sstr.selection.attributes[CONTEXT->select_length].relation_name = NULL;
        // CONTEXT->ssql->sstr.selection.attributes[CONTEXT->select_length++].attribute_name=$2;
      }
    | COMMA ID DOT ID attr_list {
			RelAttr attr;
			relation_attr_init(&attr, $2, $4);
			selects_append_attribute(&CONTEXT->ssql->sstr.selection, &attr);
        // CONTEXT->ssql->sstr.selection.attributes[CONTEXT->select_length].attribute_name=$4;
        // CONTEXT->ssql->sstr.selection.attributes[CONTEXT->select_length++].relation_name=$2;
  	  }
  	;

from_list:
    from_item
    | from_list COMMA from_item
    ;

from_item:
    ID {
        selects_append_relation(&CONTEXT->ssql->sstr.selection, $1);
      }
    | from_item ID ID ON condition condition_list {  /* inner join t on ... */
        selects_append_relation(&CONTEXT->ssql->sstr.selection, $<string>4);
      }
    | from_item ID ID ID ON condition condition_list {  /* inner/left/right join t on ... */
        selects_append_relation(&CONTEXT->ssql->sstr.selection, $<string>5);
      }
    ;
where:
    /* empty */ 
    | WHERE condition condition_list {	
				// CONTEXT->conditions[CONTEXT->condition_length++]=*$2;
			}
    ;
condition_list:
    /* empty */
    | AND condition condition_list {
				// CONTEXT->conditions[CONTEXT->condition_length++]=*$2;
			}
    ;
condition:
    expr comOp expr
		{
			Condition condition;
			memset(&condition, 0, sizeof(condition));
			condition.comp = CONTEXT->comp;
			condition.left_expr = $1;
			condition.right_expr = $3;
			CONTEXT->conditions[CONTEXT->condition_length++] = condition;
		}
    | expr ID ID
		{
			Condition condition;
			memset(&condition, 0, sizeof(condition));
			if (0 == strcasecmp($2, "is") && 0 == strcasecmp($3, "null")) {
				condition.comp = IS_NULL;
			} else if (0 == strcasecmp($2, "not") && 0 == strcasecmp($3, "null")) {
				condition.comp = IS_NOT_NULL;
			} else {
				CONTEXT->ssql->flag = SCF_ERROR;
				yyerror(scanner, "expect is null or is not null");
			}
			condition.left_expr = $1;
			condition.right_expr = NULL;
			CONTEXT->conditions[CONTEXT->condition_length++] = condition;
			free($2);
			free($3);
		}
    | expr ID ID ID
		{
			Condition condition;
			memset(&condition, 0, sizeof(condition));
			if (0 == strcasecmp($2, "is") && 0 == strcasecmp($3, "not") && 0 == strcasecmp($4, "null")) {
				condition.comp = IS_NOT_NULL;
			} else {
				CONTEXT->ssql->flag = SCF_ERROR;
				yyerror(scanner, "expect is not null");
			}
			condition.left_expr = $1;
			condition.right_expr = NULL;
			CONTEXT->conditions[CONTEXT->condition_length++] = condition;
			free($2);
			free($3);
			free($4);
		}
    ;

comOp:
  	  EQ { CONTEXT->comp = EQUAL_TO; }
    | LT { CONTEXT->comp = LESS_THAN; }
    | GT { CONTEXT->comp = GREAT_THAN; }
    | LE { CONTEXT->comp = LESS_EQUAL; }
    | GE { CONTEXT->comp = GREAT_EQUAL; }
    | NE { CONTEXT->comp = NOT_EQUAL; }
    ;

load_data:
		LOAD DATA INFILE SSS INTO TABLE ID SEMICOLON
		{
		  CONTEXT->ssql->flag = SCF_LOAD_DATA;
			load_data_init(&CONTEXT->ssql->sstr.load_data, $7, $4);
		}
		;
%%
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
