#ifndef mozilla__ipdltest_TestLatency_h
#define mozilla__ipdltest_TestLatency_h 1

#include "mozilla/_ipdltest/IPDLUnitTests.h"

#include "mozilla/_ipdltest/PTestLatencyParent.h"
#include "mozilla/_ipdltest/PTestLatencyChild.h"

#include "mozilla/TimeStamp.h"

#define NR_TRIALS 10000

namespace mozilla {
namespace _ipdltest {

class TestLatencyParent :
    public PTestLatencyParent
{
private:
    typedef mozilla::TimeStamp TimeStamp;
    typedef mozilla::TimeDuration TimeDuration;

public:
    TestLatencyParent();
    virtual ~TestLatencyParent();

    void Main();

protected:
    NS_OVERRIDE
    virtual bool RecvPong();
    NS_OVERRIDE
    virtual bool RecvPong5();

    NS_OVERRIDE
    virtual void ActorDestroy(ActorDestroyReason why)
    {
        if (NormalShutdown != why)
            fail("unexpected destruction!");  

        passed("average ping/pong latency: %g sec, "
               "average ping5/pong5 latency: %g sec, "
               "average RPC call/answer: %g sec",
               mPPTimeTotal.ToSecondsSigDigits() / (double) NR_TRIALS,
               mPP5TimeTotal.ToSecondsSigDigits() / (double) NR_TRIALS,
               mRpcTimeTotal.ToSecondsSigDigits() / (double) NR_TRIALS);

        QuitParent();
    }

private:
    void PingPongTrial();
    void Ping5Pong5Trial();
    void RpcTrials();
    void Exit();

    TimeStamp mStart;
    TimeDuration mPPTimeTotal;
    TimeDuration mPP5TimeTotal;
    TimeDuration mRpcTimeTotal;

    int mPPTrialsToGo;
    int mPP5TrialsToGo;

    // FIXME/cjones: HACK ALERT: don't need this once IPDL exposes actor state
    int mPongsToGo;
};


class TestLatencyChild :
    public PTestLatencyChild
{
public:
    TestLatencyChild();
    virtual ~TestLatencyChild();

protected:
    NS_OVERRIDE
    virtual bool RecvPing();
    NS_OVERRIDE
    virtual bool RecvPing5();
    NS_OVERRIDE
    virtual bool AnswerRpc();

    NS_OVERRIDE
    virtual void ActorDestroy(ActorDestroyReason why)
    {
        if (NormalShutdown != why)
            fail("unexpected destruction!");
        QuitChild();
    }
};


} // namespace _ipdltest
} // namespace mozilla


#endif // ifndef mozilla__ipdltest_TestLatency_h
