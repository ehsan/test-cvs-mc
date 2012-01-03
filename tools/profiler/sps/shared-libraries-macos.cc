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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Benoit Girard <bgirard@mozilla.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#include <AvailabilityMacros.h>
#include <mach-o/loader.h>
#include <mach-o/dyld_images.h>
#include <mach/task_info.h>
#include <mach/task.h>
#include <mach/mach_init.h>
#include <mach/mach_traps.h>
#include <string.h>
#include <stdlib.h>
#include <vector>

#include "shared-libraries.h"

#ifndef MAC_OS_X_VERSION_10_6
#define MAC_OS_X_VERSION_10_6 1060
#endif

#if MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_6
// borrowed from Breakpad
// Fallback declarations for TASK_DYLD_INFO and friends, introduced in
// <mach/task_info.h> in the Mac OS X 10.6 SDK.
#define TASK_DYLD_INFO 17
struct task_dyld_info {
    mach_vm_address_t all_image_info_addr;
    mach_vm_size_t all_image_info_size;
  };
typedef struct task_dyld_info task_dyld_info_data_t;
typedef struct task_dyld_info *task_dyld_info_t;
#define TASK_DYLD_INFO_COUNT (sizeof(task_dyld_info_data_t) / sizeof(natural_t))

#endif

// Architecture specific abstraction.
#ifdef __i386__
typedef mach_header platform_mach_header;
typedef segment_command mach_segment_command_type;
#define MACHO_MAGIC_NUMBER MH_MAGIC
#define CMD_SEGMENT LC_SEGMENT
#define seg_size uint32_t
#else
typedef mach_header_64 platform_mach_header;
typedef segment_command_64 mach_segment_command_type;
#define MACHO_MAGIC_NUMBER MH_MAGIC_64
#define CMD_SEGMENT LC_SEGMENT_64
#define seg_size uint64_t
#endif

static
void addSharedLibrary(const platform_mach_header* header, char *name, SharedLibraryInfo &info) {
  const struct load_command *cmd =
    reinterpret_cast<const struct load_command *>(header + 1);

  seg_size size;
  // Find the cmd segment in the macho image. It will contain the offset we care about.
  for (unsigned int i = 0; cmd && (i < header->ncmds); ++i) {
    if (cmd->cmd == CMD_SEGMENT) {
      const mach_segment_command_type *seg =
        reinterpret_cast<const mach_segment_command_type *>(cmd);

      if (!strcmp(seg->segname, "__TEXT")) {
        size = seg->vmsize;
        unsigned long long start = reinterpret_cast<unsigned long long>(header);
        info.AddSharedLibrary(SharedLibrary(start, start+seg->vmsize, seg->vmsize, name));
        return;
      }
    }

    cmd = reinterpret_cast<const struct load_command *>
      (reinterpret_cast<const char *>(cmd) + cmd->cmdsize);
  }
}

// Use dyld to inspect the macho image information. We can build the SharedLibraryEntry structure
// giving us roughtly the same info as /proc/PID/maps in Linux.
SharedLibraryInfo SharedLibraryInfo::GetInfoForSelf()
{
  SharedLibraryInfo sharedLibraryInfo;

  task_dyld_info_data_t task_dyld_info;
  mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
  if (task_info(mach_task_self (), TASK_DYLD_INFO, (task_info_t)&task_dyld_info,
                &count) != KERN_SUCCESS) {
    return sharedLibraryInfo;
  }

  struct dyld_all_image_infos* aii = (struct dyld_all_image_infos*)task_dyld_info.all_image_info_addr;
  size_t infoCount = aii->infoArrayCount;

  // Iterate through all dyld images (loaded libraries) to get their names
  // and offests.
  for (size_t i = 0; i < infoCount; ++i) {
    const dyld_image_info *info = &aii->infoArray[i];

    // If the magic number doesn't match then go no further
    // since we're not pointing to where we think we are.
    if (info->imageLoadAddress->magic != MACHO_MAGIC_NUMBER) {
      continue;
    }

    const platform_mach_header* header =
      reinterpret_cast<const platform_mach_header*>(info->imageLoadAddress);

    // Add the entry for this image.
    addSharedLibrary(header, (char*)info->imageFilePath, sharedLibraryInfo);

  }
  return sharedLibraryInfo;
}

