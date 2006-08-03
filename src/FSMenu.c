#include <WINGs/WINGsP.h>
/* #include <WMaker.h> */

#include "files.h"
#include "FSFileButton.h"
#include "filebrowser.h"
#include "FSViewer.h"
#include "FSFileView.h"
#include "FSMenu.h"
#include "FSPanel.h"
#include "FSUtils.h"
#include "FSFinder.h"


static void fsSetViewerMode(FSViewer *fsViewer, FSFileViewType mode);

FileInfo *
fsGetFSViewerCurrentFileInfo(FSViewer *fsViewer)
{
    FSFileView *fView;
    
    fView = FSGetFSViewerCurrentView(fsViewer);

    return FSGetFileViewFileInfo(fView);
}

void
FSQuitCB(WMWidget *w, void *data)
{
    exit(0);
}

void 
FSHideCB(FSViewer *fsViewer, int item, Time time)
{
    WMHideApplication(FSGetFSViewerWMContext(fsViewer));
}

void 
FSHideOthersCB(FSViewer *fsViewer, int item, Time time)
{
        WMHideOthers(FSGetFSViewerWMContext(fsViewer));
}

void 
FSInfoPanelCB(FSViewer *fsViewer, int item, Time time)
{
    FSRunInfoPanel(fsViewer, "FSViewer", _("Info"));
}

void 
FSLegalCB(FSViewer *fsViewer, int item, Time time)
{
    FSRunLegalPanel(fsViewer, _("Legal"), _("License"));
}

void 
FSPrefCB(FSViewer *fsViewer, int item, Time time)
{
    FSRunPreferencesPanel(fsViewer, _("Preferences"));
}

void 
FSWMPrefCB(FSViewer *fsViewer, int item, Time time)
{
    FSRunPreferencesPanel(fsViewer, _("Preferences"));
}

void 
FSRunCB(FSViewer *fsViewer, int item, Time time)
{
    FSLaunchApp(fsViewer, AppExec);
}

void 
FSViewCB(FSViewer *fsViewer, int item, Time time)
{
    FSLaunchApp(fsViewer, AppView);
}

void 
FSEditCB(FSViewer *fsViewer, int item, Time time)
{
    FSLaunchApp(fsViewer, AppEdit);    
}

void 
FSInspectorCB(FSViewer *fsViewer, int item, Time time)
{
    FileInfo *fileInfo;

    fileInfo = fsGetFSViewerCurrentFileInfo(fsViewer);
    FSShowInspectorWindow(FSGetFSViewerScreen(fsViewer), fileInfo);
}

void 
FSBrowseCB(FSViewer *fsViewer, int item, Time time)
{
    fsSetViewerMode(fsViewer, Browser);
}

void 
FSIconCB(FSViewer *fsViewer, int item, Time time)
{
    fsSetViewerMode(fsViewer, Icon);
}

void 
FSListCB(FSViewer *fsViewer, int item, Time time)
{
    fsSetViewerMode(fsViewer, List);
}

static void 
fsSetViewerMode(FSViewer *fsViewer, FSFileViewType mode)
{
    FSFileView *fView;
    
    fView = FSGetFSViewerCurrentView(fsViewer);

    FSSetFileViewMode(fView, mode);
}

void 
FSSortOrderCB(FSViewer *fsViewer, int item, Time time)
{
    FSToggleSort();
    FSUpdateFileViewPath(FSGetFSViewerCurrentView(fsViewer), 
			 FileSync, NULL, NULL);
}

void 
FSNewViewCB(FSViewer *fsViewer, int item, Time time)
{
    FSFileView *fView = FSGetFSViewerCurrentView(fsViewer);
    
    if (!(FSCreateFileView(fsViewer, FSGetFileViewPath(fView), False)))
    {
	WMRunAlertPanel(FSGetFSViewerScreen(fsViewer), NULL,
                        _("New View"),
                        _("Error creating New FileViewer"),
                        _("OK"), NULL, NULL);
    }
}

void 
FSDotFilesCB(FSViewer *fsViewer, int item, Time time)
{
    FSToggleDisplayHiddenFiles();
    FSUpdateFileViewPath(FSGetFSViewerCurrentView(fsViewer), 
			 FileSync, NULL, NULL);
}

void 
FSCopyCB(FSViewer *fsViewer, int item, Time time)
{
    FileInfo *fileInfo;

    fileInfo = fsGetFSViewerCurrentFileInfo(fsViewer);
    FSSetFSViewerClip(fsViewer, fileInfo);
    FSSetFSViewerClipAction(fsViewer, ClipCopy);
}

