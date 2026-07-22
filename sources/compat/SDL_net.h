#ifndef __F1SPIRIT_SDL_NET_COMPAT_H
#define __F1SPIRIT_SDL_NET_COMPAT_H

/*
 * F1 Spirit Remake - classic SDL_net (SDL 1.2 / SDL2 era) compatibility
 * shim, implemented directly on top of BSD sockets / Winsock2.
 *
 * WHY THIS EXISTS: SDL3 does not ship SDL_net at all. Anthropic's/libsdl's
 * successor satellite library, SDL3_net, replaced the old blocking
 * IPaddress/TCPsocket/UDPsocket/SDLNet_SocketSet API with a completely
 * different asynchronous design (NET_* functions, NET_StreamSocket, etc).
 * Porting F1 Spirit's ~700-line online lobby/chat protocol (state_menu.cpp)
 * to that new API would mean redesigning the whole networking state
 * machine - a separate project in itself, and one that can't really be
 * tested any more since the original lobby servers
 * (braingames.getput.com) are long gone.
 *
 * Instead, this header re-implements the small subset of the old
 * SDLNet_* API that this codebase actually calls, directly on raw
 * sockets. Every existing #include "SDL_net.h" in the source now points
 * here (via the migration pass), so state_menu.cpp and friends compile
 * completely unchanged.
 *
 * This is plain portable C++ (no SDL3 types needed beyond Uint8/16/32),
 * so it can't clash with SDL3 headers.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef uint8_t  Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;

#ifdef _WIN32
	#include <winsock2.h>
	#include <ws2tcpip.h>
	typedef int socklen_t_compat;
	#pragma comment(lib, "ws2_32.lib")
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <errno.h>
	#include <fcntl.h>
	typedef int SOCKET;
	#define INVALID_SOCKET (-1)
	#define SOCKET_ERROR (-1)
	#define closesocket close
#endif

/* ------------------------------------------------------------------ */

struct IPaddress {
	Uint32 host; /* network byte order */
	Uint16 port; /* network byte order */
};

/* Common header shared by TCPsocket/UDPsocket so a socket set can hold
   either kind, mirroring the original SDLNet_GenericSocket trick. */
struct SDLNet_GenericSocketData {
	SOCKET fd;
	int ready;
};

struct _TCPsocket : SDLNet_GenericSocketData {
	IPaddress remote_address;
	bool is_server;
};
typedef _TCPsocket *TCPsocket;

struct _UDPsocket : SDLNet_GenericSocketData {
	Uint16 port;
};
typedef _UDPsocket *UDPsocket;

struct UDPpacket {
	int channel;
	Uint8 *data;
	int len;
	int maxlen;
	int status;
	IPaddress address;
};

struct _SDLNet_SocketSet {
	int maxsockets;
	int numsockets;
	SDLNet_GenericSocketData **sockets;
};
typedef _SDLNet_SocketSet *SDLNet_SocketSet;

/* ------------------------------------------------------------------ */

static char g_sdlnet_error[256] = "";

static inline const char *SDLNet_GetError(void)
{
	return g_sdlnet_error;
}

static inline void SDLNet_SetError_compat(const char *msg)
{
	strncpy(g_sdlnet_error, msg, sizeof(g_sdlnet_error) - 1);
	g_sdlnet_error[sizeof(g_sdlnet_error) - 1] = 0;
}

static inline int SDLNet_Init(void)
{
#ifdef _WIN32
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		SDLNet_SetError_compat("WSAStartup failed");
		return -1;
	}
#endif
	return 0;
}

static inline void SDLNet_Quit(void)
{
#ifdef _WIN32
	WSACleanup();
#endif
}

static inline void SDLNet_SetNonBlocking(SOCKET fd, bool nonblock)
{
#ifdef _WIN32
	u_long mode = nonblock ? 1 : 0;
	ioctlsocket(fd, FIONBIO, &mode);
#else
	int flags = fcntl(fd, F_GETFL, 0);
	if (nonblock)
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	else
		fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
#endif
}

static inline int SDLNet_ResolveHost(IPaddress *address, const char *host, Uint16 port)
{
	address->port = htons(port);

	if (host == NULL || host[0] == 0) {
		address->host = htonl(INADDR_ANY);
		return 0;
	}

	struct addrinfo hints;
	struct addrinfo *res = NULL;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(host, NULL, &hints, &res) != 0 || res == NULL) {
		SDLNet_SetError_compat("could not resolve host");
		return -1;
	}

	struct sockaddr_in *sin = (struct sockaddr_in *)res->ai_addr;
	address->host = sin->sin_addr.s_addr;
	freeaddrinfo(res);
	return 0;
}

