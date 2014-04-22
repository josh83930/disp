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
 $Id: dispstat.c,v 1.5 1995/03/20 20:28:38 gurin Exp $
*/


/* query the dispatcher status */

#include "displowl.h"
#if defined(VM) || defined(MVS)
#include "ascebcd.h"
#endif

int main(int argc, char *argv[])
  {
  int i,rc;
  char ch;
  char *host;
  char *defaulthost = "localhost";

  if( argc > 2 )
    {
    fprintf(stderr,"Format: %s [ disp_host ]\n",argv[0]);
    return 1;
    }
  else if( argc == 2 )
    host = argv[1];
  else
    host = defaulthost; 

  while (1) {
  i = create_client_socket(host,DISPATCH_PORT);
 
  if( i < 0 || put_tagged_bwait(i,DISPTAG_ShowStat,NULL,0) < 0 )
    {
    socket_perror(argv[0]);
    return 1;
    }
  while( (rc=socket_read(i,&ch,sizeof(ch))) > 0 )
#if defined(VM) || defined(MVS)
    putchar(c2ebcdic(ch));
#else
#if defined OS9
    putchar( (ch == 0xa)?'\n':ch);
#else
    putchar(ch);
#endif
#endif

  socket_close(i);
  sleep (2);
  }
  return 0;
  }
