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
 $Id: pat_c_send_wait.c,v 1.4 1995/04/25 18:13:41 ruten Exp $
*/

/* This is a template for buliding data sending   
   client, which sends the data to the dispatcher
   in blocking mode, which means, that when data
   transfer of the data block is initiated, the
   process blocks until it is finished.
*/

#include "dispatch.h"

static char buf[...];   /* send data buffer */

int main(int argc, char *argv)
  {
  char * host;
  int rc;
  int nbytes;

  host = "name of the host where dispatcher runs"; 
or
  host = (char *)0; /* if you run on the same host as dispatcher */

  rc = init_disp_link(host,"");
  if( rc < 0 )
    exit(1); /* no connection */

  for(;;)
    {
    /* main loop */

    get the data from somewhere (or generate them)
    and put them into buf
    nbytes = size of data in buf ( bytes! )

    rc = put_fulldata("TAG-OF-DATA-TO-BE-SENT",buf,nbytes);     

    if( rc < 0 )
      exit(2); /* connection is broken */
    } /* end of main loop */
  }
