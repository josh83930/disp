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
 $Id: vme.C,v 1.32 1996/02/22 16:49:24 gurin Exp $
*/

// VME transfer support

#include "headers.h"

#include    <sys/ipc.h>
#include    <sys/shm.h>

#include "dispvme.h"

#ifndef SIMULATED_VME
extern "C" int phys_shmget(key_t,int,int,int);
#endif

static bool duplicated = false;

extern Tag *pTagVMEaddr;

static char *atpmem(unsigned long physaddr, unsigned long ataddr, int atsize);
static void detvme(char *membase);

class VMEframe;
dclRing(VMEframe);

static VMEframeRing frame_root;

class VMEframe : public VMEframeRing {
  private:
     long vmebase;
     char *membase;
     int size;
  public:
     VMEframe(long vmeaddr,int vmesize) {
        size = vmesize;
        vmebase = vmeaddr;
        membase = atpmem(vmeaddr,0,vmesize);
        if( membase == NULL )
          delete this;
	else
	  frame_root += *this; 
     }               
     ~VMEframe() { detvme(membase); }
     static void replay() {
       int L = sizeof(vme_dsp_init_t);
       VMEframe *p;
       ringloop(frame_root,p)
         {
         vme_dsp_init_t *ip = (vme_dsp_init_t *)malloc(sizeof(L));
         ip->base = p->vmebase;
         ip->size = p->size;
         new Info(pTagVMEaddr,L,ip);
         }
     }
     static int del_overlap(long vmeaddr, int vmesize) {
       VMEframe *p;
       ringloop(frame_root,p)
         if( vmeaddr == p->vmebase ) 
           {
           if( vmesize == p->size )
             return -1;
           else
              {
              delete p;
              return 1;
              }
           }
         else if( vmeaddr >= p->vmebase && vmeaddr < p->vmebase+p->size ||
                  p->vmebase >= vmeaddr && p->vmebase < vmeaddr+vmesize )
           {
           delete p;
           return 1;
           }
       return 0;
     }
     static char *convert_addr(long addr) {
       VMEframe *p;
       ringloop(frame_root,p)
         if( addr >= p->vmebase && addr < p->vmebase+p->size)
           return p->membase + (addr-p->vmebase);
       return NULL;
     }
};

/*
 * map physical address physaddr to ataddr (or any if 0) in shared memory
 * for atsize bytes. return address corresponding to physical address
 */
#define ROUNDMASK 	((unsigned long)0x1FFF)

static
char *atpmem(unsigned long physaddr, unsigned long ataddr, int atsize)
{
    register int offset, shmid;
    void *addrbase;
    unsigned long atmask;

// printf("atpmem physaddr %x attdar %x atsize %d\n",physaddr,ataddr,atsize);

    atmask = ROUNDMASK;

#ifdef SIMULATED_VME
    offset = 0;
    shmid = physaddr; // id arrives instead of address
#else
    offset = physaddr & atmask;

    shmid = phys_shmget(IPC_PRIVATE, atsize + offset,
			     physaddr & ~atmask,
			     IPC_CREAT | IPC_EXCL);
#endif 
    if (shmid <= 0) {
	perror("phys_shmget");
	return NULL;
    }

//  printf("shmid = %x, (ataddr & ~atmask) = %x, size %d\n", shmid, (ataddr & ~atmask), atsize);
    
    addrbase = shmat(shmid, (char *)(ataddr & ~atmask), SHM_RND);
    if (addrbase == (void *)(-1)) {
	perror("shmat");
	return NULL;
    }
#ifndef SIMULATED_VME
    /* remove the shared memory structures from the kernel.
     * the region will be removed when the process exits. */
    if (shmctl(shmid, IPC_RMID, (struct shmid_ds *) 0) == -1) {
	perror("shmctl");
	return NULL;
    }
#endif
    return ((char *)addrbase) + offset;
}

/* Detach the shared memory segment */
static void detvme(char * VMEbase)
  {
  if( VMEbase != NULL )
    if (shmdt((char *)VMEbase) == -1)
	printf("shmdt %s: errno %d\n", "dram", errno);
  }

defTag(VMEaddr)
  {
// printf("VMEaddr entered\n");
  if( pcl->in_pref.__size != sizeof(vme_dsp_init_t) )
    {
    pcl->wrongdatasize();
    return;
    }
  vme_dsp_init_t *ip = (vme_dsp_init_t *)(pcl->in_buff);
  long ipbase = ip->base;
  int  ipsize = ip->size;
// printf("VMEaddr %lx %d\n",ipbase,ipsize);
  if( !subscr_root.is_empty_ring() )
    pcl->store();
  int rc;
  while( (rc = VMEframe::del_overlap(ipbase,ipsize)) > 0);
  if( rc == 0 && ipsize > 0 )
     {
     new VMEframe(ipbase,ipsize);
     if( VMEframe::convert_addr(ipbase) == NULL )
        pcl->kill();
     }
  }

Tag *pTagVMEaddr = &wVMEaddr;