void 
FSCutCB(FSViewer *fsViewer, int item, Time time)
{
    FileInfo *fileInfo;

    fileInfo = fsGetFSViewerCurrentFileInfo(fsViewer);
    FSSetFSViewerClip(fsViewer, fileInfo);
    FSSetFSViewerClipAction(fsViewer, ClipCut);
}

void 
FSPasteCB(FSViewer *fsViewer, int item, Time time)
{
    ClipAction clipAction;
    FileInfo *dest = NULL;
    FileInfo *src  = NULL;

    dest = fsGetFSViewerCurrentFileInfo(fsViewer);
    src  = FSGetFSViewerClip(fsViewer);

    if(src == NULL)
	return;

    clipAction = FSGetFSViewerClipAction(fsViewer);

    if(clipAction == ClipCopy)
	FSCopy(src, dest);
    else if(clipAction == ClipCut)
	FSMove(src, dest);

    FSSetFSViewerClip(fsViewer, NULL);
}

void 
FSLinkCB(FSViewer *fsViewer, int item, Time time)
{
    char     *new  = NULL;
    char     *old  = NULL;
    FileInfo *src;
    FileInfo *dest = FSCreateFileInfo();

    src = fsGetFSViewerCurrentFileInfo(fsViewer);
    old = GetPathnameFromPathName(src->path, src->name);
    new = WMRunInputPanel(FSGetFSViewerScreen(fsViewer), NULL,
			  _("Link Name"), _("Link: Enter the link name"),
			  old, _("OK"), _("Cancel"));

    if(new != NULL)
    {
	dest->path = GetPathFromPathname(new);
	dest->name= GetNameFromPathname(new);
	FSLink(src, dest);
    }

    if(new != NULL)
	free(new);
    if(old != NULL)
	free(old);
}

void 
FSRenameCB(FSViewer *fsViewer, int item, Time time)
{
    char     *new  = NULL;
    char     *old  = NULL;
    FileInfo *src;
    FileInfo *dest = FSCreateFileInfo();

    src = fsGetFSViewerCurrentFileInfo(fsViewer);
    old = GetPathnameFromPathName(src->path, src->name);
    new = WMRunInputPanel(FSGetFSViewerScreen(fsViewer), NULL,
			  _("Rename File"), _("Rename: Enter the new file name"),
			  old, _("OK"), _("Cancel"));

    if(new != NULL)
    {
	dest->path = GetPathFromPathname(new);
	dest->name = GetNameFromPathname(new);
	FSRename(src, dest);
    }

    if(new != NULL)
	free(new);
    if(old != NULL)
	free(old);

}

void 
FSDeleteCB(FSViewer *fsViewer, int item, Time time)
{
    FileInfo *fileInfo;

    fileInfo = fsGetFSViewerCurrentFileInfo(fsViewer);

    FSDelete(fileInfo);
}

void 
FSNewFileCB(FSViewer *fsViewer, int item, Time time)
{
    char *new = NULL;
    char *initPath = NULL;
    FileInfo *fileInfo;
    mode_t umask = FSGetUMask();

    fileInfo = fsGetFSViewerCurrentFileInfo(fsViewer);

    if(isDirectory(fileInfo->fileType))
    {
	initPath = GetPathnameFromPathName(fileInfo->path, fileInfo->name);
    }
    else
    {
	initPath = wstrdup(fileInfo->path);
    }

    new = WMRunInputPanel(FSGetFSViewerScreen(fsViewer), NULL,
			  _("Create a new file"), 
			  _("New File: Enter the new file name"),
			  GetPathnameFromPathName(initPath, ""),
			  _("OK"), _("Cancel"));

    if(new)
    {
	if(FSCreateNewFile(new, umask & 0777)) 
	{
	    char s[0xff];
	    
	    sprintf(s, _("Error creating file %s"), new);
	    FSErrorDialog(_("File Operation Error"), s);
	}
	else
	{
	    FileInfo *src = FSCreateFileInfo();
	    FileInfo *dest = FSGetFileInfo(new);

	    FSUpdateFileView(FileCopy, src, dest);
	    if(src)
		FSFreeFileInfo(src);
	    if(dest)
		FSFreeFileInfo(dest);
	}
    }

    if(new)
	free(new);
    if(initPath)
	free(initPath);
}


