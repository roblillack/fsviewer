/***********************************************************************
Project:	WMFileViewer
Filename:	timestampWidget.c
Author:		Charles Gamble
                Original code by
		  Michael J. Mitchell <mitch@gw2.redback.com.au>
Purpose:	Source file for a NeXT style timestamp widget.
$Id: timestampWidget.c,v 1.2 1999/01/13 22:46:22 gambcl Exp $
***********************************************************************/




/* Header Files *******************************************************/

#include <X11/xpm.h>
#include <WINGs/WINGsP.h>
#include "timestampWidget.h"
#include "xpm/clk.xpm"
#include "xpm/led.xpm"
#include "xpm/month.xpm"
#include "xpm/date.xpm"
#include "xpm/weekday.xpm"
#include "xpm/year.xpm"


/* Definitions ********************************************************/

/****************************************************
* Define a debugging macro that can toggled on/off. *
****************************************************/

/*#define DEBUG_WIDGET_CODE  1*/
#undef DEBUG_WIDGET_CODE

#ifdef DEBUG_WIDGET_CODE
#define DEBUG_PRINT(msg)    fprintf(stderr, "TIMESTAMP_WIDGET: %s: %s\n", __FILE__, msg)
#else
#define DEBUG_PRINT(msg)
#endif




/* LED positions. */

static int  twelve[5]   = { 5, 14, 24, 28, 37 };
static int  twfour[5]   = { 4,  8, 17, 22, 31 };

static int  ns_posx[11] = { 5, 5, 5, 5, 5, 45, 21, 21, 26, 31, 19 };
static int  ns_posy[4]  = { 7, 25, 34, 49 };

static int  posx[11];
static int  posy[4];


/* Year digit widths & positions. */

static int  year_width[10] = { 5, 1, 4, 4, 5, 4, 5, 4, 5, 5 };
static int  year_pos[10]   = { 0, 5, 6, 10, 14, 19, 23, 28, 32, 37 };


static char		LedColor[] = "LightSeaGreen";




/* Type Definitions **********************************************************/

typedef struct _XpmIcon
{
    Pixmap pixmap;
    Pixmap mask;
    XpmAttributes attributes;
} XpmIcon;


typedef struct W_TimeStamp
{
    /********************************************************
    * These two fields must be present in all WINGs widgets *
    * in this exact position.                               *
    ********************************************************/
    W_Class widgetClass;
    WMView *view;
    

    /******************************************************
    * The fields below hold data specific to this widget. *
    ******************************************************/

    W_Pixmap	*image;
    XpmIcon     *result;
    XpmIcon     asclock;
    XpmIcon     led;
    XpmIcon     month;
    XpmIcon     date;
    XpmIcon     year;
    XpmIcon     weekday;
    XpmIcon     asvisible;
    GC          gc;
    WMColor     *color;
    time_t      time;

    struct flags
    {
        unsigned int blank: 1;      /* No LED, Day, Date, Month or Year. */
        unsigned int showAMPM: 1;   /* Default value is 24h format.*/
        unsigned int damaged: 1;    /* Re-render? */
    } flags;
} _TimeStamp;




/* Globals ************************************************************/

static char rcsid[] = "$Id: timestampWidget.c,v 1.2 1999/01/13 22:46:22 gambcl Exp $";
#ifdef DEBUG_WIDGET_CODE
static char debug_msg[1024];
#endif


/* Widget class ID. */
static W_Class timeStampWidgetClass = 0;




/* Local Prototypes ***************************************************/

static int InitClockResources(TimeStamp *timestamp);
static int GetXPM(TimeStamp *timestamp);
static void destroyTimeStamp(TimeStamp *timestamp);
static void handleEvents(XEvent *event, void *data);
static void paintTimeStamp(TimeStamp *timestamp);
static void setBackgroundColor(WMWidget *widget, WMColor *color);
/* static void resizeTimeStamp(WMWidget *widget, unsigned int width, unsigned int height); */
static void ASTwelve(TimeStamp *timestamp, struct tm *clk);
static void ASTwentyFour(TimeStamp *timestamp, struct tm *clk);
static void RenderASClock(TimeStamp *timestamp);
static void willResizeTimeStamp(W_ViewDelegate*, WMView*,
				unsigned int*, unsigned int*);


/* Some procedures you might want to override. Don't forget to call  */
/* the equivalent view procedure after (or before) doing your stuff. */
/* See the source for the other widgets to see how to use.           */
/* You won't need to use this most of the time.                      */
static W_ViewDelegate _TimeStampViewDelegate = {
        NULL,
        NULL,
        NULL,
        NULL,
        willResizeTimeStamp
};

