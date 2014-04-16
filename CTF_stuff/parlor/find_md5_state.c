/*
 * find_md5_state.cc
 */ 

#include <stdint.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>

#define htonl(n) (((((unsigned long)(n) & 0xFF)) << 24) | \
                  ((((unsigned long)(n) & 0xFF00)) << 8) | \
				  ((((unsigned long)(n) & 0xFF0000)) >> 8) | 									((((unsigned long)(n) & 0xFF000000)) >> 24))


typedef unsigned int DWORD;

/*
 * find_md5_state
 *
 *	Bruteforce the missing bits of an MD5 state[partial_state] to 
 *  find a starting state  which when updated with [appendum], 
 *  produces [target_partial]	
 *
 * @parameter	state	A DWORD array of the context of the starting
 								partial MD5 state of the form [A, B, C, D]
 * @parameter	target_state	The partial target md5 when appending appendum 
 * @parameter	appendum		The string to append to the full starting state
 *								In order to get to the full target md5 state.
 * @parameter	result			The full starting state we found is copied here.
 *
 * @returns		The full MD5 starting state that satisfies the conditions
 */
void find_md5_state(DWORD *state, 
					unsigned char *target_state, 
					char *appendum,
					DWORD *result)
{

	MD5_CTX ctx;
	DWORD A, B, C, D;	//The four state DWORDs of an MD5 state.
	unsigned char buffer[32] = {0};
		
	A = state[0];
	B = state[1];
	C = state[2];
	D = state[3];

	//
	/// Bruteforce the missing bits to find a starting state which when
	/// updated with [appendum], produces [target_partial]
	///
	unsigned int i = 0;
	for (i = 0x0; i < 0x10000000; i++) {
		MD5_Init(&ctx);
		MD5_Update(&ctx, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", 64);

		ctx.A = htonl((i << 4) + A);
		ctx.B = htonl(B);
		ctx.C = htonl(C);
		ctx.D = htonl(D); 

		MD5_Update(&ctx, appendum, strlen(appendum));
		MD5_Final(buffer,  &ctx);
		
		if (memcmp((const char *)buffer + 4, target_state + 1, 12) == 0) {
			printf("\nFound it! :) \n");
			break;
		
		}
		else if ((i % 10000000) == 0) {	//every 10 million iterations..
			printf("\nProgress: %02f%%", ((i * 100.0)/0x10000000));
		}
	} 


	result[0] = htonl((i << 4) + A);
	result[1] = htonl(B);
	result[2] = htonl(C);
	result[3] = htonl(D); 

}


/*
 * update_md5_state
 *
 * Takes an initial md5 state and updates it with appendum
 */
void update_md5_state(DWORD *state, char *appendum, char *result)
{
	MD5_CTX ctx;
	unsigned char buffer[32] = {0};
		
	MD5_Init(&ctx);
	MD5_Update(&ctx, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA", 64);

	ctx.A = htonl(state[0]);
	ctx.B = htonl(state[1]);
	ctx.C = htonl(state[2]);
	ctx.D = htonl(state[3]); 

	MD5_Update(&ctx, appendum, strlen(appendum));
	MD5_Final(buffer,  &ctx);

	memcpy(result, buffer, sizeof buffer);
	for (int i = 0; i < 16; i++) {
		sprintf(result + i*2, "%02x", buffer[i]);
	}
}



