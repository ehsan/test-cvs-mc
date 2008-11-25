// Copyright 2006 Google Inc. All Rights Reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


// This file contains the definitions for a DWARF2/3 information
// collector that uses the DWARF2/3 reader interface to build a mapping
// of addresses to files, lines, and functions.

#ifndef COMMON_MAC_DWARF_FUNCTIONINFO_H__
#define COMMON_MAC_DWARF_FUNCTIONINFO_H__

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "common/mac/dwarf/dwarf2reader.h"


namespace dwarf2reader {

struct FunctionInfo {
  // Name of the function
  string name;
  // File containing this function
  string file;
  // Line number for start of function.
  uint32 line;
  // Beginning address for this function
  uint64 lowpc;
  // End address for this function.
  uint64 highpc;
};

struct SourceFileInfo {
  // Name of the source file name
  string name;
  // Low address of source file name
  uint64 lowpc;
};

typedef map<uint64, FunctionInfo*> FunctionMap;
typedef map<uint64, pair<string, uint32> > LineMap;

// This class is a basic line info handler that fills in the dirs,
// file, and linemap passed into it with the data produced from the
// LineInfoHandler.
class CULineInfoHandler: public LineInfoHandler {
 public:

  //
  CULineInfoHandler(vector<SourceFileInfo>* files,
                    vector<string>* dirs,
                    LineMap* linemap);
  virtual ~CULineInfoHandler() { }

  // Called when we define a directory.  We just place NAME into dirs_
  // at position DIR_NUM.
  virtual void DefineDir(const string& name, uint32 dir_num);

  // Called when we define a filename.  We just place
  // concat(dirs_[DIR_NUM], NAME) into files_ at position FILE_NUM.
  virtual void DefineFile(const string& name, int32 file_num,
                          uint32 dir_num, uint64 mod_time, uint64 length);


  // Called when the line info reader has a new line, address pair
  // ready for us.  ADDRESS is the address of the code, FILE_NUM is
  // the file number containing the code, LINE_NUM is the line number
  // in that file for the code, and COLUMN_NUM is the column number
  // the code starts at, if we know it (0 otherwise).
  virtual void AddLine(uint64 address, uint32 file_num, uint32 line_num,
                       uint32 column_num);


 private:
  LineMap* linemap_;
  vector<SourceFileInfo>* files_;
  vector<string>* dirs_;
};

class CUFunctionInfoHandler: public Dwarf2Handler {
 public:
  CUFunctionInfoHandler(vector<SourceFileInfo>* files,
                        vector<string>* dirs,
                        LineMap* linemap,
                        FunctionMap* offset_to_funcinfo,
                        FunctionMap* address_to_funcinfo,
                        CULineInfoHandler* linehandler,
                        const SectionMap& sections,
                        ByteReader* reader)
      : files_(files), dirs_(dirs), linemap_(linemap),
        offset_to_funcinfo_(offset_to_funcinfo),
        address_to_funcinfo_(address_to_funcinfo),
        linehandler_(linehandler), sections_(sections),
        reader_(reader), current_function_info_(NULL) { }

  virtual ~CUFunctionInfoHandler() { }

  // Start to process a compilation unit at OFFSET from the beginning of the
  // debug_info section.  We want to see all compilation units, so we
  // always return true.

  virtual bool StartCompilationUnit(uint64 offset, uint8 address_size,
                                    uint8 offset_size, uint64 cu_length,
                                    uint8 dwarf_version);

  // Start to process a DIE at OFFSET from the beginning of the
  // debug_info section.  We only care about function related DIE's.
  virtual bool StartDIE(uint64 offset, enum DwarfTag tag,
                        const AttributeList& attrs);

  // Called when we have an attribute with unsigned data to give to
  // our handler.  The attribute is for the DIE at OFFSET from the
  // beginning of compilation unit, has a name of ATTR, a form of
  // FORM, and the actual data of the attribute is in DATA.
  virtual void ProcessAttributeUnsigned(uint64 offset,
                                        enum DwarfAttribute attr,
                                        enum DwarfForm form,
                                        uint64 data);

  // Called when we have an attribute with string data to give to
  // our handler.  The attribute is for the DIE at OFFSET from the
  // beginning of compilation unit, has a name of ATTR, a form of
  // FORM, and the actual data of the attribute is in DATA.
  virtual void ProcessAttributeString(uint64 offset,
                                      enum DwarfAttribute attr,
                                      enum DwarfForm form,
                                      const string& data);

  // Called when finished processing the DIE at OFFSET.
  // Because DWARF2/3 specifies a tree of DIEs, you may get starts
  // before ends of the previous DIE, as we process children before
  // ending the parent.
  virtual void EndDIE(uint64 offset);

 private:
  vector<SourceFileInfo>* files_;
  vector<string>* dirs_;
  LineMap* linemap_;
  FunctionMap* offset_to_funcinfo_;
  FunctionMap* address_to_funcinfo_;
  CULineInfoHandler* linehandler_;
  const SectionMap& sections_;
  ByteReader* reader_;
  FunctionInfo* current_function_info_;
};

}  // namespace dwarf2reader
#endif  // COMMON_MAC_DWARF_FUNCTIONINFO_H__