void 
FSNewDirCB(FSViewer *fsViewer, int item, Time time)
{
    char *new = NULL;
    char *initPath = NULL;
    FileInfo *fileInfo;
    mode_t umask = FSGetUMask();

    fileInfo = fsGetFSViewerCurrentFileInfo(fsViewer);

    if(isDirectory(fileInfo->fileType))
    {
	initPath = GetPathnameFromPathName(fileInfo->path, fileInfo->name);
    }
    else
    {
	initPath = wstrdup(fileInfo->path);
    }

    new = WMRunInputPanel(FSGetFSViewerScreen(fsViewer), NULL,
			  _("Create a new directory"), 
			  _("New Dir: Enter the new directory name"),
			  GetPathnameFromPathName(initPath, ""),
			  _("OK"), _("Cancel"));

    if(new)
    {
	if (FSCreateNewDirectory(new, umask & 0777)) 
	{
	    char s[0xff];
	    
	    sprintf(s, _("Error creating folder %s"), new);
	    FSErrorDialog(_("File Operation Error"), s);
	}
	else
	{
	    FileInfo *src = FSCreateFileInfo();
	    FileInfo *dest = FSGetFileInfo(new);
	
	    src->name = dest->name;
	    dest->name = wstrdup("");
	    FSUpdateFileView(FileCopy, src, dest);
	    if(src)
		FSFreeFileInfo(src);
	    if(dest)
		FSFreeFileInfo(dest);
	}
    }

    if(new)
	free(new);
    if(initPath)
	free(initPath);
}

void 
FSConsoleCB(FSViewer *fsViewer, int item, Time time)
{
    FileInfo *fileInfo;
    FileInfo *pathInfo;

    fileInfo = FSCreateFileInfo();

    fileInfo->name = (char *) wmalloc(8);
    strcpy(fileInfo->name, "CONSOLE");

    pathInfo = fsGetFSViewerCurrentFileInfo(fsViewer);

    if(isDirectory(pathInfo->fileType))
    {
	fileInfo->path = GetPathnameFromPathName(pathInfo->path, 
						 pathInfo->name);
    }
    else
    {
	fileInfo->path = pathInfo->path;
    }

    LaunchApp(fsViewer, fileInfo, AppExec);

    if(pathInfo)
	FSFreeFileInfo(pathInfo);
    if(fileInfo)
	FSFreeFileInfo(fileInfo);

}

void 
FSFilterViewCB(FSViewer *fsViewer, int item, Time time)
{
    char *filter      = NULL;
    char *init        = NULL;
    char *title       = _("Filter Current FileView");
    char *prompt      = _("Enter Filter Value:");
    FSFileView *fView = NULL;
    
    fView  = FSGetFSViewerCurrentView(fsViewer);
    init   = FSGetFileViewFilter(fView);
    if((filter = WMRunInputPanel(FSGetFSViewerScreen(fsViewer), 
			     FSGetFileViewWindow(fView), 
			     title, prompt, init, _("OK"), _("Cancel"))) != NULL)
    {
	if(strcmp("", filter) == 0)
	    filter = NULL;
	
	FSSetFileViewFilter(fView, filter);
	
	if(filter)
	    free(filter);
    }
}

void 
FSSelectAllCB(FSViewer *fsViewer, int item, Time time)
{
    WMRunAlertPanel(FSGetFSViewerScreen(fsViewer), NULL, _("Menu Error"), 
		    _("This menu option has not yet been implemented"),
		    _("OK"), NULL, NULL);
}

void 
FSDeselectAllCB(FSViewer *fsViewer, int item, Time time)
{
    WMRunAlertPanel(FSGetFSViewerScreen(fsViewer), NULL, _("Menu Error"), 
		    _("This menu option has not yet been implemented"),
		    _("OK"), NULL, NULL);
}

void 
FSMountCB(Disk *disk, int item, Time time)
{
    int noerror = 1;
    char *execStr = NULL;
/*     char *cmd = FSGetStringForNameKey("DISKS", "mount"); */

/*     if(cmd) */
    if(disk->mntCmd)
    {
/* 	execStr = FSParseExecString(disk->point, cmd); */
	execStr = FSParseExecString(disk->point, disk->mntCmd);
	
	if(execStr)
	{
	    noerror = execCommand(execStr);
	    free(execStr);
	}
	else
	    noerror = 0;
/* 	free(cmd); */
    }
    else
	noerror = 0;

    if(!noerror)
    {
	char s[0xff];
	
	sprintf(s, "Unable to mount \"%s\"", disk->name);
	WMRunAlertPanel(FSGetFSViewerScreen(disk->app), NULL, _("Exec Error"), 
			s, _("OK"), NULL, NULL);
    }
}

