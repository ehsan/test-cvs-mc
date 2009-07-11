//
// Automatically generated by ipdlc.
// Edit at your own risk
//

#ifndef mozilla_ipc_TestShellProtocol_h
#define mozilla_ipc_TestShellProtocol_h

#include "base/basictypes.h"
#include "nscore.h"
#include "IPC/IPCMessageUtils.h"
#include "mozilla/ipc/MessageTypes.h"
#include "mozilla/ipc/ProtocolUtils.h"

namespace mozilla {
namespace ipc {
namespace TestShellProtocol {


enum State {
    StateStart = 0,
    StateError,
    StateLast
};

enum MessageType {
    TestShellProtocolStart = TestShellProtocolMsgStart << 12,
    TestShellProtocolPreStart = (TestShellProtocolMsgStart << 12) - 1,
    Msg_SendCommand__ID,
    Reply_SendCommand__ID,
    TestShellProtocolEnd
};

class Msg_SendCommand :
    public IPC::Message
{
private:
    typedef mozilla::ipc::String String;
    typedef mozilla::ipc::StringArray StringArray;

public:
    enum {
        ID = Msg_SendCommand__ID
    };
    Msg_SendCommand(const String& aCommand) :
        IPC::Message(MSG_ROUTING_NONE, ID, PRIORITY_NORMAL)
    {
        IPC::WriteParam(this, aCommand);
    }

    static bool Read(
                const Message* msg,
                String* aCommand)
    {
        void* iter = 0;

        if (!(IPC::ReadParam(msg, &(iter), aCommand))) {
            return false;
        }

        return true;
    }
};
class Reply_SendCommand :
    public IPC::Message
{
private:
    typedef mozilla::ipc::String String;
    typedef mozilla::ipc::StringArray StringArray;

public:
    enum {
        ID = Reply_SendCommand__ID
    };
    Reply_SendCommand() :
        IPC::Message(MSG_ROUTING_NONE, ID, PRIORITY_NORMAL)
    {
    }

    static bool Read(const Message* msg)
    {
        return true;
    }
};


} // namespace TestShellProtocol
} // namespace ipc
} // namespace mozilla

#endif // ifndef mozilla_ipc_TestShellProtocol_h