/* static W_ViewProcedureTable _TimeStampViewProcedures = */
/* { */
/*     setBackgroundColor, */
/*     resizeTimeStamp, */
/*     NULL */
/* }; */




/***********************************************************************
Name:		InitTimeStamp
Purpose:	Class initializer.
                Must be called before creating any instances of the
                TimeStamp widget.
Inputs:		scrPtr - Pointer to a WMScreen.
Outputs:	None.
Returns:	A W_Class assigned to this widget by WINGs.
Status:		Finished.
Errors: 	None known.

***********************************************************************/
W_Class InitTimeStamp(WMScreen *scrPtr)
{
    /* Register the widget with WINGs and get the widget class ID. */
    if (!timeStampWidgetClass)
    {
/*         timeStampWidgetClass = W_RegisterUserWidget(&_TimeStampViewProcedures); */
        timeStampWidgetClass = W_RegisterUserWidget();
    }
    DEBUG_PRINT("Inside InitTimeStamp()");
    
    return timeStampWidgetClass;
}




/***********************************************************************
	Name:		CreateTimeStamp
	Purpose:	Creates an instance of the TimeStamp widget.
	Inputs:		parent - Parent WINGs widget.
	Outputs:	None.
	Returns:	An allocated TimeStamp structure on success,
                        NULL otherwise.
	Status:		Finished.
	Errors: 	None known.
***********************************************************************/
TimeStamp *CreateTimeStamp(WMWidget *parent)
{
    W_Screen    *scr;
    TimeStamp   *timestamp;

    DEBUG_PRINT("Inside CreateTimeStamp()");
    /* Allocate storage for a new instance. */
    timestamp = wmalloc(sizeof(TimeStamp));
    /* Initialize it. */
    memset(timestamp, 0, sizeof(TimeStamp));

    /* Set the class ID. */
    timestamp->widgetClass = timeStampWidgetClass;

    /* Create the view for our widget. Note: the Window for the view is */
    /* only created after the view is realized with W_RealizeView()     */
    /* Consider the returned view as read-only.                         */
    if (!(timestamp->view = W_CreateView(W_VIEW(parent))))
    {
        free(timestamp);
        return ((TimeStamp *)NULL);
    }
    
    DEBUG_PRINT("Created View");
    /* Always do this. */
    timestamp->view->self = timestamp;
    timestamp->view->delegate = &_TimeStampViewDelegate;

    scr = timestamp->view->screen;

    /* Create event handler. */
    WMCreateEventHandler(timestamp->view,
            ExposureMask | StructureNotifyMask, handleEvents, timestamp);

    /* Default GC for paint. */
    timestamp->color = WMGrayColor(scr);
    timestamp->gc = WMColorGC(timestamp->color);

    W_ResizeView(timestamp->view, TIMESTAMP_MIN_WIDTH, TIMESTAMP_MIN_HEIGHT);
    DEBUG_PRINT("Resized View");

    if (InitClockResources(timestamp) == -1)
    {
        wfatal("Unable to initialize Clock Resources");
    }
    DEBUG_PRINT("Done InitClockResources()");
    
    /* Create XPMs. */
    if (GetXPM(timestamp) == -1)
    {
        wfatal("Unable to initialize Clock Pixmaps");
    }
    DEBUG_PRINT("Done GetXPM()");

    timestamp->flags.damaged = 1;
    timestamp->result = &timestamp->asvisible;

    return (timestamp);
}




/***********************************************************************
Name:		InitClockResources
Purpose:	Initialises some coordinate data used in painting the
                timestamp.
Inputs:		timestamp - The TimeStamp.
Outputs:	None.
Returns:	0 for success, -1 otherwise.
Status:		Finished.
Errors: 	None known.
***********************************************************************/
static int InitClockResources(TimeStamp *timestamp)
{
    int i;

    /* Initialize LED positions. */
    for (i = 0; i < 4; i++)
    {
        posy[i] = ns_posy[i];
    }

    for (i = 0; i < 11; i++)
    {
        posx[i] = ns_posx[i];
    }
    
    for (i = 0; i < 5; i++)
    {
        posx[i] += ((timestamp->flags.showAMPM)) ? twfour[i] : twelve[i];
    }

    return (0);
}