void 
FSUnmountCB(Disk *disk, int item, Time time)
{
    int noerror = 1;
    char *execStr = NULL;
/*     char *cmd = FSGetStringForNameKey("DISKS", "umount"); */

/*     if(cmd) */
    if(disk->umntCmd)
    {
/* 	execStr = FSParseExecString(disk->point, cmd); */
	execStr = FSParseExecString(disk->point, disk->umntCmd);
	
	if(execStr)
	{
	    noerror = execCommand(execStr);
	    free(execStr);
	}
	else
	    noerror = 0;
/* 	free(cmd); */
    }
    else
	noerror = 0;

    if(!noerror)
    {
	char s[0xff];
	
	sprintf(s, "Unable to unmount \"%s\"", disk->name);
	WMRunAlertPanel(FSGetFSViewerScreen(disk->app), NULL, _("Exec Error"), 
			s, _("OK"), NULL, NULL); 
    }
}

void 
FSEjectCB(Disk *disk, int item, Time time)
{
    int noerror = 1;
    char *execStr = NULL;
/*     char *cmd = FSGetStringForNameKey("DISKS", "eject"); */

/*     if(cmd) */
    if(disk->ejectCmd)
    {
/* 	execStr = FSParseExecString(disk->device, cmd); */
	execStr = FSParseExecString(disk->device, disk->ejectCmd);
	
	if(execStr)
	{
	    noerror = execCommand(execStr);
	    free(execStr);
	}
	else
	    noerror = 0;
/* 	free(cmd); */
    }
    else
	noerror = 0;

    if(!noerror)
    {
	char s[0xff];
	
	sprintf(s, "Unable to eject \"%s\"", disk->name);
	WMRunAlertPanel(FSGetFSViewerScreen(disk->app), NULL, _("Exec Error"), 
			s, _("OK"), NULL, NULL);
    }
}

void 
FSCloseDiskCB(Disk *disk, int item, Time time)
{
    int noerror = 1;
    char *execStr = NULL;
/*     char *cmd = FSGetStringForNameKey("DISKS", "eject"); */

/*     if(cmd) */
    if(disk->closeCmd)
    {
/* 	execStr = FSParseExecString(disk->device, cmd); */
	execStr = FSParseExecString(disk->point, disk->closeCmd);
	
	if(execStr)
	{
	    noerror = execCommand(execStr);
	    free(execStr);
	}
	else
	    noerror = 0;
/* 	free(cmd); */
    }
    else
	noerror = 0;

    if(!noerror)
    {
	char s[0xff];
	
	sprintf(s, "Unable to close \"%s\"", disk->name);
	WMRunAlertPanel(FSGetFSViewerScreen(disk->app), NULL, _("Exec Error"), 
			s, _("OK"), NULL, NULL);
    }
}

void 
FSFinderCB(FSViewer *fsViewer, int item, Time time)
{
    if(!FSGetFSViewerFinder(fsViewer))
    {
	if(!FSCreateFinder(fsViewer))
	    wwarning(_("%s %d: Unable to create FSFinder"), 
		     __FILE__, __LINE__);
    }
    else
    {
	/*
	 * Should check if the window is minimized or covered
	 * and bring it into view.
	 */
    }
	
}

void 
FSMiniaturizeCB(FSViewer *fsViewer, int item, Time time)
{
    WMRunAlertPanel(FSGetFSViewerScreen(fsViewer), NULL, _("Menu Info"), 
		    _("This option is only available via the window manager."),
		    _("OK"), NULL, NULL);
}

void 
FSOpenAsFolderCB(FSViewer *fsViewer, int item, Time time)
{
    WMRunAlertPanel(FSGetFSViewerScreen(fsViewer), NULL, _("Menu Error"), 
		    _("This menu option has not yet been implemented"),
		    _("OK"), NULL, NULL);
}

void 
FSProcessCB(FSViewer *fsViewer, int item, Time time)
{
    FileInfo *fileInfo;
    FileInfo *pathInfo;

    fileInfo = FSCreateFileInfo();

    fileInfo->name = (char *) wmalloc(8);
    strcpy(fileInfo->name, "PROCESS");

    pathInfo = fsGetFSViewerCurrentFileInfo(fsViewer);

    if(isDirectory(pathInfo->fileType))
    {
	fileInfo->path = GetPathnameFromPathName(pathInfo->path, 
						 pathInfo->name);
    }
    else
    {
	fileInfo->path = pathInfo->path;
    }

    LaunchApp(fsViewer, fileInfo, AppExec);

    if(pathInfo)
	FSFreeFileInfo(pathInfo);
    if(fileInfo)
	FSFreeFileInfo(fileInfo);

}

