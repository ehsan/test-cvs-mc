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
 * The Original Code is the Mozilla GNOME integration code.
 *
 * The Initial Developer of the Original Code is
 * IBM Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Brian Ryner <bryner@brianryner.com>
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

#include "nsGnomeVFSService.h"
#include "nsStringAPI.h"
#include "nsIURI.h"
#include "nsTArray.h"
#include "nsIStringEnumerator.h"
#include "nsAutoPtr.h"

extern "C" {
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
}

class nsGnomeVFSMimeApp : public nsIGnomeVFSMimeApp
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGNOMEVFSMIMEAPP

  nsGnomeVFSMimeApp(GnomeVFSMimeApplication* aApp) : mApp(aApp) {}
  ~nsGnomeVFSMimeApp() { gnome_vfs_mime_application_free(mApp); }

private:
  GnomeVFSMimeApplication *mApp;
};

NS_IMPL_ISUPPORTS1(nsGnomeVFSMimeApp, nsIGnomeVFSMimeApp)

NS_IMETHODIMP
nsGnomeVFSMimeApp::GetId(nsACString& aId)
{
  aId.Assign(mApp->id);
  return NS_OK;
}

NS_IMETHODIMP
nsGnomeVFSMimeApp::GetName(nsACString& aName)
{
  aName.Assign(mApp->name);
  return NS_OK;
}

NS_IMETHODIMP
nsGnomeVFSMimeApp::GetCommand(nsACString& aCommand)
{
  aCommand.Assign(mApp->command);
  return NS_OK;
}

NS_IMETHODIMP
nsGnomeVFSMimeApp::GetCanOpenMultipleFiles(PRBool* aCanOpen)
{
  *aCanOpen = mApp->can_open_multiple_files;
  return NS_OK;
}

NS_IMETHODIMP
nsGnomeVFSMimeApp::GetExpectsURIs(PRInt32* aExpects)
{
  *aExpects = mApp->expects_uris;
  return NS_OK;
}

NS_IMETHODIMP
nsGnomeVFSMimeApp::Launch(const nsACString &aUri)
{
  char *uri = gnome_vfs_make_uri_from_input(PromiseFlatCString(aUri).get());

  if (! uri)
    return NS_ERROR_FAILURE;

  GList uris = { 0 };
  uris.data = uri;

  GnomeVFSResult result = gnome_vfs_mime_application_launch(mApp, &uris);
  g_free(uri);

  if (result != GNOME_VFS_OK)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

class UTF8StringEnumerator : public nsIUTF8StringEnumerator
{
public:
  UTF8StringEnumerator() : mIndex(0) { }
  ~UTF8StringEnumerator() { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIUTF8STRINGENUMERATOR

  nsTArray<nsCString> mStrings;
  PRUint32            mIndex;
};

NS_IMPL_ISUPPORTS1(UTF8StringEnumerator, nsIUTF8StringEnumerator)

NS_IMETHODIMP
UTF8StringEnumerator::HasMore(PRBool *aResult)
{
  *aResult = mIndex < mStrings.Length();
  return NS_OK;
}

NS_IMETHODIMP
UTF8StringEnumerator::GetNext(nsACString& aResult)
{
  if (mIndex >= mStrings.Length())
    return NS_ERROR_UNEXPECTED;

  aResult.Assign(mStrings[mIndex]);
  ++mIndex;
  return NS_OK;
}

NS_IMETHODIMP
nsGnomeVFSMimeApp::GetSupportedURISchemes(nsIUTF8StringEnumerator** aSchemes)
{
  *aSchemes = nsnull;

  nsRefPtr<UTF8StringEnumerator> array = new UTF8StringEnumerator();
  NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);

  for (GList *list = mApp->supported_uri_schemes; list; list = list->next) {
    if (!array->mStrings.AppendElement((char*) list->data)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  NS_ADDREF(*aSchemes = array);
  return NS_OK;
}

NS_IMETHODIMP
nsGnomeVFSMimeApp::GetRequiresTerminal(PRBool* aRequires)
{
  *aRequires = mApp->requires_terminal;
  return NS_OK;
}

nsresult
nsGnomeVFSService::Init()
{
  return gnome_vfs_init() ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMPL_ISUPPORTS1(nsGnomeVFSService, nsIGnomeVFSService)

NS_IMETHODIMP
nsGnomeVFSService::GetMimeTypeFromExtension(const nsACString &aExtension,
                                            nsACString& aMimeType)
{
  nsCAutoString fileExtToUse(".");
  fileExtToUse.Append(aExtension);

  const char *mimeType = gnome_vfs_mime_type_from_name(fileExtToUse.get());
  aMimeType.Assign(mimeType);

  // |mimeType| points to internal gnome-vfs data, so don't free it.

  return NS_OK;
}

NS_IMETHODIMP
nsGnomeVFSService::GetAppForMimeType(const nsACString &aMimeType,
                                     nsIGnomeVFSMimeApp** aApp)
{
  *aApp = nsnull;
  GnomeVFSMimeApplication *app =
   gnome_vfs_mime_get_default_application(PromiseFlatCString(aMimeType).get());

  if (app) {
    nsGnomeVFSMimeApp *mozApp = new nsGnomeVFSMimeApp(app);
    NS_ENSURE_TRUE(mozApp, NS_ERROR_OUT_OF_MEMORY);

    NS_ADDREF(*aApp = mozApp);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGnomeVFSService::GetDescriptionForMimeType(const nsACString &aMimeType,
                                             nsACString& aDescription)
{
  const char *desc =
    gnome_vfs_mime_get_description(PromiseFlatCString(aMimeType).get());
  aDescription.Assign(desc);

  // |desc| points to internal gnome-vfs data, so don't free it.

  return NS_OK;
}

NS_IMETHODIMP
nsGnomeVFSService::ShowURI(nsIURI *aURI)
{
  nsCAutoString spec;
  aURI->GetSpec(spec);

  if (gnome_vfs_url_show_with_env(spec.get(), NULL) == GNOME_VFS_OK)
    return NS_OK;

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsGnomeVFSService::ShowURIForInput(const nsACString &aUri)
{
  char* spec = gnome_vfs_make_uri_from_input(PromiseFlatCString(aUri).get());
  nsresult rv = NS_ERROR_FAILURE;

  if (gnome_vfs_url_show_with_env(spec, NULL) == GNOME_VFS_OK)
    rv = NS_OK;

  g_free(spec);
  return rv;
}
