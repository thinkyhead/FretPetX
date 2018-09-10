/*
 *  AppStoreValidation.c
 *  FretPetX
 *
 *  Created by Scott Lahteine on 3/10/11.
 *  Copyright 2012 Thinkyhead Software. All rights reserved.
 *
 *  Based primarily on sample code by Alan Quatermain
 *  https://github.com/AlanQuatermain/mac-app-store-validation-sample
 *
 */

#include "AppStoreValidation.h"
#include "TString.h"
#include "asn1/Payload.h"

#import <openssl/pkcs7.h>
#import <openssl/objects.h>
#import <openssl/sha.h>
#import <openssl/x509.h>
#import <openssl/err.h>
#import <Security/SecBase.h>
#import <Security/SecKeychain.h>
#import <Security/SecKeychainItem.h>
#import <Security/SecKeychainSearch.h>
#import <Security/SecCertificate.h>
#import <Security/SecStaticCode.h>
#import <IOKit/IOKitLib.h>
#import <CommonCrypto/CommonDigest.h>

#include <sys/stat.h>

// Return a CFData object with the computer's "GUID"
CFDataRef copy_mac_address() {
	kern_return_t			kernResult;
	mach_port_t				master_port;
	CFMutableDictionaryRef	matchingDict;
	io_iterator_t			iterator;
	io_object_t				service;
	CFDataRef				macAddress = nil;

	kernResult = IOMasterPort(MACH_PORT_NULL, &master_port);
	if (kernResult != KERN_SUCCESS) {
		printf("IOMasterPort returned %d\n", kernResult);
		return nil;
	}

    matchingDict = IOBSDNameMatching(master_port, 0, "en0");
	if (!matchingDict) {
		printf("IOBSDNameMatching returned empty dictionary\n");
		return nil;
	}

	kernResult = IOServiceGetMatchingServices(master_port, matchingDict, &iterator);
	if (kernResult != KERN_SUCCESS) {
		printf("IOServiceGetMatchingServices returned %d\n", kernResult);
		return nil;
	}

	while ((service = IOIteratorNext(iterator)) != 0) {
		io_object_t parentService;
 
		kernResult = IORegistryEntryGetParentEntry(service, kIOServicePlane, &parentService);
		if (kernResult == KERN_SUCCESS) {
			if (macAddress) CFRELEASE(macAddress);
 
			macAddress = (CFDataRef) IORegistryEntryCreateCFProperty(parentService, CFSTR("IOMACAddress"), kCFAllocatorDefault, 0);
			IOObjectRelease(parentService);
		} else {
			printf("IORegistryEntryGetParentEntry returned %d\n", kernResult);
		}

		IOObjectRelease(iterator);
		IOObjectRelease(service);
	}

	return macAddress;
}

static inline SecCertificateRef AppleRootCA(void) {
    SecKeychainRef roots = NULL;
    SecKeychainSearchRef search = NULL;
    SecCertificateRef cert = NULL;
    
    // there's a GC bug with this guy it seems
    OSStatus err = SecKeychainOpen("/System/Library/Keychains/SystemRootCertificates.keychain", &roots);

	if (err == noErr) {
		SecKeychainAttribute labelAttr;
		labelAttr.tag = kSecLabelItemAttr;
		labelAttr.length = 13;
		labelAttr.data = (void *)"Apple Root CA";
		SecKeychainAttributeList attrs;
		attrs.count = 1;
		attrs.attr = &labelAttr;

		err = SecKeychainSearchCreateFromAttributes(roots, kSecCertificateItemClass, &attrs, &search);
		if (err == noErr) {
			SecKeychainItemRef item = NULL;
			err = SecKeychainSearchCopyNext(search, &item);
			CFRELEASE(search);
			if (err == noErr)
				cert = (SecCertificateRef)item;
		}
		CFRELEASE(roots);
	}

	if (err) {
		TString errStr = SecCopyErrorMessageString(err, NULL);
		char *cstring = errStr.GetCString();
		printf("Error: %d (%s)", (int)err, cstring);
		delete [] cstring;
	}

	return cert;
}

// 1. Check that the receipt exists
//    If successful init PKCS7, X509, and OpenSSL data.
//    (Sets the Receipt Path for the next step!)
int firstCheck(int argc, startup_call_t *theCall, CFTypeRef *pathPtr) {
	struct stat statBuf;

	// Look for the receipt. If no receipt is present, verification fails.
#ifdef CONFIG_Debug
	*pathPtr = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, CFSTR("./Deployment/Package/_MASReceipt/receipt"), kCFURLPOSIXPathStyle, false);
