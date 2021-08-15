#ifndef WINGs_DND
#define WINGs_DND

#include <WINGs/WINGs.h>

void DndInitialize(WMWidget *w);

void DndRegisterDropWidget(WMWidget *w, WMEventProc *handler, void *data);
void DndRegisterDrop(WMEventProc *handler);

void DndGetData(unsigned char **Data,unsigned long *Size);
void DndSetData(int Type,unsigned char *Data,unsigned long Size);

int DndDataType(XEvent *event);
unsigned int DndDragButtons(XEvent *event);

int DndHandleDragging(WMWidget *w, XEvent *event, WMPixmap *pixmap);

void DndRegisterDragWidget (WMWidget *w, WMEventProc * handler, void *data);

#define DndNotDnd       -1
#define DndUnknown      0
#define DndRawData      1
#define DndFile         2
#define DndFiles        3
#define DndText         4
#define DndDir          5
#define DndLink         6
#define DndExe          7

#define DndEND          8

extern int DragStarted;

#endif
