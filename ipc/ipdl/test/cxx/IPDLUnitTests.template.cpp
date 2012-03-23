//
// Autogenerated from Python template.  Hands off.
//

#include <stdlib.h>
#include <string.h>

#include "IPDLUnitTests.h"

#include "base/command_line.h"
#include "base/string_util.h"
#include "base/thread.h"

#include "nsRegion.h"

#include "IPDLUnitTestSubprocess.h"

//-----------------------------------------------------------------------------
//===== TEMPLATED =====
${INCLUDES}
//-----------------------------------------------------------------------------

using namespace base;
using namespace std;

namespace mozilla {
namespace _ipdltest {

void* gParentActor;
IPDLUnitTestSubprocess* gSubprocess;

void* gChildActor;

// Note: in threaded mode, this will be non-null (for both parent and
// child, since they share one set of globals).  
Thread* gChildThread;
MessageLoop *gParentMessageLoop;
bool gParentDone;
bool gChildDone;

//-----------------------------------------------------------------------------
// data/functions accessed by both parent and child processes

char* gIPDLUnitTestName = NULL;

const char* const
IPDLUnitTestName()
{
    if (!gIPDLUnitTestName) {
#if defined(OS_WIN)
        vector<wstring> args =
            CommandLine::ForCurrentProcess()->GetLooseValues();
        gIPDLUnitTestName = ::strdup(WideToUTF8(args[0]).c_str());
#elif defined(OS_POSIX)
        vector<string> argv = CommandLine::ForCurrentProcess()->argv();
        gIPDLUnitTestName = ::moz_xstrdup(argv[1].c_str());
#else
#  error Sorry
#endif
    }
    return gIPDLUnitTestName;
}

} // namespace _ipdltest
} // namespace mozilla


namespace {

enum IPDLUnitTestType {
    NoneTest = 0,

//-----------------------------------------------------------------------------
//===== TEMPLATED =====
${ENUM_VALUES}
    
    LastTest = ${LAST_ENUM}
//-----------------------------------------------------------------------------
};


IPDLUnitTestType
IPDLUnitTestFromString(const char* const aString)
{
    if (!aString)
        return static_cast<IPDLUnitTestType>(0);
//-----------------------------------------------------------------------------
//===== TEMPLATED =====
${STRING_TO_ENUMS}
//-----------------------------------------------------------------------------
    else
        return static_cast<IPDLUnitTestType>(0);
}


const char* const
IPDLUnitTestToString(IPDLUnitTestType aTest)
{
    switch (aTest) {
//-----------------------------------------------------------------------------
//===== TEMPLATED =====
${ENUM_TO_STRINGS}
//-----------------------------------------------------------------------------

    default:
        return NULL;
    }
}


IPDLUnitTestType
IPDLUnitTest()
{
    return IPDLUnitTestFromString(mozilla::_ipdltest::IPDLUnitTestName());
}


} // namespace <anon>


//-----------------------------------------------------------------------------
// parent process only

