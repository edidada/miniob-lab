#pragma once

#include <memory>
#include <string>
#include <vector>

#include "sql/operator/operator.h"
#include "sql/expr/tuple.h"
#include "storage/common/table.h"

class AggregationOperator : public Operator
{
public:
  AggregationOperator() = default;
  virtual ~AggregationOperator() = default;

  void add_group_field(const Table *table, const FieldMeta *field);
  void add_item(int agg, const Table *table, const FieldMeta *field);
  void set_multi_table(bool multi) { multi_table_ = multi; }

  const std::vector<std::string> &header() const { return header_; }

  RC open() override;
  RC next() override;
  RC close() override;
  Tuple *current_tuple() override;

private:
  struct Item
  {
    int agg = 0;
    const Table *table = nullptr;
    const FieldMeta *field = nullptr;
    std::string alias;
  };

  struct AggState
  {
    long count = 0;
    double sum = 0;
    bool init = false;
    std::vector<char> value;  // min/max 保留的原始值
    AttrType type = UNDEFINED;
  };

  struct Bucket
  {
    std::vector<AggState> states;
    std::vector<TupleCell> plain_cells;
    std::vector<std::vector<char>> storage;
  };

  class AggTuple : public Tuple
  {
  public:
    void set_cells(const std::vector<TupleCell> &cells) { cells_ = cells; }
    int cell_num() const override { return static_cast<int>(cells_.size()); }
    RC find_cell(const Field &field, TupleCell &cell) const override { return RC::NOTFOUND; }
    RC cell_spec_at(int index, const TupleCellSpec *&spec) const override { return RC::NOTFOUND; }
    RC cell_at(int index, TupleCell &cell) const override
    {
      if (index < 0 || index >= static_cast<int>(cells_.size())) {
        return RC::INVALID_ARGUMENT;
      }
      cell = cells_[index];
      return RC::SUCCESS;
    }

  private:
    std::vector<TupleCell> cells_;
  };

  RC build_key(const Tuple &tuple, std::string &key) const;

  std::vector<std::pair<const Table *, const FieldMeta *>> group_fields_;
  std::vector<Item> items_;
  std::vector<std::string> header_;
  bool multi_table_ = false;

  std::vector<std::unique_ptr<Bucket>> buckets_;
  std::vector<TupleCell> output_cells_;
  std::vector<std::vector<char>> agg_storage_;
  size_t current_index_ = 0;
  AggTuple tuple_;
};
