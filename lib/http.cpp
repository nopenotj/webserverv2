#include "http.h"
#include "log.h"
#include <cctype>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>
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

HttpRequest parse_http_meta(const std::string &raw, HttpRequest &req) {

  /*
   * Format:
   *  METHOD<space>PATH<space>VERSION\r\n(HEADER\r\n)*\r\n
   *  The last \r\n\r\n is already stripped out.
   */

  int pos = 0;
  int len;

  // Method
  len = raw.find(" ", pos) - pos;
  std::string method = raw.substr(pos, len);
  pos += len + 1;

  // Path
  len = raw.find(" ", pos) - pos;
  req.path = raw.substr(pos, len);
  pos += len + 1;

  // HTTP Version
  len = raw.find("\r\n", pos) - pos;
  std::string httpversion = raw.substr(pos, len);
  pos += len + 2;

  // Parse method
  if (method == "GET") {
    req.method = GET;
  }
  // Parse version
  // version = HTTP/1.1
  req.version = std::stof(httpversion.substr(5));

  // Parse headers
  while (pos < raw.length()) {

    // Find next delim
    int idx = raw.find("\r\n", pos);
    if (idx == std::string::npos)
      break;

    // Extract out header
    len = idx - pos;
    std::string header = raw.substr(pos, len);
    pos += len + 2;

    // Split key and value
    int splt_idx = header.find(":");
    std::string key = header.substr(0, splt_idx);
    std::string value = header.substr(splt_idx + 1);

    req.headers[key] = value;
  }

  return req;
}

std::string lower(const std::string &s) {
  std::string res;
  for (const auto &c : s) {
    res.push_back(tolower(c));
  }
  return res;
}

HttpRequest read_sock(int sfd) {
  SocketStream stream(sfd);
  HttpRequest req;

  std::string meta = stream.get_token("\r\n\r\n") + "\r\n";
  parse_http_meta(meta, req);

  for (const auto &[key, value] : req.headers) {
    if (lower(key) == "content-length") {
      logger.log("Content-length detected" + value);
    }
  }

  return req;
}
