/*
 
Header: socketp.c[1.4] Sun Aug  9 03:48:03 1992 nickel@cs.tu-berlin.de proposed
This file is part of socket(1).
Copyright (C) 1992 by Juergen Nickelsen <nickel@cs.tu-berlin.de>
except the file siglist.c, which is Copyright (C) 1989 Free Software
Foundation, Inc.

This applies to the Socket program, release Socket-1.0.

Socket is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 1, or (at your option) any
later version.

It is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You may have received a copy of the GNU General Public License along
with GNU Emacs or another GNU program; see the file COPYING.  If not,
write to me at the electronic mail address given above or to 
Juergen Nickelsen, Hertzbergstr. 28, 1000 Berlin 44, Germany.

*/

/*
Modified by Ruten Gurin, <ruten@caspur.it>.
    setsockopt call included.
    Adjusted for non-Unix platforms (OS-9,VMS,VM,MVS), 
    routines my_inet_addr,his_inet_addr,set_nowait,extract_nowait added
    Unused parameter 'protocol' in resolve_service eliminated
    Routine create_client_socket_no_wait added.
*/

#include "socksupp.h"
 
#if defined(UNIX) || defined(VM) || defined(MVS)
#include <fcntl.h>
#endif

#ifdef VMS
/* I don't know the way to get nowait status on VMS */
/* so I keep it explicitly */
static fd_set nowait_bits;
static int fd_set_width = 0;
#endif
 
/*
 * create a server socket on PORT accepting QUEUE_LENGTH connections
 */
int create_server_socket(int port, int queue_length)
{
    struct sockaddr_in sa ;
    int s;
    int on;
 
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	return -1 ;
    }
    
    on = 1;
    if( setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(const char *)&on,sizeof(on)) < 0 )
      perror("Set REUSEADDR");

    bzero((char *) &sa, sizeof(sa)) ;
    sa.sin_family = AF_INET ;
    sa.sin_addr.s_addr = htonl(INADDR_ANY) ;
    sa.sin_port = htons((u_short)port) ;
 
    if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
	return -1 ;
    }
    if (listen(s, queue_length) < 0) {
	return -1 ;
    }
#ifdef VMS
    FD_CLR(s,&nowait_bits);
    if( s >= fd_set_width )
      fd_set_width = s+1;
