/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsAutoPtr.h"
#include "nsComponentManagerUtils.h"
#include "nsDependentString.h"
#include "nsIAtom.h"
#include "nsIInterfaceInfoManager.h"
#include "nsServiceManagerUtils.h"
#include "txExpr.h"
#include "txIFunctionEvaluationContext.h"
#include "txIXPathContext.h"
#include "txNodeSetAdaptor.h"
#include "txXPathTreeWalker.h"
#include "xptcall.h"
#include "txXPathObjectAdaptor.h"
#include "mozilla/Attributes.h"

NS_IMPL_ISUPPORTS1(txXPathObjectAdaptor, txIXPathObject)

class txFunctionEvaluationContext MOZ_FINAL : public txIFunctionEvaluationContext
{
public:
    txFunctionEvaluationContext(txIEvalContext *aContext, nsISupports *aState);

    NS_DECL_ISUPPORTS
    NS_DECL_TXIFUNCTIONEVALUATIONCONTEXT

    void ClearContext()
    {
        mContext = nullptr;
    }

private:
    txIEvalContext *mContext;
    nsCOMPtr<nsISupports> mState;
};

txFunctionEvaluationContext::txFunctionEvaluationContext(txIEvalContext *aContext,
                                                         nsISupports *aState)
    : mContext(aContext),
      mState(aState)
{
}

NS_IMPL_ISUPPORTS1(txFunctionEvaluationContext, txIFunctionEvaluationContext)

NS_IMETHODIMP
txFunctionEvaluationContext::GetPosition(PRUint32 *aPosition)
{
    NS_ENSURE_TRUE(mContext, NS_ERROR_FAILURE);

    *aPosition = mContext->position();

    return NS_OK;
}

NS_IMETHODIMP
txFunctionEvaluationContext::GetSize(PRUint32 *aSize)
{
    NS_ENSURE_TRUE(mContext, NS_ERROR_FAILURE);

    *aSize = mContext->size();

    return NS_OK;
}

NS_IMETHODIMP
txFunctionEvaluationContext::GetContextNode(nsIDOMNode **aNode)
{
    NS_ENSURE_TRUE(mContext, NS_ERROR_FAILURE);

    return txXPathNativeNode::getNode(mContext->getContextNode(), aNode);
}

NS_IMETHODIMP
txFunctionEvaluationContext::GetState(nsISupports **aState)
{
    NS_IF_ADDREF(*aState = mState);

    return NS_OK;
}

enum txArgumentType {
    eBOOLEAN = nsXPTType::T_BOOL,
    eNUMBER = nsXPTType::T_DOUBLE,
    eSTRING = nsXPTType::T_DOMSTRING,
    eNODESET,
    eCONTEXT,
    eOBJECT,
    eUNKNOWN
};

class txXPCOMExtensionFunctionCall : public FunctionCall
{
public:
    txXPCOMExtensionFunctionCall(nsISupports *aHelper, const nsIID &aIID,
                                 PRUint16 aMethodIndex,
#ifdef TX_TO_STRING
                                 nsIAtom *aName,
#endif
                                 nsISupports *aState);

    TX_DECL_FUNCTION

private:
    txArgumentType GetParamType(const nsXPTParamInfo &aParam,
                                nsIInterfaceInfo *aInfo);

    nsCOMPtr<nsISupports> mHelper;
    nsIID mIID;
    PRUint16 mMethodIndex;
#ifdef TX_TO_STRING
    nsCOMPtr<nsIAtom> mName;
#endif
    nsCOMPtr<nsISupports> mState;
};

txXPCOMExtensionFunctionCall::txXPCOMExtensionFunctionCall(nsISupports *aHelper,
                                                           const nsIID &aIID,
                                                           PRUint16 aMethodIndex,
#ifdef TX_TO_STRING
                                                           nsIAtom *aName,
#endif
                                                           nsISupports *aState)
    : mHelper(aHelper),
      mIID(aIID),
      mMethodIndex(aMethodIndex),
#ifdef TX_TO_STRING
      mName(aName),
#endif
      mState(aState)
{
}

class txInterfacesArrayHolder
{
public:
    txInterfacesArrayHolder(nsIID **aArray, PRUint32 aCount) : mArray(aArray),
                                                               mCount(aCount)
    {
    }
    ~txInterfacesArrayHolder()
    {
        NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(mCount, mArray);
    }

private:
    nsIID **mArray;
    PRUint32 mCount;
};

