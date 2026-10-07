#include "http.h"
#include "log.h"
#include "string.h"
#include <arpa/inet.h>
#include <cstdint>
#include <exception>
#include <format>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
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
  if (content_length)
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

void send(HttpResponse res, int sock) {
  if (res.content.length() > 0)
    res.headers["content-length"] = std::to_string(res.content.length());
  std::string headers_str;
  for (const auto &[key, val] : res.headers) {
    headers_str += std::format("{}:{}\r\n", key, val);
  }
  std::string raw_resp =
      std::format("HTTP/1.1 {} {}\r\n{}\r\n", res.status_code, "Random Message",
                  headers_str);

  write(sock, raw_resp.c_str(), raw_resp.length());
  write(sock, res.content.c_str(), res.content.length());
}

void Server::handle_req(IncomingConn conn) {
  try {
    HttpRequest http_req = read_sock(conn.sock);

    // max is 16: 255.255.255.255\0
    char in_addr[16];
    inet_ntop(AF_INET, &conn.in_addr.sin_addr, in_addr, conn.in_addr_len);
    logger.info(std::format("{} - {} {} HTTP/{:.1f} {}", in_addr,
                            to_string(http_req.method), http_req.path,
                            http_req.version, http_req.content));

    // Default response
    HttpResponse response;
    response.status_code = 500;

    // Call the handler
    if (handlers.contains(http_req.path))
      response = handlers[http_req.path](http_req);

    send(response, conn.sock);
  } catch (...) {
    // Catch all for all things that could go wrong hehe
    close(conn.sock);
  }
}

void Server::run(std::string host, int port) {

  // Convert opts to actual server params
  uint32_t ip;
  inet_pton(AF_INET, host.c_str(), &ip);

  // Didnt go down the RTFM route and just followed
  // https://www.geeksforgeeks.org/cpp/socket-programming-in-cpp/

  // Obtain a socket, wtf is protocol of the socket? domain = ipv4, type = tcp,
  // protocol = ?
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  // Allow rebinding while the old socket is still in TIME_WAIT
  int reuse = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  // converts from host byte order to network byte order
  sockaddr_in addr{AF_INET, htons(port), ip};

  logger.info("Binding to socket..");
  // Bind to address
  if (bind(sock, (sockaddr *)&addr, sizeof(addr))) {
    throw std::runtime_error("Unable to bind to port");
  }

  logger.info("Listening to socket...");
  listen(sock, 3);
  while (1) {
    IncomingConn req;
    req.in_addr_len = sizeof(req.in_addr);
    req.sock = accept(sock, (sockaddr *)&req.in_addr, &req.in_addr_len);

    if (req.sock == -1)
      continue;

    // Following : https://www.geeksforgeeks.org/cpp/multithreading-in-cpp/
    std::thread t(&Server::handle_req, this, std::move(req));
    t.detach();
  }
}
