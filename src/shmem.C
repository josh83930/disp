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
 $Id: shmem.C,v 1.5 1996/05/15 09:44:13 ruten Exp $
*/

// heap manager
// "Allocation With Boundary Flags" used
//    see D.Knuth "The Art of Programming", vol. 1,  for details 
//
//  If you don't want to read this book, then just a few comments:
//     used block format:
//        upper word      contains negated total number of words in block
//                           (including upper and down words)
//        data word
//        .......
//        data word
//        down  word      is equal to upper word
//                  
//                   When user gets or frees the block,
//                   he operates with the pointer to first data word
//
//     free block format:   
//        upper word      contains total number of words in block 
//        next-ref word   contains the index of word in pool,
//                             where the next free block starts
//        prev-ref word   ----- // ----- prev free block ---- // --
//        garbage words 
//        down word       is equal to upper word
//
//                   Free blocks are linked in a ring by prev-ref/next-ref
//
//     There is also one special pseudo-used block - one word -1,
//     and one pseudo-free with zeroes in both upper and down words.
//     This zeroes prevent it from being allocated/merged, so we always
//        have at least one block in the ring of free blocks.

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "shmem.h"

#define sh_map1(title) /*sh_map(title)*/  

static int *mem;            // address of heap memory
static int totsize = 0;     // its size (in words)
static int maxfree = -1;    // value "-1" means "unknown"

class MemBlock;

static inline MemBlock *Ptr(int i)   // converts index in pool 
				     // to pointer to MemBlock
	{ return (MemBlock *)(mem+i); }

static MemBlock *current = NULL;     // current free block in the ring,
                                     // it's a start point of search,
				     // when we try to allocate a new block
#ifdef OSF1
#define INTLINK
#endif

class MemBlock    // describes format of the block in pool
  {
  private:
    int _size;         // upper word (see initial comments)
#ifdef INTLINK
    int _next;
    int _prev;
#else
    MemBlock * _next;         // links in the ring (free blocks only)
    MemBlock * _prev;         // ---------//------------ 
#endif
    int dummy;

    int *wp(int n) { return (int *)this + n; }

  public:
    int size() { return _size; }
#ifdef INTLINK
    MemBlock *next() { return Ptr(_next); }
    MemBlock *prev() { return Ptr(_prev); }
#else
    MemBlock *next() { return _next; }
    MemBlock *prev() { return _prev; }
#endif
    
  void init() {
    assert( (int *)this == mem );
#ifdef INTLINK
    _next = _prev = 0;
#else
    _next = _prev = this;
#endif
    _size = dummy = 0;
    }

  void link() {  
#ifdef INTLINK
    const int me = (int *)this - mem;
#else
    MemBlock *me = this;
#endif
    MemBlock* nxt = current->next();
    _next = current->_next;
    _prev = nxt->_prev; 
    nxt->_prev = me;
    current->_next = me;
    }

  void unlink() {
    MemBlock *nxt = next();
    if( this == current )
      current = nxt;
    prev()->_next = _next;
    nxt->_prev = _prev;
    }

  int& rbound(int sz)
      { return *wp(sz-1); }

  void setsize(int sz)
      { _size = sz; rbound(sz) = sz; }

  void setnegsize(int sz)
      { _size = -sz; rbound(sz) = -sz; }

  void *data()       // user data pointer 
      { return wp(1); }

  static inline MemBlock *block(void *p) { return (MemBlock *) ((int *)p - 1); }

  MemBlock *downF()  // pointer to the block, which is adjacent to down word
                     //  of free block
      { return (MemBlock *)wp(_size); }

  MemBlock *downU() // pointer to the block, which is adjacent to down word
                    // of used block
      { return (MemBlock *)wp(- _size); }

  MemBlock *upfree() // pointer to the block, which is adjacent to upper word,
                     //   is free and is not special . NULL if no such block 
      { int sz = *wp(-1); 
        return ( sz <= 0 ) ? NULL : (MemBlock *)wp(-sz);
      }
  };

static const int UNIT = sizeof(int); // bytes/words ratio 

static const int MINSZ = (sizeof(MemBlock)+UNIT-1)/UNIT;
	// minimal possible block size (words)

static inline int normsize(int size)
  {
  assert( size >= 0 );
  size = (size+UNIT-1)/UNIT;
  size += 2;    // for boundaries
  if( size < MINSZ )
    size = MINSZ;
  return size;
  }

static const int MINFREE = MINSZ+2;
				// we avoid splitting the block
				// if the rest of it is less than
				// MINFREE words, to avoid creation
				// of useless small free blocks

void sh_init(void *memory, long size)
  {
  totsize = size/UNIT; 
  assert( totsize > 1+2*MINSZ );
  mem = (int *)memory;

  current = Ptr(0);    // pseudo-free block 
  current->init();

  MemBlock *first = Ptr(MINSZ);   // real free block
  first->link();
  first->setsize(totsize-MINSZ-1);
  current = first;

  mem[totsize-1] = -1;  // special pseudo used block
  
  sh_map1("Init");
  }

static void checkptr(void *p, char *title = "checkptr failed", int ignore_errors = 0)
  {
  assert( (int *)p > mem );
  int s = (char *)p - (char *)mem;
  assert( s%UNIT == 0 );
  s /= UNIT;
  s--;
  assert( s >= MINSZ );
  assert( s <= totsize - MINSZ );
  MemBlock *q = MemBlock::block(p);
  int size = q->size();
  int sz = size;
  if( sz < 0 ) sz = -sz;
  assert( sz >= MINSZ );
  if(! (&(q->rbound(sz)) < mem+totsize-1) && ignore_errors )
    {
    printf("p %p s %d size %d mem[s] %d mem[s+1] %d\n",
       p,s,size,mem[s],mem[s+1]);
    sh_map(title);
    }
  assert( &(q->rbound(sz)) < mem+totsize-1 );
  assert( q->rbound(sz) == size );
  if( size > 0 )
    {
    assert( q->next()->prev() == q );
    assert( q->prev()->next() == q );
    }
  }