class sTagVMEready : public PermanentTag {
  public:
    sTagVMEready() : PermanentTag(DISPTAG_VMEready) {}
    virtual void Allocbuf(Client *p);
    virtual void React(Client *p);
  };

static sTagVMEready wVMEready;

void sTagVMEready::Allocbuf(Client *p)
  {
  Tag *newtag =NULL;
// printf("VMEready allocbuf entered %p %d\n",p->in_buff,p->in_pref.__size);
  vme_dsp_notify_t *ip=(vme_dsp_notify_t *)(p->in_buff);
  p->in_buff = NULL;
  if( ip == NULL ) 
    {
    // allocation for notification
    if( p->in_pref.__size != sizeof(vme_dsp_notify_t) )
      {
      p->wrongdatasize();
      return;
      }
    }
  else
    {
    newtag = Tag::find(p->in_pref.__tag);
    if( newtag->may_be_ignored() )
       p->in_pref.__size = 0;
    }
  Tag::Allocbuf(p);
  if( ip == NULL )
    return;
  if( !p->alive() )
    {
    free_buff(ip);
    return;
    }
  if( p->buff_pending() )
    {
    p->in_buff = ip;
    return;
    }
   
  // ip points to shift, in_buff points to real data space
  vme_dsp_buffer_t *bp =
    (vme_dsp_buffer_t *)VMEframe::convert_addr(ip->buffer_vmepos);
// printf("VMEready allocbuf bp %x, vmepos %x\n",bp,ip->buffer_vmepos);
  if( bp == NULL )
    {
    free_buff(ip);
    p->kill();
    return;
    }   
  memcpy(p->in_buff,bp->buffer,p->in_pref.__size);
  // now we have to send secondary notification
  if( !subscr_root.is_empty_ring() )
    new Info(this,sizeof(vme_dsp_notify_t),ip);
  else
    {
    if( duplicated )
      {
      time_t t;
      time(&t);
      fprintf(stderr,"Lost contact with secondary VME client: %s\n",ctime(&t));
      duplicated = false;
      } 
    free_buff(ip);
    } 
  if( !duplicated )
    {
// printf("unlocking bp %x %x\n",bp,bp->lock);
    bp->lock = 0;
    }
  newtag->received();
  p->in_tag = newtag;
  p->react();
  }

void sTagVMEready::React(Client *p)
  {
  // the buffer with the data shift is here
  vme_dsp_notify_t *ip=(vme_dsp_notify_t *)(p->in_buff);
  vme_dsp_buffer_t *bp = 
    (vme_dsp_buffer_t *)VMEframe::convert_addr(ip->buffer_vmepos);
// printf("VMEready read bp %x, vmepos %x\n",bp,ip->buffer_vmepos);
  if( bp == NULL )
    {
    p->kill();
    return;
    }   
  // get the real data tag and size
  memcpy(&(p->in_pref),&(bp->header),PSIZE);
  p->in_pref.__size = ntohl(p->in_pref.__size);
  if( size_unacceptable(p->in_pref.__size) )
    {
    if( !duplicated )
      {
// printf("unlocking bp %x\n",bp);
      bp->lock = 0;
      }
    p->wrongdatasize();
    return;
    }
  p->in_pos = PSIZE + p->in_pref.__size;
  sTagVMEready::Allocbuf(p);
  }

defTag(VMEdupl)
{
  const char *dupl_nick = "Duplicate VME traffic";
  const char *dupl_subscr = "a "DISPTAG_VMEaddr" a "DISPTAG_VMEready;
  pcl->cutstrbuff();
  Client *p;
  ringloop(client_root,p)
    if( p->alive() )
      if( p->nick && msequ(dupl_nick,strlen(dupl_nick),p->nick) )
        p->kill();
  duplicated = false;
  int L = pcl->in_pref.__size;
  if( L == 0 )
    return;
  char *qhost = (char *)malloc(L+1);
  memcpy(qhost,pcl->in_buff,L);
  qhost[L] = 0;
  free_buff(pcl->in_buff);
  pcl->in_buff = NULL;
  int clihostid = his_inet_addr(qhost);
  int s;
  int port;
#ifdef SIMULATED_VME
  if( !host_is_local(clihostid) || (s=create_client_socket(qhost,DISPATCH_PORT+1)) < 0 )
#else
  if( host_is_local(clihostid) || (s=create_client_socket(qhost,DISPATCH_PORT)) < 0 )
#endif
    {
    free(qhost);
    return;
    }
  Client * newpcl = new Client(s,qhost,true); // true == remote
  newpcl->simulate_receive(DISPTAG_MyId,strdup(dupl_nick),strlen(dupl_nick));
  if( newpcl->alive() )    
    newpcl->simulate_receive(DISPTAG_Subscribe,
         strdup(dupl_subscr),strlen(dupl_subscr));
  if( newpcl->alive() )
    newpcl->simulate_receive(DISPTAG_Always,NULL,0);
  if( newpcl->alive() )
    {
    duplicated = true;
    VMEframe::replay();
    }
}
