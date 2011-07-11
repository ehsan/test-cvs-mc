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

#ifndef jsion_cpu_x86_stack_assignment_h__
#define jsion_cpu_x86_stack_assignment_h__

namespace js {
namespace ion {

class StackAssignmentX86
{
    js::Vector<uint32, 4, IonAllocPolicy> normalSlots;
    js::Vector<uint32, 4, IonAllocPolicy> doubleSlots;
    uint32 height_;

  public:
    StackAssignmentX86() : height_(0)
    { }

    void freeSlot(uint32 index) {
        normalSlots.append(index);
    }
    void freeDoubleSlot(uint32 index) {
        doubleSlots.append(index);
    }

    bool allocateDoubleSlot(uint32 *index) {
        if (!doubleSlots.empty()) {
            *index = doubleSlots.popCopy();
            return false;
        }
        if (ComputeByteAlignment(height_, DOUBLE_STACK_ALIGNMENT)) {
            normalSlots.append(height_++);
            JS_ASSERT(!ComputeByteAlignment(height_, DOUBLE_STACK_ALIGNMENT));
        }
        *index = height_;
        height_ += 2;
        return height_ < MAX_STACK_SLOTS;
    }

    bool allocateSlot(uint32 *index) {
        if (!normalSlots.empty()) {
            *index = normalSlots.popCopy();
            return true;
        }
        if (!doubleSlots.empty()) {
            *index = doubleSlots.popCopy();
            return normalSlots.append(*index + 1);
        }
        *index = height_++;
        return height_ < MAX_STACK_SLOTS;
    }

    uint32 stackHeight() const {
        return height_;
    }
};

typedef StackAssignmentX86 StackAssignment;

} // namespace ion
} // namespace js

#endif // jsion_cpu_x86_stack_assignment_h__

