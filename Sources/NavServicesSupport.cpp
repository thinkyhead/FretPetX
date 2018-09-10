/*!
	@file NavServicesSupport.cpp

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#include "NavServicesSupport.h"
//#include <LaunchServices.h>
#include "FPApplication.h"
#include "FPMacros.h"

NavEventUPP				openDialogUPP = NULL;
NavObjectFilterProcPtr	openFilterUPP = NULL;
pascal void				OpenDialogProc(const NavEventCallbackMessage callbackSelector, NavCBRecPtr callbackParms, NavCallBackUserData callbackUD);
Boolean					OpenFilterProc(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes filterMode);


#pragma mark -
Handle NewOpenHandle(OSType applicationSignature, SInt16 numTypes, OSType typeList[]) {
	Handle hdl = NULL;

	if ( numTypes > 0 ) {

		hdl = NewHandle(sizeof(NavTypeList) + numTypes * sizeof(OSType));

		if ( hdl != NULL ) {
			NavTypeListHandle open		= (NavTypeListHandle)hdl;

			(*open)->componentSignature = applicationSignature;
			(*open)->osTypeCount		= numTypes;
			BlockMoveData(typeList, (*open)->osType, numTypes * sizeof(OSType));
		}
	}

	return hdl;
}


OSStatus SendOpenAE(AEDescList list) {
	OSStatus		err;
	AEAddressDesc	theAddress;
	AppleEvent		dummyReply;
	AppleEvent		theEvent;

	theAddress.descriptorType	= typeNull;
	theAddress.dataHandle		= NULL;

    dummyReply.descriptorType	= typeNull;
    dummyReply.dataHandle		= NULL;

    theEvent.descriptorType		= typeNull;
    theEvent.dataHandle			= NULL;

	do {
		ProcessSerialNumber psn;

		err = GetCurrentProcess(&psn);
		if ( err != noErr) break;

		err =AECreateDesc(typeProcessSerialNumber, &psn, sizeof(ProcessSerialNumber), &theAddress);
		if ( err != noErr) break;

		err = AECreateAppleEvent(kCoreEventClass, kAEOpenDocuments, &theAddress, kAutoGenerateReturnID, kAnyTransactionID, &theEvent);
		if ( err != noErr) break;

		err = AEPutParamDesc(&theEvent, keyDirectObject, &list);
		if ( err != noErr) break;

		err = AESend(&theEvent, &dummyReply, kAENoReply, kAENormalPriority, kAEDefaultTimeout, NULL, NULL);
		if ( err != noErr) break;


	} while (false);

    if ( theAddress.dataHandle != NULL ) {
        AEDisposeDesc( &theAddress );
    }

    if ( dummyReply.dataHandle != NULL ) {
        AEDisposeDesc( &dummyReply );
    }

    if ( theEvent.dataHandle != NULL ) {
        AEDisposeDesc( &theEvent );
    }

	return err;
}


void TerminateNavDialog(NavDialogRef inDialog) {
	NavCustomControl(inDialog, kNavCtlTerminate, NULL);
}


#pragma mark -
NavEventUPP GetOpenDialogUPP() {
	if ( openDialogUPP == NULL )
		openDialogUPP = NewNavEventUPP( OpenDialogProc );

	return openDialogUPP;
}


pascal void OpenDialogProc(const NavEventCallbackMessage callbackSelector, NavCBRecPtr callbackParms, NavCallBackUserData callbackUD) {
//	fprintf(stderr, "OpenDialogProc [%d]\n", callbackSelector);
	switch ( callbackSelector ) {
		case kNavCBEvent: {
//			fprintf(stderr, "OLD kNavCBEvent\n");
/*
			//
			// Update and activate don't seem to apply to Mac OS X
			//
			fprintf(stderr, "kNavCBEvent [%d]\n", callbackParms->eventData.eventDataParms.event->what);
			switch (callbackParms->eventData.eventDataParms.event->what) {
				case updateEvt:
				case activateEvt:
//					fretpet->HandleEvent(callbackParms->eventData.eventDataParms.event);
				break;
			}
*/
		}
		break;

		case kNavCBUserAction: {
//			fprintf(stderr, "OLD kNavCBUserAction\n");
			if ( callbackParms->userAction == kNavUserActionOpen ) {
				// This is an open files action, send an AppleEvent
				NavReplyRecord	reply;
				OSStatus		status;

				status = NavDialogGetReply( callbackParms->context, &reply );
				if ( status == noErr ) {
					SendOpenAE( reply.selection );
					NavDisposeReply( &reply );
				}
			}
		}
		break;

		case kNavCBTerminate: {
//			fprintf(stderr, "OLD kNavCBTerminate\n");
			if ( callbackParms->context == fretpet->openDialog )
				fretpet->DisposeOpenFileDialog();
		}
		break;
	}
}


NavObjectFilterUPP GetOpenFilterUPP() {
	if ( openFilterUPP == NULL )
		openFilterUPP = NewNavObjectFilterUPP( OpenFilterProc );

	return openFilterUPP;
}


Boolean OpenFilterProc(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes filterMode) {
	Boolean				display = true;
	NavFileOrFolderInfo	*theInfo = (NavFileOrFolderInfo*)info;
	FSRef				ref;
	LSItemInfoRecord	outInfo;

	if (theInfo->isFolder == true)
		return true;

	AECoerceDesc(theItem, typeFSRef, theItem);

	if (AEGetDescData(theItem, &ref, sizeof(FSRef)) == noErr) {
		outInfo.extension = NULL;

		if ( LSCopyItemInfoForRef(&ref, kLSRequestExtension|kLSRequestTypeCreator, &outInfo) == noErr ) {
			CFStringRef	itemUTI = NULL;
			if ( outInfo.extension != NULL ) {
				itemUTI = UTTypeCreatePreferredIdentifierForTag(
					kUTTagClassFilenameExtension,
					outInfo.extension,
					NULL);
				CFRELEASE(outInfo.extension);
			}
			else {
				CFStringRef typeString = UTCreateStringForOSType(outInfo.filetype);
				itemUTI = UTTypeCreatePreferredIdentifierForTag(
					kUTTagClassOSType,
					typeString,
					NULL);
				CFRELEASE(typeString);
			}

			if (itemUTI != NULL) {
				display = UTTypeConformsTo(itemUTI, CFSTR("com.thinkyhead.fretpet.bank"));
				CFRELEASE(itemUTI);
			}
		}
	}

	return display;
}

