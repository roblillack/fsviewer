/* FSViewer.c  */

#include <locale.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <WMaker.h>
#include <WINGs/WINGs.h>
#include <X11/Xlocale.h>
#include <X11/keysym.h>
#include <unistd.h> /* for access() */

#include "FSViewer.h"
#include "FSFileButton.h"
#include "filebrowser.h"
#include "FSFileView.h"
#include "FSUtils.h"
#include "FSMenu.h"
#include "FSFinder.h"
#include "xpm/FSViewer.xpm"

#define DEBUG 0

static FSViewer *fsViewer;
static Bool      focusIn;
int ModifierFromKey(Display *dpy, char *key);

void
wAbort(Bool foo)
{
    exit(1);
}

static void
FSLoadFSViewerConfigurations(FSViewer *fsViewer)
{
    char *iconDir   = NULL;
    char *magicFile = NULL;

    defaultsDB = WMGetStandardUserDefaults();

    /*if there is no user database available, create one*/
    if (WMGetUDStringForKey(defaultsDB, "DIRECTORY") == NULL)
    {
	InitFilesDB(fsViewer);
    }	
    iconDir = FSGetStringForName("ICONDIR");

    if (iconDir)
    {
        /* test for existing ICONDIR */
        if (access(iconDir, F_OK) != 0) {
            wwarning(_("User database entry for ICONDIR is not "
                        "valid: %s. Taking default value."), iconDir);
            free(iconDir);

            /*try compile time default value*/
            WMSetUDStringForKey(defaultsDB, ICONDIR, "ICONDIR");
            iconDir = FSGetStringForName("ICONDIR");

            if (iconDir)
            {
                if (access(iconDir, F_OK) != 0) {
                    wwarning(_("Default ICONDIR not found: %s"), iconDir);
                    free(iconDir);
                    exit (1);
                }
                else {
                    WMSetResourcePath(iconDir);
                    free(iconDir);
                }
            }
            else {
                wwarning(_("No valid value for ICONDIR found. "
                            "Even default directory is broken!"));
                exit (1);
            }


        }
        else {
            WMSetResourcePath(iconDir);
            free(iconDir);
        }
    }
    
    else {
        wwarning(_("No value for ICONDIR found! May be your %s"
                    " preferences file is broken."), PACKAGE_NAME);
	exit (1);
    }

    magicFile = FSGetStringForName("MAGICFILE");
    if(magicFile)
    {
	magic_parse_file(magicFile);
	free(magicFile);
    }
    else
	magic_parse_file("");
}

static void
printHelp(char *progname)
{
    printf(_("Usage: %s [options]\n"), progname);
    puts(_("options:")); 
    puts(_(" -p <initial path>  initial path to display")); 
    puts(_(" -v                 print version number and exit")); 
}

Bool
FSIsFSViewerClipSet(FSViewer *fsViewer)
{
    return fsViewer->clip ? True : False;
}

void
FSSetFSViewerClip(FSViewer *fsViewer, FileInfo *fileInfo)
{
    /* Need to free previous clip???? */
    /* Or need to copy fileInfo????   */
    fsViewer->clip = fileInfo;
}

FileInfo *
FSGetFSViewerClip(FSViewer *fsViewer)
{
    return fsViewer->clip;
}

void
FSSetFSViewerClipAction(FSViewer *fsViewer, ClipAction action)
{
    fsViewer->clipAction = action;
}

ClipAction
FSGetFSViewerClipAction(FSViewer *fsViewer)
{
    return fsViewer->clipAction;
}

void 
FSSetFSViewerTransientWindow(FSViewer *fsViewer, Window window)
{
    XWMHints               *hints;
    GNUstepWMAttributes     attributes;

    // WM_DELETE should be set here...
    if ((hints = XAllocWMHints()))
    {
	hints->window_group = fsViewer->leader;
	hints->flags = WindowGroupHint;
	XSetWMHints(fsViewer->dpy, window, hints);
	XFree((void *) hints);
    }

    memset((void *) &attributes, 0, sizeof(GNUstepWMAttributes));
    attributes.window_style = (WMTitledWindowMask | WMClosableWindowMask);
    attributes.window_level = WMFloatingWindowLevel;
    attributes.extra_flags = GSFullKeyboardEventsFlag;
    attributes.flags =
	(GSWindowStyleAttr | GSWindowLevelAttr | GSExtraFlagsAttr);
    WMSetWindowAttributes(fsViewer->dpy, window, &attributes);
    WMAppAddWindow(fsViewer->wmContext, window);

}

