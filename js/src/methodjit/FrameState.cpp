/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=4 sw=4 et tw=99:
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
 * The Original Code is Mozilla SpiderMonkey JavaScript 1.9 code, released
 * May 28, 2008.
 *
 * The Initial Developer of the Original Code is
 *   Brendan Eich <brendan@mozilla.org>
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
#include "jscntxt.h"
#include "FrameState.h"

using namespace js;
using namespace js::mjit;

bool
FrameState::init(uint32 nargs)
{
    base = (FrameEntry *)cx->malloc(sizeof(FrameEntry) * (script->nslots + nargs));
    if (!base)
        return false;

    memset(base, 0, sizeof(FrameEntry) * (script->nslots + nargs));
    memset(regstate, 0, sizeof(regstate));

    args = base;
    locals = args + nargs;
    sp = locals + script->nfixed;

    return true;
}

void
FrameState::evictSomething()
{
}

FrameState::~FrameState()
{
    cx->free(base);
}

void
FrameState::assertValidRegisterState()
{
    Registers temp;

    RegisterID reg;
    uint32 index = 0;
    for (FrameEntry *fe = base; fe < sp; fe++, index++) {
        if (fe->type.inRegister()) {
            reg = fe->type.reg();
            temp.allocSpecific(reg);
            JS_ASSERT(regstate[reg].tracked);
            JS_ASSERT(regstate[reg].index == index);
            JS_ASSERT(regstate[reg].part == RegState::Part_Type);
        }
        if (fe->data.inRegister()) {
            reg = fe->data.reg();
            temp.allocSpecific(reg);
            JS_ASSERT(regstate[reg].tracked);
            JS_ASSERT(regstate[reg].index == index);
            JS_ASSERT(regstate[reg].part == RegState::Part_Data);
        }
    }

    JS_ASSERT(temp == regalloc);
}

void
FrameState::invalidate(FrameEntry *fe)
{
    if (!fe->type.synced()) {
        JS_NOT_REACHED("wat");
    }
    if (!fe->data.synced()) {
        JS_NOT_REACHED("wat2");
    }
    fe->type.setMemory();
    fe->data.setMemory();
    fe->copies = 0;
}

void
FrameState::flush()
{
    for (FrameEntry *fe = base; fe < sp; fe++)
        invalidate(fe);
}

