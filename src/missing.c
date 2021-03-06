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
 $Id: missing.c,v 1.4 1995/05/27 19:30:13 gurin Exp $
*/

/* useful routines, missing on some architectures */

#include "socksupp.h"
 
#ifdef UNIX
#include <fcntl.h>
#endif
 
#ifdef OS9
unsigned usleep(unsigned microsec)
  {
  if( !microsec )
    return 0;
  microsec = microsec>>12;
  if( !microsec )
    microsec = 1;
  tsleep(microsec | 0x80000000);
  return 0;
  }
#endif
 
#if defined(VM) || defined(MVS) || defined(VMS) || defined(HP_UX) || defined(IRIX)
unsigned usleep(unsigned microsec)
  {
  struct timeval timeout;
  timeout.tv_sec = microsec/1000000;
  timeout.tv_usec = microsec%1000000;
  select(0,NULL,NULL,NULL,&timeout);
/* we return some garbage here ! */
  return 0;
  }
#endif

#ifdef AIX
void usleep_(int *t) /* Fortran interface for usleep */
 {
 usleep(*t);
 }
#endif

#if !defined(Solaris)
#ifdef SunOS
void atexit(void (*p)(void))
  {
  on_exit(p,0);
  }
#endif
#endif