void 
FSAddWindow(FSViewer *fsViewer, Window window)
{
    XWMHints               *hints;
    GNUstepWMAttributes     attributes;

    // WM_DELETE should be set here...
    if ((hints = XAllocWMHints()))
    {
	hints->window_group = fsViewer->leader;
/* 	hints->flags = WindowGroupHint; */
	XSetWMHints(fsViewer->dpy, window, hints);
	XFree((void *) hints);
    }

    memset((void *) &attributes, 0, sizeof(GNUstepWMAttributes));
    attributes.window_style = (WMTitledWindowMask | 
			       WMClosableWindowMask |
			       WMMiniaturizableWindowMask);
    attributes.window_level = WMNormalWindowLevel;
    attributes.extra_flags = GSFullKeyboardEventsFlag;
    attributes.flags =
	(GSWindowStyleAttr | GSWindowLevelAttr | GSExtraFlagsAttr);
    WMSetWindowAttributes(fsViewer->dpy, window, &attributes);
    WMAppAddWindow(fsViewer->wmContext, window);
}

void 
FSSetFSViewerConfirmWindow(FSViewer *fsViewer, Window window)
{
    XWMHints               *hints;
    GNUstepWMAttributes     attributes;
    
    // WM_DELETE should be set here...
    if ((hints = XAllocWMHints()))
    {
	hints->window_group = fsViewer->leader;
	hints->flags = WindowGroupHint;
	XSetWMHints(fsViewer->dpy, window, hints);
	XFree((void *) hints);
    }

    // This is horrible: there must be a better way?
    XStoreName(fsViewer->dpy, window, " ");
    memset((void *) &attributes, 0, sizeof(GNUstepWMAttributes));
    attributes.window_style = (WMTitledWindowMask);
    attributes.window_level = WMFloatingWindowLevel;
    attributes.extra_flags = GSFullKeyboardEventsFlag;
    attributes.flags =
	(GSWindowStyleAttr | GSWindowLevelAttr | GSExtraFlagsAttr);
    WMSetWindowAttributes(fsViewer->dpy, window, &attributes);
    WMAppAddWindow(fsViewer->wmContext, window);
}

WMScreen *
FSGetFSViewerScreen(FSViewer *fsViewer)
{
    return fsViewer->scr;
}

WMAppContext *
FSGetFSViewerWMContext(FSViewer *fsViewer)
{
    return fsViewer->wmContext;
}

RContext *
FSGetFSViewerRContext(FSViewer *fsViewer)
{
    return fsViewer->rcontext;
}

Display *
FSGetFSViewerDisplay(FSViewer *fsViewer)
{
    return fsViewer->dpy;
}

XContext
FSGetFSViewerXContext(FSViewer *fsViewer)
{
    return fsViewer->xContext;
}

Window 
FSGetFSViewerLeader(FSViewer *fsViewer)
{
    return fsViewer->leader;
}

void 
FSAddViewToFSViewer(FSViewer *fsViewer, FSFileView *fView)
{
        ++fsViewer->nviews;
        fsViewer->current = fView;
}

void 
FSRemoveViewFromFSViewer(FSViewer *fsViewer, FSFileView *fView)
{
        --fsViewer->nviews;
}

void
FSSetFSViewerFinder(FSViewer *fsViewer, FSFinder *finder)
{
    fsViewer->finder = finder;
}

FSFinder *
FSGetFSViewerFinder(FSViewer *fsViewer)
{
    return fsViewer->finder;
}

void
FSSetFSViewerPath(FSViewer *fsViewer, char *path)
{
    FSSetFileViewPath(fsViewer->current, path);
}