/***********************************************************************
Name:		GetXPM
Purpose:	Initialises the XPM data used for the calendar face
                and digits.
Inputs:		timestamp - The TimeStamp.
Outputs:	Sets the Pixmap fields in the timestamp.
Returns:	0 for success, -1 otherwise.
Status:		Finished.
Errors: 	None known.
***********************************************************************/
static int GetXPM(TimeStamp *timestamp)
{
    Display     *dpy = timestamp->view->screen->display;
    Colormap    cmap;
    XColor      col;
    char        brightLED[22];
    char        darkLED[22];
    char        greyStr[22];
    int         greyR = 0, greyG = 0, greyB = 0;

    /* Get the default colourmap ID for this screen. */
    cmap = DefaultColormap(dpy, DefaultScreen(dpy));

    /* Get the RGB values for the LED colour. */
    if (!XParseColor(dpy, cmap, LedColor, &col))
    {
        /* Could not resolve colour. */
        return (-1);
    }

    /* Replace the 'bright' colour in the LED pixmap with our new one. */
    sprintf(brightLED, ".      c #%04X%04X%04X", col.red, col.green, col.blue);
    led_xpm[2] = brightLED;

    /* Now make a darker version and do the same. */
    col.red   = (col.red / 10) * 3;
    col.green = (col.green / 10) * 3;
    col.blue  = (col.blue / 10) * 3;
    sprintf(darkLED, "X      c #%04X%04X%04X", col.red, col.green, col.blue);
    led_xpm[3] = darkLED;


    /* Perform a bit of colour surgery on the clk.xpm and year.xpm to replace */
    /* the transparent colour "None" with the background grey colour.         */

    greyR = WMRedComponentOfColor(WMGrayColor(timestamp->view->screen));
    greyG = WMGreenComponentOfColor(WMGrayColor(timestamp->view->screen));
    greyB = WMBlueComponentOfColor(WMGrayColor(timestamp->view->screen));
    
    greyR >>= 8;
    greyG >>= 8;
    greyB >>= 8;
    
    sprintf(greyStr, ".      c #%02X%02X%02X", greyR, greyG, greyB);
    clk_xpm[1] = greyStr;
    year_xpm[1] = greyStr;


    /* Load the background image into 'asclock'. */
    timestamp->asclock.attributes.valuemask = (XpmReturnPixels | XpmReturnExtensions | XpmSize);
    if (XpmCreatePixmapFromData(dpy, DefaultRootWindow(dpy), clk_xpm, &timestamp->asclock.pixmap, &timestamp->asclock.mask, &timestamp->asclock.attributes) != XpmSuccess)
    {
        return (-1);
    }

    /* Load the background image into 'asvisible'. */
    timestamp->asvisible.attributes.valuemask = (XpmReturnPixels | XpmReturnExtensions | XpmSize);
    if (XpmCreatePixmapFromData(dpy, DefaultRootWindow(dpy), clk_xpm, &timestamp->asvisible.pixmap, &timestamp->asvisible.mask, &timestamp->asvisible.attributes) != XpmSuccess)
    {
        XFreePixmap(dpy, timestamp->asclock.pixmap);
        XFreePixmap(dpy, timestamp->asclock.mask);

        return (-1);
    }

    /* Load the LED digits image. */
    timestamp->led.attributes.valuemask = (XpmReturnPixels | XpmReturnExtensions | XpmSize);
    if (XpmCreatePixmapFromData(dpy, DefaultRootWindow(dpy), led_xpm, &timestamp->led.pixmap, &timestamp->led.mask, &timestamp->led.attributes) != XpmSuccess)
    {
        XFreePixmap(dpy, timestamp->asclock.pixmap);
        XFreePixmap(dpy, timestamp->asclock.mask);

        XFreePixmap(dpy, timestamp->asvisible.pixmap);
        XFreePixmap(dpy, timestamp->asvisible.mask);

        return (-1);
    }

    /* Load the month names image. */
    timestamp->month.attributes.valuemask = (XpmReturnPixels | XpmReturnExtensions | XpmSize);
    if (XpmCreatePixmapFromData(dpy, DefaultRootWindow(dpy), month_xpm, &timestamp->month.pixmap, &timestamp->month.mask, &timestamp->month.attributes) != XpmSuccess)
    {
        XFreePixmap(dpy, timestamp->asclock.pixmap);
        XFreePixmap(dpy, timestamp->asclock.mask);

        XFreePixmap(dpy, timestamp->asvisible.pixmap);
        XFreePixmap(dpy, timestamp->asvisible.mask);

        XFreePixmap(dpy, timestamp->led.pixmap);
        XFreePixmap(dpy, timestamp->led.mask);

        return (-1);
    }

    /* Load the date digits image. */
    timestamp->date.attributes.valuemask = (XpmReturnPixels | XpmReturnExtensions | XpmSize);
    if (XpmCreatePixmapFromData(dpy, DefaultRootWindow(dpy), date_xpm, &timestamp->date.pixmap, &timestamp->date.mask, &timestamp->date.attributes) != XpmSuccess)
    {
        XFreePixmap(dpy, timestamp->asclock.pixmap);
        XFreePixmap(dpy, timestamp->asclock.mask);

        XFreePixmap(dpy, timestamp->asvisible.pixmap);
        XFreePixmap(dpy, timestamp->asvisible.mask);

        XFreePixmap(dpy, timestamp->led.pixmap);
        XFreePixmap(dpy, timestamp->led.mask);

        XFreePixmap(dpy, timestamp->month.pixmap);
        XFreePixmap(dpy, timestamp->month.mask);

        return (-1);
    }

    /* Load the weekday names image. */
    timestamp->weekday.attributes.valuemask = (XpmReturnPixels | XpmReturnExtensions | XpmSize);
    if (XpmCreatePixmapFromData(dpy, DefaultRootWindow(dpy), weekday_xpm, &timestamp->weekday.pixmap, &timestamp->weekday.mask, &timestamp->weekday.attributes) != XpmSuccess)
    {
        XFreePixmap(dpy, timestamp->asclock.pixmap);
        XFreePixmap(dpy, timestamp->asclock.mask);

        XFreePixmap(dpy, timestamp->asvisible.pixmap);
        XFreePixmap(dpy, timestamp->asvisible.mask);

        XFreePixmap(dpy, timestamp->led.pixmap);
        XFreePixmap(dpy, timestamp->led.mask);

        XFreePixmap(dpy, timestamp->month.pixmap);
        XFreePixmap(dpy, timestamp->month.mask);

        XFreePixmap(dpy, timestamp->date.pixmap);
        XFreePixmap(dpy, timestamp->date.mask);

        return (-1);
    }

    /* Load the year digits image. */
    timestamp->year.attributes.valuemask = (XpmReturnPixels | XpmReturnExtensions | XpmSize);
    if (XpmCreatePixmapFromData(dpy, DefaultRootWindow(dpy), year_xpm, &timestamp->year.pixmap, &timestamp->year.mask, &timestamp->year.attributes) != XpmSuccess)
    {
        XFreePixmap(dpy, timestamp->asclock.pixmap);
        XFreePixmap(dpy, timestamp->asclock.mask);

        XFreePixmap(dpy, timestamp->asvisible.pixmap);
        XFreePixmap(dpy, timestamp->asvisible.mask);

        XFreePixmap(dpy, timestamp->led.pixmap);
        XFreePixmap(dpy, timestamp->led.mask);

        XFreePixmap(dpy, timestamp->month.pixmap);
        XFreePixmap(dpy, timestamp->month.mask);

        XFreePixmap(dpy, timestamp->date.pixmap);
        XFreePixmap(dpy, timestamp->date.mask);

        XFreePixmap(dpy, timestamp->weekday.pixmap);
        XFreePixmap(dpy, timestamp->weekday.mask);

        return (-1);
    }

    return (0);
}