void 
FSHelpCB(FSViewer *fsViewer, int item, Time time)
{
    WMRunAlertPanel(FSGetFSViewerScreen(fsViewer), NULL, _("Menu Error"), 
		    _("This menu option has not yet been implemented"),
		    _("OK"), NULL, NULL);
}

void 
FSCloseWindowCB(FSViewer *fsViewer, int item, Time time)
{
    FSFileView *fView = NULL;
    
    fView  = FSGetFSViewerCurrentView(fsViewer);
    if(!FSIsFileViewPrimary(fView))
	WMCloseWindow(FSGetFileViewWindow(fView));
}

void
FSUpdateViewCB(FSViewer *fsViewer, int item, Time time)
{
    FSFileView *fView = NULL;
    
    fView  = FSGetFSViewerCurrentView(fsViewer);
    FSUpdateFileViewPath(fView, FileSync, NULL, NULL);
}

void
FSDuplicateCB(FSViewer *fsViewer, int item, Time time)
{
    ClipAction clipAction;
    FileInfo *dest = FSCreateFileInfo();
    FileInfo *src  = NULL;

    src = fsGetFSViewerCurrentFileInfo(fsViewer);
    FSCopyFileInfo(src, dest);
    
    dest->name = (char *) wrealloc(dest->name, strlen(src->name)+7);
    strcpy(dest->name, "copyof");
    strcat(dest->name, src->name);
    
    if(isDirectory(src->fileType))
    {
	char *new = NULL;
	mode_t umask = FSGetUMask();

	new = GetPathnameFromPathName(dest->path, dest->name);
	if (new && FSCreateNewDirectory(new, umask & 0777)) 
	{
	    char s[0xff];
	    
	    sprintf(s, _("Error creating folder %s"), new);
	    FSErrorDialog(_("File Operation Error"), s);
	}
	if(new)
	    free(new);
    }

    FSCopy(src, dest);
}

void 
FSCompressCB(FSViewer *fsViewer, int item, Time time)
{
    int error = 0;
    char *cmd   = FSGetStringForNameKey("COMPRESS", "exec");
    char *extn  = FSGetStringForNameKey("COMPRESS", "extn");
    
    if(!extn)
	extn = wstrdup(".tar.gz");

    if(cmd)
    {
	char *targz    = NULL;
	char *execStr  = NULL;
	char *pathname = NULL;
	FileInfo *src  = NULL;

	src = fsGetFSViewerCurrentFileInfo(fsViewer);
	pathname = GetPathnameFromPathName(src->path, src->name);
	targz = (char *) wmalloc(strlen(src->name)+strlen(extn)+1);

	strcpy(targz, src->name);
	strcat(targz, extn);

	execStr = (char *) wmalloc(strlen(cmd) + strlen(src->name) +
				   strlen(targz) + 1);

	sprintf(execStr, cmd, targz, src->name);

	if(execStr)
	{
	    error = FSExecCommand(src->path, execStr);
	    free(execStr);
	}
	else
	    error = 1;
	free(cmd);
    }
    else
	error = 1;

    if(error)
    {
	char s[0xff];
	
	sprintf(s, _("Unable to compress selection."));
	WMRunAlertPanel(FSGetFSViewerScreen(fsViewer), NULL, _("Exec Error"), 
			s, _("OK"), NULL, NULL);
    }

    if(extn)
	free(extn);
}

void 
FSEmptyRecyclerCB(FSViewer *fsViewer, int item, Time time)
{
    WMRunAlertPanel(FSGetFSViewerScreen(fsViewer), NULL, _("Menu Error"), 
		    _("This menu option has not yet been implemented"),
		    _("OK"), NULL, NULL);
}

