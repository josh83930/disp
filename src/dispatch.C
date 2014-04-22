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
 $Id: dispatch.C,v 1.18 1996/10/16 12:40:32 chorusdq Exp $
*/

// dispatcher main program

#include "headers.h"
#include <signal.h>

extern char *optarg;
extern int   optind;

// default size of shared memory (Kbytes)
static const int default_sh_mem_data_size = 
#ifdef OSF1
  3*1024;
#else
  8*1024;
#endif

//PEWG did this

#define SHMEMPROT	0666

void *data_shared_memory;
void *notif_shared_memory;
int shared_memory_notif_id = -1;
int shared_memory_data_id = -1;
static int sh_mem_data_size = 1024*default_sh_mem_data_size;

/* handle SIGPIPE errors - PH */
static void sigPipeHandler(int sig)
{
	fprintf(stderr,"Broken pipe\n");
	return;
}

extern "C" {
void init_signals();
}

int out_of_mem_counter = 0;
time_t out_of_mem_last;
bool cleanup_pending = false;

static int sx = -1; /* main socket */ 

int myhostid;

ClientRing select_done_root;

bool size_unacceptable(int size)
  {
  return size < 0 || size > sh_mem_data_size/2;
  }

static void shutup(void)
{
  myclose(sx);
  Client *p;
  ringloop(client_root,p)
    if( p->alive() )
      myclose(p->socket());
  ringloop(select_done_root,p)
    if( p->alive() )
      myclose(p->socket());
  Tag::printstat(1);
  Lock *lp;
  ringloop(hanging_lock_root,lp)
    lp->force_unlock();
  if( shared_memory_notif_id != -1 && sh_mem_destroy(shared_memory_notif_id) != 0 )
    perror("sh_mem_destroy");
  if( shared_memory_data_id != -1 && sh_mem_destroy(shared_memory_data_id) != 0 )
    perror("sh_mem_destroy");
}

static void help(const char *nm)
  {
  fprintf(stderr,"\
Format %s [ -m NNN ]\n\
   where NNN gives the size of shared memory segment in Kbytes\n\
   default value - %d\n",nm,default_sh_mem_data_size);
  exit(1);
  }

char dispident[] = "@(#) dispatcher program - author Ruten Gurin, $Id: dispatch.C,v 1.18 1996/10/16 12:40:32 chorusdq Exp $";

