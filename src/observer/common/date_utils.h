#pragma once

#include <string>

// parse a date literal like "2020-1-21" or "2020-01-21" into the integer form 20200121.
// returns false when the text is not a valid calendar date.
bool parse_date(const char *text, int &value);

// format an integer date (20200121) back into "2020-01-21"
std::string format_date(int value);
