$!###########################################################################
$!                                                                          #
$! Copyright (c) 1993-1994 CASPUR Consortium                                # 
$!                         c/o Universita' "La Sapienza", Rome, Italy       #
$! All rights reserved.                                                     #
$!                                                                          #
$! Permission is hereby granted, without written agreement and without      #
$! license or royalty fees, to use, copy, modify, and distribute this       #
$! software and its documentation for any purpose, provided that the        #
$! above copyright notice and the following two paragraphs and the team     #
$! reference appear in all copies of this software.                         #
$!                                                                          #
$! IN NO EVENT SHALL THE CASPUR CONSORTIUM BE LIABLE TO ANY PARTY FOR       #
$! DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING  #
$! OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE       #
$! CASPUR CONSORTIUM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    #
$!                                                                          #
$! THE CASPUR CONSORTIUM SPECIFICALLY DISCLAIMS ANY WARRANTIES,             #
$! INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY #
$! AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER   #
$! IS ON AN "AS IS" BASIS, AND THE CASPUR CONSORTIUM HAS NO OBLIGATION TO   #
$! PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.   #
$!                                                                          #
$!       +----------------------------------------------------------+       #
$!       |   The ControlHost Team: Ruten Gurin, Andrei Maslennikov  |       #
$!       |   Contact e-mail      : ControlHost@caspur.it            |       #
$!       +----------------------------------------------------------+       #
$!                                                                          #
$!###########################################################################
$!
$! $Id: makeVMS.com,v 1.5 1995/05/30 01:03:51 ruten Exp $
$!
$!
$! pattern .com file to create dispatcher client in VMS
$!
$! Let's compile support routines first
$!
$cc/define=(VMS) socketp
$cc/define=(VMS) tcpio
$cc/define=(VMS) tagbl
$cc/define=(VMS) getdata
$cc/define=(VMS) disploop
$cc/define=(VMS) dispreact
$cc/define=(VMS) missing
$!
$! Now compile the program
$cc/define=(VMS) 'p1'
$!
$! And link it
$!   multinet_root:[multinet]ucx$ipc_shr/share may be used instead of
$!   multinet:multinet_socket_library/share
$link 'p1',socketp,tcpio,tagbl,getdata,disploop,dispreact,missing,sys$input:/opt
multinet:multinet_socket_library/share
sys$share:vaxcrtl/share
$exit
