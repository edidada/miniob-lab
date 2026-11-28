#include <cstring>
#include <map>

#include "common/log/log.h"
#include "sql/operator/aggregation_operator.h"

void AggregationOperator::add_group_field(const Table *table, const FieldMeta *field)
{
  group_fields_.push_back(std::make_pair(table, field));
}

void AggregationOperator::add_item(int agg, const Table *table, const FieldMeta *field)
{
  Item item;
  item.agg = agg;
  item.table = table;
  item.field = field;

  static const char *names[] = {"", "count", "min", "max", "sum", "avg"};
  const char *name = (agg >= 1 && agg <= 5) ? names[agg] : "unknown";
  if (agg == 0) {
    item.alias = field->name();
    if (multi_table_) {
      item.alias = std::string(table->name()) + "." + field->name();
    }
  } else if (field == nullptr) {
    item.alias = std::string(name) + "(*)";
  } else {
    item.alias = std::string(name) + "(" + field->name() + ")";
    if (multi_table_) {
      item.alias = std::string(name) + "(" + table->name() + "." + field->name() + ")";
    }
  }
  items_.push_back(item);
}

RC AggregationOperator::build_key(const Tuple &tuple, std::string &key) const
{
  key.clear();
  for (const auto &group_field : group_fields_) {
    Field field(group_field.first, group_field.second);
    TupleCell cell;
    RC rc = tuple.find_cell(field, cell);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to find group field. field=%s", group_field.second->name());
      return rc;
    }
    int len = group_field.second->len();
    key.append(cell.data(), len);
  }
  return RC::SUCCESS;
}

RC AggregationOperator::open()
{
  if (children_.size() != 1) {
    LOG_WARN("aggregation operator must has 1 child");
    return RC::INTERNAL;
  }

  Operator *child = children_[0];
  RC rc = child->open();
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open child operator: %s", strrc(rc));
    return rc;
  }

  std::map<std::string, Bucket *> bucket_map;
  while (RC::SUCCESS == (rc = child->next())) {
    Tuple *tuple = child->current_tuple();
    if (tuple == nullptr) {
      LOG_WARN("failed to get current tuple");
      return RC::INTERNAL;
    }

    std::string key;
    rc = build_key(*tuple, key);
    if (rc != RC::SUCCESS) {
      return rc;
    }

    Bucket *bucket = nullptr;
    auto iter = bucket_map.find(key);
    if (iter == bucket_map.end()) {
      std::unique_ptr<Bucket> new_bucket(new Bucket);
      new_bucket->states.resize(items_.size());
      for (size_t i = 0; i < items_.size(); i++) {
        if (items_[i].agg == 0) {
          new_bucket->plain_cells.emplace_back();
          new_bucket->storage.emplace_back();
        }
      }
      bucket = new_bucket.get();
      buckets_.push_back(std::move(new_bucket));
      bucket_map.insert(std::make_pair(key, bucket));
    } else {
      bucket = iter->second;
    }

    for (size_t i = 0; i < items_.size(); i++) {
      Item &item = items_[i];
      if (item.agg == 0) {
        Field field(item.table, item.field);
        TupleCell cell;
        rc = tuple->find_cell(field, cell);
        if (rc != RC::SUCCESS) {
          LOG_WARN("failed to find field. field=%s", item.field->name());
          return rc;
        }
        int len = item.field->len();
        bucket->storage[i].resize(len);
        memcpy(bucket->storage[i].data(), cell.data(), len);
        bucket->plain_cells[i] = TupleCell(item.field->type(), bucket->storage[i].data());
        continue;
      }

      AggState &state = bucket->states[i];
      if (item.agg == 1 && item.field == nullptr) {  // count(*)
        state.count++;
        state.type = INTS;
        continue;
      }

      Field field(item.table, item.field);
      TupleCell cell;
      rc = tuple->find_cell(field, cell);
      if (rc != RC::SUCCESS) {
        LOG_WARN("failed to find field. field=%s", item.field->name());
        return rc;
      }
      // null 值不参与任何聚合计算（count/min/max/sum/avg 都跳过）
      if (cell.is_null()) {
        continue;
      }
      state.count++;
      if (item.agg == 1) {  // count
        state.type = INTS;
        continue;
      }
      state.type = item.field->type();

      switch (item.agg) {
      case 2:   // min
      case 3: { // max
        if (!state.init) {
          state.value.resize(item.field->len());
          memcpy(state.value.data(), cell.data(), item.field->len());
          state.init = true;
        } else {
          TupleCell current(item.field->type(), state.value.data());
          int cmp = cell.compare(current);
          if ((item.agg == 2 && cmp < 0) || (item.agg == 3 && cmp > 0)) {
            memcpy(state.value.data(), cell.data(), item.field->len());
          }
        }
      } break;
      case 4:   // sum
      case 5: { // avg
        if (item.field->type() == INTS) {
          state.sum += *(int *)cell.data();
        } else if (item.field->type() == FLOATS) {
          state.sum += *(float *)cell.data();
        }
      } break;
      default:
        break;
      }
    }
  }

  if (rc != RC::RECORD_EOF) {
    LOG_WARN("failed to scan all records. rc=%s", strrc(rc));
    return rc;
  }

  // 没有任何分组字段且表为空时，聚合函数也需要输出一行
  if (buckets_.empty() && group_fields_.empty()) {
    std::unique_ptr<Bucket> new_bucket(new Bucket);
    new_bucket->states.resize(items_.size());
    for (size_t i = 0; i < items_.size(); i++) {
      if (items_[i].agg == 0) {
        new_bucket->plain_cells.emplace_back();
        new_bucket->storage.emplace_back();
      }
    }
    buckets_.push_back(std::move(new_bucket));
  }

  header_.clear();
  for (const Item &item : items_) {
    header_.push_back(item.alias);
  }

  current_index_ = 0;
  return RC::SUCCESS;
}

