/*   CHORUS-specific dispatcher routines */
/*
 $Id: dispchorus.h,v 1.1.1.1 1999/01/07 18:43:15 qsno Exp $
*/
#ifdef __cplusplus
extern "C" {
#endif

/* convert subsystem name to evb/dispatch host (and the tag in case of evb) */
int getevbhost(const char *subsystem, char *host, char *tag);
int getdisphost(const char *subsystem, char *host);
#ifndef OS9
int writedisphost(const char *subsystem);
int dropdisphost(const char *subsystem);
int sendevb(const char *host, const char *tag, const char *line);
int sendevb_no_wait(const char *host, const char *tag, const char *line, int delay);
#endif

#ifdef __cplusplus
  }
#endif
/*   CHORUS-specific end   */ 
