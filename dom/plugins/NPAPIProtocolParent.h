//
// Automatically generated by ipdlc.
// Edit at your own risk
//

#ifndef mozilla_plugins_NPAPIProtocolParent_h
#define mozilla_plugins_NPAPIProtocolParent_h

#include "mozilla/plugins/NPAPIProtocol.h"
#include "mozilla/plugins/NPPProtocolParent.h"
#include "base/id_map.h"
#include "mozilla/ipc/RPCChannel.h"

namespace mozilla {
namespace plugins {


class /*NS_ABSTRACT_CLASS*/ NPAPIProtocolParent :
    public mozilla::ipc::RPCChannel::Listener,
    public mozilla::ipc::IProtocolManager
{
protected:
    typedef mozilla::ipc::String String;
    typedef mozilla::ipc::StringArray StringArray;

    virtual NPPProtocolParent* NPPConstructor(
                const String& aMimeType,
                const int& aHandle,
                const uint16_t& aMode,
                const StringArray& aNames,
                const StringArray& aValues,
                NPError* rv) = 0;
    virtual nsresult NPPDestructor(
                NPPProtocolParent* __a,
                NPError* rv) = 0;

private:
    typedef IPC::Message Message;
    typedef mozilla::ipc::RPCChannel Channel;

public:
    NPAPIProtocolParent() :
        mChannel(this)
    {
    }

    virtual ~NPAPIProtocolParent()
    {
    }

    bool Open(
                IPC::Channel* aChannel,
                MessageLoop* aThread = 0)
    {
        return mChannel.Open(aChannel, aThread);
    }

    void Close()
    {
        mChannel.Close();
    }

    nsresult CallNP_Initialize(NPError* rv)
    {
        Message __reply;
        Message* __msg;
        __msg = new NPAPIProtocol::Msg_NP_Initialize();
        __msg->set_routing_id(MSG_ROUTING_CONTROL);
        if (!(mChannel.Call(__msg, &(__reply)))) {
            return NS_ERROR_FAILURE;
        }
        if (!(NPAPIProtocol::Reply_NP_Initialize::Read(&(__reply), rv))) {
            return NS_ERROR_ILLEGAL_VALUE;
        }
        return NS_OK;
    }

    NPPProtocolParent* CallNPPConstructor(
                const String& aMimeType,
                const int& aHandle,
                const uint16_t& aMode,
                const StringArray& aNames,
                const StringArray& aValues,
                NPError* rv)
    {
        NPPProtocolParent* __a;
        __a = NPPConstructor(aMimeType, aHandle, aMode, aNames, aValues, rv);
        if (!(__a)) {
            return 0;
        }
        __a->mId = Register(__a);
        mozilla::ipc::ActorHandle __ah;
        __ah.mParentId = __a->mId;

        Message __reply;
        Message* __msg;
        __msg = new NPAPIProtocol::Msg_NPPConstructor(aMimeType, aHandle, aMode, aNames, aValues, __ah);
        __msg->set_routing_id(MSG_ROUTING_CONTROL);
        if (!(mChannel.Call(__msg, &(__reply)))) {
            return 0;
        }
        if (!(NPAPIProtocol::Reply_NPPConstructor::Read(&(__reply), rv, &(__ah)))) {
            return 0;
        }
        __a->mPeerId = __ah.mChildId;
        __a->mManager = this;
        __a->mChannel = &(mChannel);
        return __a;
    }

    nsresult CallNPPDestructor(
                NPPProtocolParent* __a,
                NPError* rv)
    {
        if (!(__a)) {
            return NS_ERROR_ILLEGAL_VALUE;
        }
        NPPProtocolParent* __b;
        __b = dynamic_cast<NPPProtocolParent*>(Lookup(__a->mId));
        if ((__a) != (__b)) {
            return NS_ERROR_ILLEGAL_VALUE;
        }

        mozilla::ipc::ActorHandle __ah;
        __ah.mParentId = __a->mId;
        __ah.mChildId = __a->mPeerId;

        Message __reply;
        Message* __msg;
        __msg = new NPAPIProtocol::Msg_NPPDestructor(__ah);
        __msg->set_routing_id(MSG_ROUTING_CONTROL);
        if (!(mChannel.Call(__msg, &(__reply)))) {
            return NS_ERROR_FAILURE;
        }
        if (!(NPAPIProtocol::Reply_NPPDestructor::Read(&(__reply), rv, &(__ah)))) {
            return NS_ERROR_ILLEGAL_VALUE;
        }
        Unregister(__a->mId);
        __a->mId = -1;
        __a->mManager = 0;
        __a->mPeerId = -1;
        return NS_OK;
    }

    virtual Result OnMessageReceived(const Message& msg)
    {
        switch (msg.type()) {
        default:
            {
                return MsgNotKnown;
            }
        }
    }

    virtual Result OnMessageReceived(
                const Message& msg,
                Message*& reply)
    {
        switch (msg.type()) {
        default:
            {
                return MsgNotKnown;
            }
        }
    }

    virtual Result OnCallReceived(
                const Message& msg,
                Message*& reply)
    {
        int __route;
        __route = msg.routing_id();
        if ((MSG_ROUTING_CONTROL) != (__route)) {
            Channel::Listener* __routed;
            __routed = Lookup(__route);
            if (!(__routed)) {
                return MsgRouteError;
            }
            return __routed->OnCallReceived(msg, reply);
        }

        switch (msg.type()) {
        default:
            {
                return MsgNotKnown;
            }
        }
    }

    virtual int32 Register(Channel::Listener* aRouted)
    {
        return mActorMap.Add(aRouted);
    }
    virtual Channel::Listener* Lookup(int32 aId)
    {
        return mActorMap.Lookup(aId);
    }
    virtual void Unregister(int32 aId)
    {
        return mActorMap.Remove(aId);
    }

private:
    Channel mChannel;
    IDMap<Channel::Listener> mActorMap;
};


} // namespace plugins
} // namespace mozilla

#endif // ifndef mozilla_plugins_NPAPIProtocolParent_h
