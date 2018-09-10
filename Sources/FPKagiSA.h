/*!
 *  @file FPKagiSA.h
 *
 *	@section COPYRIGHT
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 * *
 */

#import <Cocoa/Cocoa.h>
#import "KSAObject.h"
#import "KSAResult.h"
#import "FPApplication.h"

@interface FPKSAResult : NSObject <KSAResult> {
}

-(void)setKSAResultAsDictionary:(NSDictionary *)result;
-(void)setKSAError:(NSDictionary *)error;
-(void)setKSADeclined:(NSDictionary *)reason;

-(void)displayMessage:(NSString *)message;

@end
