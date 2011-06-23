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
 *   Andrew Drake <adrake@adrake.org>
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

#include <stdarg.h>

#include "JSONSpewer.h"
#include "IonLIR.h"

using namespace js;
using namespace js::ion;

void
JSONSpewer::property(const char *name)
{
    if (!fp_)
        return;

    if (!first_)
        fprintf(fp_, ",");
    fprintf(fp_, "\"%s\":", name);
    first_ = false;
}

void
JSONSpewer::beginObject()
{
    if (!fp_)
        return;

    if (!first_)
        fprintf(fp_, ",");
    fprintf(fp_, "{");
    first_ = true;
}

void
JSONSpewer::beginObjectProperty(const char *name)
{
    if (!fp_)
        return;

    property(name);
    fprintf(fp_, "{");
    first_ = true;
}

void
JSONSpewer::beginListProperty(const char *name)
{
    if (!fp_)
        return;

    property(name);
    fprintf(fp_, "[");
    first_ = true;
}

void
JSONSpewer::stringProperty(const char *name, const char *format, ...)
{
    if (!fp_)
        return;

    va_list ap;
    va_start(ap, format);

    property(name);
    fprintf(fp_, "\"");
    vfprintf(fp_, format, ap);
    fprintf(fp_, "\"");

    va_end(ap);
}

void
JSONSpewer::integerProperty(const char *name, int value)
{
    if (!fp_)
        return;

    property(name);
    fprintf(fp_, "%d", value);
}

void
JSONSpewer::integerValue(int value)
{
    if (!fp_)
        return;

    if (!first_)
        fprintf(fp_, ",");
    fprintf(fp_, "%d", value);
    first_ = false;
}

void
JSONSpewer::endObject()
{
    if (!fp_)
        return;

    fprintf(fp_, "}");
    first_ = false;
}

void
JSONSpewer::endList()
{
    if (!fp_)
        return;

    fprintf(fp_, "]");
    first_ = false;
}

bool
JSONSpewer::init(const char *path)
{
    fp_ = fopen(path, "w");
    if (!fp_)
        return false;

    beginObject();
    beginListProperty("functions");
    return true;
}

void
JSONSpewer::beginFunction(JSScript *script)
{
    beginObject();
    stringProperty("name", "%s:%d", script->filename, script->lineno);
    beginListProperty("passes");
}

void
JSONSpewer::beginPass(const char *pass)
{
    beginObject();
    stringProperty("name", pass);
}

void
JSONSpewer::spewMIR(MIRGraph *mir)
{
    if (!fp_)
        return;

    beginObjectProperty("mir");
    beginListProperty("blocks");

    for (size_t bno = 0; bno < mir->numBlocks(); bno++) {
        MBasicBlock *block = mir->getBlock(bno);
        beginObject();
        integerProperty("number", bno);

        beginListProperty("predecessors");
        for (size_t i = 0; i < block->numPredecessors(); i++)
            integerValue(block->getPredecessor(i)->id());
        endList();

        beginListProperty("successors");
        for (size_t i = 0; i < block->numSuccessors(); i++)
            integerValue(block->getSuccessor(i)->id());
        endList();

        beginListProperty("instructions");
        for (MInstructionIterator ins(block->begin());
             ins != block->end();
             ins++)
        {
            beginObject();

            integerProperty("id", ins->id());

            property("opcode");
            fprintf(fp_, "\"");
            ins->printOpcode(fp_);
            fprintf(fp_, "\"");

            beginListProperty("inputs");
            for (size_t i = 0; i < ins->numOperands(); i++)
                integerValue(ins->getInput(i)->id());
            endList();

            beginListProperty("uses");
            for (MUseIterator use(*ins); use.more(); use.next())
                integerValue(use->ins()->id());
            endList();

            endObject();
        }
        endList();

        endObject();
    }

    endList();
    endObject();
}

void
JSONSpewer::spewLIR(MIRGraph *mir)
{
    if (!fp_)
        return;

    beginObjectProperty("lir");
    beginListProperty("blocks");

    for (size_t bno = 0; bno < mir->numBlocks(); bno++) {
        LBlock *block = mir->getBlock(bno)->lir();
        if (!block)
            continue;

        beginObject();
        integerProperty("number", bno);

        beginListProperty("instructions");
        for (LInstructionIterator ins(block->begin());
             ins != block->end();
             ins++)
        {
            beginObject();

            integerProperty("id", ins->id());

            property("opcode");
            fprintf(fp_, "\"");
            ins->print(fp_);
            fprintf(fp_, "\"");

            beginListProperty("defs");
            for (size_t i = 0; i < ins->numDefs(); i++)
                integerValue(ins->getDef(i)->virtualRegister());
            endList();

            endObject();
        }
        endList();

        endObject();
    }

    endList();
    endObject();
}

void
JSONSpewer::endPass()
{
    endObject();
}

void
JSONSpewer::endFunction()
{
    endList();
    endObject();
}

void
JSONSpewer::finish()
{
    if (!fp_)
        return;

    endList();
    endObject();
    fprintf(fp_, "\n");
}

