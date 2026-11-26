/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "common/log/log.h"
#include "sql/operator/join_operator.h"
#include "sql/stmt/filter_stmt.h"
#include "storage/common/field.h"
#include "storage/common/table.h"

bool NestedLoopJoinOperator::filter_applicable(const FilterUnit *filter_unit) const
{
  auto in_joined = [this](const Table *table) {
    {
      for (const Table *t : left_tables_) {
        if (t == table) {
          return true;
        }
      }
    }
    return table == right_table_;
  };

  auto expr_ok = [&](Expression *expr) {
    if (expr == nullptr || expr->type() != ExprType::FIELD) {
      return true;
    }
    const FieldExpr *field_expr = static_cast<const FieldExpr *>(expr);
    return in_joined(field_expr->field().table());
  };

  return expr_ok(filter_unit->left()) && expr_ok(filter_unit->right());
}

RC NestedLoopJoinOperator::open()
{
  if (children_.size() != 2) {
    LOG_WARN("join operator must have 2 children");
    return RC::INTERNAL;
  }

  RC rc = children_[0]->open();
  if (rc != RC::SUCCESS) {
    return rc;
  }
  left_valid_ = false;
  right_opened_ = false;
  return RC::SUCCESS;
}

RC NestedLoopJoinOperator::next()
{
  Operator *left = children_[0];
  Operator *right = children_[1];

  while (true) {
    if (!left_valid_) {
      RC rc = left->next();
      if (rc != RC::SUCCESS) {
        return rc;
      }
      left_tuple_ = left->current_tuple();
      if (left_tuple_ == nullptr) {
        return RC::INTERNAL;
      }
      left_valid_ = true;
      if (right_opened_) {
        right->close();
        right_opened_ = false;
      }
    }

    if (!right_opened_) {
      RC rc = right->open();
      if (rc != RC::SUCCESS) {
        return rc;
      }
      right_opened_ = true;
    }

    RC rc = right->next();
    if (rc == RC::RECORD_EOF) {
      left_valid_ = false;
      right->close();
      right_opened_ = false;
      continue;
    }
    if (rc != RC::SUCCESS) {
      return rc;
    }

    right_tuple_ = right->current_tuple();
    if (right_tuple_ == nullptr) {
      return RC::INTERNAL;
    }

    std::vector<const Tuple *> tuples{left_tuple_, right_tuple_};
    joined_tuple_.set_tuples(tuples);

    if (filter_stmt_ != nullptr) {
      bool matched = true;
      for (const FilterUnit *filter_unit : filter_stmt_->filter_units()) {
        if (!filter_applicable(filter_unit)) {
          continue;
        }
        TupleCell left_cell;
        TupleCell right_cell;
        filter_unit->left()->get_value(joined_tuple_, left_cell);
        filter_unit->right()->get_value(joined_tuple_, right_cell);
        const int compare = left_cell.compare(right_cell);
        bool result = false;
        switch (filter_unit->comp()) {
          case EQUAL_TO:    result = compare == 0; break;
          case LESS_EQUAL:  result = compare <= 0; break;
          case NOT_EQUAL:   result = compare != 0; break;
          case LESS_THAN:   result = compare < 0;  break;
          case GREAT_EQUAL: result = compare >= 0; break;
          case GREAT_THAN:  result = compare > 0;  break;
          default: result = false; break;
        }
        if (!result) {
          matched = false;
          break;
        }
      }
      if (!matched) {
        continue;
      }
    }

    return RC::SUCCESS;
  }
}

RC NestedLoopJoinOperator::close()
{
  if (right_opened_) {
    children_[1]->close();
    right_opened_ = false;
  }
  children_[0]->close();
  left_valid_ = false;
  return RC::SUCCESS;
}

Tuple *NestedLoopJoinOperator::current_tuple()
{
  return &joined_tuple_;
}
