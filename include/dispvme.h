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
 $Id: dispvme.h,v 1.1.1.1 1999/01/07 18:43:15 qsno Exp $
*/

#define DISPTAG_VMEaddr         "VMEaddr"
#define DISPTAG_VMEready	"VMEready"
#define DISPTAG_VMEdupl 	"VMEdupl"

typedef struct {
  volatile int lock;  /* == -1 - locked */
  prefix_t header;      /* tag and size */
  char     buffer[1];   /* real buffer */
} vme_dsp_buffer_t;        /* buffer format */

typedef struct { 
  long base;
  int size;
} vme_dsp_init_t; /* send to dispatcher at VME init */

typedef struct { 
  long buffer_vmepos;
} vme_dsp_notify_t; /* send to dispatcher as notification */

/* Sender interface */
#ifdef __cplusplus
extern "C" { 
#endif
int vme_dsp_init(int socket, long vmeaddr, void *localaddr, int size);
void *vme_dsp_getbuf(const char *tag, int size);
int vme_dsp_notify(int socket, void *buf);
#ifdef __cplusplus
}
#endif
 
