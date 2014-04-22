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
 $Id: reactions.C,v 1.18 1996/05/15 09:44:12 ruten Exp $
*/

// reactions on special tags

#include <ctype.h>
#include "headers.h"

class sTagWrong : public PermanentTag {
  public:
    sTagWrong(const char *ctag) : PermanentTag(ctag) {}
    virtual void Allocbuf(Client *p);
  };

#define defWrong(tg) \
static sTagWrong w##tg(DISPTAG_##tg); Tag *const Tag::tag##tg = &w##tg

defWrong(Born);
defWrong(Died);
defWrong(WrongHdr);

void sTagWrong::Allocbuf(Client *p)
  {
  char buf[200];
  sprintf(buf,"Wrong tag or size from %s: \'",p->remote?p->host:"local");
  for(int i=0; i<TAGSIZE; i++)
    {
    char c = p->in_pref.__tag[i];
    if( c == 0 )
      break;
    char *b = buf+strlen(buf);
    if( isgraph(c) )
      {
      *b = c;
      *++b = 0;
      }
    else
      sprintf(b,"\\x%02x",c);
    }
  sprintf(buf+strlen(buf),"\' size %d ( 0x%x )",
    p->in_pref.__size,p->in_pref.__size);
  int L = strlen(buf);
  p->in_buff = sh_alloc(L);
  if( p->in_buff )
    { 
    p->in_pref.__size = L;
    memcpy(p->in_buff,buf,L);
    p->no_mem_for_buff = false;
    p->in_tag = tagWrongHdr; 
    p->store(); 
    p->kill();
    }
  else
    { 
    p->no_mem_for_buff = true;
    cleanup_pending = true;
    }
  }

defTag(ShowStat)
  {
  int fd = pcl->socket();
  set_nowait(fd,0);
  Client::printstat(fd,pcl);
  pcl->kill();
  }

defTag(StopDisp)
  {
  printf("StopDisp command received\n");
  exit(0);
  }

defTag(CloseAll)
  {
  Client *p;
  ringloop(client_root,p)
    p->kill();
  }

defTag(Gime)
  {
  if( pcl->waits >= 0 )
    pcl->waits++;
  }

defTag(Always)
  {
  pcl->waits = -1;
  }

defTag(MyId)
  {
  if( pcl->nick ) 
    {
    new Info(Tag::tagDied,strlen(pcl->nick),pcl->nick);
    pcl->nick = NULL;
    }
  pcl->cutstrbuff();
  if( pcl->in_buff )
    {
    /* client sent us his id */
    /* let's create announce */
    pcl->nick = (char *)malloc(pcl->in_pref.__size+1);
    assert(pcl->nick!=NULL);
    memcpy(pcl->nick,pcl->in_buff,pcl->in_pref.__size);
    pcl->nick[pcl->in_pref.__size] = 0;
    new Info(Tag::tagBorn,pcl->in_pref.__size,pcl->in_buff);
    pcl->in_buff = NULL;
    }
  }

defTag(UniqueId)
  {
  if( pcl->nick ) 
    {
    new Info(Tag::tagDied,strlen(pcl->nick),pcl->nick);
    pcl->nick = NULL;
    }
  pcl->cutstrbuff();
  if( pcl->in_buff )
    {
    /* client sent us his id */
    Client *p;
    ringloop(client_root,p)
      if( p->alive() &&
        p->nick && msequ(pcl->in_buff,pcl->in_pref.__size,p->nick) )
        {
        // that nickname is used already
        pcl->kill();
        return;
        } 
    /* let's create announce */
    pcl->nick = (char *)malloc(pcl->in_pref.__size+1);
    assert(pcl->nick!=NULL);
    memcpy(pcl->nick,pcl->in_buff,pcl->in_pref.__size);
    pcl->nick[pcl->in_pref.__size] = 0;
    new Info(Tag::tagBorn,pcl->in_pref.__size,pcl->in_buff);
    pcl->in_buff = NULL;
    }
  }

defTag(SkipMode)
  {
  pcl->cutstrbuff();
  if( pcl->in_buff )
    if( msequ(pcl->in_buff,pcl->in_pref.__size,"none") )
      pcl->discipline = skip_none;
    else if( msequ(pcl->in_buff,pcl->in_pref.__size,"all") )
      pcl->discipline = skip_all;
    else if( msequ(pcl->in_buff,pcl->in_pref.__size,"calc") )
      pcl->discipline = skip_calc;
    else
      {
      free_buff(pcl->in_buff);
      pcl->in_buff = NULL;
      }
  if( !(pcl->in_buff) )
    pcl->kill();
  else if( !pcl->send_pending() && pcl->out_info->is_real() )
    {
    pcl->nselects = 0;
    pcl->no_mem_for_lock = false;
    pcl->out_mask = 0;
    pcl->out_info->pointed--;
    pcl->find_info();
    }
  }

defTag(WhereIs)
  {
  pcl->cutstrbuff();
  char blank = 0;
  Client *p;
  ringloop(client_root,p)
    if( p->alive() &&
        p->nick && msequ(pcl->in_buff,pcl->in_pref.__size,p->nick) ) 
       {
       if( blank )
         putbwait(pcl->sock,&blank,sizeof(blank));
       putbwait(pcl->sock,p->host,strlen(p->host));
       blank = ' ';
       }
  pcl->kill();
  } 

