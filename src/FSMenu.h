#ifndef _FSMenu_H_
#define _FSMenu_H_

void FSQuitCB(WMWidget *w, void *data);
void FSHideCB(FSViewer *fsViewer, int item, Time time);
void FSLaunchApp(FSViewer *fsViewer, AppEvent event);
void FSInfoPanelCB(FSViewer *fsViewer, int item, Time time);
void FSLegalCB(FSViewer *fsViewer, int item, Time time);
void FSPrefCB(FSViewer *fsViewer, int item, Time time);
void FSRunCB(FSViewer *fsViewer, int item, Time time);
void FSViewCB(FSViewer *fsViewer, int item, Time time);
void FSEditCB(FSViewer *fsViewer, int item, Time time);
void FSInspectorCB(FSViewer *fsViewer, int item, Time time);
void FSCreateMenu(FSViewer *fsViewer);
void FSCopyCB(FSViewer *fsViewer, int item, Time time);
void FSCutCB(FSViewer *fsViewer, int item, Time time);
void FSPasteCB(FSViewer *fsViewer, int item, Time time);
void FSLinkCB(FSViewer *fsViewer, int item, Time time);
void FSRenameCB(FSViewer *fsViewer, int item, Time time);
void FSFilterViewCB(FSViewer *fsViewer, int item, Time time);
void FSConsoleCB(FSViewer *fsViewer, int item, Time time);
void FSSelectAllCB(FSViewer *fsViewer, int item, Time time);
void FSEjectDisksCB(FSViewer *fsViewer, int item, Time time);
void FSFinderCB(FSViewer *fsViewer, int item, Time time);
void FSMiniaturizeCB(FSViewer *fsViewer, int item, Time time);
void FSOpenAsFolderCB(FSViewer *fsViewer, int item, Time time);
void FSProcessCB(FSViewer *fsViewer, int item, Time time);
void FSHelpCB(FSViewer *fsViewer, int item, Time time);
void FSCloseWindowCB(FSViewer *fsViewer, int item, Time time);
void FSUpdateViewCB(FSViewer *fsViewer, int item, Time time);

#endif
