
#include "string.h"

std::string trim(const std::string &s) {
  int start = 0;

  for (; start < s.length(); start++) {
    if (s.at(start) != ' ')
      break;
  }

  int end = s.length() - 1;
  for (; end > -1; end--) {
    if (s.at(end) != ' ')
      break;
  }

  return s.substr(start, end - start + 1);
}

std::string lower(const std::string &s) {
  std::string res;
  for (const auto &c : s) {
    res.push_back(tolower(c));
  }
  return res;
}
