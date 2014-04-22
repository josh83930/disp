/*##########################################################################*/
/*                                                                          */
/* Copyright (c) 1993-1994 CASPUR Consortium                                */
/*                         c/o Universita' "La Sapienza", Rome, Italy       */
/* All rights reserved.                                                     */
/*                                                                          */
/* Permission is hereby granted, without written agreement and without      */
/* license or royalty fees, to use, copy, modify, and distribute this       */
/* software and its documentation for any purpose, provided that the        */
/* above copyright notice and the following two paragraphs and the team     */
/* reference appear in all copies of this software.                         */
/*                                                                          */
/* IN NO EVENT SHALL THE CASPUR CONSORTIUM BE LIABLE TO ANY PARTY FOR       */
/* DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING  */
/* OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE       */
/* CASPUR CONSORTIUM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    */
/*                                                                          */
/* THE CASPUR CONSORTIUM SPECIFICALLY DISCLAIMS ANY WARRANTIES,             */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY */
/* AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER   */
/* IS ON AN "AS IS" BASIS, AND THE CASPUR CONSORTIUM HAS NO OBLIGATION TO   */
/* PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.   */
/*                                                                          */
/*       +----------------------------------------------------------+       */
/*       |   The ControlHost Team: Ruten Gurin, Andrei Maslennikov  |       */
/*       |   Contact e-mail      : ControlHost@caspur.it            |       */
/*       +----------------------------------------------------------+       */
/*                                                                          */
/*##########################################################################*/

/*
  $Id: disploop.c,v 1.14 1995/09/25 18:47:12 ruten Exp $

  $Log: disploop.c,v $
  Revision 1.14  1995/09/25 18:47:12  ruten
  Small change.

  Revision 1.13  1995/09/24 21:05:48  ruten
  Small change.

 * Revision 1.12  1995/05/29  23:56:22  ruten
 * disploop logic introduced
 *
 * Revision 1.11  1995/05/10  14:01:27  ruten
 * Socket starts in waiting mode now.
 *
 * Revision 1.10  1995/05/10  08:54:17  ruten
 * Tcl/Disp communications support moved to separate file to allow
 * its using outside tcldisp module.
 * A lot of internal changes in getdata.c, e.g. get_data_addr now
 * works not only on Unix and may do mallocs.
 *
 * Revision 1.9  1995/04/25  17:48:16  ruten
 * Options.Makefile moved to patterns directory
 *
 * Revision 1.8  1995/04/24  19:44:41  gurin
 * Id inserted.
 *
 * Revision 1.7  1995/04/19  10:23:28  ruten
 * Routines dispchannels,dispqhost added.
 *
 * Revision 1.6  1995/03/21  20:20:42  gurin
 * Unused variables removed
 *
 * Revision 1.5  1995/03/03  12:12:39  ruten
 * Errors in dispselect and wait_head_/check_head_ removed.
 *
 * Revision 1.4  1995/02/16  14:13:55  ruten
 * put_head/get_head routines removed.
 *
 * Revision 1.3  1995/02/14  16:34:03  ruten
 * Routines dispconnect/dispselect/... inserted.
 *
 * Revision 1.2  1995/02/13  10:56:25  ruten
 * makefiles are changed, option -q added to bdexpander
 *
 * Revision 1.1  1995/01/31  14:57:12  ruten
 * disploop.c added
 *
*/

#include "displowl.h"

#if defined(VM) || defined(MVS)

#include "ascebcd.h"

char *dispconvert(const char *str)
  {
  int L = strlen(str);
  char *p = malloc(L+1);
  assert( p != NULL );
  memcpy(p,str,L);
  p[L] = 0;
  mem2ascii(p,L);
  return p;
  }
#define cnvrt(str) str = dispconvert(str) 
#define freecnvrt(str) free(str)
#else
#define cnvrt(str)
#define freecnvrt(str)
#endif

#ifndef OS9
static fd_set active;
static fd_width = 0;
#endif

typedef struct h_s {
  struct h_s *next;
  int prio;
  int socket;
  int pos;
  int asked;
  char *host;
  prefix_t pref;
} h_t;

static h_t *root = NULL;

#define DISP_ERRSOCK (-1)

#define findit(p,sock) for(p=root; p!=NULL && p->socket != sock; p=p->next)

int dispconnect(const char *host)
  {
  h_t *p;
  int socket = create_client_socket(host,DISPATCH_PORT);
  if( socket < 0 )
    return DISP_ERRSOCK;
  p = (h_t *)malloc(sizeof(h_t));
  if( p == NULL )
    {
    shut_line(socket);
    return DISP_ERRSOCK;
    }
  p->next = root;
  root = p;
  p->prio = 0;
  p->socket = socket;
  p->pos = 0;
  p->asked = 0;
  if( host == NULL )
    host = "local";
  p->host = malloc(strlen(host)+1);
  if( p->host != NULL )
    strcpy(p->host,host);  
#ifndef OS9
  FD_SET(socket,&active);
  if( socket >= fd_width )
    fd_width = socket+1;
#endif
  return socket;
  }

