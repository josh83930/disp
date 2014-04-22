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
 $Id: headers.h,v 1.11 1996/10/16 12:41:02 chorusdq Exp $
*/

// The dispatcher program header file

#include "displowl.h"

// #define NDEBUG

#include "shmem.h"

static const int sh_mem_notif_size = 2*1024*sizeof(int);
extern int shared_memory_notif_id;
extern int shared_memory_data_id;

#ifndef Bool_Defined
// 	enum { false, true };
//typedef int bool;
#endif

extern int out_of_mem_counter;
extern time_t out_of_mem_last;
extern bool cleanup_pending;

extern int myhostid;

inline bool host_is_local(int hostid)
{
  return hostid == myhostid || hostid == 0x7f000001;
}

extern bool size_unacceptable(int size);

extern bool cleanup(); // tries to free some memory

inline void free_buff(void *buff)
  {
  if( buff )
    if( sh_inside(buff) )
      sh_free(buff);
    else
      free((char *)buff);
  }

inline void myclose(int socket)
  {
  socket_close(socket);
  }

class Ring   // double-linked ring
  {
  private:
    virtual void _dummy_() {} // to avoid problems with the descendants
                              // having virtual functions
    Ring *_next, *_prev;
  public:
    void init() { _next = _prev = this; }
    Ring() { init(); }
    void unlink() // remove the element from the ring
	{ _prev->_next = _next; _next->_prev = _prev; init(); }
   ~Ring() { unlink(); } // destruction, remove me from the list 
    Ring *next() const { return _next; }
    Ring *prev() const { return _prev; }
    bool is_empty_ring() const { return _next == this; }
    void operator += (Ring &p) // link before me element pointed by p
        {
	p._prev = _prev; p._next = this; 
	_prev->_next = &p; _prev = &p; 
	}
  };

class Client;    // represents a client, connected to dispatcher
class Info;      // represents data buffer
class Lock;      // represents lock in shared memory
class Tag;
class Subscr;

// we define dclRing macro to avoid using templates
//                                (so, improve portability)
#define dclRing(Type) \
class Type##Ring: public Ring { \
 public: \
   Type *next() const { return (Type *)Ring::next(); }\
   Type *prev() const { return (Type *)Ring::prev(); }\
 } 

dclRing(Client);
dclRing(Info);
dclRing(Lock);
dclRing(Tag);
dclRing(Subscr);

/* the template analog would look like:
template <class Type>
class ring : public Ring
  {
  public:
    Type *next() const { return (Type *)Ring::next(); }
    Type *prev() const { return (Type *)Ring::prev(); }
  };

and instead of 
  dclRing(Client);
  ClientRing a;
  ClientRing b;
we would use
  ring<Client> a;
  ring<Client> b;

*/

extern InfoRing info_root;  
	// header of the information ready buffers list 

extern ClientRing client_root;
	// header of the list of clients

extern ClientRing select_done_root;
	// temporary list for select_out

extern LockRing hanging_lock_root;
	// locks, whose client is already destroyed

extern TagRing tag_root; 
	// header of the list of all the tags

typedef long tagbits_t;

class Tag : public TagRing  // information about particular tag
  {
  private:
    char name[TAGSIZE+1];
    unsigned long nrecv;
    unsigned long nsent;
    unsigned long nmem;
    unsigned long nmemsize;
    Tag *hlink;
    static Tag *hash(const char *ctag, char normtag[TAGSIZE+1], int *hidx);
  protected:
    Tag(const char *ctag, int hidx);
    ~Tag(); 
    bool deletable;
  public:
    int work,work1;
    tagbits_t tagbit;
    SubscrRing subscr_root;
    virtual void Allocbuf(Client *p);  // allocate the buffer for incoming data
    virtual void React(Client *p);     // react to incoming tag
    bool may_be_ignored() const { return deletable && subscr_root.is_empty_ring(); }
    const char *PrintName() const { return name; }          // get tag name 
    void received() { nrecv++; }
    void sent() { nsent++; }
    void stored(int size) { nmem++; if( size > 0 ) nmemsize += size; }
    void dropped(int size) { nmem--; if( size > 0 ) nmemsize -= size; }
    bool is_real() const { return (TagRing *)this != &tag_root; }
    Subscr *subs(Client *p) const;
    static void printstat(int fd);
    static void init_tags();
    static Tag *find(const char *ctag);
    static Tag * const tagBorn;
    static Tag * const tagDied;
    static Tag * const tagWrongHdr;
    static void cleanup();
  };

class PermanentTag : public Tag
  {
  protected:
    PermanentTag(const char *ctag):Tag(ctag,-1) {deletable = false;}
  };

class Subscr : public SubscrRing
  {
  public:
    Client *client;
    Tag *tag;
    int mask;
    Subscr(Client *p, Tag *q, int m) { client = p; tag = q; mask = m; 
                                       q->subscr_root += *this; }
  };

const int VIAMEM = 1;
const int NONLOOSE = 2;
const int SYNCR = 4;
const int WANTED = 8;

