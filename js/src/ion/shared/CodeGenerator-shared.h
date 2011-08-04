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
 *   David Anderson <danderson@mozilla.com>
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

#ifndef jsion_codegen_h__
#define jsion_codegen_h__

#include "ion/MIR.h"
#include "ion/MIRGraph.h"
#include "ion/IonLIR.h"
#include "ion/IonMacroAssembler.h"
#include "ion/IonFrames.h"

namespace js {
namespace ion {

class CodeGeneratorShared : public LInstructionVisitor
{
  protected:
    MacroAssembler masm;
    MIRGenerator *gen;
    LIRGraph &graph;
    LBlock *current;

    static inline int32 ToInt32(const LAllocation *a) {
        return a->toConstant()->toInt32();
    }

  protected:
    // The initial size of the frame in bytes. These are bytes beyond the
    // constant header present for every Ion frame, used for pre-determined
    // spills.
    int32 frameDepth_;

    // Static size of the frame (see IonFrame.h). This is >= frameDepth_.
    int32 frameStaticSize_;

    // Frame class this frame's size falls into (see IonFrame.h).
    FrameSizeClass frameClass_;

    inline int32 ArgToStackOffset(int32 slot) {
        JS_ASSERT(slot >= 0);
        return masm.framePushed() + ION_FRAME_PREFIX_SIZE + slot;
    }

    inline int32 SlotToStackOffset(int32 slot) {
        JS_ASSERT(slot > 0 && slot <= int32(graph.localSlotCount()));
        int32 offset = masm.framePushed() - slot * STACK_SLOT_SIZE;
        JS_ASSERT(offset >= 0);
        return offset;
    }

    inline bool isNextBlock(LBlock *block) {
        return (current->mir()->id() + 1 == block->mir()->id());
    }

  private:
    virtual bool generatePrologue() = 0;
    virtual bool generateEpilogue() = 0;
    bool generateBody();

  public:
    CodeGeneratorShared(MIRGenerator *gen, LIRGraph &graph);

    bool generate();

  public:
    // Opcodes that are the same on all platforms.
    virtual bool visitParameter(LParameter *param);
};

} // ion
} // js

#endif // jsion_codegen_h__