int dispdrop(int socket)
  {
  h_t *p;
  findit(p,socket);
  if( p == NULL )
    return -1;
  if( p == root )
    root = p->next;
  else
    {
    h_t *prev;
    for(prev=root; prev->next != p; prev = prev->next)
      assert( prev != NULL && prev->next != NULL );
    prev->next = p->next;
    }
  if( p->host != NULL ) free(p->host);
  free((char *)p);

  shut_line(socket);
#ifndef OS9
  FD_CLR(socket,&active);
  if( socket+1 == fd_width )
    {
    int i;
    for(i = fd_width-1; i >= 0 && !FD_ISSET(i,&active); i--)
      ;
    fd_width = i+1;
    }
#endif
  return 0;
  }

void dispalldrop(void)
  {
  while( root != NULL )
    {
    h_t *p = root->next;
    shut_line(p->socket);
    if( root->host != NULL ) free(root->host);
    free((char *)root);
    root = p;
    }
#ifndef OS9
  FD_ZERO(&active);
  fd_width = 0;
#endif
  }

int dispchannels(void)
  {
  h_t *p;
  int cnt;
  for(p=root,cnt=0; p!= NULL; p=p->next,cnt++)
    ;
  return cnt;
  }

const char *dispqhost(int socket)
  {
  h_t *p;
  findit(p,socket);
  if( p == NULL )
    return NULL;
  else
    return p->host;
  }

int dispsend(int socket, const char *tag, void *data, int datalen)
  {
  h_t *p;
  findit(p,socket);
  if( p == NULL )
    return -1;
  return put_tagged_bwait(p->socket,tag,data,datalen);
  }

int dispsubscribe(int socket, const char *subscr)
  {
  h_t *p;
  int rc;
  findit(p,socket);
  if( p == NULL )
    return -1;
  if( subscr == NULL )
    subscr = ""; 
  cnvrt(subscr);
  rc = put_tagged_bwait(p->socket,DISPTAG_Subscribe,subscr,strlen(subscr));
  freecnvrt(subscr);
  return rc;
  }

int dispmyid(int socket, const char *id)
  {
  h_t *p;
  int rc;
  findit(p,socket);
  if( p == NULL )
    return -1;
  if( id == NULL )
    id = "";
  cnvrt(id);
  rc = put_tagged_bwait(p->socket,DISPTAG_MyId,id,strlen(id));
  freecnvrt(id);
  return rc;
  }

int dispalways(int socket)
  {
  h_t *p;
  findit(p,socket);
  if( p == NULL )
    return -1;
  if( p->asked < 0 )
    return 1;
  p->asked = -1;
  return put_tagged_bwait(p->socket,DISPTAG_Always,NULL,0);
  }

int dispnext(int socket)
  {
  h_t *p;
  findit(p,socket);
  if( p == NULL )
    return -1;
  if( p->asked )
    return 1;
  p->asked = 1;
  return put_tagged_bwait(p->socket,DISPTAG_Gime,NULL,0);
  }

void dispallnext(void)
  {
  h_t *p;
  for(p=root; p!= NULL; p=p->next)
    if( ! p->asked )
      {
      p->asked = 1;
      put_tagged_bwait(p->socket,DISPTAG_Gime,NULL,0);
      }
  }

int dispprio(int socket, int prio)
  {
  h_t *p;
  findit(p,socket);
  if( p == NULL )
    return -1;
  p->prio = prio;
  return 0;
  }

int dispselect(int sec, int microsec)
  {
#ifndef OS9
  fd_set readmask;
  int selret;
  h_t *p, *bestp;
  int s;
  struct timeval *interv;
  struct timeval timeout;
#endif

  if( root == NULL )
    return -2;

#ifdef OS9
  return root->socket;
#else

  if( sec < 0 || microsec < 0 )
    interv = NULL;
  else
    {
    if( microsec >= 1000000 )
      {
      sec += microsec/1000000;
      microsec = microsec%1000000;
      }
    interv = &timeout;
    timeout.tv_sec  = sec;
    timeout.tv_usec = microsec;
    }

  memcpy(&readmask,&active,sizeof(readmask));

  selret = select(fd_width, &readmask, NULL, NULL, interv);
  /* EINTR happens when the process is stopped */
  if (selret < 0 && errno != EINTR )
    return -2;
  if( selret <= 0 )
    return -1;

  for(p=root,bestp=NULL,s=(-1); p!= NULL; p=p->next)
    if( s = p->socket, FD_ISSET(s,&readmask) )
      if( bestp == NULL || p->prio > bestp->prio )
        bestp = p;

  assert( bestp != NULL );
  return bestp->socket;
#endif
  } 

int dispcheck(int socket, char *tag, int *size, int wait)
  {
  int rc;
  h_t *p;

  findit(p,socket);
  if( p == NULL )
    return -1;
  if( wait )
    {
    rc = getbwait(socket,((char *) (&p->pref)) + p->pos, sizeof(prefix_t) - p->pos);
    }
  else
    {
    int setrc = set_nowait(socket,1);
    rc = getblock(socket,&p->pref,sizeof(prefix_t),&p->pos);
    if( setrc == 0 )
      set_nowait(socket,0);
    }
  if( rc <= 0 )
    return rc;

  p->pos = 0;
  if( p->asked == 1 )
    p->asked = 0;
  fromprefix(&p->pref,tag,size);
  return 1;
  }
