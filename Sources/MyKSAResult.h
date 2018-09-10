//
//  MyKSAResult.h
//  WorkFlowPro
//
//  Created by Sergiu Chirila on 1/13/11.
//  Copyright 2011 Kagi. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "KSAObject.h"
#import "KSAResult.h"
#import "WorkFlowProController.h"

@interface MyKSAResult : NSObject <KSAResult> {
	WorkFlowProController *workFlowProController;
}

@property (retain) WorkFlowProController *workFlowProController;

-(void)setKSAResultAsDictionary:(NSDictionary *)result;
-(void)setKSAError:(NSDictionary *)error;
-(void)setKSADeclined:(NSDictionary *)reason;

-(void)displayMessage:(NSString *)message;

@end
