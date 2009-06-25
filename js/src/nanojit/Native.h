/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: t; tab-width: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is [Open Source Virtual Machine].
 *
 * The Initial Developer of the Original Code is
 * Adobe System Incorporated.
 * Portions created by the Initial Developer are Copyright (C) 2004-2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Adobe AS3 Team
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */


#ifndef __nanojit_Native__
#define __nanojit_Native__


#ifdef NANOJIT_IA32
#include "Nativei386.h"
#elif defined(NANOJIT_ARM)
#include "NativeARM.h"
#elif defined(NANOJIT_PPC)
#include "NativePpc.h"
#elif defined(NANOJIT_SPARC)
#include "NativeSparc.h"
#elif defined(NANOJIT_AMD64)
#include "NativeAMD64.h"
#else
#error "unknown nanojit architecture"
#endif

namespace nanojit {
    const size_t NJ_PAGE_SIZE = 1 << NJ_LOG2_PAGE_SIZE;
	
    class Fragment;
    struct SideExit;
	struct SwitchInfo;
    
    struct GuardRecord 
    {
        void* jmp;
        GuardRecord* next;
        SideExit* exit;
    };
    
    struct SideExit
    {
        GuardRecord* guards;
        Fragment* from;
        Fragment* target;
		SwitchInfo* switchInfo;
        
        void addGuard(GuardRecord* gr)
        {
            NanoAssert(gr->next == NULL);
            NanoAssert(guards != gr);
            gr->next = guards;
            guards = gr;
        }
    };
}

	#ifdef NJ_STACK_GROWTH_UP
		#define stack_direction(n)   n
	#else
		#define stack_direction(n)  -n
	#endif
	
	#define isSPorFP(r)		( (r)==SP || (r)==FP )

	#if defined(_MSC_VER) && _MSC_VER < 1400
		static void asm_output(const char *f, ...) {}
		#define gpn(r)					regNames[(r)]
		#define fpn(r)					regNames[(r)]
	#elif defined(NJ_VERBOSE)
		#define asm_output(...) do { \
			counter_increment(native); \
			if (_logc->lcbits & LC_Assembly) { \
				outline[0]='\0'; \
				if (outputAddr) \
                   sprintf(outline, "%010lx   ", (unsigned long)_nIns); \
				else \
                   memset(outline, (int)' ', 10+3); \
				sprintf(&outline[13], ##__VA_ARGS__); \
				Assembler::outputAlign(outline, 35); \
				RegAlloc::formatRegisters(_allocator, outline, _thisfrag); \
				Assembler::output_asm(outline); \
				outputAddr=(_logc->lcbits & LC_NoCodeAddrs) ? false : true;	\
			} \
		} while (0) /* no semi */ 
		#define gpn(r)					regNames[(r)] 
		#define fpn(r)					regNames[(r)] 
	#else
		#define asm_output(...)
		#define gpn(r)		
		#define fpn(r)		
	#endif /* NJ_VERBOSE */

#endif // __nanojit_Native__
