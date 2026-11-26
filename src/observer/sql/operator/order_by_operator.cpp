/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include <algorithm>
#include <cstring>

#include "common/log/log.h"
#include "sql/operator/order_by_operator.h"
#include "sql/expr/tuple.h"

OrderByOperator::~OrderByOperator()
{
  for (Row &row : rows_) {
    for (char *buffer : row.buffers) {
      delete[] buffer;
    }
  }
  rows_.clear();
}

void OrderByOperator::collect_tuples(const Tuple *tuple, std::vector<const RowTuple *> &out) const
{
  const JoinedTuple *joined = dynamic_cast<const JoinedTuple *>(const_cast<Tuple *>(tuple));
  if (joined != nullptr) {
    for (const Tuple *sub : joined->tuples()) {
      collect_tuples(sub, out);
    }
    return;
  }
  out.push_back(static_cast<const RowTuple *>(tuple));
}

int OrderByOperator::compare_rows(const Row &left, const Row &right) const
{
  for (const OrderField &order_field : order_fields_) {
    int table_index = -1;
    for (size_t i = 0; i < tables_.size(); i++) {
      if (tables_[i] == order_field.table) {
        table_index = static_cast<int>(i);
        break;
      }
    }
    if (table_index < 0) {
      continue;
    }

    TupleCell left_cell(order_field.field->type(),
        const_cast<char *>(left.records[table_index].data()) + order_field.field->offset());
    TupleCell right_cell(order_field.field->type(),
        const_cast<char *>(right.records[table_index].data()) + order_field.field->offset());

    int result = left_cell.compare(right_cell);
    if (result != 0) {
      return order_field.asc ? result : -result;
    }
  }
  return 0;
}

RC OrderByOperator::open()
{
  if (children_.size() != 1) {
    LOG_WARN("order by operator must has 1 child");
    return RC::INTERNAL;
  }

  Operator *child = children_[0];
  RC rc = child->open();
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  while (RC::SUCCESS == (rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (nullptr == tuple) {
      LOG_WARN("failed to get current tuple");
      return RC::INTERNAL;
    }

    std::vector<const RowTuple *> row_tuple_list;
    collect_tuples(tuple, row_tuple_list);
    if (row_tuple_list.size() != tables_.size()) {
      LOG_WARN("row tuple number mismatch. expected=%d, got=%d",
          static_cast<int>(tables_.size()), static_cast<int>(row_tuple_list.size()));
      return RC::INTERNAL;
    }

    Row row;
    for (size_t i = 0; i < tables_.size(); i++) {
      const Record &record = row_tuple_list[i]->record();
      const int record_size = tables_[i]->table_meta().record_size();
      char *buffer = new char[record_size];
      memcpy(buffer, record.data(), record_size);

      Record copy;
      copy.set_data(buffer);
      copy.set_rid(record.rid());
      row.records.push_back(copy);
      row.buffers.push_back(buffer);
    }
    rows_.push_back(row);
  }

  if (rc != RC::RECORD_EOF) {
    LOG_WARN("failed to scan all records. rc=%s", strrc(rc));
    return rc;
  }

  std::stable_sort(rows_.begin(), rows_.end(),
      [this](const Row &left, const Row &right) { return compare_rows(left, right) < 0; });

  row_tuples_.resize(tables_.size());
  for (size_t i = 0; i < tables_.size(); i++) {
    row_tuples_[i].set_schema(tables_[i], tables_[i]->table_meta().field_metas());
  }

  current_index_ = 0;
  inited_ = true;
  return RC::SUCCESS;
}

RC OrderByOperator::next()
{
  if (!inited_) {
    return RC::INTERNAL;
  }
  if (current_index_ >= rows_.size()) {
    return RC::RECORD_EOF;
  }

  Row &row = rows_[current_index_];
  for (size_t i = 0; i < row.records.size(); i++) {
    row_tuples_[i].set_record(&row.records[i]);
  }

  tuple_ptrs_.clear();
  for (size_t i = 0; i < row_tuples_.size(); i++) {
    tuple_ptrs_.push_back(&row_tuples_[i]);
  }
  joined_tuple_.set_tuples(tuple_ptrs_);

  current_index_++;
  return RC::SUCCESS;
}

RC OrderByOperator::close()
{
  for (Row &row : rows_) {
    for (char *buffer : row.buffers) {
      delete[] buffer;
    }
  }
  rows_.clear();
  inited_ = false;
  return children_[0]->close();
}

Tuple *OrderByOperator::current_tuple()
{
  if (row_tuples_.size() == 1) {
    return &row_tuples_[0];
  }
  return &joined_tuple_;
}