defTag(ClosLine)
  {
  pcl->cutstrbuff();
  int n = pcl->in_pref.__size;
  if( n <= 0 )
    return;
  for( char *q = (char *)(pcl->in_buff); n>0; n--,q++ )
    if( !isdigit(*q) )
      break;
  int s = (n>0)? -1 : atoi((char *)(pcl->in_buff));
  
  Client *p;
  ringloop(client_root,p)
    if( p->alive() )
      if( s >= 0 && p->socket() == s ||
        p->nick && msequ(pcl->in_buff,pcl->in_pref.__size,p->nick) )
       p->kill();
  }

defTag(Ignore)
  {
  }

defTag(CleanTgs)
  {
  Tag::cleanup();
  } 

defTag(Subscribe)
  {
  tagbits_t newsignature = 0;
  Tag *tp;
  ringloop(tag_root,tp)
    {
    tp->work1 = 0;
    Subscr *sp = tp->subs(pcl);
    if( sp )
      tp->work = sp->mask;
    else
      tp->work = 0;
    }
  char *q = (char *)(pcl->in_buff);
  char *qlim = q + pcl->in_pref.__size;
  while( q < qlim )
    {
    const int via_mem_mask = (pcl->remote)? 0 : VIAMEM;
    int m = WANTED; // WANTED must be always present
    /* skip leading blanks */
    while( q < qlim && isspace(*q) ) q++; if( q >= qlim ) break;
    while( q < qlim && !isspace(*q) )
      switch( *q++ )
        {
        case 'w':  // wanted
          break;
        case 'a':  // get all of them
          m |= NONLOOSE;
          break;
        case 's':  // syncronous
          m |= SYNCR;
          break;
        case 'm':  // via memory
        case 'W':
          m |= via_mem_mask;
          break;
        case 'S':  // 's'+'m'
          m |= (SYNCR | via_mem_mask);
          break;
        default:
          char w[100];
          sprintf(w,"incorrect subscription mode \'%c\'.",*q);
          pcl->err(w);
          return;
        }
    /* skip blanks */
    while( q < qlim && isspace(*q) ) q++; if( q >= qlim ) break;
    /* copy tag to internal buffer */
    char ctag[TAGSIZE+1];
    int i;
    for(i=0; q<qlim && !isspace(*q); q++) 
      if( i < TAGSIZE )
        ctag[i++] = *q;
    ctag[i] = 0;
    tp = Tag::find(ctag);
    if( tp == Tag::tagWrongHdr && strcmp(ctag,DISPTAG_WrongHdr) != 0 )
      {
      char w[100];
      sprintf(w,"wrong tag %s in subscription",ctag); 
      pcl->err(w);
      return;
      }
    tp->work1 |= m;
    newsignature |= tp->tagbit;
    }

  ringloop(tag_root,tp)
    {
    int omode = tp->work;
    int nmode = tp->work1;
    if( omode == nmode )
      continue;
    if( omode )
      {
      Subscr *sp = tp->subs(pcl);
      assert( sp != NULL );
      if( nmode )
        sp->mask = nmode;
      else
        delete sp;
      }
    else
      new Subscr(pcl,tp,nmode);
    }

  pcl->signature = newsignature;
  Info *p = pcl->out_info;
  if( pcl->send_pending() )
    p = p->next();
  else
    {
    pcl->nselects = 0;
    pcl->no_mem_for_lock = false;
    pcl->out_mask = 0;
    }
  for(; p->is_real(); p = p->next() )
    {
    int omode = p->tag->work;
    int nmode = p->tag->work1;
    if( omode == nmode )
      continue;
    if( (omode & NONLOOSE) != (nmode & NONLOOSE) )
      if( nmode & NONLOOSE )
        p->nonloose++;
      else
        p->nonloose--;
    if( (omode & SYNCR) != 0 && (nmode & SYNCR) == 0 )
      p->checksync();
    }
  if( !pcl->send_pending() && pcl->out_info->is_real() )
    {
    pcl->out_info->pointed--;
    pcl->find_info();
    }
}
  
defTag(Duplicate)
{
  char *q = (char *)(pcl->in_buff);
  char *qlim = q + pcl->in_pref.__size;
  char *q1, *q2;
  for(q1=q; q1 < qlim && isspace(*q1); q1++); 
  for(q2=q1; q2 < qlim && !isspace(*q2); q2++); 
  int L = q2-q1;
  if( L == 0 )
    {
    pcl->kill();
    return;
    }
  char *qhost = (char *)malloc(L+1);
  memcpy(qhost,q1,L);
  qhost[L] = 0;
  memset(q1,' ',L);
  const char *npref = "Duplicate to ";
  int nL = strlen(npref);
  int clihostid = his_inet_addr(qhost);
  int s;
  if( host_is_local(clihostid) || (s=create_client_socket(qhost,DISPATCH_PORT)) < 0 )
    {
    free(qhost);
    return;
    }
  char * newnick = (char *) malloc(nL+L);
  memcpy(newnick,npref,nL);
  memcpy(newnick+nL,qhost,L);
  Client * newpcl = new Client(s,qhost,true); // true == remote
  newpcl->simulate_receive(DISPTAG_MyId,newnick,nL+L);
  if( newpcl->alive() )
    {
    newpcl->simulate_receive(DISPTAG_Subscribe,pcl->in_buff,pcl->in_pref.__size);
    pcl->in_buff = NULL;
    }
  if( newpcl->alive() )
    newpcl->simulate_receive(DISPTAG_Always,NULL,0);
}










