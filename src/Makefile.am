## Process this file with automake to produce Makefile.in
##
SUBDIRS                 = regexp
bindir			= @fsviewerdir@
bin_PROGRAMS		= FSViewer
datadir			= @fsviewerdir@
pkgdatadir              = @fsviewerdir@
FSViewer_SOURCES	= FSViewer.c files.c FSFileBrowser.c\
			  list.c misc.c prop.c timestampWidget.c \
			  attribsInspector.c editInspector.c exeInspector.c \
			  extnInspector.c iconInspector.c viewInspector.c \
			  FSBrowser.c FSPanel.c FSUtils.c FSMenu.c\
			  FSFileView.c FSInspector.c magic.c DnD.c \
			  FSPathView.c xmodifier.c FSFinder.c FSFileButton.c\
			  FSMCList.c FSPrefs.c
LDADD                   = -Lregexp -lregx
noinst_HEADERS		= FSViewer.h filebrowser.h files.h list.h misc.h \
			  timestampWidget.h FSBrowser.h FSPanel.h FSUtils.h \
			  FSMenu.h FSFileView.h magic.h DnD.h FSFileButton.h \
			  FSFinder.h FSMCList.h FSPathView.h FSPrefs.h
pkgdata_DATA		= MagicFiles 
##EXTRA_DIST		= icons/* xpm/* fsviewer.sh
EXTRA_DIST		= xpm/* MagicFiles
LIBS			= @LIBINTL@ @LIBS@
# LOCALEDIR		= @LOCALEDIR@
# DEFS			= -DLOCALEDIR=\"$(localedir)\" @DEFS@
