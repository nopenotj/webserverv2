#include "http.h"
#include "string.h"
#include <iostream>

int main() {
  test_http_socket_stream();
  test_lower();
  test_trim();
  std::cout << "Test cleared!" << std::endl;
}
