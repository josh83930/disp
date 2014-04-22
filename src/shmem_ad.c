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
 $Id: shmem_ad.c,v 1.8 1996/10/16 12:41:59 chorusdq Exp $
*/

/* shared memory access */

#include "displowl.h"
/* PEWG
 * These are needed for Solaris
 */ 
#ifdef Solaris
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#else

#ifdef UNIX


#include <sys/ipc.h>
#include <sys/shm.h>

#if !defined(OSF1)
#include <sys/shmem.h>
#endif
#endif
#endif

/*****************************************************************/
void *sh_mem_cre(char *fn, int modif, int access, long size, int *idp, int destroy_old)
{
   key_t Key;

   int id;

   *idp = -1;
   id = -1;
 
   Key=ftok(fn,modif); 

   /* unique key prepared */
   if( Key == -1 )
     return FAILPTR;
   
   if( destroy_old )
     {
     id = shmget(Key,size,0);
// PEWG messes around here.  If this thing already exists - use it as is
// This should get around the problem of not being able to restart the
// dispatcher if you are not the one who initially created the shared
// memories.  Note, though, that this fails if the new SIZE is larger than
// the old one.  In this case someone will have to explicitly delete the
// shared memory sections (ipcrm) before creating the new ones.
// Aall of this is academic, of course, if the dispatcher never crashes, 
// since it cleans up after itself on normal termination.
//
// If it doesn't already exist, create a new one
//
//     if( id >= 0 )
//       {
//       fprintf(stderr,"Shared memory segment is present already. fn=%s\n",fn);
//       if( shmctl(id,IPC_RMID,0) == 0 )
//         fprintf(stderr,"Old shared memory segment destroyed\n");
//       else
//         return FAILPTR;
//       }
     }
   
   /* initiate the shared segment, access opened for everybody */
   

//   *idp=shmget(Key,size,IPC_CREAT|IPC_EXCL|  access);
//
//   if( *idp == -1 )
//     {
//       return FAILPTR;
//     }
//PEWG again
      if (id < 0)
        id = shmget(Key,size,IPC_CREAT|IPC_EXCL|  access);
      if (id < 0)
        return FAILPTR;
      *idp = id;
  return (void *)shmat(*idp,0,0);
}

int sh_mem_destroy(int id)
  {
  return shmctl(id,IPC_RMID,0);
  }
