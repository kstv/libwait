#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>

#include "wf.h"

#define MAX_CONN 1000
#define SF_FIRST_IN   1
#define SF_FIRST_OUT  2
#define SF_READY_IN   4
#define SF_READY_OUT  8
#define SF_CLOSE_IN   16
#define SF_CLOSE_OUT  32
#define SF_MESSAGE    64

#define MAX_EVENTS 10
#define SOCK_MAGIC 0x19821133

struct sockcb {
	int s_file;
	int s_flags;
	int s_magic;
	slotcb s_rslot;
	slotcb s_wslot;
	struct epoll_event s_event;
};

static int _sock_ref = 0;
static int _epoll_fd = 0;
static int (* register_sockmsg)(int fd, int index, int flags) = 0;

struct sockcb *sock_attach(int sockfd)
{
	int error;
	struct sockcb *sockcbp;

	sockcbp = (struct sockcb *)malloc(sizeof(*sockcbp));
	assert(sockcbp != NULL);

	sockcbp->s_file = sockfd;
	sockcbp->s_magic = SOCK_MAGIC;
	sockcbp->s_rslot = 0;
	sockcbp->s_wslot = 0;
	sockcbp->s_flags = SF_FIRST_IN| SF_FIRST_OUT;

	_sock_ref++;

	sockcbp->s_event.events = 0;
	sockcbp->s_event.data.ptr = sockcbp;
	struct epoll_event *event0 = &sockcbp->s_event;
	error = epoll_ctl(_epoll_fd, EPOLL_CTL_ADD,
			sockcbp->s_file, event0);
	assert(error == 0);
	setnonblock(sockfd);
	return sockcbp;
}

int sock_detach(struct sockcb *detachp)
{
	int error;
	int s_flags = 0;
	int sockfd = -1;
	struct sockcb *sockcbp;
	assert(detachp->s_magic == SOCK_MAGIC);

	/* avoid warning */
	s_flags = s_flags;
	sockcbp = detachp;
	sockfd  = sockcbp->s_file;
	sockcbp->s_file = -1;

	_sock_ref--;

	error = epoll_ctl(_epoll_fd, EPOLL_CTL_DEL,
			sockfd, &sockcbp->s_event);
	assert(error == 0);
	free(sockcbp);
	return sockfd;
}

int getaddrbyname(const char *name, struct sockaddr_in *addr)
{
	char buf[1024];
	in_addr in_addr1;
	u_long peer_addr;
	char *port, *hostname;
	struct hostent *phost;
	struct sockaddr_in *p_addr;

	strcpy(buf, name);
	hostname = buf;

	port = strchr(buf, ':');
	if (port != NULL) {
		*port++ = 0;
	}

	p_addr = (struct sockaddr_in *)addr;
	p_addr->sin_family = AF_INET;
	p_addr->sin_port   = htons(port? atoi(port): 3478);

	peer_addr = inet_addr(hostname);
	if (peer_addr != INADDR_ANY &&
		peer_addr != INADDR_NONE) {
		p_addr->sin_addr.s_addr = peer_addr;
		return 0;
	}

	phost = gethostbyname(hostname);
	if (phost == NULL) {
		return -1;
	}

	memcpy(&in_addr1, phost->h_addr, sizeof(in_addr1));
	p_addr->sin_addr = in_addr1;
	return 0;
}

static void do_quick_scan(void *upp)
{
	int error;
	struct sockcb *sockcbp;

	int i;
	int nfds;
	struct epoll_event events[MAX_EVENTS];

	nfds = epoll_wait(_epoll_fd, events, MAX_EVENTS, slot_isbusy()? 0: 20);
	if (nfds == -1) {
		perror("epoll_pwait");
		return;
	}

	for (i = 0; i < nfds; i++) {
		int f_events = events[i].events;
		sockcbp = (struct sockcb *)events[i].data.ptr;

		if ((EPOLLIN | EPOLLERR) & f_events) {
			sockcbp->s_flags |= SF_READY_IN;
			if (sockcbp->s_rslot == NULL) { 
				sockcbp->s_event.events &= ~EPOLLIN;
				error = epoll_ctl(_epoll_fd, EPOLL_CTL_MOD,
						sockcbp->s_file, &sockcbp->s_event);
				assert(error == 0);
			} else {
				slot_wakeup(&sockcbp->s_rslot);
			}
		}

		if ((EPOLLOUT | EPOLLHUP | EPOLLERR) & f_events) {
			sockcbp->s_flags |= SF_READY_OUT;
			if (sockcbp->s_wslot == NULL) {
				sockcbp->s_event.events &= ~EPOLLOUT;
				error = epoll_ctl(_epoll_fd, EPOLL_CTL_MOD,
						sockcbp->s_file, &sockcbp->s_event);
				assert(error == 0);
			} else {
				slot_wakeup(&sockcbp->s_wslot);
			}
		}

		f_events &= ~(EPOLLOUT | EPOLLIN | EPOLLHUP | EPOLLERR);
		if (f_events != 0)
			printf("event: %x\n", f_events);
		assert(f_events == 0);
	}

}

