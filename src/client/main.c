#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SOCKS_VER 0x05
#define METHOD_NOAUTH 0x00
#define METHOD_USERPASS 0x02

void die(const char *msg) {
  perror(msg);
  exit(1);
}

int main() {
  const char *ip = "127.0.0.1";
  int port = 8888;

  printf("[+] Connecting to %s:%d...\n", ip, port);

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    die("socket");

  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_port = htons(port),
  };
  inet_pton(AF_INET, ip, &addr.sin_addr);

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    die("connect");

  printf("[+] Connected!\n");

  uint8_t greeting[] = {SOCKS_VER, 0x02, METHOD_NOAUTH, METHOD_USERPASS};

  printf("[>] Sending greeting...\n");
  if (send(fd, greeting, sizeof(greeting), 0) != sizeof(greeting))
    die("send greeting");

  uint8_t gresp[2];

  printf("[<] Waiting greeting response...\n");
  if (recv(fd, gresp, 2, 0) != 2)
    die("recv greeting");

  printf("[+] Server responded: VER=0x%02x METHOD=0x%02x\n", gresp[0],
         gresp[1]);

  if (gresp[0] != SOCKS_VER) {
    printf("[-] Server sent invalid version.\n");
    exit(1);
  }

  if (gresp[1] == METHOD_USERPASS) {
    printf("[+] Server requires USER/PASS auth\n");

    const char *user = "username";
    const char *pass = "password";

    uint8_t ulen = strlen(user);
    uint8_t plen = strlen(pass);

    uint8_t auth[3 + 255];
    size_t pos = 0;

    auth[pos++] = 0x01;
    auth[pos++] = ulen;
    memcpy(&auth[pos], user, ulen);
    pos += ulen;
    auth[pos++] = plen;
    memcpy(&auth[pos], pass, plen);
    pos += plen;

    printf("[>] Sending auth...\n");
    if (send(fd, auth, pos, 0) != (ssize_t)pos)
      die("send auth");

    uint8_t aresp[2];
    printf("[<] Waiting auth response...\n");

    if (recv(fd, aresp, 2, 0) != 2)
      die("recv auth");

    printf("[+] Auth response: VER=0x%02x STATUS=0x%02x\n", aresp[0], aresp[1]);

    if (aresp[1] != 0x00) {
      printf("[-] Authentication failed.\n");
      exit(1);
    }
    printf("[+] Auth OK!\n");

  } else if (gresp[1] == METHOD_NOAUTH) {
    printf("[+] Server selected NO AUTH\n");
  } else if (gresp[1] == 0xFF) {
    printf("[-] Server rejected all methods\n");
    exit(1);
  }

  printf("[✓] Greeting + Auth test complete.\n");

  close(fd);
  return 0;
}
