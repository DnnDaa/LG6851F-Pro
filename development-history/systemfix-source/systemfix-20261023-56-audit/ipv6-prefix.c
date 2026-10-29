/* SPDX-License-Identifier: GPL-2.0-only */
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv)
{
 struct in6_addr addr;
 char text[INET6_ADDRSTRLEN];
 unsigned long prefix;
 char *end;
 if (argc != 3 || !argv[2][0] || strlen(argv[2]) > 3) return 2;
 for (const char *p = argv[2]; *p; p++) if (*p < '0' || *p > '9') return 2;
 errno = 0; prefix = strtoul(argv[2], &end, 10);
 if (errno || *end || prefix < 1 || prefix > 64 || inet_pton(AF_INET6, argv[1], &addr) != 1) return 2;
 if (IN6_IS_ADDR_UNSPECIFIED(&addr) || IN6_IS_ADDR_LOOPBACK(&addr) ||
     IN6_IS_ADDR_MULTICAST(&addr) || IN6_IS_ADDR_LINKLOCAL(&addr) || IN6_IS_ADDR_V4MAPPED(&addr)) return 2;
 /* Do not delegate deprecated site-local space either. ULA remains supported. */
 if (addr.s6_addr[0] == 0xfe && (addr.s6_addr[1] & 0xc0) == 0xc0) return 2;
 for (unsigned bit = prefix; bit < 128; bit++) addr.s6_addr[bit / 8] &= ~(0x80u >> (bit % 8));
 if (!inet_ntop(AF_INET6, &addr, text, sizeof(text))) return 1;
 return printf("%s/%lu\n", text, prefix) < 0 ? 1 : 0;
}