namespace mozilla {
namespace _ipdltest {

void
DeferredParentShutdown();

void
IPDLUnitTestThreadMain(char *testString);

void
IPDLUnitTestMain(void* aData)
{
    char* testString = reinterpret_cast<char*>(aData);

    // Check if we are to run the test using threads instead:
    const char *prefix = "thread:";
    const int prefixLen = strlen(prefix);
    if (!strncmp(testString, prefix, prefixLen)) {
        IPDLUnitTestThreadMain(testString + prefixLen);
        return;
    }

    IPDLUnitTestType test = IPDLUnitTestFromString(testString);
    if (!test) {
        // use this instead of |fail()| because we don't know what the test is
        fprintf(stderr, MOZ_IPDL_TESTFAIL_LABEL "| %s | unknown unit test %s\n",
                "<--->", testString);
        NS_RUNTIMEABORT("can't continue");
    }
    gIPDLUnitTestName = testString;

    // Check whether this test is enabled for processes:
    switch (test) {
//-----------------------------------------------------------------------------
//===== TEMPLATED =====
${PARENT_ENABLED_CASES_PROC}
//-----------------------------------------------------------------------------

    default:
        fail("not reached");
        return;                 // unreached
    }

    // Create the two processes:
    if (NS_FAILED(nsRegion::InitStatic()))
        fail("initializing nsRegion");

    printf(MOZ_IPDL_TESTINFO_LABEL "| running test | %s\n", gIPDLUnitTestName);

    std::vector<std::string> testCaseArgs;
    testCaseArgs.push_back(testString);

    gSubprocess = new IPDLUnitTestSubprocess();
    if (!gSubprocess->SyncLaunch(testCaseArgs))
        fail("problem launching subprocess");

    IPC::Channel* transport = gSubprocess->GetChannel();
    if (!transport)
        fail("no transport");

    base::ProcessHandle child = gSubprocess->GetChildProcessHandle();

    switch (test) {
//-----------------------------------------------------------------------------
//===== TEMPLATED =====
${PARENT_MAIN_CASES_PROC}
//-----------------------------------------------------------------------------

    default:
        fail("not reached");
        return;                 // unreached
    }
}

void
IPDLUnitTestThreadMain(char *testString)
{
    IPDLUnitTestType test = IPDLUnitTestFromString(testString);
    if (!test) {
        // use this instead of |fail()| because we don't know what the test is
        fprintf(stderr, MOZ_IPDL_TESTFAIL_LABEL "| %s | unknown unit test %s\n",
                "<--->", testString);
        NS_RUNTIMEABORT("can't continue");
    }
    gIPDLUnitTestName = testString;

    // Check whether this test is enabled for threads:
    switch (test) {
//-----------------------------------------------------------------------------
//===== TEMPLATED =====
${PARENT_ENABLED_CASES_THREAD}
//-----------------------------------------------------------------------------

    default:
        fail("not reached");
        return;                 // unreached
    }

    // Create the two threads:
    if (NS_FAILED(nsRegion::InitStatic()))
        fail("initializing nsRegion");

    printf(MOZ_IPDL_TESTINFO_LABEL "| running test | %s\n", gIPDLUnitTestName);

    std::vector<std::string> testCaseArgs;
    testCaseArgs.push_back(testString);

    gChildThread = new Thread("ParentThread");
    if (!gChildThread->Start())
        fail("starting parent thread");

    gParentMessageLoop = MessageLoop::current();
    MessageLoop *childMessageLoop = gChildThread->message_loop();

    switch (test) {
//-----------------------------------------------------------------------------
//===== TEMPLATED =====
${PARENT_MAIN_CASES_THREAD}
//-----------------------------------------------------------------------------

    default:
        fail("not reached");
        return;                 // unreached
    }
}

void
DeleteParentActor()
{
    if (!gParentActor)
        return;

    switch (IPDLUnitTest()) {
//-----------------------------------------------------------------------------
//===== TEMPLATED =====
${PARENT_DELETE_CASES}
//-----------------------------------------------------------------------------
    default:  mozilla::_ipdltest::fail("???");
    }
}

void
QuitXPCOM()
{
  DeleteParentActor();

  static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);
  nsCOMPtr<nsIAppShell> appShell (do_GetService(kAppShellCID));
  appShell->Exit();
}

void
DeleteSubprocess(MessageLoop* uiLoop)
{
  // pong to QuitXPCOM
  delete gSubprocess;
  uiLoop->PostTask(FROM_HERE, NewRunnableFunction(QuitXPCOM));
}

void
DeferredParentShutdown()
{
    // ping to DeleteSubprocess
    XRE_GetIOMessageLoop()->PostTask(
        FROM_HERE,
        NewRunnableFunction(DeleteSubprocess, MessageLoop::current()));
}

void 
TryThreadedShutdown()
{
    // Stop if either: 
    // - the child has not finished, 
    // - the parent has not finished,
    // - or this code has already executed.
    // Remember: this TryThreadedShutdown() task is enqueued
    // by both parent and child (though always on parent's msg loop).
    if (!gChildDone || !gParentDone || !gChildThread)
        return;

    delete gChildThread;
    gChildThread = 0;
    DeferredParentShutdown();
}

void 
ChildCompleted()
{
    // Executes on the parent message loop once child has completed.
    gChildDone = true;
    TryThreadedShutdown();
}

void
QuitParent()
{
    if (gChildThread) {
        gParentDone = true;
        MessageLoop::current()->PostTask(
            FROM_HERE, NewRunnableFunction(TryThreadedShutdown));
    } else {
        // defer "real" shutdown to avoid *Channel::Close() racing with the
        // deletion of the subprocess
        MessageLoop::current()->PostTask(
            FROM_HERE, NewRunnableFunction(DeferredParentShutdown));
    }
}

void
QuitChild()
{
    if (gChildThread) { // Threaded-mode test
        gParentMessageLoop->PostTask(
            FROM_HERE, NewRunnableFunction(ChildCompleted));
    } else { // Process-mode test
        XRE_ShutdownChildProcess();
    }
}

} // namespace _ipdltest
} // namespace mozilla


//-----------------------------------------------------------------------------
// child process only

namespace mozilla {
namespace _ipdltest {

void
DeleteChildActor()
{
    if (!gChildActor)
        return;

    switch (IPDLUnitTest()) {
//-----------------------------------------------------------------------------
//===== TEMPLATED =====
${CHILD_DELETE_CASES}
//-----------------------------------------------------------------------------
    default:  mozilla::_ipdltest::fail("???");
    }
}

void
IPDLUnitTestChildInit(IPC::Channel* transport,
                      base::ProcessHandle parent,
                      MessageLoop* worker)
{
    if (atexit(DeleteChildActor))
        fail("can't install atexit() handler");

    switch (IPDLUnitTest()) {
//-----------------------------------------------------------------------------
//===== TEMPLATED =====
${CHILD_INIT_CASES}
//-----------------------------------------------------------------------------

    default:
        fail("not reached");
        return;                 // unreached
    }
}

} // namespace _ipdltest
} // namespace mozilla
