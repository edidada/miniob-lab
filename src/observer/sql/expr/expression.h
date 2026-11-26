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

#pragma once

#include <cstring>

#include "storage/common/field.h"
#include "sql/expr/tuple_cell.h"

class Tuple;

enum class ExprType {
  NONE,
  FIELD,
  VALUE,
};

class Expression
{
public: 
  Expression() = default;
  virtual ~Expression() = default;
  
  virtual RC get_value(const Tuple &tuple, TupleCell &cell) const = 0;
  virtual ExprType type() const = 0;
};

class FieldExpr : public Expression
{
public:
  FieldExpr() = default;
  FieldExpr(const Table *table, const FieldMeta *field) : field_(table, field)
  {}

  virtual ~FieldExpr() = default;

  ExprType type() const override
  {
    return ExprType::FIELD;
  }

  Field &field()
  {
    return field_;
  }

  const Field &field() const
  {
    return field_;
  }

  const char *table_name() const
  {
    return field_.table_name();
  }

  const char *field_name() const
  {
    return field_.field_name();
  }

  RC get_value(const Tuple &tuple, TupleCell &cell) const override;
private:
  Field field_;
};

class ValueExpr : public Expression
{
public:
  ValueExpr() = default;
  ValueExpr(const Value &value)
  {
    const char *src = static_cast<const char *>(value.data);
    switch (value.type) {
      case INTS: {
        int_value_ = *reinterpret_cast<const int *>(src);
        tuple_cell_.set_type(INTS);
        tuple_cell_.set_data(reinterpret_cast<char *>(&int_value_));
      } break;
      case FLOATS: {
        float_value_ = *reinterpret_cast<const float *>(src);
        tuple_cell_.set_type(FLOATS);
        tuple_cell_.set_data(reinterpret_cast<char *>(&float_value_));
      } break;
      case CHARS: {
        // the max length of CHARS in this version of miniob is 4 bytes, but the literal
        // may be shorter, so only copy the bytes that exist and zero the rest
        memset(char_value_, 0, sizeof(char_value_));
        if (src != nullptr) {
          const size_t src_len = strlen(src);
          memcpy(char_value_, src, src_len < sizeof(char_value_) ? src_len : sizeof(char_value_));
        }
        tuple_cell_.set_type(CHARS);
        tuple_cell_.set_data(char_value_);
      } break;
      default: {
        tuple_cell_.set_type(value.type);
        tuple_cell_.set_data(const_cast<char *>(src));
      } break;
    }
  }

  virtual ~ValueExpr() = default;

  RC get_value(const Tuple &tuple, TupleCell & cell) const override;
  ExprType type() const override
  {
    return ExprType::VALUE;
  }

  void get_tuple_cell(TupleCell &cell) const {
    cell = tuple_cell_;
  }

private:
  // the parse tree (and the Value it owns) may be gone by the time the plan is executed,
  // so the literal has to be copied into storage owned by the expression
  int int_value_ = 0;
  float float_value_ = 0;
  char char_value_[4] = {0, 0, 0, 0};
  TupleCell tuple_cell_;
};
