//
// Automatically generated by ipdlc.
// Edit at your own risk
//

#ifndef IFrameEmbeddingProtocolChild_h
#define IFrameEmbeddingProtocolChild_h

#include "IFrameEmbeddingProtocol.h"
#include "mozilla/ipc/RPCChannel.h"



class /*NS_ABSTRACT_CLASS*/ IFrameEmbeddingProtocolChild :
    public mozilla::ipc::RPCChannel::Listener
{
protected:
    typedef mozilla::ipc::String String;
    typedef mozilla::ipc::StringArray StringArray;

    virtual nsresult Answerinit(const MagicWindowHandle& parentWidget) = 0;
    virtual nsresult AnswerloadURL(const String& uri) = 0;
    virtual nsresult Answermove(
                const PRUint32& x,
                const PRUint32& y,
                const PRUint32& width,
                const PRUint32& height) = 0;

private:
    typedef IPC::Message Message;
    typedef mozilla::ipc::RPCChannel Channel;

public:
    IFrameEmbeddingProtocolChild() :
        mChannel(this)
    {
    }

    virtual ~IFrameEmbeddingProtocolChild()
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
        switch (msg.type()) {
        case IFrameEmbeddingProtocol::Msg_init__ID:
            {
                MagicWindowHandle parentWidget;

                if (!(IFrameEmbeddingProtocol::Msg_init::Read(&(msg), &(parentWidget)))) {
                    return MsgPayloadError;
                }
                if (NS_FAILED(Answerinit(parentWidget))) {
                    return MsgValueError;
                }

                reply = new IFrameEmbeddingProtocol::Reply_init();
                reply->set_reply();
                return MsgProcessed;
            }
        case IFrameEmbeddingProtocol::Msg_loadURL__ID:
            {
                String uri;

                if (!(IFrameEmbeddingProtocol::Msg_loadURL::Read(&(msg), &(uri)))) {
                    return MsgPayloadError;
                }
                if (NS_FAILED(AnswerloadURL(uri))) {
                    return MsgValueError;
                }

                reply = new IFrameEmbeddingProtocol::Reply_loadURL();
                reply->set_reply();
                return MsgProcessed;
            }
        case IFrameEmbeddingProtocol::Msg_move__ID:
            {
                PRUint32 x;
                PRUint32 y;
                PRUint32 width;
                PRUint32 height;

                if (!(IFrameEmbeddingProtocol::Msg_move::Read(&(msg), &(x), &(y), &(width), &(height)))) {
                    return MsgPayloadError;
                }
                if (NS_FAILED(Answermove(x, y, width, height))) {
                    return MsgValueError;
                }

                reply = new IFrameEmbeddingProtocol::Reply_move();
                reply->set_reply();
                return MsgProcessed;
            }
        default:
            {
                return MsgNotKnown;
            }
        }
    }

private:
    Channel mChannel;
    int mId;
    int mPeerId;
    mozilla::ipc::IProtocolManager* mManager;
};


#if 0

//-----------------------------------------------------------------------------
// Skeleton implementation of abstract actor class

// Header file contents
class ActorImpl :
    public IFrameEmbeddingProtocolChild
{
    virtual nsresult Answerinit(const MagicWindowHandle& parentWidget);
    virtual nsresult AnswerloadURL(const String& uri);
    virtual nsresult Answermove(
                const PRUint32& x,
                const PRUint32& y,
                const PRUint32& width,
                const PRUint32& height);
    ActorImpl();
    virtual ~ActorImpl();
};


// C++ file contents
nsresult ActorImpl::Answerinit(const MagicWindowHandle& parentWidget)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult ActorImpl::AnswerloadURL(const String& uri)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult ActorImpl::Answermove(
            const PRUint32& x,
            const PRUint32& y,
            const PRUint32& width,
            const PRUint32& height)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

ActorImpl::ActorImpl()
{
}

ActorImpl::~ActorImpl()
{
}

#endif // if 0

#endif // ifndef IFrameEmbeddingProtocolChild_h
