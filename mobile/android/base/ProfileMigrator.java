/* -*- Mode: Java; c-basic-offset: 4; tab-width: 20; indent-tabs-mode: nil; -*-
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
 * The Original Code is Mozilla Android code.
 *
 * The Initial Developer of the Original Code is Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2011-2012
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Gian-Carlo Pascutto <gpascutto@mozilla.com>
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

package org.mozilla.gecko;

import org.mozilla.gecko.db.BrowserContract;
import org.mozilla.gecko.db.BrowserContract.Bookmarks;
import org.mozilla.gecko.db.BrowserContract.History;
import org.mozilla.gecko.db.BrowserContract.ImageColumns;
import org.mozilla.gecko.db.BrowserContract.Images;
import org.mozilla.gecko.db.BrowserContract.URLColumns;
import org.mozilla.gecko.db.BrowserContract.SyncColumns;
import org.mozilla.gecko.db.BrowserDB;
import org.mozilla.gecko.sqlite.ByteBufferInputStream;
import org.mozilla.gecko.sqlite.SQLiteBridge;
import org.mozilla.gecko.sqlite.SQLiteBridgeException;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.SQLException;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.provider.Browser;
import android.util.Log;
import android.net.Uri;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.File;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class ProfileMigrator {
    private static final String LOGTAG = "ProfMigr";
    private File mProfileDir;
    private ContentResolver mCr;

    /*
       These queries are derived from the low-level Places schema
       https://developer.mozilla.org/en/The_Places_database
    */
    private final String kRootQuery =
        "SELECT root_name, folder_id FROM moz_bookmarks_roots";
    private final String kRootName     = "root_name";
    private final String kRootFolderId = "folder_id";

    private final String kBookmarkQuery =
        "SELECT places.url             AS p_url,"         +
        "       bookmark.guid          AS b_guid,"        +
        "       bookmark.id            AS b_id,"          +
        "       bookmark.title         AS b_title,"       +
        "       bookmark.type          AS b_type,"        +
        "       bookmark.parent        AS b_parent,"      +
        "       bookmark.dateAdded     AS b_added,"       +
        "       bookmark.lastModified  AS b_modified,"    +
        "       bookmark.position      AS b_position,"    +
        "       favicon.data           AS f_data,"        +
        "       favicon.mime_type      AS f_mime_type,"   +
        "       favicon.url            AS f_url,"         +
        "       favicon.guid           AS f_guid "        +
        "FROM ((moz_bookmarks AS bookmark "               +
        "       LEFT OUTER JOIN moz_places AS places "    +
        "       ON places.id = bookmark.fk) "             +
        "       LEFT OUTER JOIN moz_favicons AS favicon " +
        "       ON places.favicon_id = favicon.id) "      +
        // Bookmark folders don't have a places entry.
        "WHERE (places.hidden IS NULL "                   +
        "       OR places.hidden <> 1) "                  +
        // This gives us a better chance of adding a folder before
        // adding its contents and hence avoiding extra iterations below.
        "ORDER BY bookmark.id";

    // Result column of relevant data
    private final String kBookmarkUrl      = "p_url";
    private final String kBookmarkTitle    = "b_title";
    private final String kBookmarkGuid     = "b_guid";
    private final String kBookmarkId       = "b_id";
    private final String kBookmarkType     = "b_type";
    private final String kBookmarkParent   = "b_parent";
    private final String kBookmarkAdded    = "b_added";
    private final String kBookmarkModified = "b_modified";
    private final String kBookmarkPosition = "b_position";
    private final String kFaviconData      = "f_data";
    private final String kFaviconMime      = "f_mime_type";
    private final String kFaviconUrl       = "f_url";
    private final String kFaviconGuid      = "f_guid";

    // Helper constants
    private static final int kPlacesTypeBookmark = 1;
    private static final int kPlacesTypeFolder   = 2;

    /*
      The sort criterion here corresponds to the one used for the
      Awesomebar results. It's a simplification of Frecency.
      We must divide date by 1000 due to the micro (Places)
      vs milli (Android) distiction.
    */
    private final String kHistoryQuery =
        "SELECT places.url              AS p_url, "       +
        "       places.title            AS p_title, "     +
        "       MAX(history.visit_date) AS h_date, "      +
        "       COUNT(*) AS h_visits, "                   +
        // see BrowserDB.filterAllSites for this formula
        "       MAX(1, (((MAX(history.visit_date)/1000) - ?) / 86400000 + 120)) AS a_recent, " +
        "       favicon.data            AS f_data, "      +
        "       favicon.mime_type       AS f_mime_type, " +
        "       places.guid             AS p_guid, "      +
        "       favicon.url             AS f_url, "       +
        "       favicon.guid            AS f_guid "       +
        "FROM (moz_historyvisits AS history "             +
        "      JOIN moz_places AS places "                +
        "      ON places.id = history.place_id "          +
        // Add favicon data if a favicon is present for this URL.
        "      LEFT OUTER JOIN moz_favicons AS favicon "  +
        "      ON places.favicon_id = favicon.id) "       +
        "WHERE places.hidden <> 1 "                       +
        "GROUP BY p_url "                                 +
        "ORDER BY h_visits * a_recent "                   +
        "DESC LIMIT ?";

    private final String kHistoryUrl    = "p_url";
    private final String kHistoryTitle  = "p_title";
    private final String kHistoryGuid   = "p_guid";
    private final String kHistoryDate   = "h_date";
    private final String kHistoryVisits = "h_visits";

    public ProfileMigrator(ContentResolver cr, File profileDir) {
        mProfileDir = profileDir;
        mCr = cr;
    }

    public void launch() {
        new PlacesTask().run();
    }

    private class PlacesTask implements Runnable {
        private Map<Long, Long> mRerootMap;

        protected Uri getBookmarksUri() {
            return Bookmarks.CONTENT_URI;
        }

        protected Uri getHistoryUri() {
            return History.CONTENT_URI;
        }

        protected Uri getImagesUri() {
            return Images.CONTENT_URI;
        }

        private long getFolderId(String guid) {
            Cursor c = null;

            try {
                c = mCr.query(getBookmarksUri(),
                              new String[] { Bookmarks._ID },
                              Bookmarks.GUID + " = ?",
                              new String [] { guid },
                              null);
                if (c.moveToFirst())
                    return c.getLong(c.getColumnIndexOrThrow(Bookmarks._ID));
            } finally {
                if (c != null)
                    c.close();
            }
            // Default fallback
            return Bookmarks.FIXED_ROOT_ID;
        }

        // We want to know the id of special root folders in the places DB,
        // and replace them by the corresponding root id in the Android DB.
        protected void calculateReroot(SQLiteBridge db) {
            mRerootMap = new HashMap<Long, Long>();

            try {
                ArrayList<Object[]> queryResult = db.query(kRootQuery);
                final int rootCol = db.getColumnIndex(kRootName);
                final int folderCol = db.getColumnIndex(kRootFolderId);

                for (Object[] resultRow: queryResult) {
                    String name = (String)resultRow[rootCol];
                    long placesFolderId = Integer.parseInt((String)resultRow[folderCol]);
                    mRerootMap.put(placesFolderId, getFolderId(name));
                    Log.v(LOGTAG, "Name: " + name + ", pid=" + placesFolderId
                          + ", nid=" + mRerootMap.get(placesFolderId));
                }
            } catch (SQLiteBridgeException e) {
                Log.e(LOGTAG, "Failed to get bookmark roots: ", e);
                return;
            }
        }

        // Get a list of the last times an URL was accessed
        protected Map<String, Long> gatherBrowserDBHistory() {
            Map<String, Long> history = new HashMap<String, Long>();

            Cursor cursor =
                BrowserDB.getRecentHistory(mCr, BrowserDB.getMaxHistoryCount());
            final int urlCol =
                cursor.getColumnIndexOrThrow(BrowserDB.URLColumns.URL);
            final int dateCol =
                cursor.getColumnIndexOrThrow(BrowserDB.URLColumns.DATE_LAST_VISITED);

            cursor.moveToFirst();
            while (!cursor.isAfterLast()) {
                String url = cursor.getString(urlCol);
                Long date = cursor.getLong(dateCol);
                // getRecentHistory returns newest-to-oldest, which means
                // we remember the most recent access
                if (!history.containsKey(url)) {
                    history.put(url, date);
                }
                cursor.moveToNext();
            }
            cursor.close();

            return history;
        }

        protected void addHistory(Map<String, Long> browserDBHistory,
                                  String url, String title, long date, int visits) {
            boolean allowUpdate = false;

            if (!browserDBHistory.containsKey(url)) {
                // BrowserDB doesn't know the URL, allow it to be
                // inserted with places date.
                allowUpdate = true;
            } else {
                long androidDate = browserDBHistory.get(url);
                if (androidDate < date) {
                    // Places URL hit is newer than BrowserDB,
                    // allow it to be updated with places date.
                    allowUpdate = true;
                }
            }

            if (allowUpdate) {
                updateBrowserHistory(url, title, date, visits);
            }
        }

        protected void updateBrowserHistory(String url, String title,
                                            long date, int visits) {
            Cursor cursor = null;

            try {
                final String[] projection = new String[] {
                    History._ID,
                    History.VISITS
                };

                cursor = mCr.query(getHistoryUri(),
                                   projection,
                                   History.URL + " = ?",
                                   new String[] { url },
                                   null);

                ContentValues values = new ContentValues();
                values.put(History.DATE_LAST_VISITED, date);

                if (cursor.moveToFirst()) {
                    int visitsCol = cursor.getColumnIndexOrThrow(History.VISITS);
                    int oldVisits = cursor.getInt(visitsCol);

                    values.put(History.VISITS, oldVisits + visits);
                    if (title != null) {
                        values.put(History.TITLE, title);
                    }

                    int idCol = cursor.getColumnIndexOrThrow(History._ID);
                    // We use default profile anyway
                    Uri historyUri = ContentUris.withAppendedId(getHistoryUri(),
                                                                cursor.getLong(idCol));

                    mCr.update(historyUri, values, null, null);
                } else {
                    values.put(History.URL, url);
                    values.put(History.VISITS, visits);
                    if (title != null) {
                        values.put(History.TITLE, title);
                    } else {
                        values.put(History.TITLE, url);
                    }

                    mCr.insert(getHistoryUri(), values);
                }
            } finally {
                if (cursor != null)
                    cursor.close();
            }
        }

        protected BitmapDrawable decodeImageData(ByteBuffer data) {
            ByteBufferInputStream byteStream = new ByteBufferInputStream(data);
            BitmapDrawable image =
                (BitmapDrawable)Drawable.createFromStream(byteStream, "src");
            return image;
        }

        protected void addFavicon(String url, String faviconUrl, String faviconGuid,
                                  String mime, ByteBuffer data) {
            // Some GIFs can cause us to lock up completely
            // without exceptions or anything. Not cool.
            if (mime == null || mime.compareTo("image/gif") == 0) {
                return;
            }
            BitmapDrawable image = null;
            // Decode non-PNG images.
            if (mime.compareTo("image/png") != 0) {
                image = decodeImageData(data);
                // Can't decode, give up.
                if (image == null) {
                    Log.i(LOGTAG, "Cannot decode image type " + mime
                          + " for URL=" + url);
                }
            }
            try {
                ContentValues values = new ContentValues();

                // Recompress decoded images to PNG.
                if (image != null) {
                    Bitmap bitmap = image.getBitmap();
                    ByteArrayOutputStream stream = new ByteArrayOutputStream();
                    bitmap.compress(Bitmap.CompressFormat.PNG, 100, stream);
                    values.put(Images.FAVICON, stream.toByteArray());
                } else {
                    // PNG images can be passed directly. Well, aside
                    // from having to convert them into a byte[].
                    byte[] byteArray = new byte[data.remaining()];
                    data.get(byteArray);
                    values.put(Images.FAVICON, byteArray);
                }

                values.put(Images.URL, url);
                values.put(Images.FAVICON_URL, faviconUrl);
                // Restore deleted record if possible
                values.put(Images.IS_DELETED, 0);
                values.put(Images.GUID, faviconGuid);

                int updated = mCr.update(getImagesUri(),
                                         values,
                                         Images.URL + " = ?",
                                         new String[] { url });

                if (updated == 0) {
                    mCr.insert(getImagesUri(), values);
                }
            } catch (SQLException e) {
                Log.i(LOGTAG, "Migrating favicon failed: " + mime + " URL: " + url
                      + " error:" + e.getMessage());
            }
        }

        protected void migrateHistory(SQLiteBridge db) {
            Map<String, Long> browserDBHistory = gatherBrowserDBHistory();
            final ArrayList<String> placesHistory = new ArrayList<String>();

            try {
                final String[] queryParams = new String[] {
                    /* current time */
                    Long.toString(System.currentTimeMillis()),
                    /*
                       History entries to return. No point
                       in retrieving more than we can store.
                     */
                    Integer.toString(BrowserDB.getMaxHistoryCount())
                };
                ArrayList<Object[]> queryResult =
                    db.query(kHistoryQuery, queryParams);
                final int urlCol = db.getColumnIndex(kHistoryUrl);
                final int titleCol = db.getColumnIndex(kHistoryTitle);
                final int dateCol = db.getColumnIndex(kHistoryDate);
                final int visitsCol = db.getColumnIndex(kHistoryVisits);
                final int faviconMimeCol = db.getColumnIndex(kFaviconMime);
                final int faviconDataCol = db.getColumnIndex(kFaviconData);
                final int faviconUrlCol = db.getColumnIndex(kFaviconUrl);
                final int faviconGuidCol = db.getColumnIndex(kFaviconGuid);

                for (Object[] resultRow: queryResult) {
                    String url = (String)resultRow[urlCol];
                    String title = (String)resultRow[titleCol];
                    long date = Long.parseLong((String)(resultRow[dateCol])) / (long)1000;
                    int visits = Integer.parseInt((String)(resultRow[visitsCol]));
                    ByteBuffer faviconDataBuff = (ByteBuffer)resultRow[faviconDataCol];
                    String faviconMime = (String)resultRow[faviconMimeCol];
                    String faviconUrl = (String)resultRow[faviconUrlCol];
                    String faviconGuid = (String)resultRow[faviconGuidCol];

                    try {
                        placesHistory.add(url);
                        addFavicon(url, faviconUrl, faviconGuid,
                                   faviconMime, faviconDataBuff);
                        addHistory(browserDBHistory, url, title, date, visits);
                    } catch (Exception e) {
                        Log.e(LOGTAG, "Error adding history entry: ", e);
                    }
                }
            } catch (SQLiteBridgeException e) {
                Log.e(LOGTAG, "Failed to get history: ", e);
                return;
            }
            // GlobalHistory access communicates with Gecko
            // and must run on its thread
            GeckoAppShell.getHandler().post(new Runnable() {
                    public void run() {
                        for (String url : placesHistory) {
                            GlobalHistory.getInstance().addToGeckoOnly(url);
                        }
                    }
             });
        }

        protected void addBookmark(String url, String title, String guid,
                                   long parent, long added,
                                   long modified, long position,
                                   boolean folder) {
            ContentValues values = new ContentValues();
            if (title == null && url != null) {
                title = url;
            }
            if (title != null) {
                values.put(Bookmarks.TITLE, title);
            }
            if (url != null) {
                values.put(Bookmarks.URL, url);
            }
            if (guid != null) {
                values.put(SyncColumns.GUID, guid);
            }
            values.put(SyncColumns.DATE_CREATED, added);
            values.put(SyncColumns.DATE_MODIFIED, modified);
            values.put(Bookmarks.POSITION, position);
            // Restore deleted record if possible
            values.put(Bookmarks.IS_DELETED, 0);
            if (mRerootMap.containsKey(parent)) {
                parent = mRerootMap.get(parent);
            }
            values.put(Bookmarks.PARENT, parent);
            values.put(Bookmarks.IS_FOLDER, (folder ? 1 : 0));

            int updated = 0;
            if (url != null) {
                updated = mCr.update(getBookmarksUri(),
                                     values,
                                     Bookmarks.URL + " = ?",
                                     new String[] { url });
            }
            if (updated == 0) {
                mCr.insert(getBookmarksUri(), values);
            }
        }

        protected void migrateBookmarks(SQLiteBridge db) {
            try {
                ArrayList<Object[]> queryResult = db.query(kBookmarkQuery);
                final int urlCol = db.getColumnIndex(kBookmarkUrl);
                final int titleCol = db.getColumnIndex(kBookmarkTitle);
                final int guidCol = db.getColumnIndex(kBookmarkGuid);
                final int idCol = db.getColumnIndex(kBookmarkId);
                final int typeCol = db.getColumnIndex(kBookmarkType);
                final int parentCol = db.getColumnIndex(kBookmarkParent);
                final int addedCol = db.getColumnIndex(kBookmarkAdded);
                final int modifiedCol = db.getColumnIndex(kBookmarkModified);
                final int positionCol = db.getColumnIndex(kBookmarkPosition);
                final int faviconMimeCol = db.getColumnIndex(kFaviconMime);
                final int faviconDataCol = db.getColumnIndex(kFaviconData);
                final int faviconUrlCol = db.getColumnIndex(kFaviconUrl);
                final int faviconGuidCol = db.getColumnIndex(kFaviconGuid);

                // The keys are places IDs.
                Set<Long> openFolders = new HashSet<Long>();
                Set<Long> knownFolders = new HashSet<Long>(mRerootMap.keySet());

                // We iterate over all bookmarks, and add all bookmarks that
                // have their parent folders present. If there are bookmarks
                // that we can't add, we remember what these are and try again
                // on the next iteration. The number of iterations scales
                // according to the depth of the folders.
                Set<Long> processedBookmarks = new HashSet<Long>();
                int iterations = 0;
                do {
                    // Reset the set of missing folders that block us from
                    // adding entries.
                    openFolders.clear();

                    int added = 0;
                    int skipped = 0;

                    for (Object[] resultRow: queryResult) {
                        long id = Long.parseLong((String)resultRow[idCol]);

                        // Already processed? if so just skip
                        if (processedBookmarks.contains(id))
                            continue;

                        int type = Integer.parseInt((String)resultRow[typeCol]);
                        long parent = Long.parseLong((String)resultRow[parentCol]);

                        // Places has an explicit root folder, id=1 parent=0.
                        // Skip that.
                        if (id == 1 && parent == 0 && type == kPlacesTypeFolder)
                            continue;

                        String url = (String)resultRow[urlCol];
                        String title = (String)resultRow[titleCol];
                        String guid = (String)resultRow[guidCol];
                        long dateadded =
                            Long.parseLong((String)resultRow[addedCol]) / (long)1000;
                        long datemodified =
                            Long.parseLong((String)resultRow[modifiedCol]) / (long)1000;
                        long position = Long.parseLong((String)resultRow[positionCol]);
                        ByteBuffer faviconDataBuff = (ByteBuffer)resultRow[faviconDataCol];
                        String faviconMime = (String)resultRow[faviconMimeCol];
                        String faviconUrl = (String)resultRow[faviconUrlCol];
                        String faviconGuid = (String)resultRow[faviconGuidCol];

                        // Is the parent for this bookmark already added?
                        // If so, we can add the bookmark itself.
                        if (knownFolders.contains(parent)) {
                            try {
                                boolean isFolder = (type == kPlacesTypeFolder);
                                addBookmark(url, title, guid, parent,
                                            dateadded, datemodified,
                                            position, isFolder);
                                addFavicon(url, faviconUrl, faviconGuid,
                                           faviconMime, faviconDataBuff);
                                if (isFolder) {
                                    long newFolderId = getFolderId(guid);
                                    // Remap the folder IDs for parents.
                                    mRerootMap.put(id, newFolderId);
                                    knownFolders.add(id);
                                    Log.d(LOGTAG, "Added folder: " + id);
                                }
                                processedBookmarks.add(id);
                            } catch (Exception e) {
                                Log.e(LOGTAG, "Error adding bookmark: ", e);
                            }
                            added++;
                        } else {
                            // We have to postpone until parent is processed;
                            openFolders.add(parent);
                            skipped++;
                        }
                    }

                    // Now check if any of the new folders we added was a folder
                    // that we were blocked on, by intersecting openFolders and
                    // knownFolders. If this is empty, we're done because the next
                    // iteration can't make progress.
                    boolean changed = openFolders.retainAll(knownFolders);

                    // If there are no folders that we can add next iteration,
                    // but there were still folders before the intersection,
                    // those folders are orphans. Report this situation here.
                    if (openFolders.isEmpty() && changed) {
                        Log.w(LOGTAG, "Orphaned bookmarks found, not imported");
                    }
                    iterations++;
                    Log.i(LOGTAG, "Iteration = " + iterations + ", added " + added +
                          " bookmark(s), skipped " + skipped + " bookmark(s)");
                } while (!openFolders.isEmpty());
            } catch (SQLiteBridgeException e) {
                Log.e(LOGTAG, "Failed to get bookmarks: ", e);
                return;
            }
        }

        protected void migratePlaces(File aFile) {
            String dbPath = aFile.getPath() + "/places.sqlite";
            String dbPathWal = aFile.getPath() + "/places.sqlite-wal";
            String dbPathShm = aFile.getPath() + "/places.sqlite-shm";
            Log.i(LOGTAG, "Opening path: " + dbPath);

            File dbFile = new File(dbPath);
            if (!dbFile.exists()) {
                Log.i(LOGTAG, "No database");
                return;
            }
            File dbFileWal = new File(dbPathWal);
            File dbFileShm = new File(dbPathShm);

            SQLiteBridge db = null;
            GeckoAppShell.ensureSQLiteLibsLoaded(GeckoApp.mAppContext.getApplication().getPackageResourcePath());
            try {
                db = new SQLiteBridge(dbPath);
                calculateReroot(db);
                migrateBookmarks(db);
                migrateHistory(db);
                db.close();

                // Clean up
                dbFile.delete();
                dbFileWal.delete();
                dbFileShm.delete();

                Log.i(LOGTAG, "Profile migration finished");
            } catch (SQLiteBridgeException e) {
                if (db != null) {
                    db.close();
                }
                Log.e(LOGTAG, "Error on places database:", e);
                return;
            }
        }

        protected void cleanupXULLibCache() {
            File cacheFile = GeckoAppShell.getCacheDir();
            File[] files = cacheFile.listFiles();
            if (files != null) {
                Iterator<File> cacheFiles = Arrays.asList(files).iterator();
                while (cacheFiles.hasNext()) {
                    File libFile = cacheFiles.next();
                    if (libFile.getName().endsWith(".so")) {
                        libFile.delete();
                    }
                }
            }
        }

        @Override
        public void run() {
            migratePlaces(mProfileDir);
            // XXX: Land dependent bugs first
            // cleanupXULLibCache();
        }
    }
}
