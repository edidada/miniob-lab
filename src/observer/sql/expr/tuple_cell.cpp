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
// Created by WangYunlai on 2022/07/05.
//

#include "sql/expr/tuple_cell.h"
#include "common/date_utils.h"
#include "storage/common/field.h"
#include "common/log/log.h"
#include "util/comparator.h"

void TupleCell::to_string(std::ostream &os) const
{
  if (is_null_) {
    os << "NULL";
    return;
  }
  switch (attr_type_) {
  case INTS: {
    os << *(int *)data_;
  } break;
  case FLOATS: {
    os << *(float *)data_;
  } break;
  case DATES: {
    os << format_date(*(int *)data_);
  } break;
  case CHARS: {
    for (int i = 0; i < 4; i++) { // the max length of CHARS is 4
      if (data_[i] == '\0') {
        break;
      }
      os << data_[i];
    }
  } break;
  default: {
    LOG_WARN("unsupported attr type: %d", attr_type_);
  } break;
  }
}

int TupleCell::compare(const TupleCell &other) const
{
  // null 与任何值比较都是 unknown，调用方会先用 is_null() 判掉
  if (this->is_null_ || other.is_null_) {
    return 1;
  }
  if (this->attr_type_ == DATES || other.attr_type_ == DATES) {
    int this_date = 0;
    int other_date = 0;
    bool ok = true;
    ok = ok && ((this->attr_type_ == DATES) ? (this_date = *(int *)data_, true)
                                            : ((this->attr_type_ == CHARS) ? parse_date(data_, this_date) : false));
    ok = ok && ((other.attr_type_ == DATES) ? (other_date = *(int *)other.data_, true)
                                            : ((other.attr_type_ == CHARS) ? parse_date(other.data_, other_date) : false));
    if (!ok) {
      LOG_WARN("failed to compare date value");
      return 1;
    }
    return compare_int(&this_date, &other_date);
  }

  if (this->attr_type_ == other.attr_type_) {
    switch (this->attr_type_) {
    case INTS: return compare_int(this->data_, other.data_);
    case FLOATS: return compare_float(this->data_, other.data_);
    case CHARS: return compare_string(this->data_, other.data_, 4);
    default: {
      LOG_WARN("unsupported type: %d", this->attr_type_);
    }
    }
  } else if (this->attr_type_ == INTS && other.attr_type_ == FLOATS) {
    float this_data = *(int *)data_;
    return compare_float(&this_data, other.data_);
  } else if (this->attr_type_ == FLOATS && other.attr_type_ == INTS) {
    float other_data = *(int *)other.data_;
    return compare_float(data_, &other_data);
  }
  LOG_WARN("not supported");
  return -1; // TODO return rc?
}
