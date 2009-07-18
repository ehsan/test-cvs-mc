#include "mozilla/test/TestProtocolParent.h"

namespace mozilla {
namespace test {

// Header file contents
class TestParent :
    public TestProtocolParent
{
protected:
#if 1
//-----------------------------------------------------------------------------
// "Hello world" example
    virtual nsresult RecvWorld();


#elif 0
//-----------------------------------------------------------------------------
// Example solution to exercise
    virtual nsresult RecvPing();
    virtual nsresult RecvPong(const int& status);
    virtual nsresult RecvGetValue(const String& key);
    virtual nsresult RecvGetValues(const StringArray& keys);
    virtual nsresult RecvSetValue(
                const String& key,
                const String& val,
                bool* ok);
#endif

public:
    TestParent();
    virtual ~TestParent();

    void DoStuff();
};

} // namespace test
} // namespace mozilla
