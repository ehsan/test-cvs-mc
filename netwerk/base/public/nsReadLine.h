/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsReadLine_h__
#define nsReadLine_h__

#include "prmem.h"
#include "nsIInputStream.h"

/**
 * @file
 * Functions to read complete lines from an input stream.
 *
 * To properly use the helper function in here (NS_ReadLine) the caller
 * needs to declare a pointer to an nsLineBuffer, call
 * NS_InitLineBuffer on it, and pass it to NS_ReadLine every time it
 * wants a line out.
 *
 * When done, the pointer should be freed using PR_Free.
 */

/**
 * @internal
 * Buffer size. This many bytes will be buffered. If a line is longer than this,
 * the partial line will be appended to the out parameter of NS_ReadLine and the
 * buffer will be emptied.
 * Note: if you change this constant, please update the regression test in
 * netwerk/test/unit/test_readline.js accordingly (bug 397850).
 */
#define kLineBufferSize 4096

/**
 * @internal
 * Line buffer structure, buffers data from an input stream.
 * The buffer is empty when |start| == |end|.
 * Invariant: |start| <= |end|
 */
template<typename CharT>
class nsLineBuffer {
  public:
  CharT buf[kLineBufferSize+1];
  CharT* start;
  CharT* end;
};

/**
 * Initialize a line buffer for use with NS_ReadLine.
 *
 * @param aBufferPtr
 *        Pointer to pointer to a line buffer. Upon successful return,
 *        *aBufferPtr will contain a valid pointer to a line buffer, for use
 *        with NS_ReadLine. Use PR_Free when the buffer is no longer needed.
 *
 * @retval NS_OK Success.
 * @retval NS_ERROR_OUT_OF_MEMORY Not enough memory to allocate the line buffer.
 *
 * @par Example:
 * @code
 *    nsLineBuffer* lb;
 *    rv = NS_InitLineBuffer(&lb);
 *    if (NS_SUCCEEDED(rv)) {
 *      // do stuff...
 *      PR_Free(lb);
 *    }
 * @endcode
 */
template<typename CharT>
nsresult
NS_InitLineBuffer (nsLineBuffer<CharT> ** aBufferPtr) {
  *aBufferPtr = PR_NEW(nsLineBuffer<CharT>);
  if (!(*aBufferPtr))
    return NS_ERROR_OUT_OF_MEMORY;

  (*aBufferPtr)->start = (*aBufferPtr)->end = (*aBufferPtr)->buf;
  return NS_OK;
}

/**
 * Read a line from an input stream. Lines are separated by '\r' (0x0D) or '\n'
 * (0x0A), or "\r\n" or "\n\r".
 *
 * @param aStream
 *        The stream to read from
 * @param aBuffer
 *        The line buffer to use. Must have been inited with
 *        NS_InitLineBuffer before. A single line buffer must not be used with
 *        different input streams.
 * @param aLine [out]
 *        The string where the line will be stored.
 * @param more [out]
 *        Whether more data is available in the buffer. If true, NS_ReadLine may
 *        be called again to read further lines. Otherwise, further calls to
 *        NS_ReadLine will return an error.
 *
 * @retval NS_OK
 *         Read successful
 * @retval error
 *         Input stream returned an error upon read. See
 *         nsIInputStream::read.
 */
template<typename CharT, class StreamType, class StringType>
nsresult
NS_ReadLine (StreamType* aStream, nsLineBuffer<CharT> * aBuffer,
             StringType & aLine, bool *more)
{
  CharT eolchar = 0; // the first eol char or 1 after \r\n or \n\r is found

  aLine.Truncate();

  while (1) { // will be returning out of this loop on eol or eof
    if (aBuffer->start == aBuffer->end) { // buffer is empty.  Read into it.
      PRUint32 bytesRead;
      nsresult rv = aStream->Read(aBuffer->buf, kLineBufferSize, &bytesRead);
      if (NS_FAILED(rv) || NS_UNLIKELY(bytesRead == 0)) {
        *more = false;
        return rv;
      }
      aBuffer->start = aBuffer->buf;
      aBuffer->end = aBuffer->buf + bytesRead;
      *(aBuffer->end) = '\0';
    }

    /*
     * Walk the buffer looking for an end-of-line.
     * There are 3 cases to consider:
     *  1. the eol char is the last char in the buffer
     *  2. the eol char + one more char at the end of the buffer
     *  3. the eol char + two or more chars at the end of the buffer
     * we need at least one char after the first eol char to determine if
     * it's a \r\n or \n\r sequence (and skip over it), and we need one
     * more char after the end-of-line to set |more| correctly.
     */
    CharT* current = aBuffer->start;
    if (NS_LIKELY(eolchar == 0)) {
      for ( ; current < aBuffer->end; ++current) {
        if (*current == '\n' || *current == '\r') {
          eolchar = *current;
          *current++ = '\0';
          aLine.Append(aBuffer->start);
          break;
        }
      }
    }
    if (NS_LIKELY(eolchar != 0)) {
      for ( ; current < aBuffer->end; ++current) {
        if ((eolchar == '\r' && *current == '\n') ||
            (eolchar == '\n' && *current == '\r')) {
          eolchar = 1;
          continue;
        }
        aBuffer->start = current;
        *more = true;
        return NS_OK;
      }
    }

    if (eolchar == 0)
      aLine.Append(aBuffer->start);
    aBuffer->start = aBuffer->end; // mark the buffer empty
  }
}

#endif // nsReadLine_h__
