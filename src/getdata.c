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
 $Id: getdata.c,v 1.20 1996/04/02 09:45:12 ruten Exp $
*/


/* high level dispatcher communications interface */

#include <ctype.h>
#include "displowl.h"
#ifdef AIX
#include "fortsupp.h"
#endif
 
#if defined(VM) || defined(MVS)
#define cnvrt(str) str = dispconvert(str)
#define freecnvrt(str) free(str) 
#else
#define cnvrt(str)
#define freecnvrt(str) 
#endif

#ifndef OS9
static int cmdline = -1;
#endif
static int dataline = -1;
typedef enum { s_idle, s_data_pending, s_free_pending } status_t;
/* s_idle         - no events on any line         */
/* s_data_pending - header received, but not data */
/* s_free_pending - data are in shared memory or hidden buffer, or allocated buffer */
static status_t status = s_idle;
static int in_active; /* one of dataline/cmdline when status != s_idle */
static int size_active; /* data size when status != s_idle */
 
#ifdef UNIX
static shdata_descr_t shdescr;
static int real_unlock; /* if true when status == s_free_pending - the data is in shared memory */
#endif
static void *data_ptr; /* pointer to data when status == s_free_pending */
static char locbuf[MIN_VIAMEM]; /* local buffer to avoid malloc's for small data portions */

int connected(void)
  {
  return dataline >= 0;
  }

void drop_connection(void)
  {
  dispdrop(dataline);
  dataline = -1;
#ifndef OS9
  dispdrop(cmdline);
  cmdline = -1;
#endif
  if( status == s_free_pending )
    unlock_data();
  status = s_idle;
  }
 
int init_disp_link(const char *host, const char *subscr)
  {
  if( dataline >= 0 )
    return -1;
 
  dataline = dispconnect(host);
 
  if( dataline < 0 )
    return -1;

  status = s_idle;
 
  if( subscr != NULL && *subscr && 
      dispsubscribe(dataline,subscr) < 0 )
    {
    drop_connection();
    return -1;
    }

  return dataline;
  }
 
#ifndef OS9
int init_2disp_link(const char *host, const char *subscrdata, const char *subscrcmd)
  {
  if( init_disp_link(host,subscrcmd) < 0 )
    return -1;
  if( dispalways(dataline) < 0 )
    {
    drop_connection();
    return -1;
    }
  cmdline = dataline; 
  dataline = -1;
  if( init_disp_link(host,subscrdata) < 0 )
    return -1;
  dispprio(cmdline,1);
  return dataline;
  }
#endif
 
int resubscribe(const char *subscr)
  {
  if( dataline < 0 )
    return -1;
  if( dispsubscribe(dataline,subscr) < 0 )
    return -1;
  return 0;
  }

int set_skip_mode(const char *mode)
  {
  if( dataline < 0 )
    return -1;
  if( put_fullstring(DISPTAG_SkipMode,mode) < 0 )
    return -1;
  return 0;
  }
 
int whereis(const char *host, const char *id, char *reply, int maxreplen)
  {
  int i = create_client_socket(host,DISPATCH_PORT);
  int rc;
  char dummy;
  int exists = 0;
#if defined(VM) || defined(MVS)
  char *savrep;
#endif

  if( maxreplen > 0 )
    *reply = 0;
 
  if( i < 0 )
    return -1;
  
  cnvrt(id);
  rc = put_tagged_bwait(i,DISPTAG_WhereIs,id,strlen(id));
  freecnvrt(id);
  if( rc < 0 )
    {
    socket_close(i);
    return -1;
    }

  if( maxreplen > 32767 )
    maxreplen = 32767;
 
#if defined(VM) || defined(MVS)
  if(maxreplen>0)
    savrep = reply;
  else
    savrep = NULL;
#endif
 
  maxreplen--;
  for(rc=1; maxreplen > 0 && (rc=socket_read(i,reply,maxreplen)) > 0;
     maxreplen -= rc, reply += rc, exists = 1, *reply = 0 );
 
  if( rc > 0 )
    while( (rc=socket_read(i,&dummy,1)) > 0 )
      exists = 1;
 
  socket_close(i);
 
#if defined(VM) || defined(MVS)
  if( savrep )
    str2ebcdic(savrep);
#endif
 
  return exists;
  }
 
