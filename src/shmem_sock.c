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
 $Id: shmem_sock.c,v 1.6 1996/10/16 12:42:11 chorusdq Exp $
*/


/* data transfer via shared memory support */

#include "displowl.h"

#ifdef UNIX

#include <sys/ipc.h>
#include <sys/shm.h>

void *notif_shared_memory = FAILPTR;
void *data_shared_memory = FAILPTR;

void *getshared(int socket, shdata_descr_t *d)
  {
  int rc = getbwait(socket,d,sizeof(*d));
  if( rc <= 0 )
    return NULL;
/*
printf("getshared data %d %x %d notif %d %x %d\n",
  d->shared_memory_data_id,d->shared_memory_data_id,d->shift,
  d->shared_memory_notif_id,d->shared_memory_notif_id,d->shift_lock);
*/
  if( notif_shared_memory == FAILPTR )
    {
    notif_shared_memory = (void *)shmat(d->shared_memory_notif_id,0,0);
/*
printf("getshared notif ptr %p\n",notif_shared_memory);
*/
    if( notif_shared_memory == FAILPTR )
       return NULL;
    } 
  if( data_shared_memory == FAILPTR )
    {
    data_shared_memory = (void *)shmat(d->shared_memory_data_id,0,SHM_RDONLY);
/*
printf("getshared data ptr %p\n",data_shared_memory);
*/
    if( data_shared_memory == FAILPTR )
       return NULL;
    } 
  return ((int *)data_shared_memory)+d->shift;
  }

#endif
