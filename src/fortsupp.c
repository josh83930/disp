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
 $Id: fortsupp.c,v 1.8 1995/04/24 09:59:39 gurin Exp $
*/


/* Fortran interface */

#include <ctype.h>
#include "displowl.h"
#include "fortsupp.h"
 
/*#ifdef AIX*/

int strc2f(char *fstr, int len, const char *cstr)
  {
  int L;
  if( len <= 0 )
    return -1;
  L = strlen(cstr);
  if( L > len )
    {
    memcpy(fstr,cstr,len);
    return -1;
    }
  else
    {
    memcpy(fstr,cstr,L);
    memset(fstr+L,' ',len-L);
    return 0;
    }
  }

char *strf2c(const char *fstr, int len)
  {
  char *p;
  assert( len >= 0 );
  while( len > 0 && fstr[len-1] == ' ' )
    len--;
  p = (char *)malloc(len+1);
  assert( p != NULL );
  memcpy(p,fstr,len);
  p[len] = 0;
  return p;
  }

void connected_(int *irc)
  {
  *irc = connected();
  }
 
void drop_connection_(void)
  {
  drop_connection();
  }
 
void whereis_(const char *host, const char *id, 
              char *reply, int hlen, int ilen, int rlen,
   int *irc)
  {
  char *hb = strf2c(host,hlen);
  char *ib = strf2c(id,ilen);
  char *rb = (char *)malloc(rlen+1);
  *irc = whereis(hb,ib,rb,rlen+1);
  strc2f(reply,rlen,rb);
  free(hb); free(ib); free(rb);
  }
 
void resubscribe_(const char *subscrib, int *irc, int slen)
  {
  char *sb = strf2c(subscrib,slen);
  *irc = resubscribe(sb);
  free(sb);
  }
 
void set_skip_mode_(const char *mode, int *irc, int mlen)
  {
  char *sb = strf2c(mode,mlen);
  *irc = set_skip_mode(sb);
  free(sb);
  }
 
void init_disp_link_(const char *host, const char *subscrib, 
                     int *irc, int hlen, int slen)
  {
  char *hb = strf2c(host,hlen);
  char *sb = strf2c(subscrib,slen);
  *irc = init_disp_link(hb,sb);
  free(hb); free(sb);
  }
 
void init_2disp_link_(const char *host, const char *subscrdata, 
                      const char *subscrcmd, int *irc,
                    int hlen, int slen, int clen)
  {
  char *hb = strf2c(host,hlen);
  char *sb = strf2c(subscrdata,slen);
  char *cb = strf2c(subscrcmd,clen);
  *irc = init_2disp_link(hb,sb,cb);
  free(hb); free(sb); free(cb);
  }
 
void check_head_(char *tag, int *size, int *irc, int tlen)
  {
  char tg[TAGSIZE+1];
  *irc = check_head(tg,size);
  if( *irc > 0 )
    strc2f(tag,tlen,tg);
  }
 
void wait_head_(char *tag, int *size, int *irc, int tlen)
  {
  char tg[TAGSIZE+1];
  *irc = wait_head(tg,size);
  if( *irc > 0 )
    strc2f(tag,tlen,tg);
  }
 
void put_string_(const char *tag, const char *str, 
                 int *pos, int *irc, int tlen, int slen)
  {
  char *tg = strf2c(tag,tlen);
  char *sb = strf2c(str,slen);
  *irc = put_string(tg,sb,pos);
  free(tg);free(sb);
  }
 
void put_data_(const char *tag, const void *buf, 
                int *size, int *pos, int *irc, int tlen)
  {
  char *tg = strf2c(tag,tlen);
  *irc = put_data(tg,buf,*size,pos);
  free(tg);
  }
 
void put_fulldata_(const char *tag, const void *buf, 
                   int *size, int *irc, int tlen)
  {
  char *tg = strf2c(tag,tlen);
  *irc = put_fulldata(tg,buf,*size);
  free(tg);
  }
 
void put_fullstring_(const char *tag, const char *str, 
                     int *irc, int tlen, int slen)
  {
  char *tg = strf2c(tag,tlen);
  char *sb = strf2c(str,slen);
  *irc = put_fullstring(tg,sb);
  free(tg); free(sb);
  }
 
void get_data_(void *buf, const int *size, int *irc)
  {
  *irc = get_data(buf,*size);
  }
 
void get_string_(char *str, int *irc, int slen)
  {
  char *sb = (char *)malloc(slen+1);
  *irc = get_string(sb,slen+1);
  if( *irc > 0 )
    strc2f(str,slen,sb);
  free(sb);
  }
 
void send_me_always_(int *irc)
  {
  *irc = send_me_always();
  }
 
void send_me_next_(int *irc)
  {
  *irc = send_me_next();
  }
 
void my_id_(const char *id, int *irc, int idlen)
  {
  char *ib = strf2c(id,idlen);
  *irc = my_id(ib);
  free(ib);
  }
 
void unique_id_(const char *id, int *irc, int idlen)
  {
  char *ib = strf2c(id,idlen);
  *irc = unique_id(ib);
  free(ib);
  }
 
void get_data_addr_(void **adr)
  {
  *adr = get_data_addr();
  }
 
void unlock_data_(void)
  {
  unlock_data();
  }
/*#endif*/