static int headut(char *tag, int *size, int wait)
  {
  int rc;

  if( dataline < 0 )
    return -2;

  if( status != s_idle )
    if( size_active == 0 )
      {
      assert( status == s_data_pending );
      status = s_idle;
      }
    else
      return -1; /* we have to finish with the previous data */

  do
  {
#ifndef OS9
  if( cmdline >= 0 )
    {
    int wt = wait? (-1):0;
    in_active = dispselect(wt,wt);
    if( in_active < 0 )
      if( in_active < -1 )
        return -1;
      else if( wait )
        continue;
      else
        return 0;
    assert( in_active == dataline || in_active == cmdline );
    }
  else
#endif
    in_active = dataline;
  rc = dispcheck(in_active,tag,&size_active,wait);
  if( rc < 0 )
    return -1;
  else if( rc == 0 && !wait )
    return 0;
  } while( rc <= 0 ); 

  assert( rc > 0 );

  status = s_data_pending;

#ifdef UNIX
  if( size_active < 0 )
    {
    data_ptr = getshared(in_active,&shdescr);
    if( data_ptr == NULL )
       {
       status = s_idle;
       return -1;
       }
    status = s_free_pending;
    real_unlock = 1;
    size_active = -size_active;
    }
#else
  assert( size_active >= 0 );
#endif
  *size = size_active;
  return 1;
  }
 
int check_head(char *tag, int *size)
  {
  return headut(tag,size,0);
  }
 
int wait_head(char *tag, int *size)
  {
  return headut(tag,size,1);
  }
 
int put_data(const char *tag, const void *buf, int size, int *pos)
  {
  if( dataline < 0 )
    return -1;
  return put_tagged_block(dataline,tag,buf,size,pos);
  }
 
int put_string(const char *tag, const char *buf, int *pos)
  {
  int rc;
  cnvrt(buf);
  rc = put_data(tag,buf,strlen(buf),pos);
  freecnvrt(buf);
  return rc;
  }
 
int put_fulldata(const char *tag, const void *buf, int size)
  {
  if( dataline < 0 )
    return -1;
  return put_tagged_bwait(dataline,tag,buf,size);
  }
 
int put_fullstring(const char *tag, const char *buf)
  {
  int rc;
  cnvrt(buf);
  rc = put_fulldata(tag,buf,strlen(buf));
  freecnvrt(buf);
  return rc;
  }
 
int put_zfullstring(const char *tag, const char *buf)
  {
  int rc;
  cnvrt(buf);
  rc = put_fulldata(tag,buf,strlen(buf)+1);
  freecnvrt(buf);
  return rc;
  }
 
int get_data(void *buf, int lim)
  {
  int rc;
/*
fprintf(stderr,"status %d on entry to get_data\n",status);
*/
  if( status == s_idle )
    return -1; /* no header yet */

  assert( lim >= 0 );

  if( status == s_free_pending )
    {
    if( lim > size_active )
      lim = size_active;
    memcpy(buf,data_ptr,lim);
    unlock_data();
    return 1;
    }

  if( size_active == 0 )
    {
    status = s_idle;
    return 1; /* there is nothing to get, so we already got it */
    }

  if( lim > size_active )
    lim = size_active;
 
  if( lim > 0 )
    rc = getbwait(in_active,buf,lim);
  else
    rc = 1;
  if( rc > 0 && lim < size_active )
    rc = skipbwait(in_active,size_active-lim);
  status = s_idle;
  return rc;
  }
 
int get_string(char *buf, int lim)
  {
  int rc;
  if( lim > size_active )
    lim = size_active+1;
  if( lim > 0 )
    lim--;
  if( lim >= 0 )
    buf[lim] = '\0';
  rc = get_data(buf,lim);
  if( rc <= 0 )
    return rc;
#if defined(VM) || defined(MVS)
  str2ebcdic(buf);
#endif
  return 1;
  }
 
#if defined(VM) || defined(MVS)
int send_me_always()
#else
int send_me_always(void)
#endif
  {
  return dispalways(dataline);
  }
 
#if defined(VM) || defined(MVS)
int send_me_next()
#else
int send_me_next(void)
#endif
  {
  return dispnext(dataline);
  }
 
int my_id(const char *id)
  {
  return dispmyid(dataline,id);
  }

int unique_id(const char *id)
  {
  return put_fullstring(DISPTAG_UniqueId,id);
  }

void * get_data_addr(void)
  {
  int rc;
/*
fprintf(stderr,"status %d on entry to get_data_addr\n",status);
*/
  if( status == s_free_pending )
    return data_ptr;

  if( status == s_idle )
    return NULL; /* no header yet */

  assert( size_active >= 0 );

  if( size_active <= sizeof(locbuf) )
    data_ptr = locbuf;
  else
    {
    data_ptr = malloc(size_active);
    if( data_ptr == NULL )
      return NULL;
    }
#ifdef UNIX
  real_unlock = 0;
#endif
  rc = get_data(data_ptr,size_active);
  status = s_free_pending;
  if( rc <= 0 )
    {
    unlock_data();
    return NULL;
    }
  return data_ptr;
  }
 
void unlock_data(void)
  {
  if( status == s_free_pending )
    {
    status = s_idle;
#ifdef UNIX
    if( real_unlock )
      {
      unlockshared(&shdescr);
      return;
      }
#endif
    assert( data_ptr != NULL );
    if( data_ptr != locbuf )
      free((char *)data_ptr);
    }
  }
 
