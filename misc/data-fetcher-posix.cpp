#include "data-fetcher-esp.h"

#include <string>
#include <cstring>
#include <esp_log.h>

#include "common.h"

namespace {

#include <strings.h>

#include "defs.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define RETROSTORE_HOST "retrostore.org"
#define RETROSTORE_PORT 80

#define LOG(msg) printf("%s\n", msg)


bool connect_server(int* fd)
{
  const char* host = RETROSTORE_HOST;
  const int port = RETROSTORE_PORT;
  
  struct sockaddr_in serv_addr;
  struct hostent* server;

  
  *fd = socket(AF_INET, SOCK_STREAM, 0);
  if (*fd < 0) {
    // Error opening socket
    return false;
  }
  server = gethostbyname(host);
  if (server == NULL) {
    // No such host
    return false;
  }
  bzero((char*) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char*) server->h_addr,
        (char*) &serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(port);
  if (connect(*fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    // Error connecting
    return false;
  }
  return true;
}
  
bool skip_to_body(int fd)
{
  char buf[4];
  buf[3] = 0;

  while (true) {
    int result = recv(fd, buf, 1, MSG_WAITALL);
    if (result < 1) {
      return false;
    }
    if (buf[0] != '\r') {
      continue;
    }
    result = recv(fd, buf, 3, MSG_WAITALL);
    if (result < 3) {
      return false;
    }
    if (buf[0] == '\n' && buf[1] == '\r' && buf[2] == '\n') {
      break;
    }
  }
  return true;
}


};  // namespace

namespace retrostore {

bool DataFetcherEsp::Fetch(const std::string& path,
                           const RsData& params,
                           RsData* data) const {
  if (data->len > 0 || data->data != NULL) {
    LOG("Given `data` should be empty!");
    return false;
  }

  int fd;

  if (!connect_server(&fd)) {
    LOG("ERROR: Connection failed");
    return false;
  }

  char* header = (char*) malloc(512);
  snprintf(header, 512, "POST %s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "Content-type: application/octet-stream\r\n"
           "Content-Length: %d\r\n"
           "Connection: close\r\n\r\n", path.c_str(), RETROSTORE_HOST,
           params.len);
  write(fd, header, strlen(header));
  write(fd, (const char*) params.data, params.len);
  free(header);

  if (!skip_to_body(fd)) {
    return false;
  }

  uint8_t* buf;
  int br = 0;

  const int buf_size_incr = 100;
  int buf_size = buf_size_incr;

  buf = (uint8_t*) malloc(buf_size);
  if (buf == NULL) {
    return false;
  }

  int result;

  do {
    result = recv(fd, buf + br, buf_size_incr, MSG_WAITALL);
    if (result < 0) {
      free(buf);
      return false;
    }
    br += result;
    if (result == buf_size_incr) {
      buf_size += buf_size_incr;
      buf = (uint8_t*) realloc(buf, buf_size);
      if (buf == NULL) {
        return false;
      }
    }
  }  while (result != 0);
 
  close(fd);

  data->data = buf;
  data->len = br;

  return true;
}

}  // namespace retrostore
