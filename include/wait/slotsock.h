#ifndef _SLOTSOCK_H_
#define _SLOTSOCK_H_
struct sockcb;

struct sockcb *sock_attach(int sockfd);
int sock_detach(struct sockcb *sockcbp);

int sock_read_wait(struct sockcb *sockcbp, struct waitcb *waitcbp);
int sock_write_wait(struct sockcb *sockcbp, struct waitcb *waitcbp);

int getaddrbyname(const char *name, struct sockaddr_in *addr);
int get_addr_by_name(const char *name, struct in_addr *ipaddr);

struct sockop {
	int (* blocking)(int fd);
	int (* op_read)(int fd, void *buf, int len);
	int (* op_write)(int fd, void *buf, int len);
	void (* read_wait)(void *upp, struct waitcb *wait);
	void (* write_wait)(void *upp, struct waitcb *wait);
	void (* do_shutdown)(int file, int cond);
};

extern struct sockop winsock_ops;
#endif

