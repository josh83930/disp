

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
c  or
c     parameter (always = .False.)
c                   if you need only a fraction   

c      host = 'name of the host where dispatcher runs' 
      host = 'manimal.npl.washington.edu' 
c  or
c      host = ' '
c        set host empty if you run on the same host as dispatcher

c      call init_2disp_link(host,"w SOMETAG1 w SOMETAG2 .....",
c     +   "w SOMETAG3 ...",irc)
      call init_disp_link(host,"w STATE w ECALRUN", irc )
      if(irc.lt.0) then
         write (*, *) 'could not connect to dispatcher'
         stop 552
      end if

c    we want to receive the data with the tags SOMETAG1, SOMETAG2,
c    and commands with tags  SOMETAG3 ...
c    If you are a local client, (i.e. you run on the same 
c    host as dispatcher), you may replace
c    some "w"s with "W"s. In this case the data buffers
c    with corresponding tags will be shipped to you
c    via shared memory, which is much faster for big
c    amounts of data. Don't use "W" for small messages.

      call  my_id('FTEST',irc)
      if(irc.lt.0) then
         write (*, *) 'could not send id to dispatcher'
         stop 553
      end if


      if( ALWAYS ) then
        call send_me_always(irc)
      endif
      if(irc.le.0) then
         write (*, *) 'could not use send_me_next'
         stop 555
      end if

 1    continue
c
c     =======================MAIN LOOP=============================!
c

c          wait for arriving data    
      call wait_head(tag,nbytes,irc)
      if(irc.le.0) then
         write (*, *) 'could not use wait_head'
         stop 555
      end if

c       we got the tag and size (in bytes) of incoming data
c       into variables "tag" and "nbytes"

      if(tag.eq.'STATE') then
       
         call  get_string(com,irc)
         if(irc.le.0) then
            write (*, *) 'died getting string'
            stop 556
         end if
         if(com(1:9).eq.'START-RUN' ) then
           
            write (*, *) 'received start run message'

         end if
         if(com(1:7).eq.'END-RUN') then

            write (*, *) 'received end run message'

         end if

      end if

      if(tag.eq.'ECALRUN') then

         call get_data(buf, LIM, irc)
         if(irc.le.0) then
            write (*, *) 'died getting data'
            stop 557
         end if
         write (*,*) 'got ', nbytes, ' bytes of data'

      end if

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

c        process the data, depending on tag
c        ..........................
        goto 1
c            go look for next data buffer   
c       end of current portion processing   
      endif 

c      do something private
c      ....................
      goto 1
c     end of main loop
      end
