/*-
 * Copyright (c) 2020, 2021, Mahya Soleimani Jadidi 
 * msoleimanija@mun.ca
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *       * Redistributions of source code must retain the above copyright
 *         notice, this list of conditions and the following disclaimer.
 *       * Redistributions in binary form must reproduce the above copyright
 *         notice, this list of conditions and the following disclaimer in the
 *         documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h> 
#include <err.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <unistd.h> 
#include <errno.h>
#include <string.h> 
#include <sys/types.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdlib.h>
#include <sys/nv.h>
#include "gettime.h"

#define SERVER_SOCK_ENV "SERVER_SOCK"
#define BILLION 1000000000L


int main(int argc, char** argv) 
{
#if defined(__x86_64__)
	unsigned long long start,stop;
#else
	struct timespec start, stop;
#endif
	long long int diff = 0;
	int i = 0, true_val = 1;
	struct sockaddr_in address; 
	char* iter_str = argv[1];
	int iteration = atoi(iter_str);
	
	address.sin_family = AF_INET; 
	address.sin_port = htons(8080); 

	for (i = 1; i <= iteration; i++) {
		int sock = socket(AF_INET, SOCK_STREAM, 0); 
		if(sock <= 0)
			err(-1, "opening socket failed");

		if(inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0)  
		{ 
			printf("\nInvalid address/ Address not supported \n"); 
			return -1; 
		} 
#if defined(__x86_64__)
		start = rdtsc();
#else
		if( clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &start) == -1 ) {
			perror( "clock gettime" );
			exit( EXIT_FAILURE );
		}
#endif
		int ret = connect(sock, (struct sockaddr *)&address, sizeof(address));
#if defined(__x86_64__)
  		stop = rdtsc();
		diff = stop - start;
		if (ret != 0)
			fprintf(stdout, "Connection Failed: %s \n", strerror(errno)); 
		else
			fprintf(stdout, "Iteration %d: SYSCALL TOOK: %llu cycles\n", i, diff);
#else
		if( clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &stop) == -1 ) {
			perror( "clock gettime" );
			exit( EXIT_FAILURE );
		}
		
				long seconds = stop.tv_sec - start.tv_sec; 
		long ns = stop.tv_nsec - start.tv_nsec; 

		if (start.tv_nsec > stop.tv_nsec) { // clock underflow 
			--seconds; 
			ns += 1000000000; 
		}

		// Calculating the time spent in nanosecond	
		diff = BILLION * (seconds) + ns;
		if (ret != 0)
			fprintf(stdout, "Connection Failed: %s \n", strerror(errno)); 
		else
			fprintf(stdout, "Iteration %d: SYSCALL TOOK: %lld nanoseconds\n", i, diff);
#endif
		
		close(sock); 
		setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&true_val,sizeof(int));

		fflush(stdout);

		 
	}
	
	// This part is for remote_paired mode to make the supervisor awar that the client is done
	char* sock_env = getenv(SERVER_SOCK_ENV);
	if(sock_env != NULL) {
		int sock = atoi(sock_env);
		nvlist_t* nvl = nvlist_create(0);
		nvlist_add_string(nvl, "command", "last");

		if (nvlist_send(sock, nvl) < 0) {
			nvlist_destroy(nvl);
			err(1, "nvlist_send() failed");
		}
		nvlist_destroy(nvl);
	}

	return 0;
}