/***********************************************************************
Name:		destroyTimeStamp
Purpose:        Releases the XPM data and other resources used by
		TimeStamp.
Inputs:		timestamp - The TimeStamp.
Outputs:	None.
Returns:	Nothing.
Status:		Finished.
Errors: 	None known.
***********************************************************************/
static void destroyTimeStamp(TimeStamp *timestamp)
{
    Display     *dpy = timestamp->view->screen->display;

    DEBUG_PRINT("Inside destroyTimeStamp()");
    if (timestamp->asclock.pixmap != (Pixmap) None)
    {
        XFreePixmap(dpy, timestamp->asclock.pixmap);
        DEBUG_PRINT("Released asclock.pixmap");
    }

    if (timestamp->asclock.mask != (Pixmap) None)
    {
        XFreePixmap(dpy, timestamp->asclock.mask);
        DEBUG_PRINT("Released asclock.mask");
    }

    if (timestamp->led.pixmap != (Pixmap) None)
    {
        XFreePixmap(dpy, timestamp->led.pixmap);
        DEBUG_PRINT("Released led.pixmap");
    }

    if (timestamp->led.mask != (Pixmap) None)
    {
        XFreePixmap(dpy, timestamp->led.mask);
        DEBUG_PRINT("Released led.mask");
    }

    if (timestamp->month.pixmap != (Pixmap) None)
    {
        XFreePixmap(dpy, timestamp->month.pixmap);
        DEBUG_PRINT("Released month.pixmap");
    }

    if (timestamp->month.mask != (Pixmap) None)
    {
        XFreePixmap(dpy, timestamp->month.mask);
        DEBUG_PRINT("Released month.mask");
    }

    if (timestamp->date.pixmap != (Pixmap) None)
    {
        XFreePixmap(dpy, timestamp->date.pixmap);
        DEBUG_PRINT("Released date.pixmap");
    }

    if (timestamp->date.mask != (Pixmap) None)
    {
        XFreePixmap(dpy, timestamp->date.mask);
        DEBUG_PRINT("Released date.mask");
    }

    if (timestamp->weekday.pixmap != (Pixmap) None)
    {
        XFreePixmap(dpy, timestamp->weekday.pixmap);
        DEBUG_PRINT("Released weekday.pixmap");
    }

    if (timestamp->weekday.mask != (Pixmap) None)
    {
        XFreePixmap(dpy, timestamp->weekday.mask);
        DEBUG_PRINT("Released weekday.mask");
    }

    if (timestamp->asvisible.pixmap != (Pixmap) None)
    {
        XFreePixmap(dpy, timestamp->asvisible.pixmap);
        DEBUG_PRINT("Released asvisible.pixmap");
    }

    if (timestamp->asvisible.mask != (Pixmap) None)
    {
        XFreePixmap(dpy, timestamp->asvisible.mask);
        DEBUG_PRINT("Released asvisible.mask");
    }
    
    if (timestamp->year.pixmap != (Pixmap) None)
    {
        XFreePixmap(dpy, timestamp->year.pixmap);
        DEBUG_PRINT("Released year.pixmap");
    }

    if (timestamp->year.mask != (Pixmap) None)
    {
        XFreePixmap(dpy, timestamp->year.mask);
        DEBUG_PRINT("Released year.mask");
    }
    DEBUG_PRINT("Released XPMs");

    free((void *) timestamp);
    DEBUG_PRINT("Released TimeStamp");
}




