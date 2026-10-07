
#include "lib/http.h"
#include "lib/log.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

struct Options {
  std::string host;
  int port;
};

Options parse_args(int argc, char **argv) {
  // TODO: Implement some sort of arg parsing
  return Options{"0.0.0.0", 8080};
}

int main(int argc, char **argv) {
  Options opt = parse_args(argc, argv);
  Server app;

  app.add_handler("/", [](HttpRequest req) {
    HttpResponse res;
    res.status_code = 200;
    res.headers["content-type"] = "application/json";
    res.content = "{\"data\": \"Hello world!\"}";
    return res;
  });

  app.run(opt.host, opt.port);
}
