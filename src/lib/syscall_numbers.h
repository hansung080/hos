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

/*** Syscall from dynamic_mem.h ***/
#define SYSCALL_MALLOC 17
#define SYSCALL_FREE   18

/*** Syscall from hdd.h ***/
#define SYSCALL_READHDDSECTOR  19
#define SYSCALL_WRITEHDDSECTOR 20

/*** Syscall from file_system.h ***/
#define SYSCALL_FOPEN      21
#define SYSCALL_FREAD      22
#define SYSCALL_FWRITE     23
#define SYSCALL_FSEEK      24
#define SYSCALL_FCLOSE     25
#define SYSCALL_REMOVE     26
#define SYSCALL_OPENDIR    27
#define SYSCALL_READDIR    28
#define SYSCALL_REWINDDIR  29
#define SYSCALL_CLOSEDIR   30
#define SYSCALL_ISFOPEN    31

/*** Syscall from serial_port.h ***/
#define SYSCALL_SENDSERIALDATA  32
#define SYSCALL_RECVSERIALDATA  33
#define SYSCALL_CLEARSERIALFIFO 34

/*** Syscall from window.h ***/
#define SYSCALL_GETBACKGROUNDWINDOWID    35
#define SYSCALL_GETSCREENAREA            36
#define SYSCALL_CREATEWINDOW             37
#define SYSCALL_DELETEWINDOW             38
#define SYSCALL_SHOWWINDOW               39
#define SYSCALL_FINDWINDOWBYPOINT        40
#define SYSCALL_FINDWINDOWBYTITLE        41
#define SYSCALL_EXISTWINDOW              42
#define SYSCALL_GETTOPWINDOWID           43
#define SYSCALL_MOVEWINDOWTOTOP          44
#define SYSCALL_ISPOINTINTITLEBAR        45
#define SYSCALL_ISPOINTINCLOSEBUTTON     46
#define SYSCALL_MOVEWINDOW               47
#define SYSCALL_RESIZEWINDOW             48
#define SYSCALL_GETWINDOWAREA            49
#define SYSCALL_SENDEVENTTOWINDOW        50
#define SYSCALL_RECVEVENTFROMWINDOW      51
#define SYSCALL_UPDATESCREENBYID         52
#define SYSCALL_UPDATESCREENBYWINDOWAREA 53
#define SYSCALL_UPDATESCREENBYSCREENAREA 54
#define SYSCALL_DRAWWINDOWBACKGROUND     55
#define SYSCALL_DRAWWINDOWFRAME          56
#define SYSCALL_DRAWWINDOWTITLEBAR       57
#define SYSCALL_DRAWBUTTON               58
#define SYSCALL_DRAWPIXEL                59
#define SYSCALL_DRAWLINE                 60
#define SYSCALL_DRAWRECT                 61
#define SYSCALL_DRAWCIRCLE               62
#define SYSCALL_DRAWTEXT                 63
#define SYSCALL_BITBLT                   64
#define SYSCALL_MOVEMOUSECURSOR          65
#define SYSCALL_GETMOUSECURSORPOS        66

/*** Syscall from jpeg.h ***/
#define SYSCALL_INITJPEG   67
#define SYSCALL_DECODEJPEG 68

/*** Syscall from util.h ***/
#define SYSCALL_GETTOTALRAMSIZE 69
#define SYSCALL_GETTICKCOUNT    70
#define SYSCALL_SLEEP           71
#define SYSCALL_ISGRAPHICMODE   72

/*** Syscall - test ***/
#define SYSCALL_TEST 0xFFFFFFFF

#endif // __SYSCALLNUMBERS_H__