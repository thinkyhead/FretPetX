/*
 *  AppStoreValidation.h
 *  FretPetX
 *
 *  Created by Scott Lahteine on 3/10/11.
 *  Copyright 2012 Thinkyhead Software. All rights reserved.
 *
 */

#define hardcoded_bidStr "com.thinkyhead.fretpet"
#define hardcoded_dvStr FRETPET_VERSION

#define kExitAuthFail 173
#define EXIT_FAIL_AUTH(why) { *theCall = (startup_call_t)&exit; return kExitAuthFail; }

typedef int (*startup_call_t)(int);
typedef int (*validate_call_t)(int, startup_call_t *call, CFTypeRef *ptr);

int firstCheck(int argc, startup_call_t *theCall, CFTypeRef *pathPtr);
int secondCheck(int argc, startup_call_t *theCall, CFTypeRef *receiptPath);
int thirdCheck(int argc, startup_call_t *theCall, CFTypeRef *dataPtr);
int fourthCheck(int argc, startup_call_t *theCall, CFTypeRef *obj_arg);

