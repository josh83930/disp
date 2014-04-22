

c     $Id: pat_f_recv.f,v 1.3 1995/03/20 23:13:10 ruten Exp $


c      This is a template for buliding data processing
c        client, which receives the data and commands from the dispatcher.


      program main
      parameter (LIM=10000) 
      integer buf(LIM/4)
      character *200 charbuf
      charcter *40 host 
      logical always
      parameter (always = .True.)
c                   if you are capable to process all the data   
  or
      parameter (always = .False.)
c                   if you need only a fraction   

      host = 'name of the host where dispatcher runs' 
  or
      host = ' '
c        set host empty if you run on the same host as dispatcher

      call init_2disp_link(host,"w SOMETAG1 w SOMETAG2 .....",
     +   "w SOMETAG3 ...",irc)

c    we want to receive the data with the tags SOMETAG1, SOMETAG2,
c    and commands with tags  SOMETAG3 ...
c    If you are a local client, (i.e. you run on the same 
c    host as dispatcher), you may replace
c    some "w"s with "W"s. In this case the data buffers
c    with corresponding tags will be shipped to you
c    via shared memory, which is much faster for big
c    amounts of data. Don't use "W" for small messages.

      if( irc.lt.0 ) stop 331
c               no connection   


      if( ALWAYS ) then
        call send_me_always(irc)
        if( irc.lt.0 ) stop 332
c                 connection is broken   
      endif

 1    continue
c     main loop    
      if( .not.ALWAYS ) then
        call send_me_next(irc)
        if( irc.lt.0 ) stop 333
c                 connection is broken   
      endif
 
      call wait_head(tag,nbytes,irc)
c          wait for arriving data    
  or
      call check_head(tag,nbytes,irc)
c          just check without waiting    

      if( irc.lt.0 ) then
        stop 334
c                 connection is broken   
      else if( irc.gt.0 ) then
c       we got the tag and size (in bytes) of incoming data
c       into variables "tag" and "nbytes"

        if( incoming data portion is a string ) then
          call get_string(charbuf,irc)
        else
          call get_data(buf,LIM,irc)
        endif 
c       get the data into character buffer or into array,
c       depending on data tag
c       If nbytes is bigger than an input buffer
c           the excessive data portion is simply lost.
c       In other cases we get the complete incoming portion

        if( irc.lt.0 ) stop 335
c                   connection is broken   

c       here we have:    
c       data tag in variable "tag"
c       data in array "buf"
c       data size in variable "nbytes" 
c           it reflects the amount of data sent,
c           recieved portion may be less (if nbytes > sizeof(buf))

        process the data, depending on tag
        ..........................
        goto 1
c            go look for next data buffer   
c       end of current portion processing   
      endif 

      do something private
      ....................
      goto 1
c     end of main loop
      end
