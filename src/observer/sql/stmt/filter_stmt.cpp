/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2022/5/22.
//

#include "rc.h"
#include "common/log/log.h"
#include "common/lang/string.h"
#include "sql/stmt/filter_stmt.h"
#include "sql/expr/expression_parser.h"
#include "common/date_utils.h"
#include "storage/common/db.h"
#include "storage/common/table.h"

FilterStmt::~FilterStmt()
{
  for (FilterUnit *unit : filter_units_) {
    delete unit;
  }
  filter_units_.clear();
}

RC FilterStmt::create(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
		      const Condition *conditions, int condition_num,
		      FilterStmt *&stmt)
{
  RC rc = RC::SUCCESS;
  stmt = nullptr;

  FilterStmt *tmp_stmt = new FilterStmt();
  for (int i = 0; i < condition_num; i++) {
    FilterUnit *filter_unit = nullptr;
    rc = create_filter_unit(db, default_table, tables, conditions[i], filter_unit);
    if (rc != RC::SUCCESS) {
      delete tmp_stmt;
      LOG_WARN("failed to create filter unit. condition index=%d", i);
      return rc;
    }
    tmp_stmt->filter_units_.push_back(filter_unit);
  }

  stmt = tmp_stmt;
  return rc;
}

RC get_table_and_field(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
		       const RelAttr &attr, Table *&table, const FieldMeta *&field)
{
  if (common::is_blank(attr.relation_name)) {
    table = default_table;
  } else if (nullptr != tables) {
    auto iter = tables->find(std::string(attr.relation_name));
    if (iter != tables->end()) {
      table = iter->second;
    }
  } else {
    table = db->find_table(attr.relation_name);
  }
  if (nullptr == table) {
    LOG_WARN("No such table: attr.relation_name: %s", attr.relation_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  field = table->table_meta().field(attr.attribute_name);
  if (nullptr == field) {
    LOG_WARN("no such field in table: table %s, field %s", table->name(), attr.attribute_name);
    table = nullptr;
    return RC::SCHEMA_FIELD_NOT_EXIST;
  }

  return RC::SUCCESS;
}

// 日期字段与字符串字面量比较时，需要把字面量按日历解析成内部日期值
static RC convert_date_literal(const Expression *field_side, Expression *&value_side)
{
  if (field_side->type() != ExprType::FIELD || value_side->type() != ExprType::VALUE) {
    return RC::SUCCESS;
  }
  const FieldExpr *field_expr = static_cast<const FieldExpr *>(field_side);
  if (field_expr->field().meta()->type() != DATES) {
    return RC::SUCCESS;
  }
  ValueExpr *value_expr = static_cast<ValueExpr *>(value_side);
  TupleCell cell;
  value_expr->get_tuple_cell(cell);
  if (cell.attr_type() != CHARS) {
    return RC::SUCCESS;
  }

  int date_value = 0;
  if (!parse_date(cell.data(), date_value)) {
    LOG_WARN("invalid date literal: %s", cell.data());
    return RC::INVALID_ARGUMENT;
  }
  delete value_expr;
  Value value;
  value.type = DATES;
  value.data = &date_value;
  value_side = new ValueExpr(value);
  return RC::SUCCESS;
}

RC FilterStmt::create_filter_unit(Db *db, Table *default_table, std::unordered_map<std::string, Table *> *tables,
				  const Condition &condition, FilterUnit *&filter_unit)
{
  RC rc = RC::SUCCESS;

  CompOp comp = condition.comp;
  if (comp < EQUAL_TO || comp >= NO_OP) {
    LOG_WARN("invalid compare operator : %d", comp);
    return RC::INVALID_ARGUMENT;
  }

  // 表达式条件（算术表达式/括号/多表表达式）走通用的表达式求值路径
  if (condition.left_expr != nullptr && condition.right_expr != nullptr) {
    Expression *left = nullptr;
    Expression *right = nullptr;
    rc = build_expression(db, default_table, tables, condition.left_expr, left);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to build left expression");
      return rc;
    }
    rc = build_expression(db, default_table, tables, condition.right_expr, right);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to build right expression");
      delete left;
      return rc;
    }

    rc = convert_date_literal(left, right);
    if (rc != RC::SUCCESS) {
      delete left;
      delete right;
      return rc;
    }
    rc = convert_date_literal(right, left);
    if (rc != RC::SUCCESS) {
      delete left;
      delete right;
      return rc;
    }

    filter_unit = new FilterUnit;
    filter_unit->set_comp(comp);
    filter_unit->set_left(left);
    filter_unit->set_right(right);
    return RC::SUCCESS;
  }

  Value left_value = condition.left_value;
  Value right_value = condition.right_value;
  int left_date = 0;
  int right_date = 0;

  Expression *left = nullptr;
  Expression *right = nullptr;
  if (condition.left_is_attr) {
    Table *table = nullptr;
    const FieldMeta *field = nullptr;
    rc = get_table_and_field(db, default_table, tables, condition.left_attr, table, field);  
    if (rc != RC::SUCCESS) {
      LOG_WARN("cannot find attr");
      return rc;
    }
    // 日期字段与字符串字面量比较时，先把字面量校验并转换成内部日期值
    if (field->type() == DATES && !condition.right_is_attr && right_value.type == CHARS) {
      if (!parse_date(static_cast<const char *>(right_value.data), right_date)) {
        LOG_WARN("invalid date literal. field=%s", field->name());
        return RC::INVALID_ARGUMENT;
      }
      right_value.type = DATES;
      right_value.data = &right_date;
    }
    left = new FieldExpr(table, field);
  } else {
    left = new ValueExpr(left_value);
  }

  if (condition.right_is_attr) {
    Table *table = nullptr;
    const FieldMeta *field = nullptr;
    rc = get_table_and_field(db, default_table, tables, condition.right_attr, table, field);  
    if (rc != RC::SUCCESS) {
      LOG_WARN("cannot find attr");
      delete left;
      return rc;
    }
    if (field->type() == DATES && !condition.left_is_attr && left_value.type == CHARS) {
      if (!parse_date(static_cast<const char *>(left_value.data), left_date)) {
        LOG_WARN("invalid date literal. field=%s", field->name());
        delete left;
        return RC::INVALID_ARGUMENT;
      }
      left_value.type = DATES;
      left_value.data = &left_date;
      delete left;
      left = new ValueExpr(left_value);
    }
    right = new FieldExpr(table, field);
  } else {
    right = new ValueExpr(right_value);
  }

  filter_unit = new FilterUnit;
  filter_unit->set_comp(comp);
  filter_unit->set_left(left);
  filter_unit->set_right(right);

  // 检查两个类型是否能够比较
  return rc;
}
