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
 $Id: findclient.c,v 1.3 1995/03/20 23:13:23 ruten Exp $
*/

/* find out whether the client(s) with the given nickname is connected */
/* to dispatcher, and from wich host(s) the connection was done        */
 
#include <stdlib.h>
#include <stdio.h>

#include "dispatch.h"

int main(int argc, char *argv[])
  {
  char host[100];
  int rc;
  if( argc != 3 )
    {
    fprintf(stderr,"Format: findclient disp_host client_id\n");
    exit(1);
    }
  rc = whereis(argv[1],argv[2],host,sizeof(host));
  if( rc < 0 )
    exit(1);
  if( rc > 0 )
    puts(host);
  exit(0);
  }
