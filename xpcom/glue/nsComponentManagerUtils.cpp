/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsXPCOM_h__
#include "nsXPCOM.h"
#endif

#ifndef nsCOMPtr_h__
#include "nsCOMPtr.h"
#endif

#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"

#include "nsIComponentManager.h"

#ifndef MOZILLA_INTERNAL_API

nsresult
CallGetService(const nsCID &aCID, const nsIID &aIID, void **aResult)
{
    nsCOMPtr<nsIServiceManager> servMgr;
    nsresult status = NS_GetServiceManager(getter_AddRefs(servMgr));
    if (servMgr)
        status = servMgr->GetService(aCID, aIID, aResult);
    return status;
}

nsresult
CallGetService(const char *aContractID, const nsIID &aIID, void **aResult)
{
    nsCOMPtr<nsIServiceManager> servMgr;
    nsresult status = NS_GetServiceManager(getter_AddRefs(servMgr));
    if (servMgr)
        status = servMgr->GetServiceByContractID(aContractID, aIID, aResult);
    return status;
}

#else

#include "nsComponentManager.h"

nsresult
CallGetService(const nsCID &aCID, const nsIID &aIID, void **aResult)
{
    nsComponentManagerImpl *compMgr = nsComponentManagerImpl::gComponentManager;
    NS_ENSURE_TRUE(compMgr, NS_ERROR_NOT_INITIALIZED);

    return compMgr->nsComponentManagerImpl::GetService(aCID, aIID, aResult);
}

nsresult
CallGetService(const char *aContractID, const nsIID &aIID, void **aResult)
{
    nsComponentManagerImpl *compMgr = nsComponentManagerImpl::gComponentManager;
    NS_ENSURE_TRUE(compMgr, NS_ERROR_NOT_INITIALIZED);

    return compMgr->
        nsComponentManagerImpl::GetServiceByContractID(aContractID,
                                                       aIID, aResult);
}

#endif

#ifndef MOZILLA_INTERNAL_API

nsresult
CallCreateInstance(const nsCID &aCID, nsISupports *aDelegate,
                   const nsIID &aIID, void **aResult)
{
    nsCOMPtr<nsIComponentManager> compMgr;
    nsresult status = NS_GetComponentManager(getter_AddRefs(compMgr));
    if (compMgr)
        status = compMgr->CreateInstance(aCID, aDelegate, aIID, aResult);
    return status;
}

nsresult
CallCreateInstance(const char *aContractID, nsISupports *aDelegate,
                   const nsIID &aIID, void **aResult)
{
    nsCOMPtr<nsIComponentManager> compMgr;
    nsresult status = NS_GetComponentManager(getter_AddRefs(compMgr));
    if (compMgr)
        status = compMgr->CreateInstanceByContractID(aContractID, aDelegate,
                                                     aIID, aResult);
    return status;
}

nsresult
CallGetClassObject(const nsCID &aCID, const nsIID &aIID, void **aResult)
{
    nsCOMPtr<nsIComponentManager> compMgr;
    nsresult status = NS_GetComponentManager(getter_AddRefs(compMgr));
    if (compMgr)
        status = compMgr->GetClassObject(aCID, aIID, aResult);
    return status;
}

nsresult
CallGetClassObject(const char *aContractID, const nsIID &aIID, void **aResult)
{
    nsCOMPtr<nsIComponentManager> compMgr;
    nsresult status = NS_GetComponentManager(getter_AddRefs(compMgr));
    if (compMgr)
        status = compMgr->GetClassObjectByContractID(aContractID, aIID,
                                                     aResult);
    return status;
}

#else

#include "nsComponentManager.h"

nsresult
CallCreateInstance(const nsCID &aCID, nsISupports *aDelegate,
                   const nsIID &aIID, void **aResult)
{
    nsComponentManagerImpl *compMgr = nsComponentManagerImpl::gComponentManager;
    NS_ENSURE_TRUE(compMgr, NS_ERROR_NOT_INITIALIZED);

    return compMgr->
        nsComponentManagerImpl::CreateInstance(aCID, aDelegate, aIID, aResult);
}

