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
                const uint32_t& x,
                const uint32_t& y,
                const uint32_t& width,
                const uint32_t& height) = 0;

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
                if (Answerinit(parentWidget)) {
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
                if (AnswerloadURL(uri)) {
                    return MsgValueError;
                }

                reply = new IFrameEmbeddingProtocol::Reply_loadURL();
                reply->set_reply();
                return MsgProcessed;
            }
        case IFrameEmbeddingProtocol::Msg_move__ID:
            {
                uint32_t x;
                uint32_t y;
                uint32_t width;
                uint32_t height;

                if (!(IFrameEmbeddingProtocol::Msg_move::Read(&(msg), &(x), &(y), &(width), &(height)))) {
                    return MsgPayloadError;
                }
                if (Answermove(x, y, width, height)) {
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



#endif // ifndef IFrameEmbeddingProtocolChild_h