/***********************************************************************
Name:		handleEvents
Purpose:        Handles events delivered to the TimeStamp widget.
Inputs:		event - The XEvent.
                data  - Pointer to the TimeStamp structure for this event.
Outputs:	None.
Returns:	Nothing.
Status:		Finished.
Errors: 	None known.
***********************************************************************/
static void handleEvents(XEvent *event, void *data)
{
    TimeStamp   *timestamp = (TimeStamp *) data;

    switch (event->type)
    {	
    case Expose:
        DEBUG_PRINT("Caught an Expose event");
        if (event->xexpose.count)
        {
            break;
        }
        
        if (timestamp->view->flags.realized)
        {
            /* We need to repaint. */
            paintTimeStamp(timestamp);
        }
        break;
        
    case DestroyNotify:
        DEBUG_PRINT("Caught a DestroyNotify event");
        destroyTimeStamp(timestamp);
        break;
    }
}




/***********************************************************************
Name:		paintTimeStamp
Purpose:        Handles repainting of the widget.
Inputs:		timestamp - The TimeStamp.
Outputs:	None.
Returns:	Nothing.
Status:		Finished.
Errors: 	None known.
***********************************************************************/
static void paintTimeStamp(TimeStamp *timestamp)
{
    Display     *dpy = timestamp->view->screen->display;
    W_Screen    *scr = timestamp->view->screen;
    int         offset = 0;

    if (!(timestamp->view->flags.realized))
    {
        /* Widget is not realized yet so just return. */
        return;
    }

    DEBUG_PRINT("Inside paintTimeStamp()");
    /* Does the image need re-creating? */
    if (timestamp->flags.damaged)
    {
        /* Yes - So re-create it now. */
        RenderASClock(timestamp);

        /* Now turn new image into a WMPixmap ready for us to paint. */
        if (!(timestamp->image = WMCreatePixmapFromXPixmaps(scr,
                timestamp->result->pixmap,
                timestamp->result->mask,
                TIMESTAMP_MIN_WIDTH,
                TIMESTAMP_MIN_HEIGHT,
                DefaultDepth(dpy, DefaultScreen(dpy)))))
        {
            /* Error whilst trying to convert. */
            wfatal("Unable to Create Clock Image");
        }
        DEBUG_PRINT("Created new image");
    }

    /* Paint the image. */
    if (timestamp->image)
    {
        W_PaintTextAndImage(timestamp->view,
                            False,
                            timestamp->color,
                            scr->normalFont,
                            WRFlat,
                            NULL,
                            WACenter,
                            timestamp->image,
                            WIPImageOnly,
                            timestamp->color,
                            offset);
        DEBUG_PRINT("Painted image");
    }
    
    /* Reset 'damaged' flag back to 0. */
    timestamp->flags.damaged = 0;
}




/***********************************************************************
Name:		setBackgroundColor
Purpose:        Sets the background colour of the widget.
Inputs:		widget - A pointer to the TimeStamp.
                color  - The new colour.
Outputs:	None.
Returns:	Nothing.
Status:		Finished.
Errors: 	None known.
***********************************************************************/
static void setBackgroundColor(WMWidget *widget, WMColor *color)
{
    TimeStamp   *timestamp = (TimeStamp *) widget;

    WMSetColorInGC(color, timestamp->gc);

    W_SetViewBackgroundColor(timestamp->view, color);

    if (timestamp->view->flags.realized)
    {
        paintTimeStamp(timestamp);
    }
}




