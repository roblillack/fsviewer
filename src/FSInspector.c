/* FSInspector.c */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <X11/keysym.h>

#include "FSViewer.h"
#include "FSUtils.h"

#define INITIALIZED_PANEL	(1<<0)
#define FSI_WIDTH               272
#define FSI_HEIGHT              418
#define MF_X                    5
#define MF_Y                    5
#define NUM_PANEL               5/* 6 */
#define INIT_PANEL              0

enum 
{
    AttribPanel = 0,
    IconPanel   = 1,
    ViewPanel   = 2,
    EditPanel   = 3,
    ExePanel    = 4,
/*     ExtnPanel   = 5, */
};

typedef struct _FSInspector {    
    WMWindow       *win;

    WMFrame        *mainF; 
    WMFrame        *infoF; 
    WMPopUpButton  *popupMenu;

    WMLabel        *nameLabel;
    WMLabel        *pathLabel;

    Bool           isOpen;

    FileInfo       *fileInfo;

    FSViewer       *fsViewer;

    Panel          *panel[NUM_PANEL];
    Panel          *currentPanel;

} _FSInspector;


typedef struct _FSInspector FSInspector;
static FSInspector *fsInspector;

static void
FSEnableInspectorPanels(FSInspector *fsInspector)
{
    char *pathname;
    struct stat *st;

    st = (struct stat *) wmalloc(sizeof(struct stat));
    
    pathname = (char *) wmalloc(strlen(fsInspector->fileInfo->path)+
				strlen(fsInspector->fileInfo->name)+1);
    strcpy(pathname, fsInspector->fileInfo->path);
    if (fsInspector->fileInfo->fileType != ROOT)
	strcat(pathname, fsInspector->fileInfo->name);

    /* get  information of a file represented by pathname */
    if (stat(pathname, st) == -1)
	wwarning(_("%s %d: Stat Error"), __FILE__, __LINE__);

    if (S_ISDIR(st->st_mode))
    {
	WMSetPopUpButtonItemEnabled(fsInspector->popupMenu, EditPanel, False);
	WMSetPopUpButtonItemEnabled(fsInspector->popupMenu, ViewPanel, False);
	WMSetPopUpButtonItemEnabled(fsInspector->popupMenu, ExePanel,  False);
/* 	WMSetPopUpButtonItemEnabled(fsInspector->popupMenu, ExtnPanel, False); */
    }
    else
    {	
	WMSetPopUpButtonItemEnabled(fsInspector->popupMenu, EditPanel, True);
	WMSetPopUpButtonItemEnabled(fsInspector->popupMenu, ViewPanel, True);
	if (st->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
	{
	    WMSetPopUpButtonItemEnabled(fsInspector->popupMenu,
					ExePanel, True);
/* 	    WMSetPopUpButtonItemEnabled(fsInspector->popupMenu,  */
/* 					ExtnPanel, True); */
	}
	else
	{
	    WMSetPopUpButtonItemEnabled(fsInspector->popupMenu, 
					ExePanel,  False);
/* 	    WMSetPopUpButtonItemEnabled(fsInspector->popupMenu,  */
/* 					ExtnPanel, False); */
	}
    }
    
    if (st)
    {
	free(st);
	st = NULL;
    }

    if (pathname)
	free(pathname);
    
}
static void
FSUpdateInspectorFileInfoDisplay(FSInspector *fsInspector)
{
    char *buf;
    RColor color;
    WMPixmap *pixmap;
    
    color.red   = 0xae;
    color.green = 0xaa; /* aa ?*/
    color.blue  = 0xae;
    color.alpha = 0;
    
    if (fsInspector->fileInfo->fileType != ROOT)
	WMSetLabelText(fsInspector->nameLabel, fsInspector->fileInfo->name);
    else
    {
	char *tmp;
	
	tmp = FSNodeName();
	WMSetLabelText(fsInspector->nameLabel, tmp);
    }
      /* why FS... */
    pixmap = WMCreateBlendedPixmapFromFile(WMWidgetScreen(fsInspector->win),
					   fsInspector->fileInfo->imgName, 
					   &color);
    WMSetLabelImage(fsInspector->nameLabel, pixmap);

    buf = (char *) wmalloc(strlen(_("Path: ")) + 
			   strlen(fsInspector->fileInfo->path)+1);
    sprintf(buf, _("Path: %s"), fsInspector->fileInfo->path);
    WMSetLabelText(fsInspector->pathLabel, buf);

    if (buf)
	free(buf);

}

static void
FSCopyInspectorFileInfo(FileInfo *fileInfo)
{
    fsInspector->fileInfo->name = 
	(char *) wrealloc(fsInspector->fileInfo->name, 
			  strlen(fileInfo->name)+1);
    strcpy(fsInspector->fileInfo->name, fileInfo->name);

    fsInspector->fileInfo->path = 
	(char *) wrealloc(fsInspector->fileInfo->path, 
			  strlen(fileInfo->path)+1);
    strcpy(fsInspector->fileInfo->path, fileInfo->path);

    fsInspector->fileInfo->extn = 
	(char *) wrealloc(fsInspector->fileInfo->extn, 
			  strlen(fileInfo->extn)+1);
    strcpy(fsInspector->fileInfo->extn, fileInfo->extn);

   fsInspector->fileInfo->abbrev = 
	(char *) wrealloc(fsInspector->fileInfo->abbrev, 
			  strlen(fileInfo->abbrev)+1);
    strcpy(fsInspector->fileInfo->abbrev, fileInfo->abbrev);

    fsInspector->fileInfo->imgName = 
	(char *) wrealloc(fsInspector->fileInfo->imgName, 
			  strlen(fileInfo->imgName)+1);
    strcpy(fsInspector->fileInfo->imgName, fileInfo->imgName);

    fsInspector->fileInfo->fileType = fileInfo->fileType;
}

static void
FSHideInspectorPanel(Panel *panel)
{
    PanelRec *rec = (PanelRec*)panel;    
    
    WMUnmapWidget(rec->frame);
}

static void
FSShowInspectorPanel(Panel *panel)
{
    PanelRec *rec = (PanelRec*)panel;
    
    if (!(rec->callbacks.flags & INITIALIZED_PANEL)) {
	(*rec->callbacks.createWidgets)(panel);
	rec->callbacks.flags |= INITIALIZED_PANEL;
    }

    WMSetWindowTitle(fsInspector->win, rec->sectionName);
    
    (*rec->callbacks.updateDisplay)(panel);
    WMMapWidget(rec->frame);
}

void 
FSCloseInspectorWindow(WMWidget *w, void *data)
{
    fsInspector->isOpen = False;
    WMUnmapWidget((WMWindow *) w);
}

void 
FSUpdateInspectorWindow(FileInfo *fileInfo)
{
    if (fsInspector->isOpen)
    {
	fsInspector->currentPanel = fsInspector->panel[AttribPanel];
	WMSetPopUpButtonSelectedItem(fsInspector->popupMenu , AttribPanel);
/* 	WMMapWidget(fsInspector->win); */
	FSCopyInspectorFileInfo(fileInfo);
	FSUpdateInspectorFileInfoDisplay(fsInspector);
	FSEnableInspectorPanels(fsInspector);
	FSShowInspectorPanel(fsInspector->currentPanel);
	fsInspector->isOpen = True;
    }
}

void 
FSShowInspectorWindow(WMScreen *scr, FileInfo *fileInfo)
{
    fsInspector->currentPanel = fsInspector->panel[AttribPanel];
    WMSetPopUpButtonSelectedItem(fsInspector->popupMenu , AttribPanel);
    WMMapWidget(fsInspector->win);
    FSCopyInspectorFileInfo(fileInfo);
    FSUpdateInspectorFileInfoDisplay(fsInspector);
    FSEnableInspectorPanels(fsInspector);
    FSShowInspectorPanel(fsInspector->currentPanel);
    fsInspector->isOpen = True;
}

void 
FSHideInspectorWindow(WMScreen *scr)
{
    FSCloseInspectorWindow(fsInspector->win, NULL);
}

static void
FSInspectorPopupAction(WMWidget *self, void *data)
{    
    int i = WMGetPopUpButtonSelectedItem((WMPopUpButton *)self);

    if (fsInspector->currentPanel != fsInspector->panel[i])
    {
	FSHideInspectorPanel(fsInspector->currentPanel);
	fsInspector->currentPanel = fsInspector->panel[i];
	FSShowInspectorPanel(fsInspector->panel[i]);
    }
}

static void
FSCreateInspectorWindow()
{
    WMScreen *scr = FSGetFSViewerScreen(fsInspector->fsViewer);
    WMFont *font;

    fsInspector->win = WMCreateWindow(scr, "FSViewer Inspector");
    WMResizeWidget(fsInspector->win, FSI_WIDTH, FSI_HEIGHT);
    WMSetWindowTitle(fsInspector->win, _("FSViewer Inspector"));
    WMSetWindowCloseAction(fsInspector->win, FSCloseInspectorWindow, NULL);
    WMSetWindowMaxSize(fsInspector->win, FSI_WIDTH, FSI_HEIGHT);
    WMSetWindowMinSize(fsInspector->win, FSI_WIDTH, FSI_HEIGHT);
    
    /* mainF */
    fsInspector->mainF = WMCreateFrame(fsInspector->win);
    WMResizeWidget(fsInspector->mainF, FSI_WIDTH+10, FSI_HEIGHT+10);
    WMMoveWidget(fsInspector->mainF, -MF_X, -MF_Y);
    WMSetFrameRelief(fsInspector->mainF, WRFlat);

    font = WMCreateFont(WMWidgetScreen(fsInspector->win), 
			"-*-helvetica-medium-r-*-*-18-*-*-*-*-*-*-*");
    if (!font)
	font = WMBoldSystemFontOfSize(WMWidgetScreen(fsInspector->win), 18);

    fsInspector->popupMenu = WMCreatePopUpButton(fsInspector->mainF);
    WMResizeWidget(fsInspector->popupMenu, 112, 20);
    WMMoveWidget(fsInspector->popupMenu, ((FSI_WIDTH-112)/2)+MF_X, 8+MF_Y);
    WMSetPopUpButtonText(fsInspector->popupMenu, _("Select"));
    WMAddPopUpButtonItem(fsInspector->popupMenu, _("Attributes"));
    WMAddPopUpButtonItem(fsInspector->popupMenu, _("Icon"));
    WMAddPopUpButtonItem(fsInspector->popupMenu, _("Viewer"));
    WMAddPopUpButtonItem(fsInspector->popupMenu, _("Editor"));
    WMAddPopUpButtonItem(fsInspector->popupMenu, _("Executable"));
/*     WMAddPopUpButtonItem(fsInspector->popupMenu, "File Extensions"); */
    WMSetPopUpButtonAction(fsInspector->popupMenu, FSInspectorPopupAction,
			   fsInspector);
    WMSetPopUpButtonSelectedItem(fsInspector->popupMenu, INIT_PANEL);

    fsInspector->infoF = WMCreateFrame(fsInspector->mainF);
    WMResizeWidget(fsInspector->infoF, FSI_WIDTH+10, 93);
    WMMoveWidget(fsInspector->infoF, 0, 36+MF_Y);
    WMSetFrameRelief(fsInspector->infoF, WRGroove);

    fsInspector->nameLabel = WMCreateLabel(fsInspector->infoF);
    WMResizeWidget(fsInspector->nameLabel, FSI_WIDTH-10, 55);
    WMMoveWidget(fsInspector->nameLabel, 8+MF_X, 8);
    WMSetLabelFont(fsInspector->nameLabel, font);
    WMSetLabelImagePosition(fsInspector->nameLabel, WIPLeft);
    WMSetLabelTextAlignment(fsInspector->nameLabel, WALeft);
    WMReleaseFont(font);

    fsInspector->pathLabel = WMCreateLabel(fsInspector->infoF);
    WMResizeWidget(fsInspector->pathLabel, FSI_WIDTH-10, 30);
    WMMoveWidget(fsInspector->pathLabel, 8, 60);

    WMMapSubwidgets(fsInspector->win);
    
    fsInspector->panel[AttribPanel] = InitAttribs(fsInspector->win, 
						  fsInspector->fileInfo);
    fsInspector->panel[IconPanel]   = InitIcon(fsInspector->win,
					       fsInspector->fsViewer,
					       fsInspector->fileInfo,
					       0, 138);
    fsInspector->panel[ViewPanel]   = InitViewer(fsInspector->win, 
						 fsInspector->fileInfo);
    fsInspector->panel[EditPanel]   = InitEditor(fsInspector->win, 
						 fsInspector->fileInfo);
    fsInspector->panel[ExePanel]    = InitExecutable(fsInspector->win, 
						     fsInspector->fileInfo);
/*     fsInspector->panel[ExtnPanel]   = InitExtn(fsInspector->win,  */
/* 					       fsInspector->fsViewer, */
/* 					       fsInspector->fileInfo); */
}


void
FSInitInspector(FSViewer *fsViewer)
{
    
    XWMHints               *hints;
    GNUstepWMAttributes     attributes;

    if (!(fsInspector = (FSInspector *) malloc(sizeof(FSInspector))))
    {
	wfatal(_("Unable to allocate memory for FSInspector %s %d\n"),
		 __FILE__, __LINE__);
    }
    memset(fsInspector, 0, sizeof(FSInspector));

    fsInspector->fileInfo = FSCreateFileInfo();
    fsInspector->fsViewer = fsViewer;
    FSCreateInspectorWindow();

    WMRealizeWidget(fsInspector->win);
    FSAddWindow(fsInspector->fsViewer, WMWidgetXID(fsInspector->win));
    fsInspector->isOpen = False;
}
