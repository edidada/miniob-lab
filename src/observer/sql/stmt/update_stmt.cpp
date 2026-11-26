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

#include "sql/stmt/update_stmt.h"
#include "common/date_utils.h"
#include "sql/stmt/filter_stmt.h"
#include "common/log/log.h"
#include "common/lang/string.h"
#include "storage/common/db.h"
#include "storage/common/table.h"

UpdateStmt::UpdateStmt(Table *table, const char *field_name, const Value &value, FilterStmt *filter_stmt)
  : table_(table), field_name_(field_name), filter_stmt_(filter_stmt)
{
  value_ = value;
}

UpdateStmt::~UpdateStmt()
{
  if (filter_stmt_ != nullptr) {
    delete filter_stmt_;
    filter_stmt_ = nullptr;
  }
}

RC UpdateStmt::create(Db *db, const Updates &update, Stmt *&stmt)
{
  stmt = nullptr;

  const char *table_name = update.relation_name;
  if (nullptr == db || common::is_blank(table_name) || common::is_blank(update.attribute_name)) {
    LOG_WARN("invalid argument. db=%p, table_name=%p, attribute_name=%p",
        db, table_name, update.attribute_name);
    return RC::INVALID_ARGUMENT;
  }

  Table *table = db->find_table(table_name);
  if (nullptr == table) {
    LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
    return RC::SCHEMA_TABLE_NOT_EXIST;
  }

  const FieldMeta *field_meta = table->table_meta().field(update.attribute_name);
  if (nullptr == field_meta) {
    LOG_WARN("no such field. table=%s, field=%s", table_name, update.attribute_name);
    return RC::SCHEMA_FIELD_NOT_EXIST;
  }

  Value converted = update.value;
  int *date_value = nullptr;
  if (field_meta->type() == DATES && update.value.type == CHARS) {
    date_value = new int(0);
    if (!parse_date(static_cast<const char *>(update.value.data), *date_value)) {
      delete date_value;
      LOG_WARN("invalid date value. table=%s, field=%s", table_name, update.attribute_name);
      return RC::INVALID_ARGUMENT;
    }
    converted.type = DATES;
    converted.data = date_value;
  } else if (field_meta->type() != update.value.type) {
    LOG_WARN("field type mismatch. table=%s, field=%s, field type=%d, value type=%d",
        table_name, update.attribute_name, field_meta->type(), update.value.type);
    return RC::SCHEMA_FIELD_TYPE_MISMATCH;
  }

  std::unordered_map<std::string, Table *> table_map;
  table_map.insert(std::pair<std::string, Table *>(std::string(table_name), table));

  FilterStmt *filter_stmt = nullptr;
  RC rc = FilterStmt::create(db, table, &table_map, update.conditions, update.condition_num, filter_stmt);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to create filter stmt. rc=%s", strrc(rc));
    return rc;
  }

  stmt = new UpdateStmt(table, field_meta->name(), converted, filter_stmt);
  return RC::SUCCESS;
}