/***********************************************************************
Name:		resizeTimeStamp
Purpose:        Resizes the widget.
Inputs:		widget - A pointer to the TimeStamp.
                width  - The new width.
                height - The new height.
Outputs:	None.
Returns:	Nothing.
Status:		Finished.
Errors: 	None known.
***********************************************************************/
static void willResizeTimeStamp(W_ViewDelegate *self, WMView *view,
				unsigned int *width, unsigned int *height)
{
    TimeStamp   *timestamp = (TimeStamp *) view->self;

    /* Check the width. */
/*     if (*width < TIMESTAMP_MIN_WIDTH) */
/*     { */
/*         *width = TIMESTAMP_MIN_WIDTH; */
/*     } */
    
    /* Check the height. */
/*     if (*height < TIMESTAMP_MIN_HEIGHT) */
/*     { */
/*         *height = TIMESTAMP_MIN_HEIGHT; */
/*     } */

    /* Resize the widget. */
/*     W_ResizeView(timestamp->view, *width, *height); */
    if (timestamp->view->flags.realized)
    {
        /* Redraw the widget. */
        paintTimeStamp(timestamp);
    }
}

/* static void resizeTimeStamp(WMWidget *widget, unsigned int width, unsigned int height) */
/* { */
/*     TimeStamp   *timestamp = (TimeStamp *) widget; */

    /* Check the width. */
/*     if (width < TIMESTAMP_MIN_WIDTH) */
/*     { */
/*         width = TIMESTAMP_MIN_WIDTH; */
/*     } */
    
    /* Check the height. */
/*     if (height < TIMESTAMP_MIN_HEIGHT) */
/*     { */
/*         height = TIMESTAMP_MIN_HEIGHT; */
/*     } */

    /* Resize the widget. */
/*     W_ResizeView(timestamp->view, width, height); */
/*     if (timestamp->view->flags.realized) */
/*     { */
        /* Redraw the widget. */
/*         paintTimeStamp(timestamp); */
/*     } */
/* } */




/***********************************************************************
	Name:		SetTimeStampBlank
	Purpose:        Sets the TimeStamp to a blank display.
	Inputs:		timestamp - The TimeStamp.
                        blank     - Toggle flag.
	Outputs:	None.
	Returns:	Nothing.
	Status:		Finished.
	Errors: 	None known.
***********************************************************************/
void SetTimeStampBlank(TimeStamp *timestamp, Bool blank)
{
    unsigned int Blank;

    CHECK_CLASS(timestamp, timeStampWidgetClass);

    Blank = (blank == True) ? 1 : 0;

    if (Blank == timestamp->flags.blank)
    {
        /* Status is already set, so return. */
        return;
    }

    timestamp->flags.blank = Blank;
    timestamp->flags.damaged = 1;

    if (timestamp->view->flags.realized)
    {
        paintTimeStamp(timestamp);
    }
}




/***********************************************************************
	Name:		SetTimeStampWithTimeT
	Purpose:        Sets the TimeStamp using a time_t value.
	Inputs:		timestamp - The TimeStamp.
                        time      - The time value.
	Outputs:	None.
	Returns:	Nothing.
	Status:		Finished.
	Errors: 	None known.
***********************************************************************/
void SetTimeStampWithTimeT(TimeStamp *timestamp, time_t time)
{
    CHECK_CLASS(timestamp, timeStampWidgetClass);

    timestamp->time = time;
    timestamp->flags.damaged = 1;

    if (timestamp->view->flags.realized)
    {
        paintTimeStamp(timestamp);
    }
}




/***********************************************************************
Name:		SetTimeStampWithTimeTM
Purpose:        Sets the TimeStamp using a 'struct tm' value.
Inputs:		timestamp - The TimeStamp.
                time      - The time value.
Outputs:	None.
Returns:	Nothing.
Status:		Finished.
Errors: 	None known.
***********************************************************************/
void SetTimeStampWithTimeTM(TimeStamp *timestamp, struct tm *time)
{
    CHECK_CLASS(timestamp, timeStampWidgetClass);

    timestamp->time = mktime(time);
    timestamp->flags.damaged = 1;

    if (timestamp->view->flags.realized)
    {
        paintTimeStamp(timestamp);
    }
}




