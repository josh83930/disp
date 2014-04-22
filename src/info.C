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
 $Id: info.C,v 1.6 1996/01/29 18:43:37 ruten Exp $
*/

// data buffer management inside dispatcher program

#include "headers.h"

InfoRing info_root;

unsigned Info::infonum = 0;

Info::Info(Tag *srctag, int sz, void *buff) :
  pointed("Pnt"),
  locked("Lck"),
  nonloose("Nll"),
  tag(srctag),
  size(sz),
  data(buff)
  {
  info_root += *this;
  tag->stored(sz);
  if( srctag->subscr_root.is_empty_ring() )
    {
    delete this;
    return;
    }
  num = ++infonum;
  if( num == 0 ) // overflow, renumber all
    {
    Info *p;
    ringloop(info_root,p)
      p->num = ++infonum;
    }
  Subscr *p;
  ringloop(srctag->subscr_root,p)
    {
    if( p->mask & NONLOOSE )
      nonloose++;
    Client *q = p->client;
    assert( q->alive() );
    q->tot++;
    q->totlast++;
#define RECALC_BOUND 100
    if( q->totlast == RECALC_BOUND )
      {
      if( q->discipline == skip_calc )
        q->skip_coeff = 
           (q->skip_coeff + double(RECALC_BOUND)/(q->gotlast+1))/2.0;
        // skip_coeff == N means "one send of N was done"
      q->totlast = 1;
      q->gotlast = 0;
      }
    if( !q->out_info->is_real() )
      {
      q->out_info = this;
      q->find_info();
      }
    }
  }

Info::~Info()
  {
  assert( may_be_dropped() );
  tag->dropped(size);
  free_buff(data);
  data = NULL;
  if( pointed.val() )
    {
    Subscr *p;
    ringloop(tag->subscr_root,p)
      if( p->client->out_info == this )
        p->client->next_info();
    assert( !pointed.val() );
    }
  }

void Info::checksync(bool move)
  {
  Subscr *p;
  bool ready = false;
  bool skip = false;
  ringloop(tag->subscr_root,p)
    if( p->mask & SYNCR )
      {
      Client *clp = p->client;
      Info *ip = clp->out_info;
      if( ! ip->is_real() )
        continue;
      int n = ip->num;
      if( n < num )
        return;
      if( n == num )
        {
        if( ! clp->out_mask )
          return;
        if( clp->send_pending() )
          continue;
        if( clp->waits )
          ready = true;
        else if( clp->info_may_be_skipped() )
          skip = true;
        else
          return; // client is not ready and may not skip the data
        if( ready && skip )
          return;
        }
      }
  if( !ready && !skip )
    return;
  // here only one of ready,skip is true
  assert( ready == !skip );
  ringloop(tag->subscr_root,p)
    if( p->mask & SYNCR )
      {
      Client *clp = p->client;
      if( clp->send_pending() || clp->out_info != this )
        continue;
      if( ready )
        clp->prep_send();
      else
        {
        clp->next_info();
        if( move )
          {
          clp->unlink();
          client_root += *clp;
          }
        }
      }
  }