#endif
 
    return s ;
}
 
 
/* create a client socket connected to PORT on HOSTNAME */
int create_client_socket(const char * hostname, int port)
{
    struct sockaddr_in sa ;
    struct hostent *hp ;
    int s ;
    int addr ;
    int on;
 
#ifdef OS9
{ /* os9 linker needs this stuff */
  char *name;
  short tl;
  mod_exec *mod;
  if( 0 ) {
    modlink(name,tl);
    modload(name,tl);
    munlink(mod);
  }
}
#endif
 
 
    if( hostname == (char *)0 || strcmp(hostname,"local") == 0 )
	hostname = "";  /* local connection */
 
    bzero((char *)&sa, sizeof(sa)) ;
    if ((addr = inet_addr(hostname)) != -1) {
	/* is Internet addr in octet notation */
	bcopy((char *)&addr, (char *)&sa.sin_addr, 
		sizeof(addr)) ; /* set address */
	sa.sin_family = AF_INET ;
    } else {
	/* do we know the host's address? */
	if ((hp = gethostbyname(hostname)) == NULL) {
	    return -2 ;
	}
	hostname = hp->h_name ;
	bcopy(hp->h_addr, (char *)&sa.sin_addr, hp->h_length) ;
	sa.sin_family = hp->h_addrtype ;
    }
 
    sa.sin_port = htons((u_short) port) ;
 
    if ((s = socket(sa.sin_family, SOCK_STREAM, 0)) < 0) { /* get socket */
	return -1 ;
    }
    if (connect(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {  /* connect */
	socket_close(s) ;
	return -1 ;
    }
    on = 1;
    if( setsockopt(s,SOL_SOCKET,SO_KEEPALIVE,(const char *)&on,sizeof(on)) < 0 )
      perror("Set KEEPALIVE");
#ifdef VMS
    FD_CLR(s,&nowait_bits);
    if( s >= fd_set_width )
      fd_set_width = s+1;
#endif
    return s ;
}

#if defined(UNIX)

#include <signal.h>
#include <time.h>
#include <unistd.h>

static void alarm_handler(int sig)
{
/* Do nothing */
}

typedef void sigproc_t(int sig);

int create_client_socket_no_wait(const char * hostname, int port, int delay)
{
    int res;
#if defined(SIG_SETMASK) && (!defined(Linux) && !defined(Solaris)) 
    static struct sigvec svec = { NULL, ~0, 0 } ;
    static struct sigvec ovec;
    svec.sv_handler = alarm_handler;
    sigvec(SIGALRM, &svec, &ovec) ;
#else
    sigproc_t *op = signal(SIGALRM,alarm_handler);
#endif
    alarm(delay);
    res = create_client_socket(hostname,port);
#if defined(SIG_SETMASK) && (!defined(Linux) && !defined(Solaris))
    sigvec(SIGALRM, &ovec, NULL) ;
#else
    signal(SIGALRM,op);
#endif
    return res;
}
#endif  
 
/* check if the string is a number */
int is_number(register const char *s)
{
   register char c;
   while( (c = *s++) != 0 )
      if( c < '0' || c > '9' )
          return 0;
   return 1;
}
 
/* return the port number for service NAME_OR_NUMBER. If NAME is non-null,
 * the name is the service is written there.
 */
int resolve_service(char *name_or_number, char **name)
{
    struct servent *servent ;
    int port ;
 
    if (is_number(name_or_number)) {
	port = atoi(name_or_number) ;
	if (name != NULL) {
	    servent = getservbyport(htons((u_short)port), "tcp") ;
	    if (servent != NULL) {
		*name = servent->s_name ;
	    } else {
		*name = NULL ;
	    }
	}
	return port ;
    } else {
	servent = getservbyname(name_or_number, "tcp") ;
	if (servent == NULL) {
	    return -1 ;
	}
	if (name != NULL) {
	    *name = servent->s_name ;
	}
	return ntohs(servent->s_port) ;
    }
}
 
/* accept the connection on listening port */
int accept_client(int sx, char *host, int *hostid)
   {
   /* sx is the port with connection pending */
   /* host is the buffer for connected host name */
   /* returned value - newly created connected socket */
   /*   negative value means that connection failed */
   int size = sizeof (struct sockaddr_in);
   struct sockaddr_in to;
   int s;
   int on;
 
   s = accept(sx, (struct sockaddr *)&to, &size);
   if( s >= 0 )
     {
     struct hostent *he ;
     unsigned norder ;
 
     he = gethostbyaddr((char *)&to.sin_addr.s_addr,
                                   size, AF_INET) ;
     *hostid = htonl(to.sin_addr.s_addr) ;
     if (!he || !he->h_name)
        {
        norder = *hostid;
        sprintf(host, "%u.%u.%u.%u%c",
                            norder >> 24,
                           (norder >> 16) & 0xff,
                           (norder >> 8) & 0xff,
                            norder & 0xff,0) ;
        }
     else
        strcpy(host,he->h_name) ;
     on = 1;
     if( setsockopt(s,SOL_SOCKET,SO_KEEPALIVE,(const char *)&on,sizeof(on)) < 0 )
       perror("Set KEEPALIVE");
#ifdef VMS
     FD_CLR(s,&nowait_bits);
     if( s >= fd_set_width )
       fd_set_width = s+1;
#endif
     }
   else
     *host = 0;
   return s;
   }
 
void shut_line(int socket)
  {
  if( socket >= 0 )
    {
    shutdown(socket,2);
    socket_close(socket);
    }
  }

int extract_nowait(int socket)
{
#ifdef VMS
  if( socket < 0 || socket >= fd_set_width )
    return -1;
  else if( FD_ISSET(socket,&nowait_bits) )
    return 1;
  else
    return 0;
#else
#ifdef OS9
  struct sgbuf buf;
  if( _gs_opt(socket,&buf) < 0 )
    return -1;
  else if( buf.sg_noblock )
    return 1;
  else
    return 0;
#else
#ifdef HP_UX
  int mask = O_NONBLOCK;
#else
  int mask = FNDELAY;
#endif
  int flags = fcntl(socket,F_GETFL,mask);
  if( flags == -1 )
    return -1;
  else if( flags )
    return 1;
  else
    return 0;;
#endif
#endif
  }
 
/* set blocking status of socket */
int set_nowait(int socket, int value)
{
#ifdef VMS
  unsigned int nonblock_enable = value;
  if( socket_ioctl(socket,FIONBIO,&nonblock_enable) < 0 )
    return -1;
  if( socket > 0 && socket < fd_set_width )
    if( value )
      {
      if( FD_ISSET(socket,&nowait_bits) )
        return 1;
      FD_SET(socket,&nowait_bits)
      }
    else
      {
      if( !FD_ISSET(socket,&nowait_bits) )
        return 1;
      FD_CLR(socket,&nowait_bits);
      }
  return 0;
#else
#ifdef OS9
  struct sgbuf buf;
  if( _gs_opt(socket,&buf) < 0 )
    return -1;
  if( value ) value = 1; /* normalize */
  if( buf.sg_noblock == value )
    return 1;
  buf.sg_noblock = value;
  if( _ss_opt(socket,&buf) < 0 )
    return -1;
  else
    return 0;;
#else
  int flags = fcntl(socket,F_GETFL,-1);
#ifdef HP_UX
  int mask = O_NONBLOCK;
#else
  int mask = FNDELAY;
#endif
  if( flags == -1 )
    return -1;
  if( value ) value = mask;
  if( (flags & mask) == value )
    return 1;
  return fcntl(socket,F_SETFL,flags ^ mask); /* xor */
#endif
#endif
  }

#ifdef UNIX
int his_inet_addr(const char *host)
  {
  int rc = 0;
  struct hostent *he;

  he = gethostbyname(host);
  endhostent();

  if( he && he->h_addr_list && he->h_addr_list[0] )
    rc = htonl(*((int *)(he->h_addr_list[0])));

  return rc;
  }

int my_inet_addr()
  {
  static int rc = 0;
  char host[200];

  if( rc > 0 )
    return rc;

#ifndef HP_UX
  rc = gethostid();
  if( rc )
    return rc;
#endif

  if( gethostname(host,sizeof(host)) != 0 )
    return 0;

  return his_inet_addr(host);
  }
#endif
 
/*EOF*/
