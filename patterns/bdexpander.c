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
 $Id: bdexpander.c,v 1.5 1995/05/10 14:01:21 ruten Exp $
*/

/* 
  Gets local events Born/Died <nickname> and sends them
  to several hosts (from argument list)
  as events RemBorn/RemDied <nickname> <local_host_name>
*/

#include "displowl.h"

static char buffer[300];

typedef struct x_s {
  struct x_s *next;
  int iaddr;
  char *host;
  int socket;
} x_t;

void help(char *progname)
  {
  fprintf(stderr,"Format: %s [ -q ] host...\n",progname);
  exit(1);
  }

/********************************************************************/
int main (int argc, char *argv[])
  {
  x_t *queue = NULL;
  int me = my_inet_addr();
  int socket;
  prefix_t pref;
  char myhost[100];
  int L;
  x_t *x;
  int quiet = 0;
  char *myid = "BornDiedExpander";
  char *subs = "a "DISPTAG_Born" a "DISPTAG_Died;
  char *progname = argv[0];
  int  first = 1;

  if( argc >= 2 && argv[1][1] == '-' )
    if( strcmp(argv[1],"-q") == 0 )
      {
      quiet = 1;
      argc--;
      argv++;
      }
    else
      help(progname);

  if( argc < 2 )
    help(progname);

  if( gethostname(myhost,sizeof(myhost)) < 0 )
    {
    perror(progname);
    exit(1);
    }

  myhost[sizeof(myhost)-1] = 0;
  L = strlen(myhost);

  socket = create_client_socket(NULL,DISPATCH_PORT);

  if( socket < 0 ||
    put_tagged_bwait(socket,DISPTAG_Subscribe,subs,strlen(subs)) <= 0 ||
    put_tagged_bwait(socket,DISPTAG_Always,NULL,0) <= 0 ||
    put_tagged_bwait(socket,DISPTAG_MyId,myid,strlen(myid)) <= 0 )
    {
    perror(progname);
    if( socket >= 0 )
      shut_line(socket);
    exit(1);
    }

  for( ;argc > 1; argc--)
    {
    char *host = argv[argc-1];
    int iaddr = his_inet_addr(host);
    if( iaddr == me || iaddr == 0 || iaddr == 0x7f000001 )
      {
      if( !quiet )
        fprintf(stderr,"%s: Local host name %s ignored in argument list\n",
          progname,host);
      continue;
      }
    for(x=queue; x != NULL; x = x->next)
      if( x->iaddr == iaddr )
        {
        if( !quiet )
          fprintf(stderr,
            "%s: Duplicated host name %s ignored in argument list\n",
            progname,host);
        break;
        }
    if( x != NULL )
      continue;
    x = malloc(sizeof(x_t));
    assert( x != NULL );
    x->next = queue;
    queue = x;   
    x->iaddr = iaddr;
    x->host = host;
    x->socket = create_client_socket(x->host,DISPATCH_PORT);
    if( x->socket < 0 )
      fprintf(stderr,"%s: No connection to %s dispatcher. Will try later\n",
        progname,x->host);
    }
  fflush(stderr);

  if( queue == NULL )
    {
    fprintf(stderr,"%s: No non-local hosts in argument list. Exiting.\n",
       progname);
    goto fin;
    }

  while( getbwait(socket,&pref,sizeof(prefix_t)) > 0 )
    {
    char htag[TAGSIZE+1];
    char *tag;
    int hsize;
    int size;

    first = 0;
    fromprefix(&pref,htag,&hsize);
    size = hsize;
    if( size < 0 )
      break;
    if( size >= sizeof(buffer) )
      {
      fprintf(stderr,"%s: Line too long, truncated\n",progname);
      size = sizeof(buffer) - 1;
      }

    if( getbwait(socket,buffer,size) <= 0 )
      break;
    if( size != hsize && skipbwait(socket,hsize - size) <= 0 )
      break;
    if( strcmp(htag,DISPTAG_Born) == 0 )
      tag = "RemBorn";
    else if( strcmp(htag,DISPTAG_Died) == 0 )
      tag = "RemDied";
    else
      break;

    if( size + 1 + L <= sizeof(buffer) )
      {
      buffer[size] = ' ';
      memcpy(buffer+size+1,myhost,L);
      size += L+1;
      }

    for(x=queue; x != NULL; x = x->next )
      {
      if( x->socket < 0 )
        {
        x->socket = create_client_socket(x->host,DISPATCH_PORT);
        if( x->socket < 0 )
          continue;
        }
      if( put_tagged_bwait(x->socket,tag,buffer,size) <= 0 )
        {
        shut_line(x->socket);
        x->socket = -1;
        }
      }
    }
/*
  if( first )
    fprintf(stderr,"%s: Looks like other BornDiedExpander \
is running already. Exiting\n",progname);
  else
*/
    perror(progname);

fin:
  if( socket >= 0 )
    shut_line(socket);
  for(x=queue; x != NULL; x = x->next )
    if( x->socket >= 0 )
      shut_line(x->socket);
  return 1;
}
