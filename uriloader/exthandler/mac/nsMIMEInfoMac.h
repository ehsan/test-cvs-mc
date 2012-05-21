/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsMIMEInfoMac_h_
#define nsMIMEInfoMac_h_

#include "nsMIMEInfoImpl.h"

class nsMIMEInfoMac : public nsMIMEInfoImpl {
  public:
    nsMIMEInfoMac(const char* aMIMEType = "") : nsMIMEInfoImpl(aMIMEType) {}
    nsMIMEInfoMac(const nsACString& aMIMEType) : nsMIMEInfoImpl(aMIMEType) {}
    nsMIMEInfoMac(const nsACString& aType, HandlerClass aClass) :
      nsMIMEInfoImpl(aType, aClass) {}

    NS_IMETHOD LaunchWithFile(nsIFile* aFile);
  protected:
    virtual NS_HIDDEN_(nsresult) LoadUriInternal(nsIURI *aURI);
#ifdef DEBUG
    virtual NS_HIDDEN_(nsresult) LaunchDefaultWithFile(nsIFile* aFile) {
      NS_NOTREACHED("do not call this method, use LaunchWithFile");
      return NS_ERROR_UNEXPECTED;
    }
#endif
    static NS_HIDDEN_(nsresult) OpenApplicationWithURI(nsIFile *aApplication, 
                                                       const nsCString& aURI);
                                                       
    NS_IMETHOD GetDefaultDescription(nsAString& aDefaultDescription);
    
};


#endif
