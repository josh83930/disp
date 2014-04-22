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
 $Id: tag.C,v 1.16 1996/10/16 12:42:28 chorusdq Exp $
*/

// tag information management inside dispatcher program

#include <ctype.h>
#include "headers.h"

TagRing tag_root;

static Tag *htab[487];

static tagbits_t currsignature = 1;

static bool init_done = false;
static Tag *last_tag = NULL;

void Tag::init_tags()
  {
  assert( !init_done );
  if( last_tag != NULL )
    *last_tag += tag_root;
  init_done = true;
  }

Tag::Tag(const char *ctag, int hidx) :
  nrecv(0),
  nsent(0),
  nmem(0),
  nmemsize(0),
  work(0),
  work1(0),
  deletable(true)
  {
  if( !init_done )
    {
    if( last_tag == NULL )
      last_tag = this;
    else
      *last_tag += *this;
    }
  else
    tag_root += *this;
  if( hidx < 0 )
    {
    Tag *p = hash(ctag,name,&hidx);
    assert( !p );
    }
  else
    memcpy(name,ctag,TAGSIZE+1);
  hlink = htab[hidx];
  htab[hidx] = this;
  tagbit = currsignature;
  currsignature <<= 1;
  if( ! currsignature )
    currsignature = 1;
  }

Tag::~Tag()
  {
  char normtag[TAGSIZE+1];
  int hidx;
  Tag *p = hash(name,normtag,&hidx); // just to get hidx
  assert( p == this );
  if( htab[hidx] == this )
    htab[hidx] = hlink;
  else
    for(p=htab[hidx]; ;p = p->hlink)
      {
      assert( p != NULL );
      if( p->hlink == this )
        {
        p->hlink = hlink;
        break;
        }
      }
  }

void Tag::Allocbuf(Client *p)
  {
  int size = p->in_pref.__size;
  p->no_mem_for_buff = false;
  if( size_unacceptable(size) )
    p->wrongdatasize();
  else if( size > 0 && !may_be_ignored() )
    {
    p->in_buff = sh_alloc(size);
    if( !p->in_buff )
      {
      p->no_mem_for_buff = true;
      cleanup_pending = true;
      }
    }
  else
    p->in_buff = NULL;
  }

void Tag::React(Client *p)
  {
  p->store();
  }

Subscr *Tag::subs(Client *p) const
  {
  if( !(tagbit & p->signature) ) // fast check
    return NULL;
  Subscr *q;
  ringloop(subscr_root,q)
    if( q->client == p )
      return q;
  return NULL;
  }

Tag *Tag::hash(const char *ctag, char normtag[TAGSIZE+1], int *hidx)
  {
  unsigned m = 0;
  if( !*ctag )
    return tagWrongHdr;
  int i;
  for(i=0; i<TAGSIZE; i++,ctag++)
    {
    int c = *ctag;
    if( !c )
      break;
    if( !isgraph(c) )
      return tagWrongHdr;
    m += (m>>24) + (m<<8) + c; // round rotate and add the character
    normtag[i] = c;
    }
  for(;i<TAGSIZE+1;i++)
    normtag[i] = 0;
  *hidx = m % (sizeof(htab)/sizeof(htab[0]));
  Tag *p;
  for(p=htab[*hidx]; p && memcmp(p->name,normtag,TAGSIZE) != 0; p=p->hlink);
  return p;
  }

Tag *Tag::find(const char *ctag)
  {
  char normtag[TAGSIZE+1];
  int hidx;
  Tag *p = hash(ctag,normtag,&hidx);
  if( !p )
    {
    p = new Tag(normtag,hidx);
    assert( p != NULL );
    }
  return p;
  }

void Tag::printstat(int fd)
  {
  char wbuf[200];
  char *q;
  q = "======== Tag statistics =============\n";
  write(fd,q,strlen(q)); 
  for(int i=0; i<sizeof(htab)/sizeof(htab[0]); i++)
    for(Tag *p=htab[i]; p; p = p->hlink)
      if( p->nrecv || p->nsent )
        { 
        sprintf(wbuf,
	  "%-8.8s: recv %7ld, sent %7ld, inmem %7ld/%ld\n%c",
          p->name,p->nrecv,p->nsent,
	  p->nmem,p->nmemsize,0);
        write(fd,wbuf,strlen(wbuf)); 
        }
  q = "======== Tag statistics end =========\n";
  write(fd,q,strlen(q)); 
  }

void Tag::cleanup()
  {
  Tag *p;
  ringloop(tag_root,p)
    if( !p->nmem && p->may_be_ignored() )
      p->work = 1;
    else
      p->work = 0;
  Client *q;
  ringloop(client_root,q)
    if( q->in_tag )
      q->in_tag->work = 0;
  Tag *pnext;
  for(p=tag_root.next(); p->is_real(); p = pnext)
    {
    pnext = p->next();
    if( p->work )
      delete p;
    }
  }

bool cleanup()
  {
  bool res = false;

  Client *p;
  ringloop(client_root,p)
    if( p->alive() && p->checklocks() )
      res = true;
  Lock *nextlp = NULL; // initialized just to avoid compiler warnings
  for( Lock *lp = hanging_lock_root.next(); lp != &hanging_lock_root; 
	lp = nextlp)
    {
    nextlp = lp->next();
    if( !lp->active() )
      {
      delete lp;
      res = true;
      }
    else if( lp->expired() )
      {
      // if connection with client is already broken a for a long time
      // and lock is still here, than client probably have forgotten
      // to drop it. So we do it ourselves.
      // But we may have problems if client is still alive and will try
      // to do unlock. Than shared memory will be corrupted.
      lp->force_unlock();
      delete lp;
      res = true;
      }
    }
  Info* nextip = NULL; // initialized just to avoid compiler warnings
  for(Info *ip = info_root.next(); ip->is_real(); ip = nextip )
    {
    nextip = ip->next();
    if( ip->may_be_dropped() )
      {
      delete ip;
      res = true;
      }
    }
  return res;
  }
