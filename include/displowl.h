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
 $Id: displowl.h,v 1.1.1.1 1999/01/07 18:43:15 qsno Exp $
*/

/* Low level dispatcher communications interface */

#define DISPTAG_Subscribe	"Subscrib"
#define DISPTAG_Gime		"Gime"
#define DISPTAG_Always    	"Always"
#define DISPTAG_SkipMode	"SkipMode"
#define DISPTAG_MyId		"MyId"
#define DISPTAG_UniqueId	"UniqueId"
#define DISPTAG_ClosLine	"ClosLine"
#define DISPTAG_CloseAll	"CloseAll"
#define DISPTAG_CleanTgs	"CleanTgs"
#define DISPTAG_StopDisp	"StopDisp"
#define DISPTAG_Born		"Born"
#define DISPTAG_Died		"Died"
#define DISPTAG_ShowStat	"ShowStat"
#define DISPTAG_WhereIs 	"WhereIs"
#define DISPTAG_WrongHdr	"WrongHdr"
#define DISPTAG_Ignore  	"Ignore"
#define DISPTAG_Duplicate	"Duplicat"

#define MIN_VIAMEM 300 /* minimal portion to be shipped via shared memory */

#ifdef __cplusplus
extern "C" {
#endif

int dispnext(int socket);
void dispallnext(void);
int dispchannels(void);
const char *dispqhost(int socket);
int dispcheck(int socket, char *tag, int *size, int wait);
int dispselect(int sec, int microsec); /* on os9 - dummy version */

#ifdef __cplusplus
}
#endif

#include "socksupp.h"
#include "dispatch.h"
#include "disploop.h"

#define DISPATCH_PORT 5553

typedef struct 
  {
  char __tag[TAGSIZE];
  int  __size;
  } prefix_t;

#ifdef UNIX
typedef struct
  {
  int shared_memory_notif_id;
  int shift;
  int shift_lock;
  int shared_memory_data_id;
  } shdata_descr_t;
#endif

#define PSIZE sizeof(prefix_t)

#ifdef __cplusplus
extern "C" {
#endif

#if defined(VM) || defined(MVS)
#define put_tagged_block(a,b,c,d,e) puttagbl(a,b,c,d,e)
#endif


#ifdef UNIX
extern void *notif_shared_memory;
extern void *data_shared_memory;
#define FAILPTR ((void *)(-1))

void *sh_mem_cre(char *fn, int modif, int access, long size, int *id, int destroy_old);
int   sh_mem_destroy(int id);
void *getshared(int socket, shdata_descr_t *shdescrp);
#define unlockshared(shdescrp) \
	(void)(((int *)notif_shared_memory)[(shdescrp)->shift_lock]=0)
#endif

void fillprefix(prefix_t *p, const char *tag, int size);
void fromprefix(const prefix_t *p, char *tag, int *size);
int put_tagged_block(int socket, const char *tag, const void *buf, int size, int *pos);
int put_tagged_bwait(int socket, const char *tag, const void *buf, int size);

#ifdef __cplusplus
}
#endif
