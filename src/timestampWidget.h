#ifndef	__TIMESTAMPWIDGET_H
#define __TIMESTAMPWIDGET_H
/******************************************************************************

	Project:	WMFileViewer
        
	Filename:	timestampWidget.h
        
	Author:		Charles Gamble
                        Original code by Michael J. Mitchell <mitch@gw2.redback.com.au>
        
	Purpose:	Header file for a NeXT style timestamp widget.

	$Id: timestampWidget.h,v 1.2 1999/01/13 22:47:09 gambcl Exp $

******************************************************************************/




/* Header Files **************************************************************/

#include <WINGs/WINGsP.h>
#include <WINGs/WINGs.h>
#include <time.h>




/* Definitions ***************************************************************/

#define	TIMESTAMP_MIN_WIDTH     64
#define	TIMESTAMP_MIN_HEIGHT    72




/* Type Definitions **********************************************************/

typedef struct W_TimeStamp TimeStamp;




/* Function Prototypes - timestampWidget.c ***********************************/

extern  W_Class     InitTimeStamp(WMScreen*);
extern  TimeStamp   *CreateTimeStamp(WMWidget*);
extern  void        SetTimeStampBlank(TimeStamp*, Bool);
extern  void        SetTimeStampWithTimeT(TimeStamp*, time_t);
extern  void        SetTimeStampWithTimeTM(TimeStamp*, struct tm*);
extern  void        SetTwentyFour(TimeStamp*, Bool);




/* End Of File - timestampWidget.h *******************************************/
#endif
