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
 * The Original Code is Android Sync Client.
 *
 * The Initial Developer of the Original Code is
 * the Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Richard Newman <rnewman@mozilla.com>
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

package org.mozilla.gecko.sync.stage;

import java.net.URISyntaxException;

import org.mozilla.gecko.sync.GlobalSession;
import org.mozilla.gecko.sync.MetaGlobal;
import org.mozilla.gecko.sync.delegates.MetaGlobalDelegate;
import org.mozilla.gecko.sync.net.SyncStorageResponse;

public class FetchMetaGlobalStage implements GlobalSyncStage {

  public class StageMetaGlobalDelegate implements MetaGlobalDelegate {

    private GlobalSession session;
    public StageMetaGlobalDelegate(GlobalSession session) {
      this.session = session;
    }

    @Override
    public void handleSuccess(MetaGlobal global, SyncStorageResponse response) {
      session.processMetaGlobal(global);
    }

    @Override
    public void handleFailure(SyncStorageResponse response) {
      session.handleHTTPError(response, "Failure fetching meta/global.");
    }

    @Override
    public void handleError(Exception e) {
      session.abort(e, "Failure fetching meta/global.");
    }

    @Override
    public void handleMissing(MetaGlobal global, SyncStorageResponse response) {
      session.processMissingMetaGlobal(global);
    }

    @Override
    public MetaGlobalDelegate deferred() {
      // TODO: defer!
      return this;
    }
  }

  @Override
  public void execute(GlobalSession session) throws NoSuchStageException {
    try {
      session.fetchMetaGlobal(new StageMetaGlobalDelegate(session));
    } catch (URISyntaxException e) {
      session.abort(e, "Invalid URI.");
    }
  }

}
