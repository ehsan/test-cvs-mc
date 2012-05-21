/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * vim: sw=2 ts=2 et lcs=trail\:.,tab\:>~ :
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsPlacesIndexes_h__
#define nsPlacesIndexes_h__

#define CREATE_PLACES_IDX(__name, __table, __columns, __type) \
  NS_LITERAL_CSTRING( \
    "CREATE " __type " INDEX IF NOT EXISTS " __table "_" __name \
      " ON " __table " (" __columns ")" \
  )

/**
 * moz_places
 */
#define CREATE_IDX_MOZ_PLACES_URL \
  CREATE_PLACES_IDX( \
    "url_uniqueindex", "moz_places", "url", "UNIQUE" \
  )

#define CREATE_IDX_MOZ_PLACES_FAVICON \
  CREATE_PLACES_IDX( \
    "faviconindex", "moz_places", "favicon_id", "" \
  )

#define CREATE_IDX_MOZ_PLACES_REVHOST \
  CREATE_PLACES_IDX( \
    "hostindex", "moz_places", "rev_host", "" \
  )

#define CREATE_IDX_MOZ_PLACES_VISITCOUNT \
  CREATE_PLACES_IDX( \
    "visitcount", "moz_places", "visit_count", "" \
  )

#define CREATE_IDX_MOZ_PLACES_FRECENCY \
  CREATE_PLACES_IDX( \
    "frecencyindex", "moz_places", "frecency", "" \
  )

#define CREATE_IDX_MOZ_PLACES_LASTVISITDATE \
  CREATE_PLACES_IDX( \
    "lastvisitdateindex", "moz_places", "last_visit_date", "" \
  )

#define CREATE_IDX_MOZ_PLACES_GUID \
  CREATE_PLACES_IDX( \
    "guid_uniqueindex", "moz_places", "guid", "UNIQUE" \
  )

/**
 * moz_historyvisits
 */

#define CREATE_IDX_MOZ_HISTORYVISITS_PLACEDATE \
  CREATE_PLACES_IDX( \
    "placedateindex", "moz_historyvisits", "place_id, visit_date", "" \
  )

#define CREATE_IDX_MOZ_HISTORYVISITS_FROMVISIT \
  CREATE_PLACES_IDX( \
    "fromindex", "moz_historyvisits", "from_visit", "" \
  )

#define CREATE_IDX_MOZ_HISTORYVISITS_VISITDATE \
  CREATE_PLACES_IDX( \
    "dateindex", "moz_historyvisits", "visit_date", "" \
  )

/**
 * moz_bookmarks
 */

#define CREATE_IDX_MOZ_BOOKMARKS_PLACETYPE \
  CREATE_PLACES_IDX( \
    "itemindex", "moz_bookmarks", "fk, type", "" \
  )

#define CREATE_IDX_MOZ_BOOKMARKS_PARENTPOSITION \
  CREATE_PLACES_IDX( \
    "parentindex", "moz_bookmarks", "parent, position", "" \
  )

#define CREATE_IDX_MOZ_BOOKMARKS_PLACELASTMODIFIED \
  CREATE_PLACES_IDX( \
    "itemlastmodifiedindex", "moz_bookmarks", "fk, lastModified", "" \
  )

#define CREATE_IDX_MOZ_BOOKMARKS_GUID \
  CREATE_PLACES_IDX( \
    "guid_uniqueindex", "moz_bookmarks", "guid", "UNIQUE" \
  )

/**
 * moz_annos
 */

#define CREATE_IDX_MOZ_ANNOS_PLACEATTRIBUTE \
  CREATE_PLACES_IDX( \
    "placeattributeindex", "moz_annos", "place_id, anno_attribute_id", "UNIQUE" \
  )

/**
 * moz_items_annos
 */

#define CREATE_IDX_MOZ_ITEMSANNOS_PLACEATTRIBUTE \
  CREATE_PLACES_IDX( \
    "itemattributeindex", "moz_items_annos", "item_id, anno_attribute_id", "UNIQUE" \
  )

/**
 * moz_favicons
 */

#define CREATE_IDX_MOZ_FAVICONS_GUID \
  CREATE_PLACES_IDX( \
    "guid_uniqueindex", "moz_favicons", "guid", "UNIQUE" \
  )

#endif // nsPlacesIndexes_h__
