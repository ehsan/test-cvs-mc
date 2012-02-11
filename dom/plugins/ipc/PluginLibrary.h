/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: sw=4 ts=4 et :
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
 * The Original Code is Mozilla Foundation.
 *
 * The Initial Developer of the Original Code is
 *   Josh Aas <josh@mozilla.com>
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

#ifndef mozilla_PluginLibrary_h
#define mozilla_PluginLibrary_h 1

#include "prlink.h"
#include "npapi.h"
#include "npfunctions.h"
#include "nscore.h"
#include "nsTArray.h"
#include "nsPluginError.h"

class gfxASurface;
class gfxContext;
class nsCString;
struct nsIntRect;
struct nsIntSize;
class nsNPAPIPlugin;
class nsGUIEvent;

namespace mozilla {
namespace layers {
class Image;
class ImageContainer;
}
}

using namespace mozilla::layers;

namespace mozilla {

class PluginLibrary
{
public:
  virtual ~PluginLibrary() { }

  /**
   * Inform this library about the nsNPAPIPlugin which owns it. This
   * object will hold a weak pointer to the plugin.
   */
  virtual void SetPlugin(nsNPAPIPlugin* plugin) = 0;

  virtual bool HasRequiredFunctions() = 0;

#if defined(XP_UNIX) && !defined(XP_MACOSX) && !defined(MOZ_WIDGET_GONK)
  virtual nsresult NP_Initialize(NPNetscapeFuncs* bFuncs, NPPluginFuncs* pFuncs, NPError* error) = 0;
#else
  virtual nsresult NP_Initialize(NPNetscapeFuncs* bFuncs, NPError* error) = 0;
#endif
  virtual nsresult NP_Shutdown(NPError* error) = 0;
  virtual nsresult NP_GetMIMEDescription(const char** mimeDesc) = 0;
  virtual nsresult NP_GetValue(void *future, NPPVariable aVariable,
                               void *aValue, NPError* error) = 0;
#if defined(XP_WIN) || defined(XP_MACOSX) || defined(XP_OS2)
  virtual nsresult NP_GetEntryPoints(NPPluginFuncs* pFuncs, NPError* error) = 0;
#endif
  virtual nsresult NPP_New(NPMIMEType pluginType, NPP instance,
                           uint16_t mode, int16_t argc, char* argn[],
                           char* argv[], NPSavedData* saved,
                           NPError* error) = 0;

  virtual nsresult NPP_ClearSiteData(const char* site, uint64_t flags,
                                     uint64_t maxAge) = 0;
  virtual nsresult NPP_GetSitesWithData(InfallibleTArray<nsCString>& aResult) = 0;

  virtual nsresult AsyncSetWindow(NPP instance, NPWindow* window) = 0;
  virtual nsresult GetImageContainer(NPP instance, ImageContainer** aContainer) = 0;
  virtual nsresult GetImageSize(NPP instance, nsIntSize* aSize) = 0;
  virtual bool UseAsyncPainting() = 0;
#if defined(XP_MACOSX)
  virtual nsresult IsRemoteDrawingCoreAnimation(NPP instance, bool *aDrawing) = 0;
#endif

  /**
   * The next three methods are the third leg in the trip to
   * PluginInstanceParent.  They approximately follow the ReadbackSink
   * API.
   */
  virtual nsresult SetBackgroundUnknown(NPP instance) = 0;
  virtual nsresult BeginUpdateBackground(NPP instance,
                                         const nsIntRect&, gfxContext**) = 0;
  virtual nsresult EndUpdateBackground(NPP instance,
                                       gfxContext*, const nsIntRect&) = 0;
#if defined(MOZ_WIDGET_QT) && (MOZ_PLATFORM_MAEMO == 6)
  virtual nsresult HandleGUIEvent(NPP instance, const nsGUIEvent&, bool*) = 0;
#endif
};


} // namespace mozilla

#endif  // ifndef mozilla_PluginLibrary_h