nsresult
CallCreateInstance(const char *aContractID, nsISupports *aDelegate,
                   const nsIID &aIID, void **aResult)
{
    nsComponentManagerImpl *compMgr = nsComponentManagerImpl::gComponentManager;
    NS_ENSURE_TRUE(compMgr, NS_ERROR_NOT_INITIALIZED);

    return compMgr->
        nsComponentManagerImpl::CreateInstanceByContractID(aContractID,
                                                           aDelegate, aIID,
                                                           aResult);
}

nsresult
CallGetClassObject(const nsCID &aCID, const nsIID &aIID, void **aResult)
{
    nsComponentManagerImpl *compMgr = nsComponentManagerImpl::gComponentManager;
    NS_ENSURE_TRUE(compMgr, NS_ERROR_NOT_INITIALIZED);

    return compMgr->
        nsComponentManagerImpl::GetClassObject(aCID, aIID, aResult);
}

nsresult
CallGetClassObject(const char *aContractID, const nsIID &aIID, void **aResult)
{
    nsComponentManagerImpl *compMgr = nsComponentManagerImpl::gComponentManager;
    NS_ENSURE_TRUE(compMgr, NS_ERROR_NOT_INITIALIZED);

    return compMgr->
        nsComponentManagerImpl::GetClassObjectByContractID(aContractID, aIID,
                                                           aResult);
}

#endif

nsresult
nsCreateInstanceByCID::operator()( const nsIID& aIID, void** aInstancePtr ) const
{
    nsresult status = CallCreateInstance(mCID, mOuter, aIID, aInstancePtr);
    if ( NS_FAILED(status) )
        *aInstancePtr = 0;
    if ( mErrorPtr )
        *mErrorPtr = status;
    return status;
}

nsresult
nsCreateInstanceByContractID::operator()( const nsIID& aIID, void** aInstancePtr ) const
{
    nsresult status = CallCreateInstance(mContractID, mOuter, aIID, aInstancePtr);
    if (NS_FAILED(status))
        *aInstancePtr = 0;
    if ( mErrorPtr )
        *mErrorPtr = status;
    return status;
}

nsresult
nsCreateInstanceFromFactory::operator()( const nsIID& aIID, void** aInstancePtr ) const
{
    nsresult status = mFactory->CreateInstance(mOuter, aIID, aInstancePtr);
    if ( NS_FAILED(status) )
        *aInstancePtr = 0;
    if ( mErrorPtr )
        *mErrorPtr = status;
    return status;
}


nsresult
nsGetClassObjectByCID::operator()( const nsIID& aIID, void** aInstancePtr ) const
{
    nsresult status = CallGetClassObject(mCID, aIID, aInstancePtr);
    if ( NS_FAILED(status) )
        *aInstancePtr = 0;
    if ( mErrorPtr )
        *mErrorPtr = status;
    return status;
}

nsresult
nsGetClassObjectByContractID::operator()( const nsIID& aIID, void** aInstancePtr ) const
{
    nsresult status = CallGetClassObject(mContractID, aIID, aInstancePtr);
    if ( NS_FAILED(status) )
        *aInstancePtr = 0;
    if ( mErrorPtr )
        *mErrorPtr = status;
    return status;
}


nsresult
nsGetServiceByCID::operator()( const nsIID& aIID, void** aInstancePtr ) const
{
    nsresult status = CallGetService(mCID, aIID, aInstancePtr);
    if ( NS_FAILED(status) )
        *aInstancePtr = 0;

    return status;
}

nsresult
nsGetServiceByCIDWithError::operator()( const nsIID& aIID, void** aInstancePtr ) const
{
    nsresult status = CallGetService(mCID, aIID, aInstancePtr);
    if ( NS_FAILED(status) )
        *aInstancePtr = 0;

    if ( mErrorPtr )
        *mErrorPtr = status;
    return status;
}

nsresult
nsGetServiceByContractID::operator()( const nsIID& aIID, void** aInstancePtr ) const
{
    nsresult status = CallGetService(mContractID, aIID, aInstancePtr);
    if ( NS_FAILED(status) )
        *aInstancePtr = 0;
    
    return status;
}

nsresult
nsGetServiceByContractIDWithError::operator()( const nsIID& aIID, void** aInstancePtr ) const
{
    nsresult status = CallGetService(mContractID, aIID, aInstancePtr);
    if ( NS_FAILED(status) )
        *aInstancePtr = 0;
    
    if ( mErrorPtr )
        *mErrorPtr = status;
    return status;
}
