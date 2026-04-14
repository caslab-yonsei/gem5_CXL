/*
 * Copyright (c) 2015, University of Kaiserslautern
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Matthias Jung
 */

#include <stdio.h>

int main(void)
{
    
	unsigned int r = 1337;
	unsigned char* ptrx = (unsigned char*)0x80000000;  //CXL mem addr
	unsigned char* ptrx2 = (unsigned char*)0xa0000000; //Host(main) mem addr
	
	unsigned char a[] = "abcd\0";
	unsigned char b;

	 // printf("Write & Read from host CPU (dest addr=%x)\n", ptrx2);
	*ptrx = a[1]; // Write cxl mem  (CXL.mem)
	b = *ptrx; // Read cxl mem (CXL.mem)
	// *ptrx2 = a[1]; // Write host mem (Possibility of snooping (if cached on the cxl))(CXL.cache)
	// b = *ptrx2; // Read host mem (Possibility of snooping (if cached on the cxl))(CXL.cache)



//	printf("host CPU write addr(0x%x). value = %c\n", ptrx, *ptrx);
	

	while (1)
	{
	}
}

