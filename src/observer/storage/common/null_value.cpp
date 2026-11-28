#include <cstdint>
#include <cstring>

#include "storage/common/null_value.h"

// FLOATS 用 NaN(0x7fc00000) 作为 NULL；INTS 用 INT32_MIN；DATES 用 0（yyyymmdd 不会为 0）；
// CHARS 用全 0xFF（正常字符串不会出现该字节）
static const uint32_t NULL_FLOAT_BITS = 0x7fc00000u;

bool value_is_null(AttrType type, const char *data, int len)
{
  if (data == nullptr) {
    return true;
  }
  switch (type) {
  case INTS: return *reinterpret_cast<const int *>(data) == INT32_MIN;
  case FLOATS: {
    uint32_t bits = 0;
    memcpy(&bits, data, sizeof(bits));
    return bits == NULL_FLOAT_BITS;
  }
  case DATES: return *reinterpret_cast<const int *>(data) == 0;
  case CHARS: {
    for (int i = 0; i < len; i++) {
      if (data[i] != static_cast<char>(0xFF)) {
        return false;
      }
    }
    return len > 0;
  }
  default: return false;
  }
}

void set_value_null(AttrType type, char *data, int len)
{
  switch (type) {
  case INTS: {
    int value = INT32_MIN;
    memcpy(data, &value, sizeof(value));
  } break;
  case FLOATS: {
    uint32_t bits = NULL_FLOAT_BITS;
    memcpy(data, &bits, sizeof(bits));
  } break;
  case DATES: {
    int value = 0;
    memcpy(data, &value, sizeof(value));
  } break;
  case CHARS: {
    memset(data, 0xFF, len);
  } break;
  default: break;
  }
}
