//
//  MyKSAResult.m
//  WorkFlowPro
//
//  Created by Sergiu Chirila on 1/13/11.
//  Copyright 2011 Kagi. All rights reserved.
//

#import "MyKSAResult.h"

@implementation MyKSAResult

@synthesize workFlowProController;

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
