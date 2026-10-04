
#include "../lib/string.h"
#include "string.h"
#include "testkit.h"

void test_lower() { testkit::is_equal(lower("HeLLo"), "hello"); }
void test_trim() { testkit::is_equal(trim("   HeLLo "), "HeLLo"); }
