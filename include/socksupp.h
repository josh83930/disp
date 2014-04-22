/*
############################################################################
#                                                                          #
# Copyright (c) 1993-1994 CASPUR Consortium                                # 
#                         c/o Universita' "La Sapienza", Rome, Italy       #
# All rights reserved.                                                     #
#                                                                          #
# Permission is hereby granted, without written agreement and without      #
# license or royalty fees, to use, copy, modify, and distribute this       #
# software and its documentation for any purpose, provided that the        #
# above copyright notice and the following two paragraphs and the team     #
# reference appear in all copies of this software.                         #
#                                                                          #
# IN NO EVENT SHALL THE CASPUR CONSORTIUM BE LIABLE TO ANY PARTY FOR       #
# DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING  #
# OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE       #
# CASPUR CONSORTIUM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    #
#                                                                          #
# THE CASPUR CONSORTIUM SPECIFICALLY DISCLAIMS ANY WARRANTIES,             #
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY #
# AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER   #
# IS ON AN "AS IS" BASIS, AND THE CASPUR CONSORTIUM HAS NO OBLIGATION TO   #
# PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.   #
#                                                                          #
#       +----------------------------------------------------------+       #
#       |   The ControlHost Team: Ruten Gurin, Andrei Maslennikov  |       #
#       |   Contact e-mail      : ControlHost@caspur.it            |       #
#       +----------------------------------------------------------+       #
#                                                                          #
############################################################################
*/

/*
 $Id: socksupp.h,v 1.1.1.1 1999/01/07 18:43:15 qsno Exp $
*/

/* socket support routines for Unix,OS-9,VM,MVS,VMS */

#if !defined(OS9) && !defined(VMS) && !defined(VM) && !defined(MVS)
#define UNIX
#endif

#if defined(VM)
#include <manifest.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#ifdef UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#if !defined(AIX) || !defined(__cplusplus)
#include <netinet/in.h>
#endif
#else
#ifdef VMS
#include "multinet_root:[multinet.include.sys]types.h"
#include "multinet_root:[multinet.include.sys]socket.h"
#include "multinet_root:[multinet.include.netinet]in.h"
#else
#ifdef OS9
#include <sys/types.h>
#else
#include <types.h>
#endif
#include <in.h>
#include <socket.h>
#endif
#endif

#ifndef VMS
#include <time.h>
#include <errno.h>
#include <netdb.h>
#else
#include "multinet_root:[multinet.include.sys]time.h"
#include "multinet_root:[multinet.include.sys]ioctl.h"
#include "multinet_root:[multinet.include]errno.h"
#include "multinet_root:[multinet.include]netdb.h"
#endif

#ifdef OS9
#define assert(exp) ((exp)?0:(fprintf(stderr,\
	"\nAssertion failed: %s line %d: %s\n",__FILE__,__LINE__,#exp),exit(1)))
#else
#include <assert.h>
#endif

#ifdef OS9
#include <strings.h>
#else
#include <string.h>
#endif


#if defined(VM)
#include <inet.h>
#include <tcperrno.h>
#endif

#ifdef IRIX
#include <bstring.h>
#include <sys/stream.h>
#include <sys/select.h>
#endif

#ifdef AIX
#include <sys/uio.h>
#include <sys/stream.h>
#include <sys/select.h>
#include <net/nh.h>
#endif

#ifdef HP_UX
#include <sys/file.h>
#endif

#ifdef OSF1
#include <machine/endian.h> /* htons,ntohl... */
#endif

#ifdef Solaris
#include <strings.h>
#include <sys/file.h>
#include <unistd.h>
long gethostid (void);
int gethostname (char *name, int namelen);
#endif
/* Function prototypes */

#ifndef VMS
#define socket_errno errno
#define socket_read(s,buf,size) recv(s,buf,size,0)
#define socket_write(s,buf,size) send(s,buf,size,0)
#if defined(VM) || defined(MVS)
#define socket_perror(str) tcperror(str)
#define socket_close(s) sock_clo(s)
#else
#define socket_perror(msg) perror(msg)
#define socket_close(s) close(s)
#ifdef OS9
#define perror(msg) _errmsg(errno,msg)
#endif
#endif
#endif
 
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(VM) && !defined(MVS)
extern int errno;
#endif

#if defined(VMS) || defined(OSF1) || defined(OS9) || defined(AIX)

#ifndef bzero
#define bzero(buf,sz) memset(buf,0,sz)
#endif
#ifndef bcopy
#define bcopy(from,to,sz) memcpy(to,from,sz)
#endif

#endif

#ifdef OS9

  unsigned usleep( unsigned microseconds );

#endif

#ifdef HP_UX

  unsigned usleep( unsigned microseconds );

#endif

 
#ifdef IRIX

  unsigned usleep( unsigned microseconds );

#endif

#ifdef AIX

  int socket (int AddressFamily, int Type, int Protocol);
  int accept(int Socket, struct sockaddr *Name, int* NameLength);
  int listen(int Socket, int Backlog);
  int usleep( unsigned microseconds );
  int shutdown(int socket, int how);

#endif

#ifdef OSF1

  int select(
          int nfds,
          fd_set *readfds,
          fd_set *writefds,
          fd_set *exceptfds,
          struct timeval *timeout) ;
  int socket(
          int addr_family,
          int type,
          int protocol );
  unsigned int sleep (
          unsigned int seconds );
  unsigned usleep( unsigned microseconds );
  int accept(int Socket, struct sockaddr *Name, int* NameLength);
  int listen(int Socket, int Backlog);
  int shutdown(int socket, int how);

#endif

/* Functions defined here */

#ifdef VM
#define create_server_socket(p,q) servsocket(p,q)
#define create_client_socket(h,p) clientsocket(h,p)
#define accept_client(s,h,hi) acceptclient(s,h,hi)
#endif

#ifdef UNIX
int create_client_socket_no_wait(const char *hostname, int port, int maxdelay);
#endif
int create_server_socket(int port, int queue_length);
int create_client_socket(const char *hostname, int port);
int accept_client(int socket, char *host, int *hostid);
void shut_line(int socket);
int is_number(const char *s);
int resolve_service(char *name_or_number, char **name);
int set_nowait(int socket, int value);
int extract_nowait(int socket);
int getblock(int socket, void *buf, int bsize, int *pos);
int putblock(int socket, const void *buf, int bsize, int *pos);
int getbwait(int socket, void *buf, int bsize);
int putbwait(int socket, const void *buf, int bsize);
int skipblock(int socket, int *count);
int skipbwait(int socket, int count);

#ifdef UNIX
int my_inet_addr(void);
int his_inet_addr(const char *host);
#endif

#ifdef __cplusplus
}
#endif