int main(int argc, char* argv[])
{
  Tag::init_tags();
  
  // Set up SIGPIPE handler - PH 04/14/00
  struct sigaction act;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  act.sa_handler = sigPipeHandler;
  if (sigaction(SIGPIPE, &act, (struct sigaction *)NULL) < 0) {
    fprintf(stderr,"Error installing SIGPIPE handler\n");
  }
	
  int ch;
  while( (ch=getopt(argc,argv,"hm:")) != EOF )
    switch(ch)
      {
      case 'm':
        sh_mem_data_size = 1024*atoi(optarg);
        if( sh_mem_data_size <= 0 )
          help(argv[0]);
        break;
      case 'h':
      default:
        help(argv[0]);
      }

  if( optind != argc )
    help(argv[0]);

  int s = 0; // work variable

  init_signals();
  atexit(shutup);

  /*
   * Create a socket for internet stream protocol.
   */
  sx = create_server_socket(DISPATCH_PORT,50);
  if( sx < 0 ) 
    {
    socket_perror("Cannot open dispatcher socket");
    exit(errno);
    }
  /* Let's do initial socket non-blocking */
  if( set_nowait(sx,1) < 0 )
    {
    socket_perror("Cannot make dispatcher socket non-blocking");
    exit(errno);
    }

//PEWG did this too - to try to allow anyone to re-start the dispatcher
//data_shared_memory = sh_mem_cre(argv[0],1,0644,sh_mem_data_size,&shared_memory_data_id,1); 
  data_shared_memory = sh_mem_cre(argv[0],1,SHMEMPROT,sh_mem_data_size,&shared_memory_data_id,1);
  // argv[0] used for creating uniq segment id
  if( shared_memory_data_id == -1 ) 
    {
    perror("Cannot create shared memory segment");
    exit(1);
    }
  sh_init(data_shared_memory,sh_mem_data_size); 

  notif_shared_memory = sh_mem_cre(argv[0],2,0666,sh_mem_notif_size,&shared_memory_notif_id,1); 
  // argv[0] used for creating uniq segment id
  if( shared_memory_notif_id == -1 ) 
    {
    perror("Cannot create shared memory segment");
    exit(1);
    }
  {
  int *p = (int *) notif_shared_memory;
  int lim = sh_mem_notif_size/sizeof(int);
  for(int i = 0; i < lim; ++i,++p)
    *p = -1;
  }

  myhostid = my_inet_addr();
// Just a hint to people of which one they are running
   printf ("Dispatcher - PEWG/PH version (04/14/00)\n");
  for(;;)
    {
    fd_set readmask,writemask/*,excptmask*/;
    int fdset_width;
    register Client *p;

// Let's drop some unused info
    for(;;)
      {
      Info *p = info_root.next();
      if( p->is_real() && !p->is_pointed() && p->may_be_dropped() )
        delete p;
      else
        break;
      }

    if( cleanup_pending )
      {
      cleanup_pending = false;
      cleanup();
      ringloop(client_root,p)
        {
        if( p->alive() && p->lock_pending() )
          p->prep_send();
        if( p->alive() && p->buff_pending() )
          p->in_tag->Allocbuf(p);
        }
      if( cleanup_pending )
        {
        out_of_mem_counter++;
        out_of_mem_last = time(NULL);
        }
      }

    FD_ZERO(&readmask);
    FD_ZERO(&writemask);

    FD_SET(sx,&readmask);
    fdset_width = sx+1;

    /* Let's look at clients */
    while( !client_root.is_empty_ring() )
      {
      p = client_root.next();
      if( !p->alive() )
        {
        delete p;
        continue;
        }
      p->unlink();
      select_done_root += *p;
      p->checklocks(); // remove inactive locks
      if( p->out_info_is_real() && ! p->send_pending() )
        p->select_out();
      s = p->socket();
      if( p->lock_pending() )
        ;
      else if( p->send_pending() )
        {
        FD_SET(s,&writemask);
        if( s >= fdset_width )
          fdset_width = 1 + s;
        }
      if( p->buff_pending() )
        ;
      else
        {
        FD_SET(s,&readmask);
        if( s >= fdset_width )
          fdset_width = 1 + s;
        }
      }


    select_done_root += client_root;
    select_done_root.unlink();

    struct timeval *interv = NULL;
    if( cleanup_pending && Lock::exist() )
      {
      static struct timeval timeout = { 0, 3000};
      interv = &timeout; // check the locks after 3 msec
      }
#ifdef HP_UX
    int selret = select(fdset_width, (int *)&readmask, (int *)&writemask, NULL, interv);
#else
    int selret = select(fdset_width, &readmask, &writemask, NULL, interv);
#endif
    /* EINTR happens when the process is stopped */
    if (selret < 0 && errno != EINTR )
      {
      socket_perror("select") ;
      exit(1);
      }
    if( selret <= 0 )
      continue;

    if( FD_ISSET(sx,&readmask) ) /* connection is pending */
      {
      char host[200];
      int hostid;

      host[0] = 0;
      s = accept_client(sx,host,&hostid);
      if( set_nowait(s,1) < 0 )
        {
        perror("set_nowait");
        myclose(s);
        s = -1;
        }
      if( s >= 0 )
        new Client(s,host,!host_is_local(hostid));
      }

    ringloop(client_root,p)
      {
      if( ! p->alive() ) continue;
      s = p->socket();
      if( FD_ISSET(s,&writemask) )
        {
        p->send();
        if( ! p->alive() ) continue;
        }
      if( FD_ISSET(s,&readmask) )
        {
        p->receive();
        }
      }
    }
  return 0; // just to avoid warning
}