static void
FSAddDisksMenu(FSViewer *fsViewer, WMMenu *menu)
{
    WMMenu *submenu = NULL;
    char *cmd = NULL;
    WMPropList* devArray = NULL;
    WMPropList* array    = NULL;

/*     devArray = FSGetArrayForNameKey("DISKS", "devices"); */
    devArray = FSGetUDObjectForKey(defaultsDB, "DISCS");

    if(devArray)
    {
	int i, numElem;
	WMPropList* tmp;

	numElem = WMGetPropListItemCount(devArray);
	for(i = 0; i < numElem; i++)
	{
	    array = WMGetFromPLArray(devArray, i);
	    if(array && WMIsPLArray(array))
	    {
		Disk *disk = NULL;

		disk = (Disk *) wmalloc(sizeof(Disk));
		memset(disk, 0, sizeof(Disk));

		disk->app = fsViewer;
		tmp = WMGetFromPLArray(array, 0);
		if(WMIsPLString(tmp))
		    disk->name = wstrdup(WMGetFromPLString(tmp));
		
		tmp = WMGetFromPLArray(array, 1);
		if(WMIsPLString(tmp))
		    disk->point = wstrdup(WMGetFromPLString(tmp));
		
		tmp = WMGetFromPLArray(array, 2);
		if(WMIsPLString(tmp))
		    disk->device = wstrdup(WMGetFromPLString(tmp));
		
		tmp = WMGetFromPLArray(array, 3);
		if(WMIsPLString(tmp))
		    disk->mntCmd = wstrdup(WMGetFromPLString(tmp));
		
		tmp = WMGetFromPLArray(array, 4);
		if(WMIsPLString(tmp))
		    disk->umntCmd = wstrdup(WMGetFromPLString(tmp));
		
		tmp = WMGetFromPLArray(array, 5);
		if(WMIsPLString(tmp))
		    disk->ejectCmd = wstrdup(WMGetFromPLString(tmp));
		
		tmp = WMGetFromPLArray(array, 6);
		if(WMIsPLString(tmp))
		    disk->closeCmd = wstrdup(WMGetFromPLString(tmp));
		
		if(disk->name)
		{
		    submenu = WMMenuCreate(fsViewer->wmContext, disk->name); 
		    WMMenuAddSubmenu(menu, disk->name, submenu);
		    WMMenuAddItem(submenu, _("Mount"),  
				  (WMMenuAction)FSMountCB, disk, NULL, NULL); 
		    WMMenuAddItem(submenu, _("Unmount"), 
				  (WMMenuAction)FSUnmountCB, disk, NULL, NULL); 
		    WMMenuAddItem(submenu, _("Eject"),  
				  (WMMenuAction)FSEjectCB, disk, NULL, NULL); 
		    WMMenuAddItem(submenu, _("Close"),  
				  (WMMenuAction)FSCloseDiskCB, disk, NULL, NULL); 
		}
	    }
	}
    }
}

