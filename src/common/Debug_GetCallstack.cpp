/*
 *  Debug_GetCallstack.cpp
 *  OpenLieroX
 *
 *  Created by Albert Zeyer on 06.04.12.
 *  code under LGPL
 *
 */

#include "Debug.h"

#ifndef HAVE_EXECINFO
#	if defined(__linux__)
#		define HAVE_EXECINFO 1
#	elif defined(__DARWIN_VERS_1050)
#		define HAVE_EXECINFO 1
#	else
#		define HAVE_EXECINFO 0
#	endif
#endif

#if HAVE_EXECINFO
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#endif



#if defined(__APPLE__)

// see: http://www.opensource.apple.com/source/Libc/Libc-594.9.4/gen/thread_stack_pcs.c
// see: http://www.opensource.apple.com/source/Libc/Libc-594.9.4/gen/backtrace.c

// This was also suggested here:
// http://stackoverflow.com/questions/4765158/printing-a-stack-trace-from-another-thread


/*
 * Copyright (c) 1999, 2007 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

/*	Bertrand from vmutils -> CF -> System */


#include <mach/mach.h>
#include <mach/vm_statistics.h>
#include <stdlib.h>

#if defined(__i386__) || defined(__x86_64__) || defined(__arm__)
#define FP_LINK_OFFSET 1
#elif defined(__ppc__) || defined(__ppc64__)
#define FP_LINK_OFFSET 2
#else
#error  ********** Unimplemented architecture
#endif

#define INSTACK(a)  ((a) >= stackbot && (a) <= stacktop)
#if defined(__ppc__) || defined(__ppc64__) || defined(__x86_64__)
#define ISALIGNED(a)    ((((uintptr_t)(a)) & 0xf) == 0)
#elif defined(__arm__)
#define ISALIGNED(a)    ((((uintptr_t)(a)) & 0x1) == 0)
#elif defined(__i386__)
#define ISALIGNED(a)    ((((uintptr_t)(a)) & 0xf) == 8)
#endif

__private_extern__  __attribute__((noinline))
void
internal_arbitrary_thread_stack_pcs(pthread_t thread, vm_address_t *buffer, unsigned max, unsigned *nb, unsigned skip)
{
    void *frame, *next;
    char *stacktop = (char*)pthread_get_stackaddr_np(thread);
    char *stackbot = stacktop - pthread_get_stacksize_np(thread);
	
    *nb = 0;
	
    /* make sure return address is never out of bounds */
    stacktop -= (FP_LINK_OFFSET + 1) * sizeof(void *);
	
    /*
     * The original implementation called the first_frame_address() function,
     * which returned the stack frame pointer.  The problem was that in ppc,
     * it was a leaf function, so no new stack frame was set up with
     * optimization turned on (while a new stack frame was set up without
     * optimization).  We now inline the code to get the stack frame pointer,
     * so we are consistent about the stack frame.
     */
#if defined(__i386__) || defined(__x86_64__) || defined(__arm__)
    frame = __builtin_frame_address(0);
#elif defined(__ppc__) || defined(__ppc64__)
    /* __builtin_frame_address IS BROKEN IN BEAKER: RADAR #2340421 */
    __asm__ volatile("mr %0, r1" : "=r" (frame));
#endif
    if(!INSTACK(frame) || !ISALIGNED(frame))
        return;
#if defined(__ppc__) || defined(__ppc64__)
    /* back up the stack pointer up over the current stack frame */
    next = *(void **)frame;
    if(!INSTACK(next) || !ISALIGNED(next) || next <= frame)
        return;
    frame = next;
#endif
    while (skip--) {
        next = *(void **)frame;
        if(!INSTACK(next) || !ISALIGNED(next) || next <= frame)
            return;
        frame = next;
    }
    while (max--) {
        buffer[*nb] = *(vm_address_t *)(((void **)frame) + FP_LINK_OFFSET);
        (*nb)++;
        next = *(void **)frame;
        if(!INSTACK(next) || !ISALIGNED(next) || next <= frame)
            return;
        frame = next;
    }
}

void
arbitrary_thread_stack_pcs(pthread_t thread, vm_address_t *buffer, unsigned max, unsigned *nb)
{
    internal_arbitrary_thread_stack_pcs(thread, buffer, max, nb, 0);
	
    // The following prevents thread_stack_pcs() from getting tail-call-optimized into _thread_stack_pcs() on 64-bit environments,
    // thus making the "number of hot frames to skip" be more predictable, giving more consistent backtraces.
    // See <rdar://problem/5364825> "stack logging: frames keep getting truncated" for why this is necessary.
    __asm__ volatile("");
}

int GetCallstack(ThreadId thread, void** buffer, int size)
{
	// TODO (IMPORTANT): Suspend/Resume thread
	
    unsigned int num_frames;
    internal_arbitrary_thread_stack_pcs((pthread_t)thread, (vm_address_t*)buffer, size, &num_frames, 1);
    while (num_frames >= 1 && buffer[num_frames-1] == NULL) num_frames -= 1;
    return num_frames;
}


#else

// default implementation

int GetCallstack(ThreadId threadId, void **buffer, int size) {
#ifdef HAVE_EXECINFO
	if(threadId == 0)
		return backtrace(buffer, size);
#endif

	// stub
	return 0;
}

#endif