/******************************************************************************
Name:		SetTwentyFour
Purpose:        Sets the TimeStamp into 24 or 12 hour mode.
Inputs:		timestamp  - The TimeStamp.
                twentyfour - Toggle flag.
Outputs:	None.
Returns:	Nothing.
Status:		Finished.
Errors: 	None known.
******************************************************************************/
void SetTwentyFour(TimeStamp *timestamp, Bool twentyfour)
{
    CHECK_CLASS(timestamp, timeStampWidgetClass);

    timestamp->flags.showAMPM = (twentyfour == True) ? 0 : 1;
    /* Re-calculate LED positions. */
    InitClockResources(timestamp);
    timestamp->flags.damaged = 1;

    if (timestamp->view->flags.realized)
    {
        paintTimeStamp(timestamp);
    }
}




/***********************************************************************
Name:		ASTwelve
Purpose:        Draws the TimeStamp LED in 12 hour mode.
Inputs:		timestamp - The TimeStamp.
                clk       - The time to set.
Outputs:	None.
Returns:	Nothing.
Status:		Finished.
Errors: 	None known.
***********************************************************************/
static void ASTwelve(TimeStamp *timestamp, struct tm *clk)
{
    Display *dpy = timestamp->view->screen->display;
    int     thishour;

    DEBUG_PRINT("Inside ASTwelve()");
    /* Convert hour to 12 hour format. */
    thishour = clk->tm_hour % 12;
    if (!thishour)
    {
        thishour = 12;
    }

    if (clk->tm_hour >= 12)
    {
        /* PM */
        XCopyArea(dpy,
                  timestamp->led.pixmap,        /* src            */
                  timestamp->asvisible.pixmap,  /* dest           */
                  timestamp->gc,                /* use GC         */
                  107, 5,                       /* src_x, src_y   */
                  11, 6,                        /* width, height  */
                  posx[5], posy[0] + 5);        /* dest_x, dest_y */
    }
    else
    {
        /* AM */
        XCopyArea(dpy,
                  timestamp->led.pixmap,
                  timestamp->asvisible.pixmap,
                  timestamp->gc,
                  94, 5,
                  12, 6,
                  posx[5], posy[0] + 5);
    }

    /* Do we need a leading '1' digit for the hour? */
    if (thishour > 9)
    {
        /* Yes. */
        XCopyArea(dpy,
                  timestamp->led.pixmap,
                  timestamp->asvisible.pixmap,
                  timestamp->gc,
                  13, 0,
                  5, 11,
                  posx[0], posy[0]);
    }

    /* Second digit of the hour. */
    XCopyArea(dpy,
              timestamp->led.pixmap,
              timestamp->asvisible.pixmap,
              timestamp->gc,
              9 * (thishour % 10), 0,
              9, 11,
              posx[1], posy[0]);

    /* Minute, drawn first, so am/pm won't be overwritten. */
    XCopyArea(dpy,
              timestamp->led.pixmap,
              timestamp->asvisible.pixmap,
              timestamp->gc,
              9 * (clk->tm_min / 10), 0,
              9, 11,
              posx[3], posy[0]);

    XCopyArea(dpy,
              timestamp->led.pixmap,
              timestamp->asvisible.pixmap,
              timestamp->gc,
              9 * (clk->tm_min % 10), 0,
              9, 11,
              posx[4], posy[0]);
}




/***********************************************************************
	Name:		ASTwentyFour
	Purpose:        Draws the TimeStamp LED in 24 hour mode.
	Inputs:		timestamp - The TimeStamp.
                        clk       - The time to set.
	Outputs:	None.
	Returns:	Nothing.
	Status:		Finished.
	Errors: 	None known.
***********************************************************************/
static void ASTwentyFour(TimeStamp *timestamp, struct tm *clk)
{
    Display *dpy = timestamp->view->screen->display;

    DEBUG_PRINT("Inside ASTwentyFour()");
    
    /* Draw the first digit for the hour. */
    XCopyArea(dpy,
              timestamp->led.pixmap,
              timestamp->asvisible.pixmap,
              timestamp->gc,
              9 * (clk->tm_hour / 10), 0,
              9, 11,
              posx[0], posy[0]);

    /* Draw the second digit for the hour. */
    XCopyArea(dpy,
              timestamp->led.pixmap,
              timestamp->asvisible.pixmap,
              timestamp->gc,
              9 * (clk->tm_hour % 10), 0,
              9, 11,
              posx[1], posy[0]);

    /* Draw the first digit for the minute. */
    XCopyArea(dpy,
              timestamp->led.pixmap,
              timestamp->asvisible.pixmap,
              timestamp->gc,
              9 * (clk->tm_min / 10), 0,
              9, 11,
              posx[3], posy[0]);

    /* Draw the second digit for the minute. */
    XCopyArea(dpy,
              timestamp->led.pixmap,
              timestamp->asvisible.pixmap,
              timestamp->gc,
              9 * (clk->tm_min % 10), 0,
              9, 11,
              posx[4], posy[0]);
}




