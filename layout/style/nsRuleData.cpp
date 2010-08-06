/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is Mozilla Communicator client code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Original Author: David W. Hyatt (hyatt@netscape.com)
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

#include "nsRuleData.h"
#include "nsCSSProps.h"

namespace {

struct PropertyOffsetInfo {
  // XXX These could probably be pointer-to-member, if the casting can
  // be done correctly.
  size_t struct_offset; // offset of nsRuleDataThing* in nsRuleData
  size_t member_offset; // offset of value in nsRuleDataThing
};

const PropertyOffsetInfo kOffsetTable[eCSSProperty_COUNT_no_shorthands] = {
  #define CSS_PROP_BACKENDONLY(name_, id_, method_, flags_, datastruct_,     \
                               member_, type_, kwtable_)                     \
      { size_t(-1), size_t(-1) },
  #define CSS_PROP(name_, id_, method_, flags_, datastruct_, member_, type_, \
                   kwtable_, stylestruct_, stylestructoffset_, animtype_)    \
      { offsetof(nsRuleData, m##datastruct_##Data),                          \
        offsetof(nsRuleData##datastruct_, member_) },
  #include "nsCSSPropList.h"
  #undef CSS_PROP
  #undef CSS_PROP_BACKENDONLY
};

} // anon namespace

void*
nsRuleData::StorageFor(nsCSSProperty aProperty)
{
  NS_ABORT_IF_FALSE(aProperty < eCSSProperty_COUNT_no_shorthands,
                    "invalid or shorthand property");

  const PropertyOffsetInfo& offsets = kOffsetTable[aProperty];
  NS_ABORT_IF_FALSE(offsets.struct_offset != size_t(-1),
                    "backend-only property");

  char* cssstruct = *reinterpret_cast<char**>
    (reinterpret_cast<char*>(this) + offsets.struct_offset);
  NS_ABORT_IF_FALSE(cssstruct, "substructure pointer should never be null");

  return reinterpret_cast<void*>(cssstruct + offsets.member_offset);
}
