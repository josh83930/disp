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
 $Id: vmesend.c,v 1.11 1996/02/02 12:01:34 gurin Exp $
*/

/* VME transfer support. Sender part */

#include "displowl.h"
#include "dispvme.h"

/* #define DBG */

#define LOCKED (-1)

static long vmebase;
static char *mem = NULL;
static int   memsize;
static int firstbuf,lastbuf,maxbuf,nbufs;

#define ROUND (sizeof(int)-1)

#define bufP(shift) ((vme_dsp_buffer_t *)(mem + (shift)))

/* Size of the control part of the buffer */
#define CBsize ( sizeof(int) + sizeof(prefix_t) )
/* Size of the buffer with all the control fileds and rounded to size of int */
#define Bsize(size)  ( ( CBsize + (size) + ROUND ) & ~ROUND )

#ifdef DBG
static void show(const char *title)
{
  int i;
  int cbuf;
  printf("%s\n",title);
  printf("  firstbuf %x, lastbuf %x, maxbuf %x, nbufs %d\n",firstbuf,lastbuf,maxbuf,nbufs);
fflush(stdout);
  for(cbuf=firstbuf,i=0;i<nbufs;i++)
    {
    vme_dsp_buffer_t *bp = bufP(cbuf);
    printf(" lock %2d, bpos %x,  bsize %x,  tag %-8.8s\n",
      bp->lock,cbuf,bp->header.__size,bp->header.__tag);
    assert( Bsize(bp->header.__size) >= CBsize );
    cbuf += Bsize(bp->header.__size);
    assert( cbuf >= 0 );
    assert( cbuf <= memsize );
    if( cbuf > maxbuf )
      cbuf = 0;
    }
  puts("=================");
fflush(stdout);
}
#endif

int vme_dsp_init(int socket, long vmeaddr, void *buf, int size)
{
  vme_dsp_init_t initdat;

  vmebase = vmeaddr;
  mem = (char *)buf;
  memsize = size & ~ROUND;
  nbufs = 0; 

  initdat.base = vmeaddr;
  initdat.size = memsize;

  return put_tagged_bwait(socket,DISPTAG_VMEaddr,&initdat,sizeof(initdat));
}

void *vme_dsp_getbuf(const char *tag, int size)
{
  int unlcnt;
  int res;
  vme_dsp_buffer_t *bp;
  int need_size = Bsize(size);
#ifdef DBG
show("------- getbuf Start");
#endif

  if( mem == NULL )
    return NULL;
  if( size < 0 || need_size > memsize )
    return NULL;
  for(;;)
    {
    if( nbufs == 0 )
      {
      firstbuf = 0;
      maxbuf = 0;
      res = 0;
      break; 
      }
    res = lastbuf + Bsize(bufP(lastbuf)->header.__size);
    assert( res >= 0 && res <= memsize );
    assert( maxbuf >= 0 && maxbuf < memsize );
    assert( firstbuf >= 0 && firstbuf < memsize );
    assert( lastbuf >= 0 && lastbuf < memsize );
    if( firstbuf <= lastbuf )
      {
      if( res + need_size <= memsize )
        break; 
      if( need_size <= firstbuf )
        {
        res = 0;
        break;
        }  
      }
    else 
      {
      if( res + need_size <= firstbuf )
        break;
      }
    bp = bufP(firstbuf);
#ifdef DBG
printf("Dropping block %x,  lock %d\n",firstbuf,bp->lock); 
#endif
/*
    for(unlcnt=0;  bp->lock == LOCKED; unlcnt++ )
      {
      if( unlcnt == 100000 )
        {
        fprintf(stderr,"Still trying to unlock %x, lock %d\n",firstbuf,bp->lock);
        unlcnt = 0;
        } 
      }
*/
    while( bp->lock == LOCKED )  
      ;
    nbufs--;
    firstbuf += Bsize(bp->header.__size);
    if( firstbuf > maxbuf )
      {
      maxbuf = lastbuf;
      firstbuf = 0;
      }
#ifdef DBG
show("drop done");
#endif
    }
  nbufs++;  
  lastbuf = res;
  if( lastbuf > maxbuf )
    maxbuf = lastbuf; 
  bp = bufP(res);
  bp->lock = LOCKED; 
                     
  fillprefix(&(bp->header),tag,size);
#ifdef DBG
printf("newbuf %x\n",res);
show("------- getbuf End");
#endif
  return bp->buffer;
}

int vme_dsp_notify(int socket, void *buf)
{
  vme_dsp_notify_t notify;

  if( mem == NULL )
    return -1; 

  assert( (char *) buf > mem && (char *)buf < mem + memsize );

  notify.buffer_vmepos = (char *) buf - mem - CBsize + vmebase;
/*
printf("notifying %x %x\n",notify.buffer_vmepos,notify.buffer_vmepos-vmebase);
*/
  return put_tagged_bwait(socket,DISPTAG_VMEready,&notify,sizeof(notify));
}
