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
 $Id: tcpio.c,v 1.5 1995/03/20 23:13:33 ruten Exp $
*/

/* 
   multi-attempt I/O support 
*/

#include "socksupp.h"

#ifdef HP_UX
#define NO_DATA_YET EAGAIN
#else
#define NO_DATA_YET EWOULDBLOCK
#endif

/* some platforms don't allow too big length in socket calls */
#define MAXLEN 32767

static int wouldblock;

/* get bsize bytes from socket
  result:
     -2    - wrong parameteres or some system error
     -1    - connection is closed
      0    - transfer is not complete yet, we have to keep trying
      1    - done with it
*/
int getblock(int socket, void *buf, int bsize, int *pos)
{
  int L = bsize - *pos;
  int rc;
  assert(*pos >= 0 );

  wouldblock = 0;

  while(L>0)
  {
  int L1 = (L>MAXLEN)?MAXLEN:L;
  rc = socket_read(socket,((char *)buf) + *pos,L1);
  if( rc == 0 )
    return -1;  /* connection is closed */
  if( rc < 0 )
    if( socket_errno == NO_DATA_YET )
      { 
      wouldblock = 1;
      return 0;  /* no data yet */
      }
    else
      return -2;  /* some error occured */
  *pos += rc;
  L -= L1;
  if( rc < L1 )
    return 0;  /* transmission incomplete, continue after a while */
  }
  return 1;  /* buffer is full */
}

/* skip *count bytes from socket
  result:
     -2    - wrong parameteres or some system error
     -1    - connection is closed
      0    - skipping is not complete yet, we have to keep trying
      1    - done with it
*/
int skipblock(int socket, int *count)
{
  wouldblock = 0;

  while( *count > 0 )
    {
    char buf[1000];
    int rc; 
    int L = ( *count > sizeof(buf) ) ? sizeof(buf) : *count;

    rc = socket_read(socket,buf,L);
    if( rc == 0 )
      return -1;  /* connection is closed */
    if( rc < 0 )
      if( socket_errno == NO_DATA_YET )
        {
        wouldblock = 1;
        return 0;  /* no data yet */
        }
      else
        return -2;  /* some error occured */
    *count -= rc;
    if( rc < L )
       return 0;  /* transmission incomplete, continue after a while */
    }

  return 1;  /* done */
}

/* put bsize bytes to socket
  result:
     -2    - wrong parameteres or some system error
     -1    - connection is closed
      0    - transfer is not complete yet, we have to keep trying
      1    - done with it
*/
int putblock(int socket, const void *buf, int bsize, int *pos)
{
  int L = bsize - *pos;
  int rc;
  assert(*pos >= 0 );
  
  wouldblock = 0;

  while(L>0)
  {
  int L1 = (L>MAXLEN)?MAXLEN:L;
  rc = socket_write(socket,((char *)buf) + *pos,L1);
  if( rc == 0 )
    return -1;  /* connection is closed */
  if( rc < 0 )
    if( socket_errno == NO_DATA_YET )
      {
      wouldblock = 1;
      return 0;  /* we have to wait */
      }
    else
      return -2;

  *pos += rc;
  L -= L1;
  if( rc < L1 )
    return 0;  /* transmission incomplete, continue after a while */
  }
  return 1;  /* done */
}

int getbwait(int socket, void *buf, int bsize)
  {
  int rc;
  int swaitdone = 0;
  int pos = 0;
  while( ( rc = getblock(socket,buf,bsize,&pos)) == 0 )
    if( !swaitdone && wouldblock )
      {
      set_nowait(socket,0);
      swaitdone = 1;
      }
  if( swaitdone )
    set_nowait(socket,1);
  return rc;
  }

int putbwait(int socket, const void *buf, int bsize)
  {
  int rc;
  int swaitdone = 0;
  int pos = 0;
  while( ( rc = putblock(socket,buf,bsize,&pos)) == 0 )
    if( !swaitdone && wouldblock )
      {
      set_nowait(socket,0);
      swaitdone = 1;
      }
  if( swaitdone )
    set_nowait(socket,1);
  return rc;
  }

int skipbwait(int socket, int bsize)
  {
  int rc;
  int swaitdone = 0;
  while( ( rc = skipblock(socket,&bsize)) == 0 )
    if( !swaitdone && wouldblock )
      {
      set_nowait(socket,0);
      swaitdone = 1;
      }
  if( swaitdone )
    set_nowait(socket,1);
  return rc;
  }
