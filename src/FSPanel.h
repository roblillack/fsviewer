// FSPanel.h

#ifndef _FSPanel_H_
#define _FSPanel_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef	struct	_FSInfoPanel
{
    Display    	*dpy;
    WMScreen	*scr;
    FSViewer    *app;
    WMWindow	*win;
    WMLabel    	*iconLabel;
    WMLabel    	*titleLabel;
    WMFrame    	*frame;
    WMLabel    	*author;
    WMLabel    	*sysRelease;
    WMLabel    	*fsRelease;
    WMLabel    	*xRelease;
    WMLabel    	*machine;
    WMLabel    	*memSize;
    WMLabel    	*discSize;
    WMLabel    	*copyright;
    int		done;

} FSInfoPanel;

typedef	struct	_FSLegalPanel
{
    Display	*dpy;
    WMScreen	*scr;
    FSViewer    *app;
    WMWindow	*win;
    WMLabel    	*license;

    int		done;

} FSLegalPanel;

/* typedef	struct	_FSPreferencesPanel */
/* { */
/*     Display    	*dpy; */
/*     WMScreen	*scr; */
/*     FSViewer   	*app; */
/*     WMWindow	*win; */

/*     WMLabel    	*iconLabel; */
/*     WMLabel    	*titleLabel; */
/*     WMFrame    	*frame; */

/*     WMButton    *dotFileBtn; */
/*     WMButton    *sortBtn[3]; */
/*     WMButton    *orderBtn[2]; */
/*     WMButton    *scrollviewBtn; */
    
/*     WMTextField *iconDirField; */

/*     WMButton    *okBtn; */
/*     WMButton    *revertBtn; */

/*     int	       	done; */

/* } FSPreferencesPanel; */

typedef	struct	_FSIconSelectPanel
{
    Display    	*dpy;
    WMScreen	*scr;
    FSViewer   	*app;
    WMWindow	*win;

    FileInfo    *fileInfo;
    Panel       *panel;

    int		done;
    int		cnt;

} FSIconSelectPanel;

typedef	struct	_FSAppInputPanel
{
    Display    	*dpy;
    WMScreen	*scr;
    FSViewer   	*app;
    WMWindow	*win;

    WMFrame    	*frame;

    WMButton    *iconBtn;
    WMButton    *okBtn;
    WMButton    *cancelBtn;
    WMButton    *edBtn;
    WMButton    *viewBtn;
    WMButton    *browseBtn;

    WMTextField *nameField;
    WMTextField *execField;

    int		done;
    char       *exec;
    FileInfo   *fileInfo;

} FSAppInputPanel;

typedef struct FSAlertPanel 
{
    FSViewer *app;
    Display  *dpy;
    WMScreen *scr;
    WMWindow *win;                     /* window */

    WMButton *defBtn;                  /* default button */
    WMButton *altBtn;                  /* alternative button */
    WMButton *othBtn;                  /* other button */
    WMLabel *iLbl;                     /* icon label */
    WMLabel *tLbl;                     /* title label */
    WMLabel *mLbl;                     /* message label */
    WMFrame *line;                     /* separator */
    short result;                      /* button that was pushed */
    short done;
   
    KeyCode retKey;
} FSAlertPanel;

typedef	struct	_FSSelectIconPanel
{
    Display    	*dpy;
    WMScreen	*scr;
    FSViewer   	*app;
    WMWindow	*win;

    WMFrame    	*frame;

    WMLabel     *iconLabel;
    WMList      *pathList;
    WMList      *fileList;
    WMButton    *okBtn;
    WMButton    *cancelBtn;
/*     WMButton    *edBtn; */
/*     WMButton    *viewBtn; */
/*     WMButton    *browseBtn; */

/*     WMTextField *nameField; */
/*     WMTextField *execField; */

    int		 done;
    char        *iconName;
    char        *xpmDir;
    char        *tiffDir;
/*     FileInfo   *fileInfo; */

} FSSelectIconPanel;

int   FSRunAlertPanel(FSViewer *app,
		      char *title, char *msg, char *defaultButton,
		      char *alternateButton, char *otherButton);
char *FSRunAppInputPanel(FSViewer *app, FileInfo *fileInfo, char *title);
void  FSRunIconSelectPanel(FSViewer *app, char *title, char *extn);
void  FSRunInfoPanel(FSViewer *app, char *title, char *msg);
void  FSRunLegalPanel(FSViewer *app, char *title, char *msg);
char *FSRunSelectIconPanel(WMWindow *owner, char *title, char *msg);
/* void  FSRunPreferencesPanel(FSViewer *app, char *title); */

#ifdef __cplusplus
}
#endif

#endif // _FSPanel_H_