void 
FSCreateMenu(FSViewer *fsViewer)
{
    int 	i;
    WMMenu      *submenu;

    fsViewer->menu = WMMenuCreate(fsViewer->wmContext, "FSViewer"); 

    /* Info Menu */
    submenu = WMMenuCreate(fsViewer->wmContext, _("Info")); 
    WMMenuAddSubmenu(fsViewer->menu, _("Info"), submenu); 
    WMMenuAddItem(submenu, _("Info Panel..."),  
		  (WMMenuAction)FSInfoPanelCB, fsViewer, NULL, NULL); 
    WMMenuAddItem(submenu, _("Legal..."),  
		  (WMMenuAction)FSLegalCB, fsViewer, NULL, NULL); 
    WMMenuAddItem(submenu, _("Preferences..."),  
		  (WMMenuAction)FSPrefCB, fsViewer, NULL, NULL); 
/*     WMMenuAddItem(submenu, _("WM Preferences..."),   */
/* 		  (WMMenuAction)FSWMPrefCB, fsViewer, NULL, NULL);  */
    i = WMMenuAddItem(submenu, _("Help..."),
		      (WMMenuAction)FSHelpCB, fsViewer, NULL, "?");
/*     WMMenuSetEnabled(submenu, i, 0); */

    /* File Menu */
    submenu = WMMenuCreate(fsViewer->wmContext, _("File")); 
    WMMenuAddSubmenu(fsViewer->menu, _("File"), submenu);
    i = WMMenuAddItem(submenu, _("Open"),
		      (WMMenuAction)FSViewCB, fsViewer, NULL, "o"); 
/*     WMMenuSetEnabled(submenu, i, 0); */
    i = WMMenuAddItem(submenu, _("Open as Folder"),
		      (WMMenuAction)FSOpenAsFolderCB, fsViewer, NULL, "O");
/*     WMMenuSetEnabled(submenu, i, 0); */
    WMMenuAddItem(submenu, _("New Folder"),  
		  (WMMenuAction)FSNewDirCB, fsViewer, NULL, "n"); 
    i = WMMenuAddItem(submenu, _("Duplicate"),
		      (WMMenuAction)FSDuplicateCB, fsViewer, NULL, "d");
/*     WMMenuSetEnabled(submenu, i, 0); */
    i = WMMenuAddItem(submenu, _("Compress"),
		      (WMMenuAction)FSCompressCB, fsViewer, NULL, NULL);
/*     WMMenuSetEnabled(submenu, i, 0); */
    i = WMMenuAddItem(submenu, _("UnCompress"),
		      (WMMenuAction)FSViewCB, fsViewer, NULL, NULL);
/*     WMMenuSetEnabled(submenu, i, 0); */
    i = WMMenuAddItem(submenu, _("Destroy"),
		      (WMMenuAction)FSDeleteCB, fsViewer, NULL, "r");
/*     WMMenuSetEnabled(submenu, i, 0); */
    i = WMMenuAddItem(submenu, _("Empty Recycler"),
		      (WMMenuAction)FSEmptyRecyclerCB, fsViewer, NULL, NULL);
/*     WMMenuSetEnabled(submenu, i, 0); */

    /* Edit Menu */
    submenu = WMMenuCreate(fsViewer->wmContext, _("Edit")); 
    WMMenuAddSubmenu(fsViewer->menu, _("Edit"), submenu);
    WMMenuAddItem(submenu, _("Cut"),  
		  (WMMenuAction)FSCutCB, fsViewer, NULL, "x"); 
    WMMenuAddItem(submenu, _("Copy"),  
		  (WMMenuAction)FSCopyCB, fsViewer, NULL, "c"); 
    WMMenuAddItem(submenu, _("Paste"),  
		  (WMMenuAction)FSPasteCB, fsViewer, NULL, "v"); 
    WMMenuAddItem(submenu, _("Delete"),  
		  (WMMenuAction)FSDeleteCB, fsViewer, NULL, NULL); 
    i = WMMenuAddItem(submenu, _("Select All"),
		      (WMMenuAction)FSSelectAllCB, fsViewer, NULL, "a");
/*     WMMenuSetEnabled(submenu, i, 0); */
    i = WMMenuAddItem(submenu, _("DeSelect All"),
		      (WMMenuAction)FSDeselectAllCB, fsViewer, NULL, NULL);
/*     WMMenuSetEnabled(submenu, i, 0); */

    /* Disk Menu */
    submenu = WMMenuCreate(fsViewer->wmContext, _("Disk"));
    WMMenuAddSubmenu(fsViewer->menu, _("Disk"), submenu);
    FSAddDisksMenu(fsViewer, submenu);
/*     i = WMMenuAddItem(submenu, _("Eject"), */
/* 		  NULL, fsViewer, NULL, "e"); */
/*     WMMenuSetEnabled(submenu, i, 0); */
/*     i = WMMenuAddItem(submenu, _("Initialize..."), */
/* 		  NULL, fsViewer, NULL, NULL); */
/*     WMMenuSetEnabled(submenu, i, 0); */
/*     i = WMMenuAddItem(submenu, _("Check for Disks"), */
/* 		  NULL, fsViewer, NULL, NULL); */
/*     WMMenuSetEnabled(submenu, i, 0); */

    /* View Menu */
    submenu = WMMenuCreate(fsViewer->wmContext, _("View")); 
    WMMenuAddSubmenu(fsViewer->menu, _("View"), submenu);
    WMMenuAddItem(submenu, _("Browse"),  
		  (WMMenuAction)FSBrowseCB, fsViewer, NULL, "B");
/* Diabled Icon Menu, because it does nothing then being present */
    i = WMMenuAddItem(submenu, _("Icon"),  
		  (WMMenuAction)FSIconCB, fsViewer, NULL, "I"); 
/*    WMMenuSetEnabled(submenu, i, 0);  */
    WMMenuAddItem(submenu, _("List"),  
		  (WMMenuAction)FSListCB, fsViewer, NULL, "L"); 
    WMMenuAddItem(submenu, _("Sort"),  
		  (WMMenuAction)FSSortOrderCB, fsViewer, NULL, "s");
/*     i = WMMenuAddItem(submenu, _("Clean Up Icons"), */
/* 		      NULL, fsViewer, NULL, NULL); */
/*     WMMenuSetEnabled(submenu, i, 0); */
    WMMenuAddItem(submenu, _("Show .files"),  
		  (WMMenuAction)FSDotFilesCB, fsViewer, NULL, "D");
    WMMenuAddItem(submenu, _("Filter..."),  
		  (WMMenuAction)FSFilterViewCB, fsViewer, NULL, "f");
    WMMenuAddItem(submenu, _("New Viewer..."),  
		  (WMMenuAction)FSNewViewCB, fsViewer, NULL, "N");
    WMMenuAddItem(submenu, _("Update View"),  
		  (WMMenuAction)FSUpdateViewCB, fsViewer, NULL, "u");
    
    /* Tools Menu */
    submenu = WMMenuCreate(fsViewer->wmContext, _("Tools")); 
    WMMenuAddSubmenu(fsViewer->menu, _("Tools"), submenu);
    WMMenuAddItem(submenu, _("Inspector..."),  
		  (WMMenuAction)FSInspectorCB, fsViewer, NULL, "i");
    i = WMMenuAddItem(submenu, _("Finder"),
		      (WMMenuAction)FSFinderCB, fsViewer, NULL, "F");
    /*     WMMenuSetEnabled(submenu, i, 0); */
    i = WMMenuAddItem(submenu, _("Processes..."),
		      (WMMenuAction)FSProcessCB, fsViewer, NULL, "P");
/*     WMMenuSetEnabled(submenu, i, 0); */
    WMMenuAddItem(submenu, _("Console..."),  
		  (WMMenuAction)FSConsoleCB, fsViewer, NULL, "C");

    /* Windows Menu */
    submenu = WMMenuCreate(fsViewer->wmContext, _("Windows"));
    WMMenuAddSubmenu(fsViewer->menu, _("Windows"), submenu);
/*     i = WMMenuAddItem(submenu, _("Arrange in Front"), */
/* 		      NULL, fsViewer, NULL, NULL); */
/*     WMMenuSetEnabled(submenu, i, 0); */
    /*
     * This should contain a list of the windows
     * Problem would be how to foucs them if they are
     * added to the submenu. Would setting the focus to
     * a widget work?
     */
    /* 
     * This is the standard key binding for miniaturizing windows
     * under window maker. At this moment I can find a wmlib function
     * to do this though I should put one together from the X functions
     */
    i = WMMenuAddItem(submenu, _("Miniaturize Window"),
		      (WMMenuAction)FSMiniaturizeCB, fsViewer, NULL, "m");
/*     WMMenuSetEnabled(submenu, i, 0); */
    i = WMMenuAddItem(submenu, _("Close Window"),
		      (WMMenuAction)FSCloseWindowCB, fsViewer, NULL, "w");
/*     WMMenuSetEnabled(submenu, i, 0); */

    /* Services Menu <- Custom stuff belongs here :) */
    submenu = WMMenuCreate(fsViewer->wmContext, _("Services"));
    WMMenuAddSubmenu(fsViewer->menu, _("Services"), submenu);
    WMMenuAddItem(submenu, _("File Edit..."),
          (WMMenuAction)FSEditCB, fsViewer, NULL, "E");
    WMMenuAddItem(submenu, _("File Link..."),
		  (WMMenuAction)FSLinkCB, fsViewer, NULL, "l");
    WMMenuAddItem(submenu, _("File New..."),
		  (WMMenuAction)FSNewFileCB, fsViewer, NULL, NULL);
    WMMenuAddItem(submenu, _("File Rename..."),
		  (WMMenuAction)FSRenameCB, fsViewer, NULL, NULL);
    WMMenuAddItem(submenu, _("File Run..."),
		  (WMMenuAction)FSRunCB, fsViewer, NULL, "R");
    WMMenuAddItem(submenu, _("Show Image"),
		  (WMMenuAction)FSViewCB, fsViewer, NULL, "S");
/*     i = WMMenuAddItem(submenu, _("Show PDF"), */
/* 		  (WMMenuAction)FSViewCB, fsViewer, NULL, NULL); */
/*     WMMenuSetEnabled(submenu, i, 0); */

    /* Hide and Quit */
    WMMenuAddItem(fsViewer->menu, _("Hide"),  
		  (WMMenuAction)FSHideCB, fsViewer, NULL, "h");  
    WMMenuAddItem(fsViewer->menu, _("Hide Others"),
		  (WMMenuAction)FSHideOthersCB, fsViewer, NULL, NULL);
    WMMenuAddItem(fsViewer->menu, _("Quit"),  
		  (WMMenuAction)FSQuitCB, fsViewer, NULL, "q"); 

    WMAppSetMainMenu(fsViewer->wmContext, fsViewer->menu);  
    WMRealizeMenus(fsViewer->wmContext);  

}

