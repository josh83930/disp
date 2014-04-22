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
 $Id: tagbl.c,v 1.6 1995/03/20 23:13:32 ruten Exp $
*/

/* tagged blocks transport */

#include "displowl.h"
#if defined(VM) || defined(MVS)
#include "ascebcd.h"
#endif

/* fillprefix/fromprefix may also be used to convert the prefix inplace */
void fillprefix(prefix_t *p, const char *tag, int size)
  {
  strncpy(p->__tag,tag,TAGSIZE);
#if defined(VM) || defined(MVS)
  mem2ascii(p->__tag,TAGSIZE);
#endif
  p->__size = htonl(size);
  }

void fromprefix(const prefix_t *p, char *tag, int* size)
  {
  int sz = ntohl(p->__size);
  memcpy(tag,p->__tag,TAGSIZE);
  tag[TAGSIZE] = 0;
#if defined(VM) || defined(MVS)
  mem2ebcdic(tag,TAGSIZE);
#endif
  *size = sz;
  }

int put_tagged_block(int socket, const char *tag, const void *buf, int size, int *pos)
  {
  int rc;
  prefix_t pref;

  if( *pos < PSIZE )
    {
    fillprefix(&pref,tag,size);
    rc = putblock(socket,&pref,PSIZE,pos);
    if( rc <= 0 )
      return rc;
    }
  if( size > 0 && *pos < PSIZE + size )
    {
    *pos -= PSIZE;
    rc = putblock(socket,buf,size,pos);
    *pos += PSIZE;
    return rc;
    }
  else
    return 1;
  } 
 
int put_tagged_bwait(int socket, const char *tag, const void *buf, int size)
  {
  int rc;
  prefix_t pref;

  fillprefix(&pref,tag,size);
  rc = putbwait(socket,&pref,PSIZE);
  if( rc > 0 && size > 0 )
    rc = putbwait(socket,buf,size);
  return rc;
  }