class Cnt
  {
  private:
    short cnt;
    const char *dbg; 
  public:
    Cnt(const char *what) { cnt = 0; dbg=what;}
    void operator ++(int) { cnt++; 
       if( cnt > 1000 ){ fprintf(stderr,"Bad %s counter\n",dbg); 
                         assert( cnt <= 1000 ); }; }
    void operator --(int) { cnt--; 
       if( cnt < 0 ){ fprintf(stderr,"Bad %s counter\n",dbg); 
                         assert( cnt >= 0 );} ;}
    short val() const { return cnt; }
  };


class Info : public InfoRing     // tagged block
  {
  public:
    static unsigned infonum;
    unsigned num;
    Tag *tag;
    int size;
    void *data;
    Cnt locked;
    Cnt pointed;
    Cnt nonloose;
    void checksync(bool move=false);
    Info(Tag *srctag, int bsize, void *buff);
   ~Info();
    bool may_be_dropped() const { return locked.val()==0 && nonloose.val()==0; }
    bool is_pointed() const { return pointed.val() != 0; }
    bool is_real() const { return (InfoRing *)this != &info_root; }
    const char *tagname() const { return tag->PrintName(); }
  };

typedef volatile int volint_t;

class Lock : public LockRing
	// lock prevents the information block from being dropped
	// locks for each client are linked in a list
  {
  private:
    volint_t * plock;
    Info *info; 
    static int cnt;
    static volint_t* wrk;
    time_t client_died_at;
  public:
    Lock(Info *i) { plock = wrk; *plock = 1; info = i; i->locked++; cnt++; }; // initialisation
   ~Lock() { assert( *plock == 0 ); *plock = -1; info->locked--; cnt--; };
    void force_unlock() { *plock = 0; }
    bool active() const { return *plock != 0; };// check
    int sh_shift() const { return ((int *)plock) - ((int *)notif_shared_memory); };// position of value in shared memory
    void *operator new(size_t size) 
      { 
      // let's find free lock
      int i;
      const int lim = sh_mem_notif_size/sizeof(int);
      for( wrk = (volint_t *)notif_shared_memory, i=0; i < lim; ++i,++wrk)
        if( *wrk == -1 )
          break;
      if( i == lim )
        return NULL;
      return new char[size]; 
      }
    void to_dead_list() { unlink(); client_died_at = time(NULL); 
                          hanging_lock_root += *this; }
    bool expired() { return time(NULL) - client_died_at > 60; }
    static bool exist() { return cnt > 0; }
  };

typedef enum { skip_none, skip_calc, skip_all } discipline_t;

class Client : public ClientRing
  {
  public:
    int sock;
    char *host;
    bool remote;
    time_t connected_at;

    char *nick;

    unsigned got;
    unsigned tot;
    unsigned gotlast;
    unsigned totlast;

    short nselects;
    discipline_t discipline;
    int skip_cnt;
    double skip_coeff;
    Info *out_info;
    int out_pos;
    bool no_mem_for_lock;
    int waits;
    int out_mask;
    int out_size;
    struct {
      prefix_t pref;
      shdata_descr_t descr;
    } out_header;

    prefix_t in_pref;
    void *in_buff;
    bool no_mem_for_buff;
    Tag *in_tag;
    int in_pos;
    tagbits_t signature;

    LockRing lock_root;
    SubscrRing tmp;

    void cutstrbuff();
    void kill();
    void err(const char *msg);
    void stop_send() { assert(out_info->is_real() ); assert(  send_pending() );
                       out_pos = -1; out_info->locked--; }
    void store();
    void wrongdatasize();
    void wrongtag();
    Client(int socknum, char *hostname, bool isremote);
   ~Client() { kill(); }
    bool is_real() const { return (ClientRing *)this != &client_root; }
    int socket() const { return sock; }
    bool alive() const { return sock >= 0; }

    bool buff_pending() const { return no_mem_for_buff; };
    bool lock_pending() const { return no_mem_for_lock; };
    bool send_pending() const { return out_pos >= 0; };
    bool send_started() const { return out_pos > 0; };
    bool out_info_is_real() const { return out_info->is_real(); }

    void receive();
    void simulate_receive(const char *ctag, void * buffer, int buflen);
    void react();
    int  readdata();
    void select_out(); 
    bool info_may_be_skipped();
    void prep_send();
    void send();
    bool next_info();
    bool find_info();
    bool checklocks();  // true - some locks are destroyed

    static void printstat(int fd, Client *skip);
  };

#define ringloop(root,p) for(p=root.next(); p != &root; p = p->next())

// define the tag with specific reaction
#define defTag(tg) \
class sTag##tg : public PermanentTag { \
  public:\
    sTag##tg() : PermanentTag(DISPTAG_##tg) {}\
    virtual void React(Client *p); \
  };\
static sTag##tg w##tg;\
void sTag##tg::React(Client *pcl)
// End of defTag

inline bool msequ(const void *buf, int buflen, const char *str)
  {
  return strlen(str) == buflen && memcmp(buf,str,buflen) == 0;
  }