RC AggregationOperator::next()
{
  if (current_index_ >= buckets_.size()) {
    return RC::RECORD_EOF;
  }

  Bucket &bucket = *buckets_[current_index_];
  output_cells_.clear();
  agg_storage_.clear();
  agg_storage_.resize(items_.size());
  for (size_t i = 0; i < items_.size(); i++) {
    const Item &item = items_[i];
    if (item.agg == 0) {
      output_cells_.push_back(bucket.plain_cells[i]);
      continue;
    }

    AggState &state = bucket.states[i];
    switch (item.agg) {
    case 1: { // count
      agg_storage_[i].resize(sizeof(int));
      *reinterpret_cast<int *>(agg_storage_[i].data()) = static_cast<int>(state.count);
      output_cells_.emplace_back(INTS, agg_storage_[i].data());
    } break;
    case 2:
    case 3: {
      if (state.init) {
        output_cells_.emplace_back(item.field->type(), state.value.data());
      } else {
        output_cells_.emplace_back(item.field->type(), nullptr);
        output_cells_.back().set_null(true);
      }
    } break;
    case 4: { // sum
      if (state.count == 0) {
        output_cells_.emplace_back(item.field->type(), nullptr);
        output_cells_.back().set_null(true);
        break;
      }
      agg_storage_[i].resize(sizeof(float));
      if (item.field->type() == INTS) {
        agg_storage_[i].resize(sizeof(int));
        *reinterpret_cast<int *>(agg_storage_[i].data()) = static_cast<int>(state.sum);
        output_cells_.emplace_back(INTS, agg_storage_[i].data());
      } else {
        *reinterpret_cast<float *>(agg_storage_[i].data()) = static_cast<float>(state.sum);
        output_cells_.emplace_back(FLOATS, agg_storage_[i].data());
      }
    } break;
    case 5: { // avg
      if (state.count == 0) {
        output_cells_.emplace_back(item.field->type(), nullptr);
        output_cells_.back().set_null(true);
        break;
      }
      agg_storage_[i].resize(sizeof(float));
      double avg = state.count == 0 ? 0.0 : state.sum / state.count;
      avg = static_cast<double>(static_cast<long long>(avg * 100 + (avg >= 0 ? 0.5 : -0.5))) / 100;
      *reinterpret_cast<float *>(agg_storage_[i].data()) = static_cast<float>(avg);
      output_cells_.emplace_back(FLOATS, agg_storage_[i].data());
    } break;
    default: {
      output_cells_.emplace_back(UNDEFINED, nullptr);
    } break;
    }
  }

  tuple_.set_cells(output_cells_);
  current_index_++;
  return RC::SUCCESS;
}

RC AggregationOperator::close()
{
  buckets_.clear();
  output_cells_.clear();
  agg_storage_.clear();
  return children_[0]->close();
}

Tuple *AggregationOperator::current_tuple()
{
  return &tuple_;
}