static inline Uint32 SDLNet_Read32(const void *area)
{
	const Uint8 *p = (const Uint8 *)area;
	return ((Uint32)p[0] << 24) | ((Uint32)p[1] << 16) | ((Uint32)p[2] << 8) | (Uint32)p[3];
}

static inline void SDLNet_Write32(Uint32 value, void *area)
{
	Uint8 *p = (Uint8 *)area;
	p[0] = (Uint8)(value >> 24);
	p[1] = (Uint8)(value >> 16);
	p[2] = (Uint8)(value >> 8);
	p[3] = (Uint8)(value);
}

static inline Uint16 SDLNet_Read16(const void *area)
{
	const Uint8 *p = (const Uint8 *)area;
	return (Uint16)(((Uint32)p[0] << 8) | (Uint32)p[1]);
}

static inline void SDLNet_Write16(Uint16 value, void *area)
{
	Uint8 *p = (Uint8 *)area;
	p[0] = (Uint8)(value >> 8);
	p[1] = (Uint8)(value);
}

/* ---- TCP ---- */

static inline TCPsocket SDLNet_TCP_Open(IPaddress *ip)
{
	SOCKET fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == INVALID_SOCKET) {
		SDLNet_SetError_compat("socket() failed");
		return NULL;
	}

	TCPsocket sock = new _TCPsocket();
	sock->fd = fd;
	sock->ready = 0;
	sock->is_server = false;

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = ip->port;
	addr.sin_addr.s_addr = ip->host;

	if (ip->host == htonl(INADDR_ANY)) {
		/* No remote host given: open as a listening (server) socket. */
		int reuse = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));

		if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0 ||
		        listen(fd, 16) != 0) {
			SDLNet_SetError_compat("bind()/listen() failed");
			closesocket(fd);
			delete sock;
			return NULL;
		}

		sock->is_server = true;
	} else {
		if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
			SDLNet_SetError_compat("connect() failed");
			closesocket(fd);
			delete sock;
			return NULL;
		}

		sock->remote_address = *ip;
	}

	return sock;
}

static inline TCPsocket SDLNet_TCP_Accept(TCPsocket server)
{
	struct sockaddr_in remote;
	socklen_t len = sizeof(remote);

	SOCKET fd = accept(server->fd, (struct sockaddr *)&remote, &len);
	if (fd == INVALID_SOCKET)
		return NULL;

	SDLNet_SetNonBlocking(fd, true);

	TCPsocket sock = new _TCPsocket();
	sock->fd = fd;
	sock->ready = 0;
	sock->is_server = false;
	sock->remote_address.host = remote.sin_addr.s_addr;
	sock->remote_address.port = remote.sin_port;
	return sock;
}

static inline int SDLNet_TCP_Send(TCPsocket sock, const void *data, int len)
{
	int sent = send(sock->fd, (const char *)data, len, 0);
	return sent;
}

static inline int SDLNet_TCP_Recv(TCPsocket sock, void *data, int maxlen)
{
	int n = recv(sock->fd, (char *)data, maxlen, 0);
	if (n <= 0)
		return (n == 0) ? -1 : ((errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : -1);
	return n;
}

static inline void SDLNet_TCP_Close(TCPsocket sock)
{
	if (!sock)
		return;
	closesocket(sock->fd);
	delete sock;
}

static inline IPaddress *SDLNet_TCP_GetPeerAddress(TCPsocket sock)
{
	return &sock->remote_address;
}

/* ---- UDP ---- */

static inline UDPsocket SDLNet_UDP_Open(Uint16 port)
{
	SOCKET fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == INVALID_SOCKET)
		return NULL;

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		closesocket(fd);
		return NULL;
	}

	SDLNet_SetNonBlocking(fd, true);

	UDPsocket sock = new _UDPsocket();
	sock->fd = fd;
	sock->ready = 0;
	sock->port = port;
	return sock;
}

static inline void SDLNet_UDP_Close(UDPsocket sock)
{
	if (!sock)
		return;
	closesocket(sock->fd);
	delete sock;
}

