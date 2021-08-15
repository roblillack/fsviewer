%define	GNUstepApps	/usr/X11R6/GNUstep/Apps

Summary:	FSViewer is a NeXT FileViewer lookalike for Window Maker.
Summary(de):	Dateisystembrowser aehnlich dem NeXT FileViewer.
Name:		FSViewer.app
Version:	0.2.6
Release:        1%{?dist}
Prefix:		/usr
License:	GPL
Group:		Productivity/File utilities
URL:		http://www.bayernline.de/~gscholz/linux/fsviewer/
Vendor:		Guido Scholz <guido.scholz@bayernline.de>
Source:		fsviewer-app-%{version}.tar.bz2
BuildRoot:	%{_tmppath}/%{name}-%{version}-root
Requires:	WindowMaker >= 0.90
Distribution:	SuSE 10.1

%description
FSViewer is a NeXT FileViewer lookalike for Window Maker. Viewing is
currently supported via browser and list mode. It has been written in C
using the WINGs library.  

Authors:
========
  George Clernon <clernong@tinet.ie>
  Guido Scholz <guido.scholz@bayernline.de> (port to Window Maker 0.80.0)

%description -l de
FSViewer ist ein Dateisystembrowser, der sich optisch und funktionell
stark an den NeXT FileViewer anlehnt. Die Dateiansicht besteht derzeit in
Form einer "Browser"- und einer Listenansicht. Das Programm wurde in C
unter Benutzung der WINGs-Bibliothek geschrieben. 

Autoren:
========
  George Clernon <clernong@tinet.ie>
  Guido Scholz <guido.scholz@bayernline.de> (Portierung nach Window Maker 0.80.0)

%prep
%setup -q -n fsviewer-app-%{version}

%build
CFLAGS=$RPM_OPT_FLAGS \
./configure --prefix=%{_prefix} \
	    --mandir=%{_mandir} \
            --with-appspath=%{GNUstepApps}
make

%install
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install-strip

install -d $RPM_BUILD_ROOT/%{_docdir}/%{name}

