
#include "lib/http.h"
#include "lib/log.h"
#include <iostream>

#include <format>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

Logger logger(std::cout);

struct Options {
  int port;
};

Options parse_args(int argc, char **argv) {
  // TODO: Implement some sort of arg parsing
  return Options{8080};
}

struct Request {
  socklen_t in_addr_len;
  sockaddr_in in_addr;
  int sock;
};

void handle_req(Request req) {
  logger.log(std::format("Parsing fd={}", req.sock));
  read_sock(req.sock);
}

int main(int argc, char **argv) {

  Options opt = parse_args(argc, argv);

  // Didnt go down the RTFM route and just followed
  // https://www.geeksforgeeks.org/cpp/socket-programming-in-cpp/

  // Obtain a socket, wtf is protocol of the socket? domain = ipv4, type = tcp,
  // protocol = ?
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  // Allow rebinding while the old socket is still in TIME_WAIT
  int reuse = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  // converts from host byte order to network byte order
  sockaddr_in addr{AF_INET, htons(opt.port), INADDR_ANY};

  logger.log("Binding to socket..");
  // Bind to address
  if (bind(sock, (sockaddr *)&addr, sizeof(addr))) {
    logger.log("Unable to bind to port");
    return 1;
  }

  logger.log("Listening to socket...");
  listen(sock, 3);
  while (1) {
    Request req;
    req.in_addr_len = sizeof(req.in_addr);
    req.sock = accept(sock, (sockaddr *)&req.in_addr, &req.in_addr_len);

    if (req.sock == -1)
      continue;

    // Following : https://www.geeksforgeeks.org/cpp/multithreading-in-cpp/
    std::thread t(handle_req, std::move(req));
    t.detach();
  }
}
