/* -*- Mode: Java; c-basic-offset: 4; tab-width: 20; indent-tabs-mode: nil; -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.gecko.gfx;

import android.graphics.Point;
import android.graphics.PointF;

import org.json.JSONObject;
import org.json.JSONException;
import org.mozilla.gecko.FloatUtils;

public final class PointUtils {
    public static PointF add(PointF one, PointF two) {
        return new PointF(one.x + two.x, one.y + two.y);
    }

    public static PointF subtract(PointF one, PointF two) {
        return new PointF(one.x - two.x, one.y - two.y);
    }

    public static PointF scale(PointF point, float factor) {
        return new PointF(point.x * factor, point.y * factor);
    }

    public static Point round(PointF point) {
        return new Point(Math.round(point.x), Math.round(point.y));
    }

   /* Returns a new point that is a linear interpolation between start and end points. weight conrols the weighting
    * of each of the original points (weight = 1 returns endPoint, weight = 0 returns startPoint)
    */
   public static PointF interpolate(PointF startPoint, PointF endPoint, float weight) {
       float x = FloatUtils.interpolate(startPoint.x, endPoint.x, weight);
       float y = FloatUtils.interpolate(startPoint.y, endPoint.y, weight);
       return new PointF(x, y);
   }

   /* Computes the magnitude of the given vector. */
   public static float distance(PointF point) {
        return (float)Math.sqrt(point.x * point.x + point.y * point.y);
   }

    /** Computes the scalar distance between two points. */
    public static float distance(PointF one, PointF two) {
        return PointF.length(one.x - two.x, one.y - two.y);
    }

    public static JSONObject toJSON(PointF point) throws JSONException {
        // Ensure we put ints, not longs, because Gecko message handlers call getInt().
        int x = Math.round(point.x);
        int y = Math.round(point.y);
        JSONObject json = new JSONObject();
        json.put("x", x);
        json.put("y", y);
        return json;
    }
}

