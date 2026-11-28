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

#include "sql/stmt/insert_stmt.h"
#include "common/date_utils.h"
#include "common/log/log.h"
#include "storage/common/db.h"
#include "storage/common/table.h"

InsertStmt::InsertStmt(Table *table, const Value *values, int value_amount, size_t row_num, const size_t *row_value_num)
  : table_(table), values_(values), value_amount_(value_amount), row_num_(row_num)
{
  for (size_t i = 0; i < row_num_ && i < MAX_NUM; i++) {
    row_value_num_[i] = row_value_num[i];
  }
}

RC InsertStmt::create(Db *db, const Inserts &inserts, Stmt *&stmt)
{
  const char *table_name = inserts.relation_name;
  if (nullptr == db || nullptr == table_name || inserts.value_num <= 0) {
    LOG_WARN("invalid argument. db=%p, table_name=%p, value_num=%d", 
             db, table_name, inserts.value_num);
    return RC::INVALID_ARGUMENT;
  }

  // check whether the table exists
  Table *table = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  // check the fields number and type of every value tuple
  const Value *values = inserts.values;
  const size_t value_num = inserts.value_num;
  const size_t row_num = inserts.row_num == 0 ? 1 : inserts.row_num;
  const TableMeta &table_meta = table->table_meta();
  const int field_num = table_meta.field_num() - table_meta.sys_field_num();
  const int sys_field_num = table_meta.sys_field_num();

  size_t value_offset = 0;
  for (size_t row = 0; row < row_num; row++) {
    const size_t row_value_num = inserts.row_value_num[row];
    if (row_value_num != static_cast<size_t>(field_num)) {
      LOG_WARN("schema mismatch. value num=%d, field num in schema=%d", static_cast<int>(row_value_num), field_num);
      return RC::SCHEMA_FIELD_MISSING;
    }
    if (value_offset + row_value_num > value_num) {
      LOG_WARN("invalid value list. row=%d, offset=%d, value_num=%d",
          static_cast<int>(row), static_cast<int>(value_offset), static_cast<int>(value_num));
      return RC::INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < row_value_num; i++) {
      const FieldMeta *field_meta = table_meta.field(static_cast<int>(i) + sys_field_num);
      const AttrType field_type = field_meta->type();
      const AttrType value_type = values[value_offset + i].type;
      if (values[value_offset + i].is_null) {
        if (!field_meta->nullable()) {
          LOG_WARN("field is not null. table=%s, field=%s", table_name, field_meta->name());
          return RC::SCHEMA_FIELD_TYPE_MISMATCH;
        }
        continue;
      }
      if (field_type == DATES && value_type == CHARS) {
        int date_value = 0;
        if (!parse_date(static_cast<const char *>(values[value_offset + i].data), date_value)) {
          LOG_WARN("invalid date value. table=%s, field=%s", table_name, field_meta->name());
          return RC::INVALID_ARGUMENT;
        }
        continue;
      }
      if (field_type != value_type) { // TODO try to convert the value type to field type
        LOG_WARN("field type mismatch. table=%s, field=%s, field type=%d, value_type=%d",
                 table_name, field_meta->name(), field_type, value_type);
        return RC::SCHEMA_FIELD_TYPE_MISMATCH;
      }
    }
    value_offset += row_value_num;
  }

  if (value_offset != value_num) {
    LOG_WARN("invalid value list. offset=%d, value_num=%d", static_cast<int>(value_offset), static_cast<int>(value_num));
    return RC::INVALID_ARGUMENT;
  }

  // convert date literals into the internal integer representation
  Value *converted_values = nullptr;
  bool has_date = false;
  value_offset = 0;
  for (size_t row = 0; row < row_num; row++) {
    for (size_t i = 0; i < inserts.row_value_num[row]; i++) {
      const FieldMeta *field_meta = table_meta.field(static_cast<int>(i) + sys_field_num);
      if (field_meta->type() == DATES && values[value_offset + i].type == CHARS) {
        has_date = true;
      }
    }
    value_offset += inserts.row_value_num[row];
  }
  if (has_date) {
    converted_values = new Value[value_num];
    for (size_t i = 0; i < value_num; i++) {
      converted_values[i] = values[i];
    }
    value_offset = 0;
    for (size_t row = 0; row < row_num; row++) {
      for (size_t i = 0; i < inserts.row_value_num[row]; i++) {
        const FieldMeta *field_meta = table_meta.field(static_cast<int>(i) + sys_field_num);
        if (field_meta->type() == DATES && values[value_offset + i].type == CHARS) {
          int *date_value = new int(0);
          if (!parse_date(static_cast<const char *>(values[value_offset + i].data), *date_value)) {
            delete date_value;
            delete[] converted_values;
            return RC::INVALID_ARGUMENT;
          }
          converted_values[value_offset + i].type = DATES;
          converted_values[value_offset + i].data = date_value;
        }
      }
      value_offset += inserts.row_value_num[row];
    }
    values = converted_values;
  }

  // everything alright
  stmt = new InsertStmt(table, values, static_cast<int>(value_num), row_num, inserts.row_value_num);
  return RC::SUCCESS;
}
