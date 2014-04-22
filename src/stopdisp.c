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
 $Id: stopdisp.c,v 1.5 1995/04/25 18:13:46 ruten Exp $
*/

/* stop the running dispatcher */

#include "displowl.h"

int main(int argc, char *argv[])
  {
  int rc;

  if( argc != 2 )
    {
    fprintf(stderr,"Format: %s disp_host\n",argv[0]);
    exit(1);
    }

  rc = init_disp_link(argv[1],"");
 
  if( rc < 0 )
    {
    socket_perror(argv[0]);
    exit(1);
    }

  if( put_fulldata(DISPTAG_StopDisp,NULL,0) < 0 )
    {
    socket_perror(argv[0]);
    exit(1); 
    }

  drop_connection();

  return 0;
  }
