#include "../lib/http.h"
#include "http.h"
#include "testkit.h"
#include <cassert>
#include <sys/socket.h>
#include <unistd.h>

void test_http_socket_stream() {
  int fd[2];
  socketpair(AF_UNIX, SOCK_STREAM, 0, fd);
  char raw_http[] = "GET / HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: "
                    "curl/8.22.0\r\nAccept: */*\r\n\r\n";

  write(fd[0], raw_http, sizeof(raw_http));

  SocketStream stream(fd[1]);
  testkit::is_equal(stream.get_token(" "), "GET");
  testkit::is_equal(stream.get_token(" "), "/ siadsa");
  testkit::is_equal(stream.get_token("\r\n"), "HTTP/1.1");
  testkit::is_equal(stream.get_token("\r\n"), "Host: localhost:8080");
  testkit::is_equal(stream.get_token("\r\n"), "User-Agent: curl/8.22.0");
  testkit::is_equal(stream.get_token("\r\n"), "Accept: */*");
}
