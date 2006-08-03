// FSPrefs.h

#ifndef _FSPREFS_H_
#define _FSPREFS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef	struct	_FSPreferencesPanel
{
    Display    	 *dpy;
    WMScreen	 *scr;
    FSViewer   	 *app;
    WMWindow	 *win;

    WMTabView    *tabV;
    WMFrame      *frame;

    WMButton     *okBtn;
    WMButton     *applyBtn;
    WMButton     *cancelBtn;

    /* Images */
    WMList       *imgList;
    WMButton     *imgIconBtn;
    
    /* Executables */
    WMList       *execList;
    WMButton     *execIconBtn;
    WMTextField  *execTF;
    WMButton     *execSetBtn;

    /* Display */
    WMList       *dpyList;
    WMButton     *dpyIconBtn;
    WMTextField  *dpyTF;
    WMButton     *dpySetBtn;

    /* Apps */
    WMList       *appsList;
    WMButton     *appsSetBtn;
    WMButton     *appsRemoveBtn;
    WMButton     *appsIconBtn;
    WMButton     *appsBrowseBtn;
    WMTextField  *appsExecTF;

    /* File Types */
    WMList       *typesList;
    WMButton     *typesSetBtn;
    WMButton     *typesRemoveBtn;
    WMButton     *typesIconBtn;
    WMButton     *typesViewBrowseBtn;
    WMButton     *typesEditBrowseBtn;
    WMTextField  *typesViewTF;
    WMTextField  *typesEditTF;

    /* Disks */
    WMList       *disksList;
    WMButton     *disksNewBtn;
    WMButton     *disksRemoveBtn;
    WMButton     *disksUpdateBtn;
    WMTextField  *disksNameTF;
    WMTextField  *disksMntTF;
    WMTextField  *disksDevTF;

    /* Disk Commands */
    WMTextField  *cmdMntTF;
    WMTextField  *cmdUMntTF;
    WMTextField  *cmdEjectTF;
    WMTextField  *cmdCloseTF;

/*     int	       	 done; */

    struct {
	unsigned int evars:1;
	unsigned int eapps:1;
	unsigned int ediscs:1;

	unsigned int done:1;
    } flags;

} FSPreferencesPanel;

void  FSRunPreferencesPanel(FSViewer *app, char *title);

#ifdef __cplusplus
}
#endif

#endif // _FSPREFS_H_

