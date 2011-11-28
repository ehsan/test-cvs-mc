/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=79:
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   David Anderson <dvander@alliedmods.net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef jsion_frames_h__
#define jsion_frames_h__

#include "jstypes.h"
#include "jsutil.h"
#include "IonRegisters.h"
#include "IonCode.h"

struct JSFunction;
struct JSScript;

namespace js {
namespace ion {

// In between every two frames lies a small header describing both frames. This
// header, minimally, contains a returnAddress word and a descriptor word. The
// descriptor describes the size and type of the previous frame, whereas the
// returnAddress describes the address the newer frame (the callee) will return
// to. The exact mechanism in which frames are laid out is architecture
// dependent.
//
// Two special frame types exist. Entry frames begin an ion activation, and
// therefore there is exactly one per activation of ion::Cannon. Exit frames
// are necessary to leave JIT code and enter C++, and thus, C++ code will
// always begin iterating from the topmost exit frame.

// IonFrameInfo are stored separately from the stack frame.  It can be composed
// of any field which are computed at compile time only.  It is recovered by
// using the calleeToken and the returnAddress of the stack frame.
struct IonFrameInfo
{
    // The displacement field is used to lookup the frame info among the list of
    // frameinfo of the script.
    ptrdiff_t displacement;
    SnapshotOffset snapshotOffset;
};


// The layout of an Ion frame on the C stack is roughly:
//      argN     _
//      ...       \ - These are jsvals
//      arg0      /
//   -3 this    _/
//   -2 callee
//   -1 descriptor
//    0 returnAddress
//   .. locals ..

enum FrameType
{
    IonFrame_JS,
    IonFrame_Entry,
    IonFrame_Rectifier,
    IonFrame_Exit
};
static const uint32 FRAMETYPE_BITS = 3;

// Ion frames have a few important numbers associated with them:
//      Local depth:    The number of bytes required to spill local variables.
//      Argument depth: The number of bytes required to push arguments and make
//                      a function call.
//      Slack:          A frame may temporarily use extra stack to resolve cycles.
//
// The (local + argument) depth determines the "fixed frame size". The fixed
// frame size is the distance between the stack pointer and the frame header.
// Thus, fixed >= (local + argument).
//
// In order to compress guards, we create shared jump tables that recover the
// script from the stack and recover a snapshot pointer based on which jump was
// taken. Thus, we create a jump table for each fixed frame size.
//
// Jump tables are big. To control the amount of jump tables we generate, each
// platform chooses how to segregate stack size classes based on its
// architecture.
//
// On some architectures, these jump tables are not used at all, or frame
// size segregation is not needed. Thus, there is an option for a frame to not
// have any frame size class, and to be totally dynamic.
static const uint32 NO_FRAME_SIZE_CLASS_ID = uint32(-1);

class FrameSizeClass
{
    uint32 class_;

    explicit FrameSizeClass(uint32 class_) : class_(class_)
    { }
  
  public:
    FrameSizeClass()
    { }

    static FrameSizeClass None() {
        return FrameSizeClass(NO_FRAME_SIZE_CLASS_ID);
    }
    static FrameSizeClass FromClass(uint32 class_) {
        return FrameSizeClass(class_);
    }

    // These two functions are implemented in specific CodeGenerator-* files.
    static FrameSizeClass FromDepth(uint32 frameDepth);
    uint32 frameSize() const;

    uint32 classId() const {
        JS_ASSERT(class_ != NO_FRAME_SIZE_CLASS_ID);
        return class_;
    }

    bool operator ==(const FrameSizeClass &other) const {
        return class_ == other.class_;
    }
    bool operator !=(const FrameSizeClass &other) const {
        return class_ != other.class_;
    }
};

class IonFrameIterator
{
    uint8 *current_;
    FrameType type_;

    // Cache the next frame pointer because it is used at least twice.  Once for
    // iterating and once for recovering the frame. (see
    // FrameRecovery::FromIterator)
    mutable uint8 *prevCache_;

  public:
    IonFrameIterator(uint8 *top)
      : current_(top),
        type_(IonFrame_Exit),
        prevCache_(top)
    { }

    // Current frame information.
    FrameType type() const {
        return type_;
    }
    uint8 *fp() const {
        return current_;
    }
    uint8 *returnAddress() const;

