/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include <string>
#include <unordered_map>

#include "rc.h"
#include "sql/parser/parse_defs.h"

class Db;
class Table;
class Expression;

// 把语法分析阶段构造的表达式树翻译成可以执行的 Expression。
// default_table 用于单表查询下不带表名前缀的字段解析；
// tables 非空时按名字查找表，未限定表名的字段会在所有表里找唯一匹配。
RC build_expression(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
    const ExpressionNode *node, Expression *&expr);

// 把表达式树还原成表头文本，例如 3*col1/(col2+2) -> 3*COL1/(COL2+2)
std::string expression_node_to_string(const ExpressionNode *node);
