/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsInputStreamChannel.h"

//-----------------------------------------------------------------------------
// nsInputStreamChannel

nsresult
nsInputStreamChannel::OpenContentStream(bool async, nsIInputStream **result,
                                        nsIChannel** channel)
{
  NS_ENSURE_TRUE(mContentStream, NS_ERROR_NOT_INITIALIZED);

  // If content length is unknown, then we must guess.  In this case, we assume
  // the stream can tell us.  If the stream is a pipe, then this will not work.

  PRInt64 len = ContentLength64();
  if (len < 0) {
    PRUint64 avail;
    nsresult rv = mContentStream->Available(&avail);
    if (rv == NS_BASE_STREAM_CLOSED) {
      // This just means there's nothing in the stream
      avail = 0;
    } else if (NS_FAILED(rv)) {
      return rv;
    }
    SetContentLength64(avail);
  }

  EnableSynthesizedProgressEvents(true);
  
  NS_ADDREF(*result = mContentStream);
  return NS_OK;
}

//-----------------------------------------------------------------------------
// nsInputStreamChannel::nsISupports

NS_IMPL_ISUPPORTS_INHERITED1(nsInputStreamChannel,
                             nsBaseChannel,
                             nsIInputStreamChannel)

//-----------------------------------------------------------------------------
// nsInputStreamChannel::nsIInputStreamChannel

NS_IMETHODIMP
nsInputStreamChannel::SetURI(nsIURI *uri)
{
  NS_ENSURE_TRUE(!URI(), NS_ERROR_ALREADY_INITIALIZED);
  nsBaseChannel::SetURI(uri);
  return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::GetContentStream(nsIInputStream **stream)
{
  NS_IF_ADDREF(*stream = mContentStream);
  return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::SetContentStream(nsIInputStream *stream)
{
  NS_ENSURE_TRUE(!mContentStream, NS_ERROR_ALREADY_INITIALIZED);
  mContentStream = stream;
  return NS_OK;
}
