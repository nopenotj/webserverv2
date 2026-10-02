#include "http.h"
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

SocketStream::SocketStream(int socket_fd) : socket_fd(socket_fd), buf() {
  end = curr = buf;
}

std::string SocketStream::get_token(const std::string &delim) {

  std::vector<char> token;
  bool found_delim = false;
  while (!found_delim) {

    if (curr == end) {
      int len = recv(this->socket_fd, buf, sizeof(buf), 0);
      end = buf + len;
      curr = buf;
    }

    // Check if we have hit the delim
    bool is_delim = true;
    for (int i = 0; i < delim.size(); i++) {
      if (curr + i == end || *(curr + i) != delim.at(i)) {
        is_delim = false;
        break;
      }
    }

    if (is_delim) {
      found_delim = true;
      curr += delim.size();
    } else {
      token.push_back(*curr);
      curr += 1;
    }
  }

  return std::string(token.begin(), token.end());
}

HttpRequest read_sock(int sfd) {
  HttpRequest req;

  SocketStream stream(sfd);

  // Parse method
  if (stream.get_token(" ") == "GET") {
    req.method = GET;
  }
  std::string path = stream.get_token(" ");
  std::string version = stream.get_token("\r\n");
  std::string header1 = stream.get_token("\r\n");
  std::string header2 = stream.get_token("\r\n");
  std::string header3 = stream.get_token("\r\n");
  std::string header4 = stream.get_token("\r\n");

  close(sfd);
  return req;
}
