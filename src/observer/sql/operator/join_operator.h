/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "sql/operator/operator.h"
#include <vector>

#include "sql/expr/tuple.h"

class FilterStmt;
class FilterUnit;
class Table;

/**
 * 简单的嵌套循环连接(nested loop join)。
 * 左孩子作为外层循环，右孩子作为内层循环，两者都必须能重复打开。
 */
class NestedLoopJoinOperator : public Operator
{
public:
  NestedLoopJoinOperator() = default;
  virtual ~NestedLoopJoinOperator() = default;

  RC open() override;
  RC next() override;
  RC close() override;

  Tuple *current_tuple() override;

  // 把 where 条件传进来，能在当前已连接的这些表上判断的条件就尽早过滤掉，
  // 否则多表连接会退化成巨大的笛卡尔积
  void set_filter(FilterStmt *filter_stmt) { filter_stmt_ = filter_stmt; }
  void set_left_tables(const std::vector<const Table *> &tables) { left_tables_ = tables; }
  void set_right_table(const Table *table) { right_table_ = table; }

private:
  bool filter_applicable(const FilterUnit *filter_unit) const;
  bool left_valid_ = false;
  bool right_opened_ = false;
  const Tuple *left_tuple_ = nullptr;
  const Tuple *right_tuple_ = nullptr;
  JoinedTuple joined_tuple_;
  FilterStmt *filter_stmt_ = nullptr;
  std::vector<const Table *> left_tables_;
  const Table *right_table_ = nullptr;
};
