/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include <cstdio>
#include <cstring>
#include <string>

#include "sql/expr/expression_parser.h"
#include "sql/expr/expression.h"
#include "common/log/log.h"
#include "common/lang/string.h"
#include "storage/common/db.h"
#include "storage/common/table.h"

// 未限定表名的字段可能来自 from 列表里任意一张表
static RC find_field(Table *default_table, std::unordered_map<std::string, Table *> *tables,
    const char *relation_name, const char *attribute_name, Table *&table, const FieldMeta *&field)
{
  table = nullptr;
  field = nullptr;
  if (!common::is_blank(relation_name)) {
    if (tables != nullptr) {
      auto iter = tables->find(std::string(relation_name));
      if (iter != tables->end()) {
        table = iter->second;
      }
    }
    if (table == nullptr) {
      LOG_WARN("no such table: %s", relation_name);
      return RC::SCHEMA_TABLE_NOT_EXIST;
    }
    field = table->table_meta().field(attribute_name);
    if (field == nullptr) {
      LOG_WARN("no such field: %s.%s", relation_name, attribute_name);
      table = nullptr;
      return RC::SCHEMA_FIELD_NOT_EXIST;
    }
    return RC::SUCCESS;
  }

  if (default_table != nullptr) {
    field = default_table->table_meta().field(attribute_name);
    if (field != nullptr) {
      table = default_table;
      return RC::SUCCESS;
    }
    LOG_WARN("no such field: %s", attribute_name);
    return RC::SCHEMA_FIELD_NOT_EXIST;
  }

  if (tables != nullptr) {
    for (auto &entry : *tables) {
      Table *candidate = entry.second;
      const FieldMeta *candidate_field = candidate->table_meta().field(attribute_name);
      if (candidate_field != nullptr) {
        if (table != nullptr) {
          LOG_WARN("ambiguous field: %s", attribute_name);
          return RC::SCHEMA_FIELD_MISSING;
        }
        table = candidate;
        field = candidate_field;
      }
    }
  }
  if (table == nullptr) {
    LOG_WARN("no such field: %s", attribute_name);
    return RC::SCHEMA_FIELD_NOT_EXIST;
  }
  return RC::SUCCESS;
}

RC build_expression(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
    const ExpressionNode *node, Expression *&expr)
{
  expr = nullptr;
  if (node == nullptr) {
    return RC::INVALID_ARGUMENT;
  }

  switch (node->type) {
  case EXPR_VALUE_NODE: {
    expr = new ValueExpr(node->value);
    return RC::SUCCESS;
  }
  case EXPR_ATTR_NODE: {
    Table *table = nullptr;
    const FieldMeta *field = nullptr;
    RC rc = find_field(default_table, tables, node->relation_name, node->attribute_name, table, field);
    if (rc != RC::SUCCESS) {
      return rc;
    }
    expr = new FieldExpr(table, field);
    return RC::SUCCESS;
  }
  case EXPR_PAREN_NODE: {
    return build_expression(db, default_table, tables, node->left, expr);
  }
  case EXPR_UNARY_NODE: {
    Expression *operand = nullptr;
    RC rc = build_expression(db, default_table, tables, node->left, operand);
    if (rc != RC::SUCCESS) {
      return rc;
    }
    expr = new UnaryExpr(operand);
    return RC::SUCCESS;
  }
  case EXPR_BINARY_NODE: {
    Expression *left = nullptr;
    Expression *right = nullptr;
    RC rc = build_expression(db, default_table, tables, node->left, left);
    if (rc != RC::SUCCESS) {
      return rc;
    }
    rc = build_expression(db, default_table, tables, node->right, right);
    if (rc != RC::SUCCESS) {
      delete left;
      return rc;
    }
    expr = new ArithmeticExpr(left, right, node->op);
    return RC::SUCCESS;
  }
  }
  return RC::INTERNAL;
}

static void append_upper(std::string &out, const char *text)
{
  if (text == nullptr) {
    return;
  }
  for (const char *p = text; *p != '\0'; p++) {
    out.push_back(static_cast<char>(*p >= 'a' && *p <= 'z' ? *p - 'a' + 'A' : *p));
  }
}

static void append_node_text(std::string &out, const ExpressionNode *node)
{
  if (node == nullptr) {
    return;
  }
  switch (node->type) {
  case EXPR_VALUE_NODE: {
    char buf[64] = {0};
    switch (node->value.type) {
    case INTS: snprintf(buf, sizeof(buf), "%d", *static_cast<int *>(node->value.data)); break;
    case FLOATS: snprintf(buf, sizeof(buf), "%g", *static_cast<float *>(node->value.data)); break;
    case DATES: snprintf(buf, sizeof(buf), "%d", *static_cast<int *>(node->value.data)); break;
    default: snprintf(buf, sizeof(buf), "%s", static_cast<const char *>(node->value.data)); break;
    }
    out.append(buf);
  } break;
  case EXPR_ATTR_NODE: {
    if (!common::is_blank(node->relation_name)) {
      append_upper(out, node->relation_name);
      out.push_back('.');
    }
    append_upper(out, node->attribute_name);
  } break;
  case EXPR_PAREN_NODE: {
    out.push_back('(');
    append_node_text(out, node->left);
    out.push_back(')');
  } break;
  case EXPR_UNARY_NODE: {
    out.push_back('-');
    append_node_text(out, node->left);
  } break;
  case EXPR_BINARY_NODE: {
    append_node_text(out, node->left);
    switch (node->op) {
    case EXPR_OP_ADD: out.push_back('+'); break;
    case EXPR_OP_SUB: out.push_back('-'); break;
    case EXPR_OP_MUL: out.push_back('*'); break;
    case EXPR_OP_DIV: out.push_back('/'); break;
    }
    append_node_text(out, node->right);
  } break;
  }
}

std::string expression_node_to_string(const ExpressionNode *node)
{
  std::string out;
  append_node_text(out, node);
  return out;
}
