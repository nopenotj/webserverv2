#include "http.h"
#include "log.h"
#include "string.h"
#include <format>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

SocketStream::SocketStream(int socket_fd) : socket_fd(socket_fd), buf() {
  end = curr = buf;
}

std::string SocketStream::get_token(const std::string &delim) {

  std::string token;

  // Initial fill
  if (buf_empty())
    fill();

  // Try to get token
  while (!buf_empty()) {
    for (; curr != end; curr++) {
      // Put into stack
      token.push_back(*curr);

      // Check delim
      if (token.length() >= delim.length() &&
          token.compare(token.length() - delim.length(), delim.length(),
                        delim) == 0) {

        curr++;
        return token.substr(0, token.length() - delim.length());
      }
    }

    // Fill buff
    fill();
  }
  throw std::runtime_error("End of buffer");
}
std::string SocketStream::get(int n) {
  std::string res;
  int i = 0;

  if (buf_empty())
    fill();

  while (!buf_empty() && i < n) {

    res.push_back(*curr);
    curr++;
    i++;

    if (i == n)
      return res;

    // Fill if needed
    if (buf_empty())
      fill();
  }

  throw std::logic_error("Unexpected error");
}
void SocketStream::fill() {
  int len = recv(this->socket_fd, buf, sizeof(buf), 0);
  end = buf + len;
  curr = buf;
  return;
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
  req.method = to_method(method);

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
    std::string key = trim(header.substr(0, splt_idx));
    std::string value = trim(header.substr(splt_idx + 1));

    req.headers[key] = value;
  }

  return req;
}

Method to_method(const std::string &s) {
  if (s == "GET") {
    return GET;
  }
  if (s == "PUT") {
    return PUT;
  }
  if (s == "POST") {
    return POST;
  }
  if (s == "DELETE") {
    return DELETE;
  }
  throw std::invalid_argument("Invalid enum for Method");
}
std::string to_string(Method m) {
  switch (m) {
  case GET:
    return "GET";
  case PUT:
    return "PUT";
  case POST:
    return "POST";
  case DELETE:
    return "DELETE";
  default:
    throw std::invalid_argument("Invalid enum for Method");
  }
}

HttpRequest read_sock(int sfd) {
  SocketStream stream(sfd);
  HttpRequest req;

  std::string meta = stream.get_token("\r\n\r\n") + "\r\n";
  parse_http_meta(meta, req);

  int content_length = get_content_length(req);

  req.content = stream.get(content_length);
  return req;
}

int get_content_length(const HttpRequest &req) {

  int content_length = 0;
  for (const auto &[key, value] : req.headers) {
    if (lower(key) == "content-length") {
      content_length = std::stoi(value);
    }
  }

  return content_length;
}