int sh_shift(void *p) // result in words !
  {
  int *ip = (int *)p;
  assert( ip > mem );
  int s = (char *)p - (char *)mem;
  assert( s%UNIT == 0 );
  s /= UNIT;
  s--;
  assert( s >= MINSZ );
  assert( s <= totsize - MINSZ );
  return ip - mem;
  }

int sh_inside(void *p)
  {
  int *ip = (int *)p;
  return ip >= mem && ip < mem+totsize;
  }

void sh_finis()
  {
  mem = NULL;
  totsize = 0;
  }

void *sh_alloc(int size)
  {
  size = normsize(size);
  if( maxfree >= 0 && size > maxfree )
    return NULL;
  MemBlock *res = current;
  int msize = 0;
  do
    {
    int csize = res->size();
    if( csize > msize )
      msize = csize;
    if( csize >= size )
      {
      // block found
      if( csize == maxfree )
        maxfree = -1;
      current = res;
      int left = csize - size;
      if( left >= MINFREE )
        {
        // block too large, must be splitted
        current->setsize(left);
        res = current->downF();
        }
      else
        {
        size = csize;   // we take all the block
        current->unlink();
        }
      res->setnegsize(size);
      sh_map1("After alloc");       
      return res->data(); 
      }
    res = res->next();
    }
    while( res != current );
  maxfree = msize;
  return NULL;
  }

void sh_free(void *vp)
  {
  if( !vp )
    return;
  checkptr(vp,"checkptr free");
  MemBlock *p = MemBlock::block(vp);
  int size = - p->size();
  assert( size > 0 );
  MemBlock *down = p->downU();
  if( down->size() > 0 )
    {
    // down neighbour is free
    size += down->size();     // merge it to our block 
    down->unlink(); // remove it from free list
    }
  MemBlock *up = p->upfree();
  if( up )
    {
    // up neighbour is free
    size += up->size();
    up->setsize(size); // merge our block with it
    }
  else
    {
    p->link(); // link our block to free list
    p->setsize(size); // set correct size
    }
  if( maxfree >= 0  && size > maxfree )
    maxfree = size;
  sh_map1("After free"); 
  }

void sh_stat(char *title)
  {
  printf("====== Shared memory statistics %s =======\n",title);
  MemBlock *p = Ptr(0);
  assert( p->size() == 0 );
  assert( p->next()->prev() == p );
  assert( p->prev()->next() == p );
  int maxused = 0;
  int totfree = 0;
  int totused = 0;
  int nfree = 0;
  int nused = 0;
  maxfree = 0;
  int i;
  for(i=MINSZ; i<totsize-1; )
    {
    MemBlock *p = Ptr(i);
    checkptr(p->data(),"map",1);
    int size = p->size();
    if( size <= 0 )
      {
      size = -size;
      nused++;
      totused += size;
      if( size > maxused ) maxused = size;
      }
    else
      {
      nfree++;
      totfree += size;
      if( size > maxfree ) maxfree = size;
      }
    i+=size;
    }
  printf("Used: %d tot %d max %d\n",nused,totused,maxused);
  printf("Free: %d tot %d max %d\n",nfree,totfree,maxfree);
  assert( i == totsize-1 );
  assert( mem[i] == -1 );
  printf("====== End statistics %s =======\n",title);
  }

void sh_map(char *title)
  {
  printf("====== Map %s =======\n",title);
  MemBlock *p = Ptr(0);
  assert( p->size() == 0 );
  assert( p->next()->prev() == p );
  assert( p->prev()->next() == p );
  maxfree = 0;
  int i;
  for(i=MINSZ; i<totsize-1; )
    {
    MemBlock *p = Ptr(i);
    checkptr(p->data(),"map",1);
    int size = p->size();
    if( size <= 0 )
      {
      size = -size;
      printf("Used at %d, size %d\n",i,size);
      }
    else
      {
      if( size > maxfree )
        maxfree = size;
      printf("Free at %d, size %d\n",i,size);
      }
    i+=size;
    }
  assert( i == totsize-1 );
  assert( mem[i] == -1 );
  printf("====== End map %s =======\n",title);
  }

void sh_cut(void *vp, int newsize)
  {
  checkptr(vp,"checkptr cut");
  MemBlock *p = MemBlock::block(vp);
  int size = - p->size();
  assert( size > 0 );
  MemBlock *down = p->downU();
  newsize = normsize(newsize);
  int cut = size - newsize;
  assert( cut >= 0 );
  if( cut == 0 )
    ;
  else if( down->size() > 0 )
    {
    //  down neighbour is free, expand it
    int dsize = down->size() + cut;
    down->unlink();
    p->setsize(newsize);
    down = p->downF();
    down->setsize(dsize);
    down->link();
    if( maxfree >= 0 && dsize > maxfree )
      maxfree = dsize;
    }
  else if( cut >= MINFREE )
    {
    // let's create new free block
    p->setsize(newsize);
    down = p->downF();
    down->link();
    down->setsize(cut); // set correct size
    if( maxfree >= 0 && cut > maxfree )
      maxfree = cut;
    }
  sh_map1("After cut");
  } 