/***********************************************************************
	Name:		RenderASClock
	Purpose:        Draws the TimeStamp widget.
	Inputs:		timestamp - The TimeStamp.
	Outputs:	None.
	Returns:	Nothing.
	Status:		Finished.
	Errors: 	None known.
***********************************************************************/
static void RenderASClock(TimeStamp *timestamp)
{
    Display     *dpy = timestamp->view->screen->display;
    struct tm   *clk;
    int         year;
    char        yearStr[5];
    int         digit, i;
    int         yearWidth, yearOffset;


    DEBUG_PRINT("Inside RenderASClock()");
    
    timestamp->result = &timestamp->asvisible;

    clk = localtime(&timestamp->time);

    /* Copy the calendar background image. */
    XCopyArea(dpy,
              timestamp->asclock.pixmap,
              timestamp->asvisible.pixmap,
              timestamp->gc,
              0, 0,
              TIMESTAMP_MIN_WIDTH, TIMESTAMP_MIN_HEIGHT,
              0, 0);

    /* If we are in 'blank' mode then that's all we have to do. */
    if (timestamp->flags.blank)
    {
        return;
    }

    /* Draw LED in 12 or 24 hour mode as appropriate. */
    if (timestamp->flags.showAMPM)
    {
        ASTwelve(timestamp, clk);
    }
    else
    {
        ASTwentyFour(timestamp, clk);
    }

    /* Month. */
    XCopyArea(dpy,
              timestamp->month.pixmap,
              timestamp->asvisible.pixmap,
              timestamp->gc,
              0, 6 * (clk->tm_mon),
              22, 6,
              posx[10], posy[3]);

    /* Date. */
    if (clk->tm_mday > 9)
    {
        /* We need two digits for the date. */
        XCopyArea(dpy,
                  timestamp->date.pixmap,
                  timestamp->asvisible.pixmap,
                  timestamp->gc,
                  9 * ((clk->tm_mday / 10 + 9) % 10), 0,
                  9, 13,
                  posx[7], posy[2]);

        XCopyArea(dpy,
                  timestamp->date.pixmap,
                  timestamp->asvisible.pixmap,
                  timestamp->gc,
                  9 * ((clk->tm_mday % 10 + 9) % 10), 0,
                  9, 13,
                  posx[9], posy[2]);      
    }
    else
    {
        /* Single digit date. */
        XCopyArea(dpy,
                  timestamp->date.pixmap,
                  timestamp->asvisible.pixmap,
                  timestamp->gc,
                  9 * (clk->tm_mday - 1), 0,
                  9, 13,
                  posx[8], posy[2]);
    }

    /* WeekDay. */
    XCopyArea(dpy,
              timestamp->weekday.pixmap,
              timestamp->asvisible.pixmap,
              timestamp->gc,
              0, 6 * ((clk->tm_wday + 6) % 7),
              21, 7,
              posx[6], posy[1]); 

    /* Copy LED colon. */
    XCopyArea(dpy,
              timestamp->led.pixmap,
              timestamp->asvisible.pixmap,
              timestamp->gc,
              90, 0,
              3, 11,
              posx[2], posy[0]);
    
    /* Year. */
    year = 1900 + clk->tm_year;
    if (year < 0 || year > 9999)
    {
      wwarning("Year field out of range for TimeStamp widget - %d.\n", year);
    }
    else
    {
        /* Create a string of the year. */
        sprintf(yearStr, "%04d", year);
        
        /* Calculate the width of the year label in pixels. */
        yearWidth = 0;
        for (i = 0; i < 4; i++)
        {
            digit = yearStr[i] - '0';
            yearWidth += year_width[digit];
        }
        /* Add on 3 pixels for the gaps in between the digits. */
        yearWidth += 3;
        
        /* Draw the four digits. */
        yearOffset = 0;
        for (i = 0; i < 4; i++)
        {
          digit = yearStr[i] - '0';
          XCopyArea(dpy,
            timestamp->year.pixmap,                       /* src            */
            timestamp->asvisible.pixmap,                  /* dest           */
            timestamp->gc,                                /* use GC         */
            year_pos[digit], 0,                           /* src_x, src_y   */
            year_width[digit], 5,                         /* width, height  */
            ((TIMESTAMP_MIN_WIDTH-yearWidth)/2)+yearOffset, 63); /* dest_x, dest_y */
            yearOffset += (year_width[digit]+1);
        }
    }
}


/* End Of File - timestampWidget.c ************************************/