#else
	CFURLRef bundleURL = CFBundleCopyBundleURL(CFBundleGetMainBundle());
	*pathPtr = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, bundleURL, CFSTR("Contents/_MASReceipt/receipt"), false);
	CFRELEASE(bundleURL);
#endif

	char path[PATH_MAX];
	if (!CFURLGetFileSystemRepresentation((CFURLRef)*pathPtr, FALSE, (UInt8 *)path, (CFIndex)PATH_MAX)) {
		EXIT_FAIL_AUTH("Can't get a file system representation for the receipt");
	}
	if (stat(path, &statBuf) != 0) {
		EXIT_FAIL_AUTH("Can't stat the receipt");
	}

	ERR_load_PKCS7_strings();
	ERR_load_X509_strings();
	OpenSSL_add_all_digests();

	return argc;
}

// 2. Get the Apple Root CA from the keychain or apple.com
//    If successful dispose PKCS7, X509, and OpenSSL data.
//    (Sets the Receipt Data Pointer for the next step!)
int secondCheck(int argc, startup_call_t *theCall, CFTypeRef *receiptPath) {
	// the pkcs7 container (the receipt) and the output of the verification
	PKCS7 *p7 = NULL;

	// The Apple Root CA in its OpenSSL representation.
	X509 *Apple = NULL;

	// The root certificate for chain-of-trust verification
	X509_STORE *store = X509_STORE_new();

	char path[PATH_MAX];
	if (!CFURLGetFileSystemRepresentation((CFURLRef)*receiptPath, FALSE, (UInt8 *)path, (CFIndex)PATH_MAX)) {
		EXIT_FAIL_AUTH("Can't make the receipt path");
	}

	// This is a CFURL that we own, so dispose it
	CFRELEASE(*receiptPath);

	// initialize both BIO variables using BIO_new_mem_buf() with a buffer and its size...
	//b_p7 = BIO_new_mem_buf((void *)[receiptData bytes], [receiptData length]);
	FILE *fp = fopen(path, "rb");
	if (NULL == fp) {
		EXIT_FAIL_AUTH("Can't open the receipt");
	}

	// initialize b_out as an out
	BIO *b_out = BIO_new(BIO_s_mem());

	// capture the content of the receipt file and populate the p7 variable with the PKCS #7 container
	p7 = d2i_PKCS7_fp(fp, NULL);
	fclose(fp);

	// get the Apple root CA from http://www.apple.com/certificateauthority and load it into b_X509
	//NSData * root = [NSData dataWithContentsOfURL: [NSURL URLWithString: @"http://www.apple.com/certificateauthority/AppleComputerRootCertificate.cer"]];
	SecCertificateRef cert = AppleRootCA();
	if (cert == NULL) {
		EXIT_FAIL_AUTH("Failed to load Apple Root CA");
	}

	CFDataRef data = SecCertificateCopyData(cert);
	CFRELEASE(cert);

	//b_x509 = BIO_new_mem_buf((void *)CFDataGetBytePtr(data), (int)CFDataGetLength(data));
	const unsigned char * pData = CFDataGetBytePtr(data);
	Apple = d2i_X509(NULL, &pData, (long)CFDataGetLength(data));
	X509_STORE_add_cert(store, Apple);

	// verify the signature. If the verification is correct, b_out will contain the PKCS #7 payload and rc will be 1.
	int rc = PKCS7_verify(p7, NULL, store, NULL, b_out, 0);

	// could also verify the fingerprints of the issue certificates in the receipt

	unsigned char *pPayload = NULL;
	size_t len = BIO_get_mem_data(b_out, &pPayload);

	// Data for the next test
	*receiptPath = CFDataCreate(kCFAllocatorDefault, pPayload, len);

	// clean up
	//BIO_free(b_p7);
	//BIO_free(b_x509);
	BIO_free(b_out);
	PKCS7_free(p7);
	X509_free(Apple);
	X509_STORE_free(store);
	CFRELEASE(data);

	if (rc != 1) {
		EXIT_FAIL_AUTH("PKCS7 Verify failed");
	}

	return argc;
}

