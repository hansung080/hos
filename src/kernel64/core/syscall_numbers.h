#ifndef __SYSCALLNUMBERS_H__
#define __SYSCALLNUMBERS_H__

/*** Syscall from console.h ***/
#define SYSCALL_SETCURSOR   1
#define SYSCALL_GETCURSOR   2
#define SYSCALL_PRINTSTR    3
#define SYSCALL_CLEARSCREEN 4
#define SYSCALL_GETCH       5

/*** Syscall from rtc.h ***/
#define SYSCALL_READRTCTIME 6
#define SYSCALL_READRTCDATE 7

/*** Syscall from task.h ***/
#define SYSCALL_CREATETASK         8
#define SYSCALL_SCHEDULE           9
#define SYSCALL_CHANGETASKPRIORITY 10
#define SYSCALL_ENDTASK            11
#define SYSCALL_EXIT               12
#define SYSCALL_GETTASKCOUNT       13
#define SYSCALL_EXISTTASK          14
#define SYSCALL_GETPROCESSORLOAD   15
#define SYSCALL_CHANGETASKAFFINITY 16

/*** Syscall from sync.h ***/
#define SYSCALL_LOCK   17
#define SYSCALL_UNLOCK 18

/*** Syscall from dynamic_mem.h ***/
#define SYSCALL_MALLOC 19
#define SYSCALL_FREE   20

/*** Syscall from hdd.h ***/
#define SYSCALL_READHDDSECTOR  21
#define SYSCALL_WRITEHDDSECTOR 22

/*** Syscall from file_system.h ***/
#define SYSCALL_FOPEN      23
#define SYSCALL_FREAD      24
#define SYSCALL_FWRITE     25
#define SYSCALL_FSEEK      26
#define SYSCALL_FCLOSE     27
#define SYSCALL_REMOVE     28
#define SYSCALL_OPENDIR    29
#define SYSCALL_READDIR    30
#define SYSCALL_REWINDDIR  31
#define SYSCALL_CLOSEDIR   32
#define SYSCALL_ISFOPEN    33

/*** Syscall from serial_port.h ***/
#define SYSCALL_SENDSERIALDATA  34
#define SYSCALL_RECVSERIALDATA  35
#define SYSCALL_CLEARSERIALFIFO 36

/*** Syscall from window.h ***/
#define SYSCALL_GETBACKGROUNDWINDOWID    37
#define SYSCALL_GETSCREENAREA            38
#define SYSCALL_CREATEWINDOW             39
#define SYSCALL_DELETEWINDOW             40
#define SYSCALL_SHOWWINDOW               41
#define SYSCALL_FINDWINDOWBYPOINT        42
#define SYSCALL_FINDWINDOWBYTITLE        43
#define SYSCALL_EXISTWINDOW              44
#define SYSCALL_GETTOPWINDOWID           45
#define SYSCALL_MOVEWINDOWTOTOP          46
#define SYSCALL_ISPOINTINTITLEBAR        47
#define SYSCALL_ISPOINTINCLOSEBUTTON     48
#define SYSCALL_MOVEWINDOW               49
#define SYSCALL_RESIZEWINDOW             50
#define SYSCALL_GETWINDOWAREA            51
#define SYSCALL_SENDEVENTTOWINDOW        52
#define SYSCALL_RECVEVENTFROMWINDOW      53
#define SYSCALL_UPDATESCREENBYID         54
#define SYSCALL_UPDATESCREENBYWINDOWAREA 55
#define SYSCALL_UPDATESCREENBYSCREENAREA 56
#define SYSCALL_DRAWWINDOWBACKGROUND     57
#define SYSCALL_DRAWWINDOWFRAME          58
#define SYSCALL_DRAWWINDOWTITLEBAR       59
#define SYSCALL_DRAWBUTTON               60
#define SYSCALL_DRAWPIXEL                61
#define SYSCALL_DRAWLINE                 62
#define SYSCALL_DRAWRECT                 63
#define SYSCALL_DRAWCIRCLE               64
#define SYSCALL_DRAWTEXT                 65
#define SYSCALL_BITBLT                   66
#define SYSCALL_MOVEMOUSECURSOR          67
#define SYSCALL_GETMOUSECURSORPOS        68

/*** Syscall from jpeg.h ***/
#define SYSCALL_INITJPEG   69
#define SYSCALL_DECODEJPEG 70

/*** Syscall from util.h ***/
#define SYSCALL_GETTOTALRAMSIZE 71
#define SYSCALL_GETTICKCOUNT    72
#define SYSCALL_SLEEP           73
#define SYSCALL_ISGRAPHICMODE   74

/*** Syscall - test ***/
#define SYSCALL_TEST 0xFFFFFFFF

#endif // __SYSCALLNUMBERS_H__