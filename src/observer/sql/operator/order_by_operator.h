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

#include <vector>

#include "sql/operator/operator.h"
#include "storage/common/record.h"
#include "storage/common/table.h"
#include "sql/expr/tuple.h"

class OrderByOperator : public Operator
{
public:
  OrderByOperator() = default;
  virtual ~OrderByOperator();

  void add_table(const Table *table) { tables_.push_back(table); }
  void add_order_field(const Table *table, const FieldMeta *field, bool asc)
  {
    order_fields_.push_back(OrderField{table, field, asc});
  }

  RC open() override;
  RC next() override;
  RC close() override;

  Tuple *current_tuple() override;

private:
  struct OrderField
  {
    const Table *table = nullptr;
    const FieldMeta *field = nullptr;
    bool asc = true;
  };

  struct Row
  {
    std::vector<Record> records;   // copied records, one per table
    std::vector<char *> buffers;   // owned record data buffers
  };

  int compare_rows(const Row &left, const Row &right) const;
  void collect_tuples(const Tuple *tuple, std::vector<const RowTuple *> &out) const;

  std::vector<const Table *> tables_;
  std::vector<OrderField> order_fields_;
  std::vector<Row> rows_;
  size_t current_index_ = 0;
  bool inited_ = false;

  std::vector<RowTuple> row_tuples_;
  JoinedTuple joined_tuple_;
  std::vector<const Tuple *> tuple_ptrs_;
};
