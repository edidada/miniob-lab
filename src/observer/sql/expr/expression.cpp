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
// Created by Wangyunlai on 2022/07/05.
//

#include "common/log/log.h"
#include "sql/expr/tuple.h"
#include "sql/expr/expression.h"


RC FieldExpr::get_value(const Tuple &tuple, TupleCell &cell) const
{
  return tuple.find_cell(field_, cell);
}

RC ValueExpr::get_value(const Tuple &tuple, TupleCell & cell) const
{
  cell = tuple_cell_;
  return RC::SUCCESS;
}

static bool numeric_value(const TupleCell &cell, double &value)
{
  switch (cell.attr_type()) {
  case INTS: value = *reinterpret_cast<const int *>(cell.data()); return true;
  case FLOATS: value = *reinterpret_cast<const float *>(cell.data()); return true;
  default: return false;
  }
}

RC ArithmeticExpr::get_value(const Tuple &tuple, TupleCell &cell) const
{
  TupleCell left_cell;
  TupleCell right_cell;
  RC rc = left_->get_value(tuple, left_cell);
  if (rc != RC::SUCCESS) {
    return rc;
  }
  rc = right_->get_value(tuple, right_cell);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  double left_value = 0;
  double right_value = 0;
  if (!numeric_value(left_cell, left_value) || !numeric_value(right_cell, right_value)) {
    LOG_WARN("arithmetic on non numeric value: %d, %d", left_cell.attr_type(), right_cell.attr_type());
    return RC::INVALID_ARGUMENT;
  }

  const bool all_int = (left_cell.attr_type() == INTS && right_cell.attr_type() == INTS);
  if (op_ == EXPR_OP_DIV) {
    if (right_value == 0) {
      LOG_WARN("division by zero");
      return RC::INVALID_ARGUMENT;
    }
    // 除法一律使用浮点，避免 5/4 被截断成 1
    float_result_ = static_cast<float>(left_value / right_value);
    cell.set_type(FLOATS);
    cell.set_data(reinterpret_cast<char *>(&float_result_));
    return RC::SUCCESS;
  }

  if (all_int) {
    const long long l = static_cast<long long>(left_value);
    const long long r = static_cast<long long>(right_value);
    switch (op_) {
      case EXPR_OP_ADD: int_result_ = static_cast<int>(l + r); break;
      case EXPR_OP_SUB: int_result_ = static_cast<int>(l - r); break;
      case EXPR_OP_MUL: int_result_ = static_cast<int>(l * r); break;
      default: return RC::INTERNAL;
    }
    cell.set_type(INTS);
    cell.set_data(reinterpret_cast<char *>(&int_result_));
    return RC::SUCCESS;
  }

  switch (op_) {
    case EXPR_OP_ADD: float_result_ = static_cast<float>(left_value + right_value); break;
    case EXPR_OP_SUB: float_result_ = static_cast<float>(left_value - right_value); break;
    case EXPR_OP_MUL: float_result_ = static_cast<float>(left_value * right_value); break;
    default: return RC::INTERNAL;
  }
  cell.set_type(FLOATS);
  cell.set_data(reinterpret_cast<char *>(&float_result_));
  return RC::SUCCESS;
}

RC UnaryExpr::get_value(const Tuple &tuple, TupleCell &cell) const
{
  TupleCell operand_cell;
  RC rc = operand_->get_value(tuple, operand_cell);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  switch (operand_cell.attr_type()) {
  case INTS: {
    int_result_ = -*reinterpret_cast<const int *>(operand_cell.data());
    cell.set_type(INTS);
    cell.set_data(reinterpret_cast<char *>(&int_result_));
  } break;
  case FLOATS: {
    float_result_ = -*reinterpret_cast<const float *>(operand_cell.data());
    cell.set_type(FLOATS);
    cell.set_data(reinterpret_cast<char *>(&float_result_));
  } break;
  default: {
    LOG_WARN("unary minus on non numeric value: %d", operand_cell.attr_type());
    return RC::INVALID_ARGUMENT;
  }
  }
  return RC::SUCCESS;
}
