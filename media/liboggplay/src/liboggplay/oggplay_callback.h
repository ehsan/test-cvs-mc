/*
   Copyright (C) 2003 Commonwealth Scientific and Industrial Research
   Organisation (CSIRO) Australia

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of CSIRO Australia nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * oggplay_callback.h
 * 
 * Shane Stephens <shane.stephens@annodex.net>
 */
#ifndef __OGGPLAY_CALLBACK_H__
#define __OGGPLAY_CALLBACK_H__

int
oggplay_callback_predetected (OGGZ *oggz, ogg_packet *op, long serialno,
                void *user_data);

void
oggplay_process_leftover_packet(OggPlay *me);

/**
 * Create and initialise an OggPlayDecode handle.
 *
 *  
 *
 * @param me OggPlay 
 * @param content_type 
 * @param serialno
 * @return A new OggPlayDecode handle
 * @retval NULL in case of error.
 */
OggPlayDecode *
oggplay_initialise_decoder(OggPlay *me, int content_type, long serialno);

int
oggplay_callback_info_prepare(OggPlay *me, OggPlayCallbackInfo ***info);

void
oggplay_callback_info_destroy(OggPlay *me, OggPlayCallbackInfo **info);

void
oggplay_callback_shutdown(OggPlayDecode *decoder);
#endif
