/*
   Copyright (C) 1996 César Crusius
   Copyright (C) 1999 Igor Roboul - WINGs adaptation

   This file is part of the DND Library.  This library is free
   software; you can redistribute it and/or modify it under the terms of
   the GNU Library General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your
   option) any later version.  This library is distributed in the hope
   that it will be useful, but WITHOUT ANY WARRANTY; without even the
   implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
   PURPOSE.  See the GNU Library General Public License for more details.
   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the Free Software
   Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <WINGs/WINGsP.h>
#include <X11/Xatom.h>
#include <X11/Xmu/WinUtil.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>
#include <values.h>

#include "DnD.h"

/* #define DEBUG */
/* Local variables */

static Display          *dpy;          /* current display              */
static XButtonEvent     StartEvent;    /* event that started the drag  */
static int              DragPrecision; /* minimum dx,dy to start drag  */
static int              Dragging;      /* Drag state flag              */
static int              DataOK;        /* Non-zero if data registered  */
static Atom             DndProtocol;   /* ClientMessage identifier     */
static Atom             DndSelection;  /* For the data transfers       */
static Atom             WM_STATE;      /* Needed for icon stuff        */
static Window           Target;        /* Drop window                  */
static WMWindow	       *MainWindow;    /* Main window of application   */
static int              DataType;      /* Current drag data type       */
static int              RootFlag;      /* Non-zero if dropped on root  */
static WMEventProc      *OtherDrop;    /* Handler for non-registered widgets*/
static XColor	        Black,White;   /* For the cursors              */

int 				DragStarted;

static void DndDispatchEvent (XEvent * event, void *data);
static void slideView(WMView *view, int srcX, int srcY, int dstX, int dstY);

/* Initialize Drag'n'Drop */
void
DndInitialize (WMWidget *w)
{
    int	     screen,i;
  
    dpy      = (W_VIEW(w))->screen->display;
    
    DndProtocol = XInternAtom (dpy, "DndProtocol", False);
    DndSelection = XInternAtom (dpy, "DndSelection", False);
    
    WMCreateEventHandler (W_VIEW(w), ClientMessageMask, 
			  DndDispatchEvent, W_VIEW(w));
    
    MainWindow = (WMWindow *)((W_VIEW(w))->window);
    OtherDrop = NULL;
    Dragging=0;
    DragPrecision=20;
    DragStarted=0;
    
}

void
DndRegisterOtherDrop(WMEventProc * handler)
{
  OtherDrop = handler;
}

/* Update target widget info */
static void
DndUpdateTargetProc (XEvent * event, void *data)
{
  if (event->type == EnterNotify)
    Target = event->xany.window;
  else
    Target = 0;
}

/* Register widget as drop target */
void
DndRegisterDropWidget (WMWidget *w, WMEventProc * handler, void *data)
{
  WMCreateEventHandler (W_VIEW(w), EnterWindowMask | LeaveWindowMask,
			DndUpdateTargetProc, data);
  WMCreateEventHandler (W_VIEW(w), ClientMessageMask, handler, data);
}

/* Initializes the drag and rop structures. Binded to ButtonPress event 
   in registered drag widgets */
static void
DndStartAction(XEvent *event, void *data)
{
  if (event->type==ButtonRelease) {
	DragStarted=0;
	return;
  }
  StartEvent=*((XButtonEvent *)event);
  Dragging=0; DataOK=0;
  DragStarted=!DragStarted;
#ifdef DEBUG
  fprintf(stderr,"Initializing DnD variables...\n");
  fprintf(stderr,"Root 0x%x Window 0x%x Subwindow 0x%x\n",
	  StartEvent.root, StartEvent.window, StartEvent.subwindow);
#endif
}

/* Register widget as drag source */
void 
DndRegisterDragWidget (WMWidget *w, WMEventProc * handler, void *data)
{
  WMCreateEventHandler (W_VIEW(w), ButtonPressMask|ButtonReleaseMask, DndStartAction, data);
  WMCreateEventHandler (W_VIEW(w), ButtonMotionMask, handler, data);
}

