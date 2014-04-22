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
 $Id: client.C,v 1.18 1996/10/16 12:40:15 chorusdq Exp $
*/

// Client information management inside dispatcher

#include "headers.h"

ClientRing client_root;
LockRing hanging_lock_root;
int Lock::cnt = 0;
volint_t* Lock::wrk;

Client::Client(int socknum, char *hostname, bool isremote) :
  sock(socknum),
  host(strdup(hostname)),
  remote(isremote),
  connected_at(time(NULL)),
  nick(NULL),
  got(0),
  tot(0),
  gotlast(0),
  totlast(0),
  out_info((Info *)&info_root),
  out_pos(-1),
  nselects(0),
  no_mem_for_lock(false),
  waits(0),
  out_mask(0),
  out_size(0),
  discipline(skip_calc),
  skip_coeff(0.0),
  skip_cnt(-1),
  in_buff(NULL),
  no_mem_for_buff(false),
  in_tag(NULL),
  in_pos(-1),
  signature(0)
  {
  client_root += *this;
  }

bool Client::find_info()
  {
  assert( !send_pending() );
  for(out_mask=0; out_info->is_real(); out_info = out_info->next())
    {
    Subscr *sp;
    sp = out_info->tag->subs(this);
    if( sp )
      {
      out_mask = sp->mask;
      if( out_mask & VIAMEM ) 
        if( ! sh_inside(out_info->data) || out_info->size < MIN_VIAMEM )
          out_mask &= ~VIAMEM;
      out_info->pointed++;
      return true;
      }
    }
  return false;
  }

bool Client::next_info()
  {
  assert( out_info->is_real() );
  assert( !send_pending() );
  out_info->pointed--;
  if( out_mask )
    {
    if( skip_cnt > 0 )
      skip_cnt--;
    nselects = 0;
    no_mem_for_lock = false;
    if( out_mask & NONLOOSE )
      out_info->nonloose--;
    }
  else
    {
    assert( !nselects );
    assert( !no_mem_for_lock );
    }
  out_info = out_info->next();
  return find_info();
  }

void Client::kill()
  {
  if( !alive() )
    return;
  myclose(sock);
  sock = -1;
  in_pos = -1;
  if( in_buff )
    free_buff(in_buff);
  in_buff = NULL;

  while( !lock_root.is_empty_ring() )
    {
    Lock *p = lock_root.next();
    if( p->active() )
      p->to_dead_list();
    else
      delete p;
    }

  if( send_pending() )
    {
    stop_send();
    next_info();
    }

  if( signature )
    {
    while( out_info->is_real() ) 
      {
      Info *p;
      if( out_mask & SYNCR )
        p = out_info;
      else
        p = NULL;
      next_info();
      if( p )
        p->checksync();
      }
    Tag *tp;
    ringloop(tag_root,tp)
      {
      Subscr *sp = tp->subs(this);
      if( sp ) delete sp;
      }
    signature = 0;
    }

  free(host);
  host = NULL;

  if( nick )
    {
    /* client was identified, so we must send death announce */
    new Info(Tag::tagDied,strlen(nick),nick);
    nick = NULL;
    }
  
  in_tag = NULL;
  }

static char *ago(unsigned long t)
  {
  static char buf[80];
  char *p = buf;

  if( t >= 24*60*60 )
    {
    sprintf(p,"%lu days %c",t/(24*60*60),0);
    p += strlen(p);
    t %= (24*60*60);
    }
  if( t >= 60*60 )
    {
    sprintf(p,"%lu hours %c",t/(60*60),0);
    p += strlen(p);
    t %= (60*60);
    }
  if( t >= 60 )
    {
    sprintf(p,"%lu min %c",t/(60),0);
    p += strlen(p);
    t %= (60);
    }
  if( t > 0 || p == buf )
    {
    sprintf(p,"%lu sec %c",t,0);
    p += strlen(p);
    }
  strcpy(p,"ago");
  return buf;
  }

