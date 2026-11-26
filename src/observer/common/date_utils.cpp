#include "common/date_utils.h"

#include <cstdio>
#include <cstring>

static bool is_digit(char c) { return c >= '0' && c <= '9'; }

static bool leap_year(int year)
{
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

static int days_in_month(int year, int month)
{
  static const int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) {
    return 0;
  }
  if (month == 2 && leap_year(year)) {
    return 29;
  }
  return days[month];
}

bool parse_date(const char *text, int &value)
{
  if (text == nullptr) {
    return false;
  }

  // YYYY-M-D, at most 4 digits for year, 2 for month/day
  int year = 0;
  int month = 0;
  int day = 0;
  int pos = 0;

  int digits = 0;
  while (is_digit(text[pos]) && digits < 4) {
    year = year * 10 + (text[pos] - '0');
    pos++;
    digits++;
  }
  if (digits < 1 || digits > 4 || text[pos] != '-') {
    return false;
  }
  pos++;

  digits = 0;
  while (is_digit(text[pos]) && digits < 2) {
    month = month * 10 + (text[pos] - '0');
    pos++;
    digits++;
  }
  if (digits < 1 || digits > 2 || text[pos] != '-') {
    return false;
  }
  pos++;

  digits = 0;
  while (is_digit(text[pos]) && digits < 2) {
    day = day * 10 + (text[pos] - '0');
    pos++;
    digits++;
  }
  if (digits < 1 || digits > 2 || text[pos] != '\0') {
    return false;
  }

  if (year < 1 || month < 1 || month > 12 || day < 1) {
    return false;
  }
  if (day > days_in_month(year, month)) {
    return false;
  }

  value = year * 10000 + month * 100 + day;
  return true;
}

std::string format_date(int value)
{
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", value / 10000, value / 100 % 100, value % 100);
  return std::string(buffer);
}