install -d $RPM_BUILD_ROOT/%{GNUstepApps}/%{name}/tiff
install -m 644 tiff/*.tif $RPM_BUILD_ROOT/%{GNUstepApps}/%{name}/tiff

install -d $RPM_BUILD_ROOT/%{GNUstepApps}/%{name}/xpm
install -m 644 xpm/*.xpm  $RPM_BUILD_ROOT/%{GNUstepApps}/%{name}/xpm

%clean
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{GNUstepApps}/%{name}
%{_prefix}/share/locale/*/LC_MESSAGES/FSViewer.mo
%docdir %{_mandir}/man1/*
%{_mandir}/man1/*
%{_docdir}/%{name}
%doc defs/chdef docs/* AUTHORS COPYING ChangeLog INSTALL NEWS README

%changelog
* Sun Oct 14 2007 Guido Scholz <guido.scholz@bayernline.de>
- update to version 0.2.6
- spec-File modifications for SuSE 10.1

* Sat Jun 12 2004 Guido Scholz <guido.scholz@bayernline.de>
- spec-File Modifications for SuSE 9.0

* Tue Dec 24 2002 Guido Scholz <guido.scholz@bayernline.de>
- Added menu modification scripts for SuSE 8.0

* Sun Dec 15 2002 Guido Scholz <guido.scholz@bayernline.de>
- New release FSViewer.app 0.2.5

* Sun Nov 10 2002 Guido Scholz <guido.scholz@bayernline.de>
- New release FSViewer.app 0.2.4

* Fri Nov 01 2002 Guido Scholz <guido.scholz@bayernline.de>
- New release FSViewer.app 0.2.3f

* Mon Feb 25 2002 Guido Scholz <guido.scholz@bayernline.de>
- New release FSViewer.app 0.2.3d

* Sat Jan 05 2002 Guido Scholz <guido.scholz@bayernline.de>
- Adaption to WindowMaker 0.80.0 (removed libProplist dependency,
  changed icon handling)

* Fri Oct 22 1999 Ryan Weaver <ryanw@infohwy.com>
  [fsviewer-0.2.3-1]
- Changed MAXINT to INT_MAX for NetBSD v1.3.2
- Added Preferences Window
- Fixed bug in FSGetStringForName, if the returned value is NULL,
  wstrdup is not called.
- Fixed bug in FSCreatePixmapWithBacking*** when image is bigger
  than backing.
- Created Icon Selection Dialog.
- Updated Icon Inspector to reflect look of Icon Selection Dialog.
- Set window level for some dialogues.
- Updated Edit|View Inspectors to save the extn into the EXTN
  defaults file entry.
- Modified chdefs to make the latest changes the necessary
  changes to the defaults file.
- Added DisplayMCListPixmap variable to turn on/off display of pixmaps
  indicating file/directory. The list runs better with it off!
- Add check for empty clientData in paintItem of FSMCList.

* Mon Aug  2 1999 Ryan Weaver <ryanw@infohwy.com>
  [fsviewer-0.2.2-1]
- Added icons for files and directories in list view
- Plugged a few memory leaks found by Electric Fence
- Insertion/Deletion of files now updates all views
- Changed the configure script to use get-wraster-flags,
  this will hopefully fix most "can't find library" problems
- Changed listview parsing method.
- Applied a patch by Igor Roboul to FSViewer.c and iconInspector.c 

* Fri Jul  2 1999 Ryan Weaver <ryanw@infohwy.com>
  [fsviewer-0.2.1-1]
- FSViewer now compiles with WM 0.60
- Added a preliminary list view. This is really for v0.3.0 but I left
  it in anyway.
- FSGetStringFor*** now returns a duplicated string.
- wmallocing memory with (Disk) instead of (Disk *)
- Fixed icon naming problem when the icon changes but the popup doesn't

* Fri May 28 1999 Ryan Weaver <ryanw@infohwy.com>
  [fsviewer-0.2.0-2]
- OPPS! Now installing in /usr/GNUstep instead of my typo of /usr/GNUStep.

* Tue May 25 1999 Ryan Weaver <ryanw@infohwy.com>
  [fsviewer-0.2.0-1]
- Patched Makefile.in's to allow me to install into an arbitrary prefix dir.
- FSViewer.app-0.2.0 (19990507)
- Added dialog to prompt for settings with respect to unknown
  file types.
- Added app specific alert panel.
- Added in fallback condition that if the creation of an icon fails
  it defaults to the file_plain xpm.
- Automagically generate a defaults file if one doesn't exist.
- Rewrote proplist code to use WMUserDefaults
- Reorganised app/file extension association. The extension now
  has a viewer/editor exec string associated with it.
- Inspector panels resized, they look better now :)
- Removed Extension Inpector Panel.
- Fixed bug wrt adding to shelf from column 0.
- Shelf item background momentarily changes when clicked.
- "/" cannot be added to the shelf.
- Implemented basic preferences window.
- Sort file view by ascending or descending and by files first, 
  directories first or all jumbled up!
- Modified the backing pixmap for the shelf and the scrollview.
  A 64x64 pixmap is combined with the original pixmap. This allows
  buttons to be used as the widget holder for the icons in the
  scrollview.
- The browser (kinda) mimics the NeXT movements i.e. it only scrolls
  the browser if a file/folder is selected that is outside the current
  columns. 
- Added FSFileButton widget to provide shaped background and DnD 
  related stuff.
- Rewrote shelf to use FSFileButton widget
- Updated FSBrowser to reflect changes in the WINGs library
- Implemented Path View using frames and file buttons
- Fleshed out the main menu and populated some options
- Added in platform specific #defines for memory and disk calculations
- Added in DnD support. OffiX library ported by Igor Roboul.
- Added in code to change the cursor in the Dnd code and removed the
  root drop checks
- Modified the main menu to accept only Alt+key_press or 
  Meta+key_press depending on keyboard setup.
- Created PathView widget based on wbrowser. It provides better 
  support for window/column resizing.
- Free the FSFileInfo list after populating the browser. This should 
  plug a memory leak. 
- Provided the ability to resize the columns
- Added double click to browser items
- Added tooltip like view to the FSFileButton widget. Popups when the 
  text is to wide for the button.
- Added Update Viewer menu call.
- Browser can now be updated based on file action (copy/delete etc.)
- Added in xmodifier code from WPRefs/WM to provide correct Alt|Meta
  key support.
- Added chdef to change the old defaults file to the new format.
- Added option to disable the timestamp from the Inspector Window.
- Added support for formats (besides XPM and TIFF) supported by wmlib.
  Need to uncomment the code in FSUtils to use.
- Added in front end to find and locate. Uses hardcoded defaults for
  the moment, this will change in the next rev.
- Now pass executable name to WMInitialiseApplication instead of full
  pathname.
- cdef now includes update for ROOT icon entry.

* Wed Feb 23 1999 Ryan Weaver <ryanw@infohwy.com>
  [fsviewer-0.1.1-1]
- FSViewer.app-0.1.1 (19990223)
- Commented out FSDisk related code in FSUtils.c, should fix
  some multi-platform compilation problems.
- Added a patch to automatically detect HOME dir.
- Fixed a bug that crashes the app if the inital path contains a
  long path component.
- Zombie process are now cleaned up.
- Fixed WMJustified bug (should be WAJustified) in inspector.c and 
  iconInspector.c
- Implemented Revert in attributes inspector.
- Initial launch path is now the users home dir.
- Now checks return value of getpwuid and getgrgid calls in 
  attribsInspector.c, used to seg fault if either group or user
  was unknown.
- Disabled hiding of app via 'h' key, leaving it up to Window Maker 
  and changed the quit key from 'q' to '^q' to prevent accidents :).
- Added in a "Console" menu option under the "Tools" menu. It launches
  an xterm at the current directory.
- The window and miniwindow icon titles are set with the current 
  selection.
- Added in rudimentary magic files support. This allows certain kinds
  of unknown files to be viewed. 
- Fixed bug wrt to displaying/hiding dot files.
- Added in file filtering.

* Wed Feb  3 1999 Ryan Weaver <ryanw@infohwy.com>
  [fsviewer-0.1.0-1]
- FSViewer.app-0.1.0 (19990201)
- Reorganised the initialisation of the app.
- Multiple Windows can be opened.
- Display hidden files.
- Permissions can be set from attributes inspector.
- Icons can be set for extensions from File Extension
  inspector.
- Added legal panel.
- Added info panel (broke app for other platforms though).
- Tidied up code for file browsing to easily add other
  views.
- In the process of moving odds and ends to FSUtils.c
- Rewrote the scrollview code that displays the current path.
- The window(s) can be resized in both width and height.
- Using nodename from utsname struct as the hostname.
- Using timestampWidget instead of timestamp in attribsInspector 
  as it can display the year.
- Updated the configure script to take into account datadir value
  when generating the FSViewer default file.
- Added in Cut, Copy, Paste, Rename, Delete and Link.
- Inspector window now updates automatically if it is open.
- Added in the shelf.
- Added new file/directory creation.
- Before launching an app, the pwd is changed to the selected dir.
- Added in the Tools submenu and moved the Inspector there.
- Created icons and src sub directories.

* Fri Dec 18 1998 Ryan Weaver <ryanw@infohwy.com>
  [fsviewer-0.0.2-1]
- FSViewer.app-0.0.2 (19981217)
- Added in more checks to the configure script.
- Replaced basename with GetNameFromPathname.
- Using strrchr instead of rindex.
- Searches the Window Maker defaults domain for IconPath
  instead of PixmapPath
- Using "/" if environment variable HOSTNAME is not set.
- Added command line option to start FSViewer at a specified
  path.

* Thu Dec 10 1998 Ryan Weaver <ryanw@infohwy.com>
  [fsviewer-19981214-1]
- First RPM
- 14/12/98
    The source code for FSViewer is now available. This is a preliminary
  release. It's not guaranteed to do anything. Hopefully it will compile and
  run straight out of the box. I'm currently working on getting it to compile
  on my Sun at work.