void Client::printstat(int fd, Client *skip)
  {
  char wbuf[200];
  const char *q;
  time_t t;
  time(&t);
  q = ctime(&t);
  write(fd,q,strlen(q));
  if( out_of_mem_counter )
    {
    char wbuf[100];
    time_t now;
    sprintf(wbuf,">>>> Number of memory overflows: %u\n%c",
      out_of_mem_counter,0);
    write(fd,wbuf,strlen(wbuf));
    time(&now);
    sprintf(wbuf,">>>> Last time %s at %c",
      ago(now-out_of_mem_last),0);
    write(fd,wbuf,strlen(wbuf));
    q = ctime(&out_of_mem_last);
    write(fd,q,strlen(q));
    }
  Tag::printstat(fd);
  q = "====== Active clients ==============\n";
  write(fd,q,strlen(q));
  Tag *tp;
  ringloop(tag_root,tp)
    while( ! tp->subscr_root.is_empty_ring() )
      {
      Subscr *sp = tp->subscr_root.next();
      sp->unlink();
      sp->client->tmp += *sp;
      }
  Client *p;
  ringloop(client_root,p)
    if( p != skip && p->alive() )
      {
  sprintf(wbuf,"line %3u, host %s%s%c",
     p->sock,p->remote?p->host:"local",p->nick?", nick ":"",0);
  write(fd,wbuf,strlen(wbuf));
  if( p->nick ) write(fd,p->nick,strlen(p->nick));
  sprintf(wbuf,", from %s%c",ctime(&(p->connected_at)),0);
  write(fd,wbuf,strlen(wbuf));
  int nlocks = 0;
  Lock *lp;
  ringloop(p->lock_root,lp)
    if( lp->active() )
       nlocks++;
  if( nlocks )
    {
    sprintf(wbuf,"  active sh_mem locks %d\n%c",nlocks,0);
    write(fd,wbuf,strlen(wbuf));
    }
  if( p->send_pending() )
    {
    char tag[TAGSIZE+1];
    int size;
    fromprefix(&p->out_header.pref,tag,&size);
    if( size >= 0 )
      size += sizeof(prefix_t);
    else
      size = sizeof(p->out_header);
    sprintf(wbuf,"  send pending: tag '%.8s', pos %d out_of %d\n%c",
      tag,p->out_pos,size,0);
    write(fd,wbuf,strlen(wbuf));
    }
  if( p->in_pos >= 0 ) 
    {
    sprintf(wbuf,"  recv pending: pos %d%s\n%c",
      p->in_pos,p->buff_pending()?", waiting for buffer space":"",0);
    write(fd,wbuf,strlen(wbuf));
    }
  if( p->got )   
    {
    sprintf(wbuf,"  stat:  got %8u,  tot %8u,  gotlast %8u,  totlast %8u\n%c",
       p->got,p->tot,p->gotlast,p->totlast,0);
    write(fd,wbuf,strlen(wbuf));
    } 
  bool nl = false;
  const char *q;
  Subscr *sp;
  ringloop(p->tmp,sp)
    {
    int m = sp->mask;
    if( !nl )
      {
      nl = true;
      q = "  subscr:";
      write(fd,q,strlen(q));
      }
    write(fd," ",1);
    if( m & NONLOOSE ) write(fd,"a",1);
    if( m & VIAMEM )   write(fd,"m",1);
    if( m & SYNCR )    write(fd,"s",1);
    if( (m & (NONLOOSE|VIAMEM|SYNCR)) == 0 ) 
      write(fd,"w",1);
    write(fd," ",1);
    q = sp->tag->PrintName();
    write(fd,q,strlen(q));
    }
  if( nl ) 
    write(fd,"\n",1);
      }
  q = "====== Active clients end ==========\n";
  write(fd,q,strlen(q));
  ringloop(client_root,p)
    while( ! p->tmp.is_empty_ring() )
      {
      Subscr *sp = p->tmp.next();
      sp->unlink();
      sp->tag->subscr_root += *sp;
      }
  }

bool Client::checklocks()
  {
  bool res = false;
  while( ! lock_root.is_empty_ring() && ! lock_root.next()->active() )
    {
    delete lock_root.next();
    res = true;
    }
  return res;
  }

bool Client::info_may_be_skipped()
  {
  assert( out_mask );
  if( out_mask & NONLOOSE )
    return false;
  if( waits )
    return false;
  if( discipline == skip_none )
    return false;
  if( discipline == skip_calc )
    {
    if( skip_cnt < 0 )
      skip_cnt = skip_coeff - 0.5; // we are trying to send a bit faster
    if( skip_cnt < 0 )
      skip_cnt = 0;
    if( skip_cnt <= 1 )
      return false;
    }
  return true;
  }

void Client::select_out()           
  {
  assert( out_mask );
  assert( !send_pending() );
  assert( out_info->is_real() );

  if( !nselects )
    {
    for(;;) 
      {
      if( discipline == skip_none )
        break;
      if( out_mask & SYNCR )
        break;
      if( ! info_may_be_skipped() )
        break;
      if( !next_info() )
        return;
      }
    nselects = 1;
    }
  if( out_mask & SYNCR )
    {
    if( nselects == 1 || nselects == 2 && waits )
      {
      if( waits )
        nselects = 3;
      else
        nselects = 2;
      out_info->checksync(true);
      } 
    }
  else if( waits )
    prep_send();
  }

