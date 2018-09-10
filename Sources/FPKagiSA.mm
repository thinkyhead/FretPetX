/*
 *  FPKagiSA.mm
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 *
 *	FretPet				4599-1508-8595
 *	FretPetSingle		6733-1719-9740
 *	FretPetXUser		4542-2545-6515
 *
 *	Seed Combo :nn
 *	Seed Length: 20
 *	Minimum Email Length: 4
 *	Minimum Name Length: 5
 *	Minimum HotSync ID Length: 10
 *	Length of Constant: 2
 *	Constant value: V1
 *	Sequence Length: 1
 *	Alternate Text: Contact register@fretpet.com to obtain your registration code.
 *	Scramble Order: U0,U8,U13,U16,U18,D3,C0,D0,U12,U11,U15,U4,U1,S0,D2,U3,U17,U7,C1,U10,U5,U19,D1,U9,U14,U6,U2,
 *	ASCII Digit(For text to Number Conversion):2
 *	Math Operation(Example:36A(36->Operand,A-Addition)): 6A,R,12S,22S,8A,R,8A,8A,15S,15S,R,5S,R,R,R,1A,1A,1A,1S,1S,1S,7A,33S,6A,R,6A,R,
 *	New Base: 30
 *	Base Character Set: WHCG6LTEFYAX8RMQ34U2KDNPJ15790
 *	Registration code format(^ is place holder for check digit) : FPX##-#####-#####-#####-##^##-#####-#####-#####-#[-#]
 *
 */

#import "FPKagiSA.h"

@implementation FPKSAResult

-(void)setKSAResultAsDictionary:(NSDictionary *)result;
{
	NSLog(@"!!! INSIDE supplier app: setKSAResultAsDictionary");
	NSLog(@" !!!! WORK FLOW PRO: got kagiTransactionID: %@",[result valueForKey:KeyKagiTransactionId]);
	NSLog(@" !!!! WORK FLOW PRO: got acgUserEmail: %@",[result valueForKey:KeyAcgUserEmail]);
	NSLog(@" !!!! WORK FLOW PRO: LICENSES");
	
	NSArray *myLicenses = [result objectForKey:KeyLicenses];
	if (myLicenses != nil) {
		NSEnumerator *enumerator = [myLicenses objectEnumerator];
		NSDictionary *license;
		while (license = [enumerator nextObject]) {
			if (license != nil) {
				NSLog(@" !!!! WORK FLOW PRO: got productID: %@",[license valueForKey:KeyProductId]);
				NSLog(@" !!!! WORK FLOW PRO: got userName: %@",[license valueForKey:KeyAcgUserName]);
				NSLog(@" !!!! WORK FLOW PRO: got regcode: %@",[license valueForKey:KeyAcgRegCode]);
			}
			NSLog(@" ------------------ SEPARATOR -----------------");
		}
	}
	
	[[self workFlowProController] set_kagiTransactionID:[result valueForKey:KeyKagiTransactionId]];
	[[self workFlowProController] set_acgUserEmail:[result valueForKey:KeyAcgUserEmail]];
	[[self workFlowProController] set_licenses:[result valueForKey:KeyLicenses]];
	
	[[self workFlowProController] updateLicense];
	
	NSMutableString* message = [NSMutableString stringWithFormat: @"WorkFlowPro was successfully registered!"];
	[self displayMessage:message];
}


-(void)setKSAError:(NSDictionary *)error;
{
	NSLog(@"!!! INSIDE supplier app: setKSAError");
	NSLog(@" !!!! got errorCode: %@",[error valueForKey:KeyErrorCode]);
	NSLog(@" !!!! got errorMessage: %@",[error valueForKey:KeyErrorMessage]);
	
	NSMutableString* message = [NSMutableString stringWithFormat: @"Got errorCode=%@ and errorMessage=%@",
								[error valueForKey:KeyErrorCode],
								[error valueForKey:KeyErrorMessage]];	
	[self displayMessage:message];
}

-(void)setKSADeclined:(NSDictionary *)reason;
{
	NSLog(@"!!! INSIDE supplier app: setKSADeclined");
	NSLog(@" !!!! got reasonCode: %@",[reason valueForKey:KeyReasonCode]);
	NSLog(@" !!!! got reasonMessage: %@",[reason valueForKey:KeyReasonMessage]);
	
	NSMutableString* message = [NSMutableString stringWithFormat: @"Got reasonCode=%@ and reasonMessage=%@",
								[reason valueForKey:KeyReasonCode],
								[reason valueForKey:KeyReasonMessage]];
	[self displayMessage:message];
}

- (void) displayMessage:(NSString *)message
{
	NSAlert *alert = [[NSAlert alloc] init];
	[alert setMessageText:message];
	[alert runModal];
	[alert release];
}	

@end