static inline int SDLNet_UDP_Send(UDPsocket sock, int /*channel*/, UDPpacket *packet)
{
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = packet->address.port;
	addr.sin_addr.s_addr = packet->address.host;

	int sent = sendto(sock->fd, (const char *)packet->data, packet->len, 0,
	                   (struct sockaddr *)&addr, sizeof(addr));
	return (sent == packet->len) ? 1 : 0;
}

static inline int SDLNet_UDP_Recv(UDPsocket sock, UDPpacket *packet)
{
	struct sockaddr_in from;
	socklen_t fromlen = sizeof(from);

	int n = recvfrom(sock->fd, (char *)packet->data, packet->maxlen, 0,
	                  (struct sockaddr *)&from, &fromlen);

	if (n <= 0)
		return 0;

	packet->len = n;
	packet->address.host = from.sin_addr.s_addr;
	packet->address.port = from.sin_port;
	return 1;
}

static inline UDPpacket *SDLNet_AllocPacket(int size)
{
	UDPpacket *p = new UDPpacket();
	p->data = new Uint8[size];
	p->maxlen = size;
	p->len = 0;
	p->channel = -1;
	return p;
}

static inline void SDLNet_FreePacket(UDPpacket *p)
{
	if (!p)
		return;
	delete[] p->data;
	delete p;
}

/* ---- Socket sets (used to multiplex several TCP/UDP sockets with select()) ---- */

static inline SDLNet_SocketSet SDLNet_AllocSocketSet(int maxsockets)
{
	_SDLNet_SocketSet *set = new _SDLNet_SocketSet();
	set->maxsockets = maxsockets;
	set->numsockets = 0;
	set->sockets = new SDLNet_GenericSocketData *[maxsockets];
	return set;
}

static inline void SDLNet_FreeSocketSet(SDLNet_SocketSet set)
{
	if (!set)
		return;
	delete[] set->sockets;
	delete set;
}

static inline int SDLNet_AddSocket_generic(SDLNet_SocketSet set, SDLNet_GenericSocketData *sock)
{
	if (set->numsockets >= set->maxsockets)
		return -1;
	set->sockets[set->numsockets++] = sock;
	return set->numsockets;
}

static inline int SDLNet_TCP_AddSocket(SDLNet_SocketSet set, TCPsocket sock)
{
	return SDLNet_AddSocket_generic(set, sock);
}

static inline int SDLNet_UDP_AddSocket(SDLNet_SocketSet set, UDPsocket sock)
{
	return SDLNet_AddSocket_generic(set, sock);
}

static inline int SDLNet_DelSocket_generic(SDLNet_SocketSet set, SDLNet_GenericSocketData *sock)
{
	for (int i = 0; i < set->numsockets; i++) {
		if (set->sockets[i] == sock) {
			for (int j = i; j < set->numsockets - 1; j++)
				set->sockets[j] = set->sockets[j + 1];
			set->numsockets--;
			return set->numsockets;
		}
	}
	return -1;
}

static inline int SDLNet_TCP_DelSocket(SDLNet_SocketSet set, TCPsocket sock)
{
	return SDLNet_DelSocket_generic(set, sock);
}

static inline int SDLNet_UDP_DelSocket(SDLNet_SocketSet set, UDPsocket sock)
{
	return SDLNet_DelSocket_generic(set, sock);
}

static inline int SDLNet_CheckSockets(SDLNet_SocketSet set, Uint32 timeout_ms)
{
	fd_set readfds;
	FD_ZERO(&readfds);

	SOCKET maxfd = 0;
	for (int i = 0; i < set->numsockets; i++) {
		set->sockets[i]->ready = 0;
		FD_SET(set->sockets[i]->fd, &readfds);
		if (set->sockets[i]->fd > maxfd)
			maxfd = set->sockets[i]->fd;
	}

	struct timeval tv;
	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec = (timeout_ms % 1000) * 1000;

	int n = select((int)maxfd + 1, &readfds, NULL, NULL, &tv);
	if (n <= 0)
		return n;

	int ready_count = 0;
	for (int i = 0; i < set->numsockets; i++) {
		if (FD_ISSET(set->sockets[i]->fd, &readfds)) {
			set->sockets[i]->ready = 1;
			ready_count++;
		}
	}
	return ready_count;
}

static inline int SDLNet_SocketReady(void *sock)
{
	return sock ? ((SDLNet_GenericSocketData *)sock)->ready : 0;
}

#endif /* __F1SPIRIT_SDL_NET_COMPAT_H */
