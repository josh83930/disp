/*##########################################################################*/
/*                                                                          */
/* Copyright (c) 1993-1994 CASPUR Consortium                                */
/*                         c/o Universita' "La Sapienza", Rome, Italy       */
/* All rights reserved.                                                     */
/*                                                                          */
/* Permission is hereby granted, without written agreement and without      */
/* license or royalty fees, to use, copy, modify, and distribute this       */
/* software and its documentation for any purpose, provided that the        */
/* above copyright notice and the following two paragraphs and the team     */
/* reference appear in all copies of this software.                         */
/*                                                                          */
/* IN NO EVENT SHALL THE CASPUR CONSORTIUM BE LIABLE TO ANY PARTY FOR       */
/* DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING  */
/* OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE       */
/* CASPUR CONSORTIUM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    */
/*                                                                          */
/* THE CASPUR CONSORTIUM SPECIFICALLY DISCLAIMS ANY WARRANTIES,             */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY */
/* AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER   */
/* IS ON AN "AS IS" BASIS, AND THE CASPUR CONSORTIUM HAS NO OBLIGATION TO   */
/* PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.   */
/*                                                                          */
/*       +----------------------------------------------------------+       */
/*       |   The ControlHost Team: Ruten Gurin, Andrei Maslennikov  |       */
/*       |   Contact e-mail      : ControlHost@caspur.it            |       */
/*       +----------------------------------------------------------+       */
/*                                                                          */
/*##########################################################################*/

/*
  $Id: disploop.h,v 1.1.1.1 1999/01/07 18:43:15 qsno Exp $
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef void disp_reaction_t(const char *tag, void *data, int datalen);

int  dispconnect(const char *host);
int  dispdrop(int socket);
int  dispprio(int socket,int prio);
void dispalldrop(void);
int  dispalways(int socket);
int  dispsubscribe(int socket,const char *subscr);
int  dispmyid(int socket, const char *id);
int  dispsend(int socket, const char *tag, void *data, int datalen);
void dispsetreact(const char *tag, disp_reaction_t *reaction);
void disploop(void);

#if defined(VM) || defined(MVS)
char *dispconvert(const char *s);
#endif
#ifdef __cplusplus
}
#endif
