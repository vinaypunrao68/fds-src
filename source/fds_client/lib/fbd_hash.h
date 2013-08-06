/* Copyright (c) 2013 - 2014 All Right Reserved
*  Company= Formation  Data Systems. 
*
* This source is subject to the Formation Data systems Permissive License.
* All other rights reserved.
*
* THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY 
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
* This file contains  Hash alogorithems( 32  and 64 bit) 
*	- Murmur hashing  is used to get  the object id (DOID) 
*/

#ifndef _FBDHASH_h
#define _FBDHASH_h
 
 
void MurmurHash3_x86_32  ( const void * key, int len, uint32_t seed, void * out );
void MurmurHash3_x86_128 ( const void * key, int len, uint32_t seed, void * out );
void MurmurHash3_x64_128 ( const void * key, int len, uint32_t seed, void * out );
 
#endif

 