char *
FSGetFSViewerPath(FSViewer *fsViewer)
{
    FileInfo   *fileInfo = NULL;
    FSFileView *fView;
    
    fView = FSGetFSViewerCurrentView(fsViewer);
    fileInfo = FSGetFileViewFileInfo(fView);

    return GetPathnameFromPathName(fileInfo->path, fileInfo->name);
}

FSFileView *
FSGetFSViewerCurrentView(FSViewer *fsViewer)
{
        return fsViewer->current;
}

void
FSUpdateCurrentFileViewTitles()
{
    FSFileView *fView = NULL;

    /*
      Make sure we're in a window, otherwise the app
      could crash, especially if all windows are minimized,
      then one is opened and closed. When another window is
      opened, the current fileView references the one that
      was just closed, so the app crashes as that fileView 
      isn't there. The current fileView isn't changed until 
      a window receives the focus, therefore we check to
      see if the app is focused before unpdating the titles.
    */
    if(focusIn)
    {
	fView = FSGetFSViewerCurrentView(fsViewer);
	FSUpdateFileViewTitles(fView);
    }
}

int
FSGetFSViewerMetaMask(FSViewer *fsViewer)
{
    return fsViewer->metaMask;
}

char *
parseArgs(int argc, char **argv)
{
    char *initPath = NULL;

    if (argc>1) 
    {
	int i;
        for (i=1; i<argc; i++)
	{ 
            if (strcmp(argv[i], "-v")==0) 
	    {
                printf("FSViewer %s\n", VERSION);
                exit(0);
            } 
	    else if (strcmp(argv[i], "-p")==0) 
	    {
                i++;
                if (i>=argc) 
		{
                    wwarning(_("too few arguments for %s"), argv[i-1]);
                    exit(0);
                }
                initPath = argv[i];
            } 
	    else 
	    {
                printHelp(argv[0]);
                exit(0);
            }
        }
    }

    return initPath;
}
static void
FSCreateFSViewer(FSViewer *fsViewer, char **argv, int argc)
{
    fsViewer->leader = XCreateSimpleWindow(fsViewer->dpy,
				      DefaultRootWindow(fsViewer->dpy),
				      10, 10, 10, 10, 0, 0, 0);

    // Set standard X hints.
    if ((fsViewer->class = XAllocClassHint()))
    {
	fsViewer->class->res_name = "fsviewer";
	fsViewer->class->res_class = "FSViewer";
	XSetClassHint(fsViewer->dpy, fsViewer->leader, fsViewer->class);
    }

    if ((fsViewer->hints = XAllocWMHints()))
    {
	fsViewer->hints->window_group = fsViewer->leader;
	fsViewer->hints->flags = WindowGroupHint;
	XSetWMHints(fsViewer->dpy, fsViewer->leader, fsViewer->hints);
	XSetCommand(fsViewer->dpy, fsViewer->leader, argv, argc);
    }

    // Context for carrying a FileView*.
    fsViewer->xContext = XUniqueContext();

    /* 
     *  Create application descriptor.
     */
    WMInitializeApplication("FSViewer", &argc, argv);
    fsViewer->scr = WMCreateScreen(fsViewer->dpy, 
				   DefaultScreen(fsViewer->dpy));
    
    // Create the WorkSpace menu context.
    fsViewer->wmContext = WMAppCreateWithMain(fsViewer->dpy, 
					      DefaultScreen(fsViewer->dpy),
					      fsViewer->leader);    
    fsViewer->initPath = parseArgs(argc, argv);
    fsViewer->clip = NULL;
}

