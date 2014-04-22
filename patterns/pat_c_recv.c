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
 $Id: pat_c_recv.c,v 1.4 1995/04/25 18:13:39 ruten Exp $
*/


/* This is a template for buliding data processing
   client, which receives the data and commands from the dispatcher.
*/

#include "dispatch.h"

#define LIM 100000  /* set the proper value here */

static char buf[LIM];

int main(int argc, char *argv)
  {
  char * host;
  char tag[TAGSIZE+1];
  int rc;
  int nbytes;

  host = "name of the host where dispatcher runs"; 
or
  host = (char *)0; /* if you run on the same host as dispatcher */

  rc = init_2disp_link(host,"w SOMETAG1 w SOMETAG2 .....","a SOMETAG3 ...");

  /* we want to receive the data with the tags SOMETAG1, SOMETAG2,
     and commands with tags  SOMETAG3 ...
     If you are a local client, (i.e. you run on the same 
     host as dispatcher), you may replace
     some "w"s with "m"s. In this case the data buffers
     with corresponding tags will be shipped to you
     via shared memory, which is much faster for big
     amounts of data. Don't use "m" for small messages.
  */

  if( rc < 0 )
    exit(1); /* no connection */

#define ALWAYS 1 /* if you are capable to process all the data */
or
#define ALWAYS 0 /* if you need only a fraction */

  if( ALWAYS )
    {
    rc = send_me_always();
    if( rc < 0 )
      exit(2); /* connection is broken */
    }

  for(;;)
    {
    /* main loop */
    if( !ALWAYS )
      {
      rc = send_me_next();
      if( rc < 0 )
        exit(3); /* connection is broken */
      }
 
    rc = wait_head(tag,&nbytes); /* wait for arriving data */
  or
    rc = check_head(tag,&nbytes); /* just check without waiting */

    if( rc < 0 )
      exit(4); /* connection is broken */
    else if( rc > 0 )
      { 
      /* we got the tag and size (in bytes) of incoming data
         into variables "tag" and "nbytes"
      */ 
      rc = get_data(buf,LIM);
      /* get the data. If nbytes > LIM then
             the excessive data portion is simply lost.
         if nbytes <= LIM we get exactly nbytes bytes
      */
      if( rc < 0 )
        exit(5); /* connection is broken */

      /* here we have:    
         data tag in variable "tag"
         data size in variable "nbytes" 
             it reflects the amount of data sent,
             recieved portion may be less (if nbytes > sizeof(buf))
         data in array "buf"
      */

      process the data, depending on tag
      ..........................
      continue; /* go look for next data buffer */
      } /* end of current portion processing */

    do something private
    ....................
    } /* end of main loop */
  }
