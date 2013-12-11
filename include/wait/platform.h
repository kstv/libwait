#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#undef assert
#define KASSERT(msg, exp)

#if defined(WIN32) && !defined(_WIN32_)
#define _WIN32_
#endif

#if defined(WIN32)
#ifndef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#endif

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0
#endif

#ifndef ECONNREFUSED
#define ECONNREFUSED WSAECONNREFUSED
#endif

#ifndef ECONNRESET
#define ECONNRESET WSAECONNRESET
#endif

#ifndef ENOMEM
#define ENOMEM WSAENOBUFS
#endif
#endif

#ifdef _WIN32_
#include <winsock.h>
#define assert(exp) do { if (exp); else { printf("assert %s failed %s:%d\n", #exp, __FILE__, __LINE__); Sleep(INFINITE); }; } while ( 0 );
typedef int socklen_t;
typedef unsigned long in_addr_t;
#define ECONNABORTED WSAECONNABORTED
#define SHUT_WR SD_SEND
#define bcopy(s, d, l) memcpy(d, s, l)
#else
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define assert(exp) do { if (exp); else { printf("assert %s failed %s:%d\n", #exp, __FILE__, __LINE__); }; } while ( 0 );
#define closesocket(s) close(s)

#define WSAGetLastError() errno
#define WSAEWOULDBLOCK EAGAIN
#define SD_BOTH	SHUT_RDWR
#define max(a, b) ((a) < (b)? (b): (a))
#define min(a, b) ((a) < (b)? (a): (b))
#define WSAEINVAL EINVAL
#define stricmp strcmp
unsigned int GetTickCount(void);
#endif

void setnonblock(int fd);

#endif

