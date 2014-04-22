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
 $Id: bdcollector.c,v 1.11 1995/05/29 23:56:20 ruten Exp $
*/

/* 
  Gets remote events Born/Died <nickname> 
  from several hosts (from argument list) and sends them to local host
  as events RemBorn/RemDied <nickname> <remote_host_name>
*/

#include "displowl.h"

extern int optind;

static char buffer[300];

typedef struct x_s {
  struct x_s *next;
  int iaddr;
} x_t;

void help(char *progname)
  {
  fprintf(stderr,"Format: %s [ -qi ] host...\n",progname);
  exit(1);
  }

static int localsocket;
static char *progname;
static int ignore = 0;

void sendit(const char *tag, const char *buffer, int size)
  {
  if( put_tagged_bwait(localsocket,tag,buffer,size) <= 0 )
    {
    fprintf(stderr,"%s: Local connection broken. Exiting\n",progname);
    exit(1);  
    }
  }

void broken(int s, const char *host)
  {
  if( ignore )
    {
    fprintf(stderr,"%s: No connection to %s dispatcher. Sending announcement.\n",
      progname,host);
    sendit("DISPDEAD",host,strlen(host));
    /* drop the line only after 'host' was used. Otherwise we loose 'host' */
    if( s >= 0 ) dispdrop(s);
    } 
  else 
    {
    fprintf(stderr,"%s: No connection to %s dispatcher. Exiting.\n",
      progname,host);
    dispalldrop();
    exit(1);
    }
  }
  
/********************************************************************/
int main (int argc, char *argv[])
  {
  x_t *queue = NULL;
  int me = my_inet_addr();
  x_t *x;
  int quiet = 0;
  char *myid = "BornDiedCollector";
  char *subs = "a "DISPTAG_Born" a "DISPTAG_Died;
  int c;
 
  progname = argv[0];
  while((c=getopt(argc,argv,"qi")) != EOF )
    switch(c)
      { 
      case 'q':
        quiet = 1;
        break;
      case 'i':
        ignore = 1;
        break;
      default:
        help(progname);
      }

  argc -= (optind-1);
  argv += (optind-1);
 
  if( argc < 2 )
    help(progname);

  localsocket = dispconnect("local");

  if( localsocket < 0 )
    {
    perror(progname);
    exit(1);
    }

  sendit(DISPTAG_MyId,myid,strlen(myid));

  dispprio(localsocket,1);

  for( ;argc > 1; argc--)
    {
    char *host = argv[argc-1];
    int iaddr = his_inet_addr(host);
    int s1;
    if( iaddr == me || iaddr == 0 || iaddr == 0x7f000001 )
      {
      if( !quiet )
        fprintf(stderr,"%s: Local or unknown host %s ignored in argument list.\n",
          progname,host);
      continue;
      }
    for(x=queue; x != NULL; x = x->next)
      if( x->iaddr == iaddr )
        {
        if( !quiet )
          fprintf(stderr,
            "%s: Second refernce to  host %s ignored in argument list.\n",
            progname,host);
        break;
        }
    if( x != NULL )
      continue;
    s1 = dispconnect(host);
    if( s1 < 0 ||
      dispsubscribe(s1,subs) <= 0 ||
      dispalways(s1) <= 0 )
      {
      broken(s1,host);
      continue;
      }
    x = malloc(sizeof(x_t));
    assert( x != NULL );
    x->next = queue;
    queue = x;   
    x->iaddr = iaddr;
    }
  fflush(stderr);

  if( queue == NULL )
    {
    fprintf(stderr,"%s: No non-local hosts in argument list. Exiting.\n",
       progname);
    dispalldrop();
    exit(1);
    }

  for(;;)
    {
    char *tag;
    char htag[TAGSIZE+1];
    int hsize;
    int size;
    int s1;
    int rc;
    const char *host;
    int L;

    if( dispchannels() <= 1 )
      {
      fprintf(stderr,"%s: All connections broken. Exiting\n",progname);
      break;
      }
    
    s1 = dispselect(-1,-1);
    if( s1 == -1 )
      continue;
    if( s1 < 0 || s1 == localsocket )
      {
      fprintf(stderr,"%s: Local connection broken. Exiting\n",progname);
      break;
      }

    rc = dispcheck(s1,htag,&hsize,0);
    if( rc == 0 )
      continue;
    host = dispqhost(s1);
    if( rc < 0 || hsize < 0 )
      {
      broken(s1,host);
      continue;
      } 

    size = hsize;
    if( size >= sizeof(buffer) )
      {
      fprintf(stderr,"%s: Line too long, truncated\n",progname);
      size = sizeof(buffer) - 1;
      }

    if( getbwait(s1,buffer,size) <= 0 ||
        size != hsize && skipbwait(s1,hsize - size) <= 0 )
      {
      broken(s1,host);
      continue;
      } 
    if( strcmp(htag,DISPTAG_Born) == 0 )
      tag = "RemBorn";
    else if( strcmp(htag,DISPTAG_Died) == 0 )
      tag = "RemDied";
    else
      {
      broken(s1,host);
      continue;
      } 
    L = strlen(host);

    if( size + 1 + L <= sizeof(buffer) )
      {
      buffer[size] = ' ';
      memcpy(buffer+size+1,host,L);
      size += L+1;
      }

    sendit(tag,buffer,size);
    }

  perror(progname);
  dispalldrop();
  return 1;
}
