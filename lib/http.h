#include <string>
#include <unordered_map>
enum Method {
  GET,
};
struct HttpRequest {
  Method method;
  std::string path;
  float version;
  std::unordered_map<std::string, std::string> headers;
  char *content;
  int content_len;
};

#define BUFF_SIZE 1024
class SocketStream {
public:
  int socket_fd;
  char buf[BUFF_SIZE];
  char *curr;
  char *end;
  SocketStream(int socket_fd);
  std::string get_token(const std::string &delim);
};
HttpRequest read_sock(int sfd);
