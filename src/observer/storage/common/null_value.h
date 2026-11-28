#pragma once

#include "sql/parser/parse_defs.h"

// miniob 的记录是定长的原始字节，没有独立的 null 标记位，
// 因此每种类型用一个不可能出现的哨兵值表示 NULL。
bool value_is_null(AttrType type, const char *data, int len);
void set_value_null(AttrType type, char *data, int len);