/* Dispatch Drop events */
/* Send Drop message to real drop target */
static void
DndDispatchEvent (XEvent * event, void *data)
{
  if ((event->type != ClientMessage) ||
      (event->xclient.message_type != DndProtocol))
    return;

  if (!Target && OtherDrop)	 /* Drop on non-registered widget */
	OtherDrop(event,data);
	
  if (WMWidgetXID (data) == Target)
    return;

  event->xclient.window = Target;
  XSendEvent (dpy, Target, False, NoEventMask, event);
}

void
DndGetData (unsigned char **Data, unsigned long *Size)
{
  Window root = DefaultRootWindow (dpy);

  Atom ActualType;
  int ActualFormat;
  unsigned long RemainingBytes;

  XGetWindowProperty (dpy, root, DndSelection,
		      0L, 1000000L,
		      False, AnyPropertyType,
		      &ActualType, &ActualFormat,
		      Size, &RemainingBytes,
		      Data);
}

int
DndDataType (XEvent * ev)
{
  int Type;

  if (ev->xclient.message_type != DndProtocol)
    return DndNotDnd;
  Type = (int) (ev->xclient.data.l[0]);
  if (Type >= DndEND)
    Type = DndUnknown;
  return Type;
}

unsigned int
DndDragButtons (XEvent * event)
{
  if (event->xclient.message_type != DndProtocol)
    return 0;
  return (unsigned int) (event->xclient.data.l[1]);
}

void
DndSetData (int Type, unsigned char *Data, unsigned long Size)
{
  Window root = DefaultRootWindow (dpy);
  int AuxSize;

  if (DataOK)
    return;

  /* Set the data type */
  DataType = Type >= DndEND ? 0 : Type;

  /* Set the data */
  AuxSize = Size > INT_MAX ? INT_MAX : (int) Size;
  XChangeProperty (dpy, root, DndSelection, XA_STRING, 8,
		   PropModeReplace, Data, AuxSize);

  for (Size -= (unsigned long) AuxSize; Size; Size -= (unsigned long) AuxSize)
    {
      Data += AuxSize;
      AuxSize = Size > INT_MAX ? INT_MAX : (int) Size;
      XChangeProperty (dpy, root, DndSelection, XA_STRING, 8,
		       PropModeAppend, Data, AuxSize);
    }

  /* Everything is now ok */
  DataOK = 1;
}

/* Handle dragging. This function must be called 
   from application drag handler */