static void 
FSCreateFSViewerIcons(FSViewer *fsViewer)
{
    RContextAttributes      attributes;

    memset((void *) &attributes, 0, sizeof(RContextAttributes));
    attributes.flags = (RC_RenderMode | RC_ColorsPerChannel);
    attributes.render_mode = 0;
    attributes.colors_per_channel = 4;

    if (!(fsViewer->rcontext =
	  RCreateContext(fsViewer->dpy, DefaultScreen(fsViewer->dpy), 
			 &attributes)))
    {
	/* Somethings broken here!! */
	wfatal(_("Unable to get RContext: %s %s %d\n"),
                        "boo" /*RErrorString*/, __FILE__, __LINE__);
    }

    // The application icon.
    if ((fsViewer->image = RGetImageFromXPMData(fsViewer->rcontext, 
						FSVIEWER_XPM)))
    {
	if ((RConvertImageMask(fsViewer->rcontext, fsViewer->image,
			       &fsViewer->appicon, &fsViewer->appmask, 0)))
	{
	    fsViewer->hints->icon_pixmap = fsViewer->appicon;
	    fsViewer->hints->icon_mask = fsViewer->appmask;
	    fsViewer->hints->flags |= (IconPixmapHint | IconMaskHint);
	    XSetWMHints(fsViewer->dpy, fsViewer->leader, fsViewer->hints);
	}
    }

    // The icon used in standard dialog panels.
    if ((fsViewer->wmpixmap = WMCreatePixmapFromRImage(fsViewer->scr, 
						       fsViewer->image, 0)))
    {
        WMSetApplicationIconPixmap(fsViewer->scr, fsViewer->wmpixmap);
    }
}

static void
FSInitFSViewer(FSViewer *fsViewer)
{
    int bit;

    FSLoadFSViewerConfigurations(fsViewer);
    FSInitSystemInfo(fsViewer);
    FSInitInspector(fsViewer);
    bit = ModifierFromKey(FSGetFSViewerDisplay(fsViewer), "META");
    fsViewer->metaMask = 1<<bit;
}

int 
main(int argc, char **argv)
{
    FSFileView *fView;
    focusIn = False;

    setlocale(LC_ALL,"");

#ifdef ENABLE_NLS
    if (getenv("NLSPATH"))
      bindtextdomain("FSViewer", getenv("NLSPATH"));
    else
      bindtextdomain("FSViewer", LOCALEDIR);
    textdomain("FSViewer");

    if (!XSupportsLocale()) {
      wwarning(_("X server does not support locale"));
    }
    if (XSetLocaleModifiers("") == NULL) {
      wwarning(_("cannot set locale modifiers"));
    }
#endif

     /* wmessage("LOCALEDIR= %s", LOCALEDIR);*/

    if (!(fsViewer = (FSViewer *) malloc(sizeof(FSViewer))))
    {
	wfatal(_("Unable to allocate memory for %s %s %d\n"),
		argv[0], __FILE__, __LINE__);
    }
    memset((void *) fsViewer, 0, sizeof(FSViewer));

    fsViewer->dpy = XOpenDisplay(NULL);
    if (!fsViewer->dpy) {
	wfatal(_("Unable to Open Display!"));
	exit(0);
    }
    FSCreateFSViewer(fsViewer, argv, argc);
    FSCreateFSViewerIcons(fsViewer);
    FSInitFSViewer(fsViewer);
    FSCreateMenu(fsViewer);
    
    /* 
       No path specified at start up, use home dir.
    */
    if(fsViewer->initPath == NULL)
	fsViewer->initPath = FSGetHomeDir();
    /* 
       Unable to get home dir, must use `/` 
    */
    if(fsViewer->initPath == NULL)
	fsViewer->initPath = wstrdup("/");

    if(!(fView = FSCreateFileView(fsViewer, fsViewer->initPath, True)))
    {
	wfatal(_("Unable to create a FileView instance %s %s %d"),
	       argv[0], __FILE__, __LINE__);
    }
    fsViewer->current = fView;

    while (1) 
    {
	XEvent event;
	FSFileView *fView;

	WMNextEvent(fsViewer->dpy, &event);
	WMHandleEvent(&event);
	WMProcessEvent(fsViewer->wmContext, &event);

	switch (event.type)
	{
	    case FocusIn:
		if (!(XFindContext(fsViewer->dpy, event.xfocus.window,
				   fsViewer->xContext, (XPointer *) &fView)))
		{
/* 		    if((FSFinder *)fView == fsViewer->finder) */
/* 			break; */
		    fsViewer->current = fView;
		    FSUpdateFileViewShelf(fView);
		}
		focusIn = True;
		break;
	    case FocusOut: focusIn = False;
		break;
	    case ClientMessage:
		// printf("arrgggg!\n");
		break;
	}
    }
}
