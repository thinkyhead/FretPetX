/*!
	@file NavServicesSupport.h

	Helper for the Navigation Services dialog

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#ifndef	NAVSERVICESSUPPORT_H
#define	NAVSERVICESSUPPORT_H

NavEventUPP			GetOpenDialogUPP();
NavObjectFilterUPP	GetOpenFilterUPP();
void				TerminateNavDialog(NavDialogRef inDialog);
Handle				NewOpenHandle(OSType applicationSignature, SInt16 numTypes, OSType typeList[]);
OSStatus			SendOpenAE(AEDescList list);

#endif