int
DndHandleDragging(WMWidget *w, XEvent *event, WMPixmap *pixmap)
{
    WMSize size;
    WMView *dragView;
    WMScreen *scr = WMWidgetScreen(w);
    Display  *dpy = WMScreenDisplay(scr);
/*     WMPixmap *pixmap = NULL; */
    XEvent Event;
    Window DispatchWindow;
    Window root = DefaultRootWindow(dpy);
    
    if(!DragStarted) return 0;
    if(Dragging) 
    { 
#ifdef DEBUG
	fprintf(stderr,"Dragging...\n");
#endif
	return 0;
    }
    if(abs(StartEvent.x_root - event->xmotion.x_root) < DragPrecision &&
       abs(StartEvent.y_root - event->xmotion.y_root) < DragPrecision)
    {
	return 0;
    }
#ifdef DEBUG
    fprintf(stderr,"StartEvent.x_root=%d\nevent->xmotion.x_root=%d\n",
	    StartEvent.x_root, event->xmotion.x_root);
#endif
  
    dragView = W_CreateTopView(scr);
    dragView->attribFlags |= CWOverrideRedirect | CWSaveUnder;
    dragView->attribs.event_mask = StructureNotifyMask;
    dragView->attribs.override_redirect = True;
    dragView->attribs.save_under = True;
    
    size = WMGetPixmapSize(pixmap);
    W_ResizeView(dragView, size.width, size.height);
    
    W_MoveView(dragView, event->xmotion.x_root-8, event->xmotion.y_root-8);
    
    W_RealizeView(dragView);
    
    W_MapView(dragView);
    
    XSetWindowBackgroundPixmap(dpy, dragView->window, WMGetPixmapXID(pixmap));
    XClearWindow(dpy, dragView->window);
    XUngrabPointer(dpy,CurrentTime);
    XGrabPointer(dpy,root,False, 
		 ButtonMotionMask|ButtonPressMask|ButtonReleaseMask,
		 GrabModeSync,GrabModeAsync,root,
		 None,
		 CurrentTime);
    /*   XGrabPointer(dpy,root,False,  */
    /* 	       ButtonMotionMask|ButtonPressMask|ButtonReleaseMask, */
    /* 	       GrabModeSync,GrabModeAsync,root, */
    /* 	       DndCursor[DataType].CursorID, */
    /* 	       CurrentTime); */
    
    /*     XGrabPointer(dpy, dragView->window, True, */
    /*                  ButtonMotionMask|ButtonReleaseMask|EnterWindowMask, */
    /*                  GrabModeSync, GrabModeAsync, */
    /*                  scr->rootWin, scr->defaultCursor, CurrentTime); */
    
    Dragging=1; RootFlag=0;
    while(Dragging) 
    {
	XAllowEvents(dpy,SyncPointer,CurrentTime);
	WMNextEvent(dpy, &Event);
	/* 	XNextEvent(dpy, &Event); */
	switch(Event.type)
	{
	case ButtonRelease:
	    if(Event.xbutton.subwindow) 
		RootFlag=0;
	    else
	    {
		RootFlag=1;
		slideView(dragView, 
			  Event.xbutton.x_root, Event.xbutton.y_root,
			  event->xmotion.x_root, event->xmotion.y_root);
	    }
	    Dragging=0;
	    break;
	    
	case MotionNotify:
            W_MoveView(dragView,
		       Event.xmotion.x_root+1,Event.xmotion.y_root+1);
            break;

	default:
	    WMHandleEvent(&Event);
	}
    }
    DataOK=0;
    XUngrabPointer(dpy,CurrentTime);
    W_DestroyView(dragView);
#ifdef DEBUG 
    fprintf(stderr,"Root flag : %d\n",RootFlag);
#endif
    XSendEvent(dpy, Event.xbutton.window, True, NoEventMask, &Event);
  
    if(!RootFlag)
    {
	Target=XmuClientWindow(dpy,Event.xbutton.subwindow);
	if(Target==Event.xbutton.subwindow)
	    DispatchWindow=Target;
	else
	    DispatchWindow=PointerWindow;

	/*
	 * For the moment, we do not process drops onto the root
	 * window. 
	 */
	/*   } */
	/*   else */
	/* 	Target=DispatchWindow=(Window)MainWindow; */
	
	Event.xclient.type = ClientMessage;
	Event.xclient.display = dpy;
	Event.xclient.message_type = DndProtocol;
	Event.xclient.format = 32;
	Event.xclient.window = Target;
	Event.xclient.data.l[0] = DataType;
	Event.xclient.data.l[1] = (long)event->xbutton.state;
	Event.xclient.data.l[2] = (long)w;
	Event.xclient.data.l[3] = 0;
	Event.xclient.data.l[4] = 0;
	
	XSendEvent(dpy, DispatchWindow, True, NoEventMask, &Event);
#ifdef DEBUG
	fprintf(stderr,"ClientMessage sent to 0x%x(0x%x).\n",
		DispatchWindow,Target);
#endif
    }
  
    DragStarted=0;
    return 1;
}

static void
slideView(WMView *view, int srcX, int srcY, int dstX, int dstY)
{
    double x, y, dx, dy;
    int i;

    srcX -= 8;
    srcY -= 8;
    dstX -= 8;
    dstY -= 8;

    x = srcX;
    y = srcY;

    dx = (double)(dstX-srcX)/20.0;
    dy = (double)(dstY-srcY)/20.0;

    for (i = 0; i < 20; i++) {
        W_MoveView(view, x, y);
        XFlush(view->screen->display);

        x += dx;
	y += dy;
    }
}
