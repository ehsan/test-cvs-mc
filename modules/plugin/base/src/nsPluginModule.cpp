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
 *   Pierre Phaneuf <pp@ludusdesign.com>
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

#include "nsIGenericFactory.h"
#include "nsIPluginManager.h"
#include "nsPluginsCID.h"
#include "nsPluginHostImpl.h"
#include "ns4xPlugin.h"
#include "nsJVMAuthTools.h"

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsPluginHostImpl,
                                         nsPluginHostImpl::GetInst)
NS_GENERIC_AGGREGATED_CONSTRUCTOR(nsJVMAuthTools)

static const nsModuleComponentInfo gComponentInfo[] = {
  { "Plugin Host",
    NS_PLUGIN_HOST_CID,
    "@mozilla.org/plugin/host;1",
    nsPluginHostImplConstructor
  },
  { "Plugin Manager",
    NS_PLUGINMANAGER_CID,
    "@mozilla.org/plugin/manager;1",
    nsPluginHostImplConstructor
  },
  { "JVM Authentication Service", 
    NS_JVMAUTHTOOLS_CID,  
    "@mozilla.org/oji/jvm-auth-tools;1", 
    nsJVMAuthToolsConstructor
  }
};

NS_IMPL_NSGETMODULE(nsPluginModule, gComponentInfo)