// 3. Load and check the Receipt payload data against:
//    - Bundle ID
//    - Bundle Version
//    - Generated Hash (GUID + Opaque + Bundle ID)
int thirdCheck(int argc, startup_call_t *theCall, CFTypeRef *dataPtr) {

	CFDataRef payloadData = (CFDataRef)(*dataPtr);
	const UInt8 *data = CFDataGetBytePtr(payloadData);
	size_t len = CFDataGetLength(payloadData);

	Payload_t * payload = NULL;
	asn_dec_rval_t rval;

	// parse the buffer using the asn1c-generated decoder.
	do {
		rval = asn_DEF_Payload.ber_decoder(NULL, &asn_DEF_Payload, (void **)&payload, data, len, 0);
	} while (rval.code == RC_WMORE);

	if (rval.code == RC_FAIL) {
		EXIT_FAIL_AUTH("Payload decoder failed");
	}

	OCTET_STRING_t *bundle_id = NULL;
	OCTET_STRING_t *bundle_version = NULL;
	OCTET_STRING_t *opaque = NULL;
	OCTET_STRING_t *hash = NULL;

	// iterate over the attributes, saving the values required for the hash
	long i;
	for (i = 0; i < payload->list.count; i++) {
		ReceiptAttribute_t *entry = payload->list.array[i];
		switch (entry->type) {
			case 2:
				bundle_id = &entry->value;
				break;
			case 3:
				bundle_version = &entry->value;
				break;
			case 4:
				opaque = &entry->value;
				break;
			case 5:
				hash = &entry->value;
				break;
			default:
				break;
		}
	}

	if (bundle_id == NULL || bundle_version == NULL || opaque == NULL || hash == NULL) {
		free(payload);
		EXIT_FAIL_AUTH("Incomplete receipt data");
	}

	CFStringRef bidStr = CFStringCreateWithBytes(kCFAllocatorDefault, bundle_id->buf + 2, bundle_id->size - 2, kCFStringEncodingUTF8, false);
	if (kCFCompareEqualTo != CFStringCompare(bidStr, CFSTR(hardcoded_bidStr), (CFOptionFlags)0)) {
		free(payload);
		EXIT_FAIL_AUTH("Receipt App ID is incorrect");
	}

	CFStringRef dvStr = CFStringCreateWithBytes(kCFAllocatorDefault, bundle_version->buf + 2, bundle_version->size - 2, kCFStringEncodingUTF8, false);
	if (kCFCompareEqualTo != CFStringCompare(dvStr, CFSTR(hardcoded_dvStr), (CFOptionFlags)0)) {
		free(payload);
		EXIT_FAIL_AUTH("Receipt App Version is incorrect");
	}

	CFDataRef macAddress = copy_mac_address();
	if (macAddress == NULL) {
		free(payload);
		EXIT_FAIL_AUTH("Couldn't get the GUID");
	}

	UInt8 *guid = (UInt8 *)CFDataGetBytePtr(macAddress);
	size_t guid_sz = CFDataGetLength(macAddress);

	// initialize an EVP context for OpenSSL
	EVP_MD_CTX evp_ctx;
	EVP_MD_CTX_init(&evp_ctx);

	UInt8 digest[20];

	// set up EVP context to compute an SHA-1 digest
	EVP_DigestInit_ex(&evp_ctx, EVP_sha1(), NULL);

	// concatenate the pieces to be hashed. They must be concatenated in this order.
	EVP_DigestUpdate(&evp_ctx, guid, guid_sz);
	EVP_DigestUpdate(&evp_ctx, opaque->buf, opaque->size);
	EVP_DigestUpdate(&evp_ctx, bundle_id->buf, bundle_id->size);

	// compute the hash, saving the result into the digest variable
	EVP_DigestFinal_ex(&evp_ctx, digest, NULL);

	// clean up memory
	EVP_MD_CTX_cleanup(&evp_ctx);

	// compare the hash
	int match = sizeof(digest) - hash->size;
	match |= memcmp(digest, hash->buf, MIN(sizeof(digest), (unsigned)hash->size));

	free(payload);
	if (match != 0) {
		EXIT_FAIL_AUTH("The receipt hash doesn't match");
	}

	return argc;
}

// 5. Check the static code for validity
int fourthCheck(int argc, startup_call_t *theCall, CFTypeRef *obj_arg) {
	SecStaticCodeRef staticCode = NULL;
	if ( noErr != SecStaticCodeCreateWithPath(CFBundleCopyBundleURL(CFBundleGetMainBundle()), kSecCSDefaultFlags, &staticCode) ) {
		EXIT_FAIL_AUTH("Couldn't init Secure Static Code test");
	}

	// the last parameter here could/should be a compiled requirement stating that the code
	// must be signed by YOUR certificate and no-one else's.
	if ( noErr != SecStaticCodeCheckValidity(staticCode, kSecCSCheckAllArchitectures, NULL) ) {
		EXIT_FAIL_AUTH("SecStaticCodeCheckValidity test failed");
	}

	return argc;
}

