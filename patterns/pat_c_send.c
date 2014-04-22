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
 $Id: pat_c_send.c,v 1.4 1995/04/25 18:13:39 ruten Exp $
*/


/* This is a template for buliding data sending   
   client, which sends the data to the dispatcher.
*/

#include "dispatch.h"

static char buf[...];   /* send data buffer */

int main(int argc, char *argv)
  {
  char * host;
  int rc;
  int nbytes = -1;
  int send_pos = -1;

  host = "name of the host where dispatcher runs"; 
or
  host = (char *)0; /* if you run on the same host as dispatcher */

  rc = init_disp_link(host,"");
  if( rc < 0 )
    exit(1); /* no connection */

  for(;;)
    {
    /* main loop */

    if( nbytes < 0 )
      {
      /* the buffer is ready for new data portion */

      put something into buf
      ............
      nbytes = size of data in buf ( bytes! )
      send_pos = 0;
      }

    if( nbytes >= 0 )
      {
      /* the data portion sending is not finished yet */
      rc = put_data("TAG-OF-DATA-TO-BE-SENT" /* 8 chars max */
              ,buf,nbytes,&send_pos);
    or
      rc = put_fulldata("TAG-OF-DATA-TO-BE-SENT",buf,nbytes);     
      /* put_data sends what can be sent and returns rc=0    
         if the data transfer is not finished completely.
         put_fulldata always send the whole block, waiting
         for the end of transmission
      */

      if( rc < 0 )
        exit(5); /* connection is broken */
      if( rc > 0 )
        {
        /* data portion shipped completely */
        send_pos = -1;
        nbytes = -1;
        }
      }

    do something useful
    .....................

    } /* end of main loop */
  }