static nsresult
LookupFunction(const char *aContractID, nsIAtom* aName, nsIID &aIID,
               PRUint16 &aMethodIndex, nsISupports **aHelper)
{
    nsresult rv;
    nsCOMPtr<nsISupports> helper = do_GetService(aContractID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIClassInfo> classInfo = do_QueryInterface(helper, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIInterfaceInfoManager> iim =
        do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID);
    NS_ENSURE_TRUE(iim, NS_ERROR_FAILURE);

    nsIID** iidArray = nullptr;
    PRUint32 iidCount = 0;
    rv = classInfo->GetInterfaces(&iidCount, &iidArray);
    NS_ENSURE_SUCCESS(rv, rv);

    txInterfacesArrayHolder holder(iidArray, iidCount);

    // Remove any minus signs and uppercase the following letter (so
    // foo-bar becomes fooBar). Note that if there are any names that already
    // have uppercase letters they might cause false matches (both fooBar and
    // foo-bar matching fooBar).
    const PRUnichar *name = aName->GetUTF16String();
    nsCAutoString methodName;
    PRUnichar letter;
    bool upperNext = false;
    while ((letter = *name)) {
        if (letter == '-') {
            upperNext = true;
        }
        else {
            methodName.Append(upperNext ? nsCRT::ToUpper(letter) : letter);
            upperNext = false;
        }
        ++name;
    }

    PRUint32 i;
    for (i = 0; i < iidCount; ++i) {
        nsIID *iid = iidArray[i];

        nsCOMPtr<nsIInterfaceInfo> info;
        rv = iim->GetInfoForIID(iid, getter_AddRefs(info));
        NS_ENSURE_SUCCESS(rv, rv);

        PRUint16 methodIndex;
        const nsXPTMethodInfo *methodInfo;
        rv = info->GetMethodInfoForName(methodName.get(), &methodIndex,
                                        &methodInfo);
        if (NS_SUCCEEDED(rv)) {
            // Exclude notxpcom and hidden. Also check that we have at least a
            // return value (the xpidl compiler ensures that that return value
            // is the last argument).
            PRUint8 paramCount = methodInfo->GetParamCount();
            if (methodInfo->IsNotXPCOM() || methodInfo->IsHidden() ||
                paramCount == 0 ||
                !methodInfo->GetParam(paramCount - 1).IsRetval()) {
                return NS_ERROR_FAILURE;
            }

            aIID = *iid;
            aMethodIndex = methodIndex;
            return helper->QueryInterface(aIID, (void**)aHelper);
        }
    }

    return NS_ERROR_XPATH_UNKNOWN_FUNCTION;
}

/* static */
nsresult
TX_ResolveFunctionCallXPCOM(const nsCString &aContractID, PRInt32 aNamespaceID,
                            nsIAtom* aName, nsISupports *aState,
                            FunctionCall **aFunction)
{
    nsIID iid;
    PRUint16 methodIndex = 0;
    nsCOMPtr<nsISupports> helper;

    nsresult rv = LookupFunction(aContractID.get(), aName, iid, methodIndex,
                                 getter_AddRefs(helper));
    NS_ENSURE_SUCCESS(rv, rv);

    if (!aFunction) {
        return NS_OK;
    }

    *aFunction = new txXPCOMExtensionFunctionCall(helper, iid, methodIndex,
#ifdef TX_TO_STRING
                                                  aName,
#endif
                                                  aState);

    return *aFunction ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

txArgumentType
txXPCOMExtensionFunctionCall::GetParamType(const nsXPTParamInfo &aParam,
                                           nsIInterfaceInfo *aInfo)
{
    PRUint8 tag = aParam.GetType().TagPart();
    switch (tag) {
        case nsXPTType::T_BOOL:
        case nsXPTType::T_DOUBLE:
        case nsXPTType::T_DOMSTRING:
        {
            return txArgumentType(tag);
        }
        case nsXPTType::T_INTERFACE:
        case nsXPTType::T_INTERFACE_IS:
        {
            nsIID iid;
            aInfo->GetIIDForParamNoAlloc(mMethodIndex, &aParam, &iid);
            if (iid.Equals(NS_GET_IID(txINodeSet))) {
                return eNODESET;
            }
            if (iid.Equals(NS_GET_IID(txIFunctionEvaluationContext))) {
                return eCONTEXT;
            }
            if (iid.Equals(NS_GET_IID(txIXPathObject))) {
                return eOBJECT;
            }
        }
        // FALLTHROUGH
        default:
        {
            // XXX Error!
            return eUNKNOWN;
        }
    }
}

class txParamArrayHolder
{
public:
    txParamArrayHolder()
        : mCount(0)
    {
    }
    ~txParamArrayHolder();

    bool Init(PRUint8 aCount);
    operator nsXPTCVariant*() const
    {
      return mArray;
    }

private:
    nsAutoArrayPtr<nsXPTCVariant> mArray;
    PRUint8 mCount;
};

txParamArrayHolder::~txParamArrayHolder()
{
    PRUint8 i;
    for (i = 0; i < mCount; ++i) {
        nsXPTCVariant &variant = mArray[i];
        if (variant.DoesValNeedCleanup()) {
            if (variant.type.TagPart() == nsXPTType::T_DOMSTRING)
                delete (nsAString*)variant.val.p;
            else {
                NS_ABORT_IF_FALSE(variant.type.TagPart() == nsXPTType::T_INTERFACE ||
                                  variant.type.TagPart() == nsXPTType::T_INTERFACE_IS,
                                  "We only support cleanup of strings and interfaces "
                                  "here, and this looks like neither!");
                static_cast<nsISupports*>(variant.val.p)->Release();
            }
        }
    }
}

bool
txParamArrayHolder::Init(PRUint8 aCount)
{
    mCount = aCount;
    mArray = new nsXPTCVariant[mCount];
    if (!mArray) {
        return false;
    }

    memset(mArray, 0, mCount * sizeof(nsXPTCVariant));

    return true;
}

nsresult
txXPCOMExtensionFunctionCall::evaluate(txIEvalContext* aContext,
                                       txAExprResult** aResult)
{
    nsCOMPtr<nsIInterfaceInfoManager> iim =
        do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID);
    NS_ENSURE_TRUE(iim, NS_ERROR_FAILURE);

    nsCOMPtr<nsIInterfaceInfo> info;
    nsresult rv = iim->GetInfoForIID(&mIID, getter_AddRefs(info));
    NS_ENSURE_SUCCESS(rv, rv);

    const nsXPTMethodInfo *methodInfo;
    rv = info->GetMethodInfo(mMethodIndex, &methodInfo);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint8 paramCount = methodInfo->GetParamCount();
    PRUint8 inArgs = paramCount - 1;

    txParamArrayHolder invokeParams;
    if (!invokeParams.Init(paramCount)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    const nsXPTParamInfo &paramInfo = methodInfo->GetParam(0);
    txArgumentType type = GetParamType(paramInfo, info);
    if (type == eUNKNOWN) {
        return NS_ERROR_FAILURE;
    }

    txFunctionEvaluationContext *context;
    PRUint32 paramStart = 0;
    if (type == eCONTEXT) {
        if (paramInfo.IsOut()) {
            // We don't support out values.
            return NS_ERROR_FAILURE;
        }

        // Create context wrapper.
        context = new txFunctionEvaluationContext(aContext, mState);
        if (!context) {
            return NS_ERROR_OUT_OF_MEMORY;
        }

        nsXPTCVariant &invokeParam = invokeParams[0];
        invokeParam.type = paramInfo.GetType();
        invokeParam.SetValNeedsCleanup();
        NS_ADDREF((txIFunctionEvaluationContext*&)invokeParam.val.p = context);

        // Skip first argument, since it's the context.
        paramStart = 1;
    }
    else {
        context = nullptr;
    }

    // XXX varargs
    if (!requireParams(inArgs - paramStart, inArgs - paramStart, aContext)) {
        return NS_ERROR_FAILURE;
    }

    PRUint32 i;
    for (i = paramStart; i < inArgs; ++i) {
        Expr* expr = mParams[i - paramStart];

        const nsXPTParamInfo &paramInfo = methodInfo->GetParam(i);
        txArgumentType type = GetParamType(paramInfo, info);
        if (type == eUNKNOWN) {
            return NS_ERROR_FAILURE;
        }

        nsXPTCVariant &invokeParam = invokeParams[i];
        if (paramInfo.IsOut()) {
            // We don't support out values.
            return NS_ERROR_FAILURE;
        }

        invokeParam.type = paramInfo.GetType();
        switch (type) {
            case eNODESET:
            {
                nsRefPtr<txNodeSet> nodes;
                rv = evaluateToNodeSet(expr, aContext, getter_AddRefs(nodes));
                NS_ENSURE_SUCCESS(rv, rv);

                txNodeSetAdaptor *adaptor = new txNodeSetAdaptor(nodes);
                if (!adaptor) {
                    return NS_ERROR_OUT_OF_MEMORY;
                }

                nsCOMPtr<txINodeSet> nodeSet = adaptor;
                rv = adaptor->Init();
                NS_ENSURE_SUCCESS(rv, rv);

                invokeParam.SetValNeedsCleanup();
                nodeSet.swap((txINodeSet*&)invokeParam.val.p);
                break;
            }
            case eBOOLEAN:
            {
                rv = expr->evaluateToBool(aContext, invokeParam.val.b);
                NS_ENSURE_SUCCESS(rv, rv);

                break;
            }
            case eNUMBER:
            {
                double dbl;
                rv = evaluateToNumber(mParams[0], aContext, &dbl);
                NS_ENSURE_SUCCESS(rv, rv);

                invokeParam.val.d = dbl;
                break;
            }
            case eSTRING:
            {
                nsString *value = new nsString();
                if (!value) {
                    return NS_ERROR_OUT_OF_MEMORY;
                }

                rv = expr->evaluateToString(aContext, *value);
                NS_ENSURE_SUCCESS(rv, rv);

                invokeParam.SetValNeedsCleanup();
                invokeParam.val.p = value;
                break;
            }
            case eOBJECT:
            {
              nsRefPtr<txAExprResult> exprRes;
              rv = expr->evaluate(aContext, getter_AddRefs(exprRes));
              NS_ENSURE_SUCCESS(rv, rv);

              nsCOMPtr<txIXPathObject> adaptor =
                new txXPathObjectAdaptor(exprRes);
              if (!adaptor) {
                  return NS_ERROR_OUT_OF_MEMORY;
              }

              invokeParam.SetValNeedsCleanup();
              adaptor.swap((txIXPathObject*&)invokeParam.val.p);
              break;
            }
            case eCONTEXT:
            case eUNKNOWN:
            {
                // We only support passing the context as the *first* argument.
                return NS_ERROR_FAILURE;
            }
        }
    }

    const nsXPTParamInfo &returnInfo = methodInfo->GetParam(inArgs);
    txArgumentType returnType = GetParamType(returnInfo, info);
    if (returnType == eUNKNOWN) {
        return NS_ERROR_FAILURE;
    }

    nsXPTCVariant &returnParam = invokeParams[inArgs];
    returnParam.type = returnInfo.GetType();
    if (returnType == eSTRING) {
        nsString *value = new nsString();
        if (!value) {
            return NS_ERROR_FAILURE;
        }

        returnParam.SetValNeedsCleanup();
        returnParam.val.p = value;
    }
    else {
        returnParam.SetIndirect();
        if (returnType == eNODESET || returnType == eOBJECT) {
            returnParam.SetValNeedsCleanup();
        }
    }

    rv = NS_InvokeByIndex(mHelper, mMethodIndex, paramCount, invokeParams);

    // In case someone is holding on to the txFunctionEvaluationContext which
    // could thus stay alive longer than this function.
    if (context) {
        context->ClearContext();
    }

    NS_ENSURE_SUCCESS(rv, rv);

    switch (returnType) {
        case eNODESET:
        {
            txINodeSet* nodeSet = static_cast<txINodeSet*>(returnParam.val.p);
            nsCOMPtr<txIXPathObject> object = do_QueryInterface(nodeSet, &rv);
            NS_ENSURE_SUCCESS(rv, rv);

            NS_ADDREF(*aResult = object->GetResult());

            return NS_OK;
        }
        case eBOOLEAN:
        {
            aContext->recycler()->getBoolResult(returnParam.val.b, aResult);

            return NS_OK;
        }
        case eNUMBER:
        {
            return aContext->recycler()->getNumberResult(returnParam.val.d,
                                                         aResult);
        }
        case eSTRING:
        {
            nsString *returned = static_cast<nsString*>
                                            (returnParam.val.p);
            return aContext->recycler()->getStringResult(*returned, aResult);
        }
        case eOBJECT:
        {
            txIXPathObject *object =
                 static_cast<txIXPathObject*>(returnParam.val.p);

            NS_ADDREF(*aResult = object->GetResult());

            return NS_OK;
        }
        default:
        {
            // Huh?
            return NS_ERROR_FAILURE;
        }
    }
}

Expr::ResultType
txXPCOMExtensionFunctionCall::getReturnType()
{
    // It doesn't really matter what we return here, but it might
    // be a good idea to try to keep this as unoptimizable as possible
    return ANY_RESULT;
}

bool
txXPCOMExtensionFunctionCall::isSensitiveTo(ContextSensitivity aContext)
{
    // It doesn't really matter what we return here, but it might
    // be a good idea to try to keep this as unoptimizable as possible
    return true;
}

#ifdef TX_TO_STRING
nsresult
txXPCOMExtensionFunctionCall::getNameAtom(nsIAtom** aAtom)
{
    NS_ADDREF(*aAtom = mName);

    return NS_OK;
}
#endif
