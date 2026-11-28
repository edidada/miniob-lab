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
// Created by Wangyunlai on 2022/6/6.
//

#include "sql/stmt/select_stmt.h"
#include "sql/stmt/filter_stmt.h"
#include "sql/expr/expression_parser.h"
#include "common/log/log.h"
#include "common/lang/string.h"
#include "storage/common/db.h"
#include "storage/common/table.h"

SelectStmt::~SelectStmt()
{
  if (nullptr != filter_stmt_) {
    delete filter_stmt_;
    filter_stmt_ = nullptr;
  }
}

static const char *agg_type_name(int agg_type)
{
  switch (agg_type) {
  case 1: return "count";
  case 2: return "min";
  case 3: return "max";
  case 4: return "sum";
  case 5: return "avg";
  default: return "unknown";
  }
}

static void wildcard_fields(Table *table, std::vector<Field> &field_metas)
{
  const TableMeta &table_meta = table->table_meta();
  const int field_num = table_meta.field_num();
  for (int i = table_meta.sys_field_num(); i < field_num; i++) {
    field_metas.push_back(Field(table, table_meta.field(i)));
  }
}

RC SelectStmt::create(Db *db, const Selects &select_sql, Stmt *&stmt)
{
  if (nullptr == db) {
    LOG_WARN("invalid argument. db is null");
    return RC::INVALID_ARGUMENT;
  }

  // collect tables in `from` statement
  std::vector<Table *> tables;
  std::unordered_map<std::string, Table *> table_map;
  for (int i = 0; i < select_sql.relation_num; i++) {
    const char *table_name = select_sql.relations[i];
    if (nullptr == table_name) {
      LOG_WARN("invalid argument. relation name is null. index=%d", i);
      return RC::INVALID_ARGUMENT;
    }

    Table *table = db->find_table(table_name);
    if (nullptr == table) {
      LOG_WARN("no such table. db=%s, table_name=%s", db->name(), table_name);
      return RC::SCHEMA_TABLE_NOT_EXIST;
    }

    tables.push_back(table);
    table_map.insert(std::pair<std::string, Table*>(table_name, table));
  }
  
  Table *default_table = nullptr;
  if (tables.size() == 1) {
    default_table = tables[0];
  }

  // collect select items (plain fields and aggregate functions)
  std::vector<SelectStmt::SelectItem> select_items;
  std::vector<Field> query_fields;
  bool has_aggregate = false;

  auto resolve_field = [&](const RelAttr &relation_attr, Table *&table, const FieldMeta *&field_meta) -> RC {
    if (!common::is_blank(relation_attr.relation_name)) {
      auto iter = table_map.find(relation_attr.relation_name);
      if (iter == table_map.end()) {
        LOG_WARN("no such table in from list: %s", relation_attr.relation_name);
        return RC::SCHEMA_FIELD_MISSING;
      }
      table = iter->second;
    } else {
      if (tables.size() != 1) {
        LOG_WARN("invalid. I do not know the attr's table. attr=%s", relation_attr.attribute_name);
        return RC::SCHEMA_FIELD_MISSING;
      }
      table = tables[0];
    }

    field_meta = table->table_meta().field(relation_attr.attribute_name);
    if (field_meta == nullptr) {
      LOG_WARN("no such field. field=%s.%s", table->name(), relation_attr.attribute_name);
      return RC::SCHEMA_FIELD_MISSING;
    }
    return RC::SUCCESS;
  };

  for (size_t i = 0; i < select_sql.attr_num; i++) {
    const RelAttr &relation_attr = select_sql.attributes[i];
    SelectStmt::SelectItem item;
    item.agg = relation_attr.agg_type;

    // 表达式列：select id, 3*col1/(col2+2) ... 表头用表达式原文
    if (relation_attr.expr != nullptr) {
      Expression *expr = nullptr;
      RC rc = build_expression(db, default_table, &table_map, relation_attr.expr, expr);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to build projection expression");
        return rc;
      }
      item.expr = expr;
      item.alias = expression_node_to_string(relation_attr.expr);
      select_items.push_back(item);
      continue;
    }

    if (relation_attr.agg_type != 0) {
      has_aggregate = true;
      if (0 == strcmp(relation_attr.attribute_name, "*")) {
        if (relation_attr.agg_type != AGG_COUNT) {
          LOG_WARN("only count accepts *. agg=%d", relation_attr.agg_type);
          return RC::SCHEMA_FIELD_MISSING;
        }
        item.alias = std::string(agg_type_name(relation_attr.agg_type)) + "(*)";
        select_items.push_back(item);
        continue;
      }
      if (common::is_blank(relation_attr.attribute_name)) {
        LOG_WARN("empty attribute in aggregate function. agg=%d", relation_attr.agg_type);
        return RC::SCHEMA_FIELD_MISSING;
      }

      Table *table = nullptr;
      const FieldMeta *field_meta = nullptr;
      RC rc = resolve_field(relation_attr, table, field_meta);
      if (rc != RC::SUCCESS) {
        return rc;
      }
      item.table = table;
      item.field = field_meta;
      item.alias = std::string(agg_type_name(relation_attr.agg_type)) + "(" + field_meta->name() + ")";
      select_items.push_back(item);

      // 聚合函数参数字段也要参与元组查找，加入 query_fields 以便后续使用
      query_fields.push_back(Field(table, field_meta));
      continue;
    }

    if (common::is_blank(relation_attr.relation_name) && 0 == strcmp(relation_attr.attribute_name, "*")) {
      for (Table *table : tables) {
        wildcard_fields(table, query_fields);
        SelectStmt::SelectItem wildcard_item;
        wildcard_item.table = table;
        const TableMeta &table_meta = table->table_meta();
        for (int j = table_meta.sys_field_num(); j < table_meta.field_num(); j++) {
          wildcard_item.field = table_meta.field(j);
          wildcard_item.alias = std::string(table_meta.field(j)->name());
          select_items.push_back(wildcard_item);
        }
      }
      continue;
    }

    Table *table = nullptr;
    const FieldMeta *field_meta = nullptr;
    RC rc = resolve_field(relation_attr, table, field_meta);
    if (rc != RC::SUCCESS) {
      return rc;
    }
    item.table = table;
    item.field = field_meta;
    item.alias = std::string(field_meta->name());
    select_items.push_back(item);
    query_fields.push_back(Field(table, field_meta));
  }

  // 旧路径：单表且没有聚合函数时，沿用原有 query_fields 逻辑
  if (!has_aggregate && select_sql.attr_num > 0) {
    query_fields.clear();
    for (size_t i = 0; i < select_sql.attr_num; i++) {
      const RelAttr &relation_attr = select_sql.attributes[i];
      if (relation_attr.expr != nullptr) {
        continue;
      }
      if (common::is_blank(relation_attr.relation_name) && 0 == strcmp(relation_attr.attribute_name, "*")) {
        for (Table *table : tables) {
          wildcard_fields(table, query_fields);
        }
        continue;
      }
      Table *table = nullptr;
      const FieldMeta *field_meta = nullptr;
      RC rc = resolve_field(relation_attr, table, field_meta);
      if (rc != RC::SUCCESS) {
        return rc;
      }
      query_fields.push_back(Field(table, field_meta));
    }
  }

  std::vector<Field> group_by_fields;
  for (size_t i = 0; i < select_sql.group_by_num; i++) {
    Table *table = nullptr;
    const FieldMeta *field_meta = nullptr;
    RC rc = resolve_field(select_sql.group_by_attributes[i], table, field_meta);
    if (rc != RC::SUCCESS) {
      return rc;
    }
    group_by_fields.push_back(Field(table, field_meta));
  }

  std::vector<SelectStmt::OrderByField> order_by_fields;
  for (size_t i = 0; i < select_sql.order_by_num; i++) {
    Table *table = nullptr;
    const FieldMeta *field_meta = nullptr;
    RC rc = resolve_field(select_sql.order_by_attributes[i], table, field_meta);
    if (rc != RC::SUCCESS) {
      return rc;
    }
    SelectStmt::OrderByField order_field;
    order_field.table = table;
    order_field.field = field_meta;
    order_field.asc = select_sql.order_by_asc[i] != 0;
    order_by_fields.push_back(order_field);
  }

  LOG_INFO("got %d tables in from stmt and %d fields in query stmt", tables.size(), query_fields.size());

  // create filter statement in `where` statement
  FilterStmt *filter_stmt = nullptr;
  RC rc = FilterStmt::create(db, default_table, &table_map,
           select_sql.conditions, select_sql.condition_num, filter_stmt);
  if (rc != RC::SUCCESS) {
    LOG_WARN("cannot construct filter stmt");
    return rc;
  }

  // everything alright
  SelectStmt *select_stmt = new SelectStmt();
  select_stmt->tables_.swap(tables);
  select_stmt->query_fields_.swap(query_fields);
  select_stmt->filter_stmt_ = filter_stmt;
  select_stmt->order_by_fields_.swap(order_by_fields);
  select_stmt->select_items_.swap(select_items);
  select_stmt->group_by_fields_.swap(group_by_fields);
  select_stmt->has_aggregate_ = has_aggregate;
  stmt = select_stmt;
  return RC::SUCCESS;
}