int sock_read_wait(struct sockcb *sockcbp, struct waitcb *waitcbp)
{
	int error;
	int flags;
	assert(sockcbp->s_magic == SOCK_MAGIC);

	if (waitcb_active(waitcbp) &&
		   	waitcbp->wt_data == &sockcbp->s_rslot)
		return 0;

	slot_record(&sockcbp->s_rslot, waitcbp);
	flags = sockcbp->s_flags & (SF_READY_IN| SF_FIRST_IN);

	sockcbp->s_flags &= ~SF_FIRST_IN;
	if (flags == (SF_READY_IN| SF_FIRST_IN)) {
		slot_wakeup(&sockcbp->s_rslot);
		return 0;
	}

	if (sockcbp->s_flags & SF_CLOSE_IN) {
		slot_wakeup(&sockcbp->s_rslot);
		return 0;
	}
	
	sockcbp->s_event.events |= EPOLLIN;
	error = epoll_ctl(_epoll_fd, EPOLL_CTL_MOD,
			sockcbp->s_file, &sockcbp->s_event);
	assert(error == 0);
	waitcbp->wt_data = &sockcbp->s_rslot;
	return 0;
}

int sock_write_wait(struct sockcb *sockcbp, struct waitcb *waitcbp)
{
	int flags;
	int error;
	assert(sockcbp->s_magic == SOCK_MAGIC);

	if (waitcb_active(waitcbp) &&
			waitcbp->wt_data == &sockcbp->s_wslot)
		return 0;

	slot_record(&sockcbp->s_wslot, waitcbp);
	flags = sockcbp->s_flags & (SF_READY_OUT| SF_FIRST_OUT);

	sockcbp->s_flags &= ~SF_FIRST_OUT;
	if (flags == (SF_READY_OUT| SF_FIRST_OUT)) {
		slot_wakeup(&sockcbp->s_wslot);
		return 0;
	}

	if (sockcbp->s_flags & SF_CLOSE_OUT) {
		slot_wakeup(&sockcbp->s_wslot);
		return 0;
	}
	
	sockcbp->s_event.events |= EPOLLOUT;
	error = epoll_ctl(_epoll_fd, EPOLL_CTL_MOD,
			sockcbp->s_file, &sockcbp->s_event);
	assert(error == 0);
	waitcbp->wt_data = &sockcbp->s_wslot;
	return 0;
}

static struct waitcb _sock_waitcb;
static void module_init(void)
{
	int i;

	_epoll_fd = epoll_create(10);

	memset(&_sock_waitcb, 0, sizeof(_sock_waitcb));
	waitcb_init(&_sock_waitcb, do_quick_scan, 0);
	_sock_waitcb.wt_flags &= ~WT_EXTERNAL;
	_sock_waitcb.wt_flags |= WT_WAITSCAN;
	waitcb_switch(&_sock_waitcb);
}

static void module_clean(void)
{
	waitcb_clean(&_sock_waitcb);
	close(_epoll_fd);
}

struct module_stub slotsock_mod = {
	module_init, module_clean
};

int get_addr_by_name(const char *name, struct in_addr *ipaddr)
{
	struct hostent *host;
	host = gethostbyname(name);
	if (host == NULL)
		return -1;

	memcpy(ipaddr, host->h_addr_list[0], sizeof(*ipaddr));
	return 0;
}

static int blocking(int fd)
{
	return (WSAGetLastError() == WSAEWOULDBLOCK);
}

static int op_read(int fd, void *buf, int len)
{
	return recv(fd, (char *)buf, len, 0);
}

static int op_write(int fd, void *buf, int len)
{
	return send(fd, (char *)buf, len, 0);
}

static void read_wait(void *upp, struct waitcb *wait)
{
	sock_read_wait((struct sockcb *)upp, wait);
}

static void write_wait(void *upp, struct waitcb *wait)
{
	sock_write_wait((struct sockcb *)upp, wait);
}

static void do_shutdown(int file, int cond)
{
	if (cond != 0)
		shutdown(file, SHUT_WR);
	return;
}

struct sockop winsock_ops = {
	blocking, op_read, op_write, read_wait, write_wait, do_shutdown
};

