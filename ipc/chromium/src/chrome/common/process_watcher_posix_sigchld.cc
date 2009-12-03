/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: set ts=8 et sw=2 tw=80: 
 */
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
 * The Original Code is Mozilla IPC.
 *
 * The Initial Developer of the Original Code is
 *   Mozilla Foundation
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Chris Jones <jones.chris.g@gmail.com>
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

//-----------------------------------------------------------------------------
//   XXXXXXXXXXXXXXXX
//
// How is this code supposed to be licensed?  I don't /think/ that
// this code is doing anything different than, say,
// GeckoChildProcess.h/cpp, so I /think/ this gets a MoFo copyright
// and license.  Yes?
//
//   XXXXXXXXXXXXXXXX
//-----------------------------------------------------------------------------

#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "base/eintr_wrapper.h"
#include "base/message_loop.h"
#include "base/process_util.h"

#include "chrome/common/process_watcher.h"

// Maximum amount of time (in milliseconds) to wait for the process to exit.
// XXX/cjones: fairly arbitrary, chosen to match process_watcher_win.cc
static const int kMaxWaitMs = 2000;

namespace {

bool
IsProcessDead(pid_t process)
{
  bool exited = false;
  // don't care if the process crashed, just if it exited
  base::DidProcessCrash(&exited, process);
  return exited;
}

// Fear the reaper
class ChildReaper : public Task,
                    public base::MessagePumpLibevent::SignalEvent,
                    public base::MessagePumpLibevent::SignalWatcher
{
public:
  explicit ChildReaper(pid_t process) : process_(process)
  {
  } 

  virtual ~ChildReaper()
  {
    if (process_)
      KillProcess();
    // StopCatching() is implicit
  }

  // @override
  virtual void OnSignal(int sig)
  {
    DCHECK(SIGCHLD == sig);
    DCHECK(process_);

    // this may be the SIGCHLD for a process other than |process_|
    if (IsProcessDead(process_)) {
      process_ = 0;
      StopCatching();
    }
  }

  // @override
  virtual void Run()
  {
    // we may have already been signaled by the time this runs
    if (process_)
      KillProcess();
  }

private:
  void KillProcess()
  {
    DCHECK(process_);

    if (IsProcessDead(process_)) {
      process_ = 0;
      return;
    }

    if (0 == kill(process_, SIGKILL)) {
      // XXX this will block for whatever amount of time it takes the
      // XXX OS to tear down the process's resources.  might need to
      // XXX rethink this if it proves expensive
      HANDLE_EINTR(waitpid(process_, NULL, 0));
    }
    else {
      LOG(ERROR) << "Failed to deliver SIGKILL to " << process_ << "!"
                 << "("<< errno << ").";
    }
    process_ = 0;
  }

  pid_t process_;

  DISALLOW_EVIL_CONSTRUCTORS(ChildReaper);
};

}  // namespace <anon>


void
ProcessWatcher::EnsureProcessTerminated(base::ProcessHandle process)
{
  DCHECK(process != base::GetCurrentProcId());
  DCHECK(process > 0);

  if (IsProcessDead(process))
    return;

  MessageLoopForIO* loop = MessageLoopForIO::current();
  ChildReaper* reaper = new ChildReaper(process);

  // there are three ways |process| will be reaped:
  //  (1) catch SIGCHLD after its death (common case)
  //  (2) kMaxWaitMs timeout fires, |kill(SIGKILL)|
  //  (3) shutdown before (1) or (2), |reaper| dtor does |kill(SIGKILL)|
  loop->CatchSignal(SIGCHLD, reaper, reaper);
  // |loop| takes ownership of |reaper|
  loop->PostDelayedTask(FROM_HERE, reaper, kMaxWaitMs);
}
