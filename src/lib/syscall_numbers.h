#ifndef __SYSCALLNUMBERS_H__
#define __SYSCALLNUMBERS_H__

/*** Syscall from console.h ***/
#define SYSCALL_SETCURSOR   100
#define SYSCALL_GETCURSOR   101
#define SYSCALL_PRINTSTR    102
#define SYSCALL_CLEARSCREEN 103
#define SYSCALL_GETCH       104

/*** Syscall from rtc.h ***/
#define SYSCALL_READRTCTIME 200
#define SYSCALL_READRTCDATE 201

/*** Syscall from task.h ***/
#define SYSCALL_CREATETASK         300
#define SYSCALL_SCHEDULE           301
#define SYSCALL_CHANGETASKPRIORITY 302
#define SYSCALL_ENDTASK            303
#define SYSCALL_EXIT               304
#define SYSCALL_GETTASKCOUNT       305
#define SYSCALL_EXISTTASK          306
#define SYSCALL_GETPROCESSORLOAD   307
#define SYSCALL_CHANGETASKAFFINITY 308
#define SYSCALL_CREATETHREAD       309

/*** Syscall from sync.h ***/
#define SYSCALL_LOCK   400
#define SYSCALL_UNLOCK 401

/*** Syscall from dynamic_mem.h ***/
#define SYSCALL_MALLOC 500
#define SYSCALL_FREE   501

/*** Syscall from hdd.h ***/
#define SYSCALL_READHDDSECTOR  600
#define SYSCALL_WRITEHDDSECTOR 601

/*** Syscall from file_system.h ***/
#define SYSCALL_FOPEN      700
#define SYSCALL_FREAD      701
#define SYSCALL_FWRITE     702
#define SYSCALL_FSEEK      703
#define SYSCALL_FCLOSE     704
#define SYSCALL_REMOVE     705
#define SYSCALL_OPENDIR    706
#define SYSCALL_READDIR    707
#define SYSCALL_REWINDDIR  708
#define SYSCALL_CLOSEDIR   709
#define SYSCALL_ISFOPEN    710

/*** Syscall from serial_port.h ***/
#define SYSCALL_SENDSERIALDATA  800
#define SYSCALL_RECVSERIALDATA  801
#define SYSCALL_CLEARSERIALFIFO 802

/*** Syscall from window.h ***/
#define SYSCALL_GETBACKGROUNDWINDOWID    900
#define SYSCALL_GETSCREENAREA            901
#define SYSCALL_CREATEWINDOW             902
#define SYSCALL_DELETEWINDOW             903
#define SYSCALL_SHOWWINDOW               904
#define SYSCALL_FINDWINDOWBYPOINT        905
#define SYSCALL_FINDWINDOWBYTITLE        906
#define SYSCALL_EXISTWINDOW              907
#define SYSCALL_GETTOPWINDOWID           908
#define SYSCALL_MOVEWINDOWTOTOP          909
#define SYSCALL_ISPOINTINTITLEBAR        910
#define SYSCALL_ISPOINTINCLOSEBUTTON     911
#define SYSCALL_MOVEWINDOW               912
#define SYSCALL_RESIZEWINDOW             913
#define SYSCALL_GETWINDOWAREA            914
#define SYSCALL_SENDEVENTTOWINDOW        915
#define SYSCALL_RECVEVENTFROMWINDOW      916
#define SYSCALL_UPDATESCREENBYID         917
#define SYSCALL_UPDATESCREENBYWINDOWAREA 918
#define SYSCALL_UPDATESCREENBYSCREENAREA 919
#define SYSCALL_DRAWWINDOWBACKGROUND     920
#define SYSCALL_DRAWWINDOWFRAME          921
#define SYSCALL_DRAWWINDOWTITLEBAR       922
#define SYSCALL_DRAWBUTTON               923
#define SYSCALL_DRAWPIXEL                924
#define SYSCALL_DRAWLINE                 925
#define SYSCALL_DRAWRECT                 926
#define SYSCALL_DRAWCIRCLE               927
#define SYSCALL_DRAWTEXT                 928
#define SYSCALL_BITBLT                   929
#define SYSCALL_MOVEMOUSECURSOR          930
#define SYSCALL_GETMOUSECURSORPOS        931

/*** Syscall from jpeg.h ***/
#define SYSCALL_INITJPEG   1000
#define SYSCALL_DECODEJPEG 1001

/*** Syscall from util.h ***/
#define SYSCALL_GETTOTALRAMSIZE 1100
#define SYSCALL_GETTICKCOUNT    1101
#define SYSCALL_SLEEP           1102
#define SYSCALL_ISGRAPHICMODE   1103

/*** Syscall from loader.h ***/
#define SYSCALL_EXECUTEAPP 1200

/*** Syscall - test ***/
#define SYSCALL_TEST 0xFFFFFFFFFFFFFFFF

#endif // __SYSCALLNUMBERS_H__