    // Previous frame information extracted from the current frame.
    size_t prevFrameLocalSize() const;
    FrameType prevType() const;
    uint8 *prevFp() const;

    // Funtctions used to iterate on frames.
    // When prevType is IonFrame_Entry, the current frame is the last frame.
    bool more() const {
        return prevType() != IonFrame_Entry;
    }
    void prev();
};

// Information needed to recover machine register state.
class MachineState
{
    uintptr_t *regs_;
    double *fpregs_;

  public:
    MachineState()
      : regs_(NULL), fpregs_(NULL)
    { }
    MachineState(uintptr_t *regs, double *fpregs)
      : regs_(regs), fpregs_(fpregs)
    { }

    double readFloatReg(FloatRegister reg) const {
        return fpregs_[reg.code()];
    }
    uintptr_t readReg(Register reg) const {
        return regs_[reg.code()];
    }
};

// Duplicated from Bailouts.h, which we can't include here.
typedef uint32 BailoutId;
typedef uint32 SnapshotOffset;

class IonJSFrameLayout;

// Information needed to recover the content of the stack frame.
class FrameRecovery
{
    IonJSFrameLayout *fp_;
    uint8 *sp_;             // fp_ + frameSize

    MachineState machine_;
    uint32 snapshotOffset_;

    JSObject *callee_;
    JSFunction *fun_;
    JSScript *script_;

  private:
    FrameRecovery(uint8 *fp, uint8 *sp, const MachineState &machine);

    void setSnapshotOffset(uint32 snapshotOffset) {
        snapshotOffset_ = snapshotOffset;
    }
    void setBailoutId(BailoutId bailoutId);

  public:
    static FrameRecovery FromBailoutId(uint8 *fp, uint8 *sp, const MachineState &machine,
                                       BailoutId bailoutId);
    static FrameRecovery FromSnapshot(uint8 *fp, uint8 *sp, const MachineState &machine,
                                      SnapshotOffset offset);
    static FrameRecovery FromFrameIterator(const IonFrameIterator& it);

    MachineState &machine() {
        return machine_;
    }
    const MachineState &machine() const {
        return machine_;
    }
    uintptr_t readSlot(uint32 offset) const {
        JS_ASSERT((offset % STACK_SLOT_SIZE) == 0);
        return *(uintptr_t *)(sp_ + offset);
    }
    double readDoubleSlot(uint32 offset) const {
        JS_ASSERT((offset % STACK_SLOT_SIZE) == 0);
        return *(double *)(sp_ + offset);
    }
    JSObject *callee() const {
        return callee_;
    }
    JSFunction *fun() const {
        return fun_;
    }
    JSScript *script() const {
        return script_;
    }
    IonScript *ionScript() const;
    uint32 snapshotOffset() const {
        return snapshotOffset_;
    }
};

// Data needed to recover from an exception.
struct ResumeFromException
{
    void *stackPointer;
};

void HandleException(ResumeFromException *rfe);

static inline uint32
MakeFrameDescriptor(uint32 frameSize, FrameType type)
{
    return (frameSize << FRAMETYPE_BITS) | type;
}

typedef void * CalleeToken;

static inline CalleeToken
CalleeToToken(JSObject *fun)
{
    return (CalleeToken *)fun;
}
static inline CalleeToken
CalleeToToken(JSScript *script)
{
    return (CalleeToken *)(uintptr_t(script) | 1);
}
static inline bool
IsCalleeTokenFunction(CalleeToken token)
{
    return (uintptr_t(token) & 1) == 0;
}
static inline JSObject *
CalleeTokenToFunction(CalleeToken token)
{
    JS_ASSERT(IsCalleeTokenFunction(token));
    return (JSObject *)token;
}
static inline JSScript *
CalleeTokenToScript(CalleeToken token)
{
    JS_ASSERT(!IsCalleeTokenFunction(token));
    return (JSScript*)(uintptr_t(token) & ~uintptr_t(1));
}

} // namespace ion
} // namespace js

#if defined(JS_CPU_X86) || defined (JS_CPU_X64)
# include "ion/shared/IonFrames-x86-shared.h"
#elif defined (JS_CPU_ARM)
# include "ion/arm/IonFrames-arm.h"
#else
# error "unsupported architecture"
#endif

#endif // jsion_frames_h__