void Client::prep_send()
  { 
  assert( out_info->is_real() ); 
  assert( !send_pending() );
  assert( waits );

  if( out_mask & VIAMEM ) 
    {
    Lock *p = new Lock(out_info);
    if( !p )
      {
      no_mem_for_lock = true;
      cleanup_pending = true;
      return;
      }
    no_mem_for_lock = false;
    lock_root += *p;
    out_size = - out_info->size;
    out_header.descr.shift_lock = p->sh_shift();
    out_header.descr.shift = ::sh_shift(out_info->data);
    out_header.descr.shared_memory_data_id = shared_memory_data_id;
    out_header.descr.shared_memory_notif_id = shared_memory_notif_id;
    }
  else
    out_size = out_info->size;

  fillprefix(&out_header.pref,out_info->tag->PrintName(),out_size);
  out_pos = 0;  
  out_info->locked++; 
  }

void Client::send()           
  {
  int rc;
  assert( send_pending() );
  if( out_mask & VIAMEM ) // send the prepared header only
    rc = putblock(sock,&out_header,sizeof(out_header),&out_pos);
  else
    { 
    // send the tag and size  
    if( out_pos < PSIZE )
      rc = putblock(sock,&out_header.pref,PSIZE,&out_pos);
    else 
      rc = 1;
    if( rc > 0 && out_size > 0 )
      {
      // send the buffer 
      out_pos -= PSIZE;
      rc = putblock(sock,out_info->data,out_size,&out_pos);
      out_pos += PSIZE;
      }
    }

  if( rc == 0 )
    return;  // transmission is unfinished

  if( rc < 0 )
    {
    // line broken, the client must be destroyed 
    kill();
    return;
    }

  if( waits > 0 )
    waits--;
  got++;
  gotlast++;
  skip_cnt = -1;
  out_info->tag->sent();
  stop_send();
  next_info();
  }

void Client::err(const char *msg)
  {
  time_t t = time(NULL);
  fputs(ctime(&t),stderr);
  fprintf(stderr,"Dropping the client%s%s on host %s, line %d - %s\n",
     nick?" ":"",nick?nick:"",remote?host:"local",sock,msg);
  fflush(stderr);
  kill();
  }

void Client::wrongdatasize()
  {
  Tag::tagWrongHdr->Allocbuf(this);
  }

void Client::wrongtag()
  {
  Tag::tagWrongHdr->Allocbuf(this);
  }

int Client::readdata()
  {
  int rc = 1;
  if( in_pref.__size )
    {
    in_pos -= PSIZE;
    if( in_buff )
      rc = getblock(sock,in_buff,in_pref.__size,&in_pos);
    else
      {
      int cnt = in_pref.__size - in_pos;
      rc = skipblock(sock,&cnt);
      in_pos = in_pref.__size - cnt;
      }
    in_pos += PSIZE;
    }
  return rc;
  }

void Client::receive()
  {
  int rc;
// printf("receive in_pos %d\n",in_pos);
  if(in_pos < 0 )
    in_pos = 0;
  if( in_pos < PSIZE )
    {
    assert( !in_buff );
    rc = getblock(sock,&in_pref,sizeof(prefix_t),&in_pos);
    if( rc <= 0 ) 
      goto escape;
    fromprefix(&in_pref,in_pref.__tag,&in_pref.__size);
    in_tag = Tag::find(in_pref.__tag);
    no_mem_for_buff = true; // to force Allocbuff call
    }
// printf("in receive 00 %s %-8.8s %d %d\n",in_tag->PrintName(),in_pref.__tag,in_pref.__size,in_pos);
  if( buff_pending() )
    {
    in_tag->Allocbuf(this);
    if( !alive() ) // Allocbuff could kill me
      return;
    if( buff_pending() )
      return;
    }
  rc = readdata();
  if( rc <= 0 )
    goto escape;
  in_tag->received();
  react();
  return;

escape:
  if( rc < 0 )
    kill();
  }

void Client::simulate_receive(const char *ctag, void * buffer, int buflen)
{
  in_tag = Tag::find(ctag);
  in_buff = buffer;
  in_pref.__size = buflen;
  in_pos = PSIZE + buflen;
  no_mem_for_buff = false;
  react();
}

void Client::react()
{
  assert(in_tag!=NULL);
  in_tag->React(this); // reaction depends on tag
  if( !alive() ) // React could kill me
    return;
  if( buff_pending() ) // React is unfinished
    return;
  in_tag = NULL;
  free_buff(in_buff);
  in_buff = NULL;
  in_pos = -1;
}

void Client::cutstrbuff()
  {
  char *q;
  assert( !in_buff || in_pref.__size > 0 );
  if( in_buff && (q = (char *)memchr(in_buff,'\0',in_pref.__size)) != NULL)
    {
    int len = q - (char *)in_buff;
    if( !len )
      {
      free_buff(in_buff);
      in_buff = NULL;
      }
    else if( sh_inside(in_buff) )
      sh_cut(in_buff,len);
    else
      in_buff = realloc(in_buff,len);
    in_pref.__size = len;
    }
  }

void Client::store()
  {
  new Info(in_tag,in_pref.__size,in_buff);
  in_buff = NULL;
  }
