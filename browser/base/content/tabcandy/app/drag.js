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
 * The Original Code is drag.js.
 *
 * The Initial Developer of the Original Code is
 * Michael Yoshitaka Erlewine <mitcho@mitcho.com>.
 * Portions created by the Initial Developer are Copyright (C) 2010
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

// **********
// Title: drag.js 

// ----------
// Variable: drag
// The Drag that's currently in process. 
var drag = {
  info: null,
  zIndex: 100
};


// ##########
// Class: Drag (formerly DragInfo)
// Helper class for dragging <Item>s
// 
// ----------
// Constructor: Drag
// Called to create a Drag in response to an <Item> draggable "start" event.
// Note that it is also used partially during <Item>'s resizable method as well.
// 
// Parameters: 
//   item - The <Item> being dragged
//   event - The DOM event that kicks off the drag
//   isResizing - (boolean) is this a resizing instance? or (if false) dragging?
var Drag = function(item, event, isResizing) {
  try {
    Utils.assert('must be an item, or at least a faux item',
                 item && (item.isAnItem || item.isAFauxItem));
    
    this.isResizing = isResizing || false;
    this.item = item;
    this.el = item.container;
    this.$el = iQ(this.el);
    this.parent = this.item.parent;
    this.startPosition = new Point(event.clientX, event.clientY);
    this.startTime = Utils.getMilliseconds();
    
    this.item.isDragging = true;
    this.item.setZ(999999);
    
    if (this.item.isATabItem && !isResizing)
      this.safeWindowBounds = Items.getSafeWindowBounds( true );
    else
      this.safeWindowBounds = Items.getSafeWindowBounds( );

    Trenches.activateOthersTrenches(this.el);
    
    // When a tab drag starts, make it the focused tab.
    if (this.item.isAGroup) {
      var tab = Page.getActiveTab();
      if (!tab || tab.parent != this.item) {
        if (this.item._children.length)
          Page.setActiveTab(this.item._children[0]);
      }
    } else if (this.item.isATabItem) {
      Page.setActiveTab(this.item);
    }
  } catch(e) {
    Utils.log(e);
  }
};

Drag.prototype = {
  // ----------
  // Function: snapBounds
  // Adjusts the given bounds according to the currently active trenches. Used by <Drag.snap>
  // 
  // Parameters: 
  //   bounds             - (<Rect>) bounds
  //   stationaryCorner   - which corner is stationary? by default, the top left.
  //                        "topleft", "bottomleft", "topright", "bottomright"
  //   assumeConstantSize - (boolean) whether the bounds' dimensions are sacred or not.
  //   keepProportional   - (boolean) if assumeConstantSize is false, whether we should resize 
  //                        proportionally or not
  snapBounds: function Drag_snapBounds(bounds, stationaryCorner, assumeConstantSize, keepProportional) {
    var stationaryCorner = stationaryCorner || 'topleft';
    var update = false; // need to update
    var updateX = false;
    var updateY = false;
    var newRect;
    var snappedTrenches = {};

    // OH SNAP!
    if ( // if we aren't holding down the meta key...
         !Keys.meta
         // and we aren't a tab on top of something else...
         && !(this.item.isATabItem && this.item.overlapsWithOtherItems()) ) { 
      newRect = Trenches.snap(bounds,stationaryCorner,assumeConstantSize,keepProportional);
      if (newRect) { // might be false if no changes were made
        update = true;
        snappedTrenches = newRect.snappedTrenches || {};
        bounds = newRect;
      }
    }

    // make sure the bounds are in the window.
    newRect = this.snapToEdge(bounds,stationaryCorner,assumeConstantSize,keepProportional);
    if (newRect) {
      update = true;
      bounds = newRect;
      iQ.extend(snappedTrenches,newRect.snappedTrenches);
    }

    Trenches.hideGuides();
    for (var edge in snappedTrenches) {
      var trench = snappedTrenches[edge];
      if (typeof trench == 'object') {
        trench.showGuide = true;
        trench.show();
      } else if (trench === 'edge') {
        // show the edge...?
      }
    }

    return update ? bounds : false;
  },
  
  // ----------
  // Function: snap
  // Called when a drag or mousemove occurs. Set the bounds based on the mouse move first, then
  // call snap and it will adjust the item's bounds if appropriate. Also triggers the display of
  // trenches that it snapped to.
  // 
  // Parameters: 
  //   stationaryCorner   - which corner is stationary? by default, the top left.
  //                        "topleft", "bottomleft", "topright", "bottomright"
  //   assumeConstantSize - (boolean) whether the bounds' dimensions are sacred or not.
  //   keepProportional   - (boolean) if assumeConstantSize is false, whether we should resize 
  //                        proportionally or not
  snap: function Drag_snap(stationaryCorner, assumeConstantSize, keepProportional) {
    var bounds = this.item.getBounds();
    bounds = this.snapBounds(bounds, stationaryCorner, assumeConstantSize, keepProportional);
    if (bounds) {
      this.item.setBounds(bounds,true);
      return true;
    }
    return false;
  },
  
  // --------
  // Function: snapToEdge
  // Returns a version of the bounds snapped to the edge if it is close enough. If not, 
  // returns false. If <Keys.meta> is true, this function will simply enforce the 
  // window edges.
  // 
  // Parameters:
  //   rect - (<Rect>) current bounds of the object
  //   stationaryCorner   - which corner is stationary? by default, the top left.
  //                        "topleft", "bottomleft", "topright", "bottomright"
  //   assumeConstantSize - (boolean) whether the rect's dimensions are sacred or not
  //   keepProportional   - (boolean) if we are allowed to change the rect's size, whether the
  //                                  dimensions should scaled proportionally or not.
  snapToEdge: function Drag_snapToEdge(rect, stationaryCorner, assumeConstantSize, keepProportional) {
  
    var swb = this.safeWindowBounds;
    var update = false;
    var updateX = false;
    var updateY = false;
    var snappedTrenches = {};

    var snapRadius = ( Keys.meta ? 0 : Trenches.defaultRadius );
    if (rect.left < swb.left + snapRadius ) {
      if (stationaryCorner.indexOf('right') > -1)
        rect.width = rect.right - swb.left;
      rect.left = swb.left;
      update = true;
      updateX = true;
      snappedTrenches.left = 'edge';
    }
    
    if (rect.right > swb.right - snapRadius) {
      if (updateX || !assumeConstantSize) {
        var newWidth = swb.right - rect.left;
        if (keepProportional)
          rect.height = rect.height * newWidth / rect.width;
        rect.width = newWidth;
        update = true;
      } else if (!updateX || !Trenches.preferLeft) {
        rect.left = swb.right - rect.width;
        update = true;
      }
      snappedTrenches.right = 'edge';
      delete snappedTrenches.left;
    }
    if (rect.top < swb.top + snapRadius) {
      if (stationaryCorner.indexOf('bottom') > -1)
        rect.height = rect.bottom - swb.top;
      rect.top = swb.top;
      update = true;
      updateY = true;
      snappedTrenches.top = 'edge';
    }
    if (rect.bottom > swb.bottom - snapRadius) {
      if (updateY || !assumeConstantSize) {
        var newHeight = swb.bottom - rect.top;
        if (keepProportional)
          rect.width = rect.width * newHeight / rect.height;
        rect.height = newHeight;
        update = true;
      } else if (!updateY || !Trenches.preferTop) {
        rect.top = swb.bottom - rect.height;
        update = true;
      }
      snappedTrenches.top = 'edge';
      delete snappedTrenches.bottom;
    }
    
    if (update) {
      rect.snappedTrenches = snappedTrenches;
      return rect;
    }
    return false;
  },
  
  // ----------  
  // Function: drag
  // Called in response to an <Item> draggable "drag" event.
  drag: function(event, ui) {
    this.snap('topleft',true);
      
    if (this.parent && this.parent.expanded) {
      var now = Utils.getMilliseconds();
      var distance = this.startPosition.distance(new Point(event.clientX, event.clientY));
      if (/* now - this.startTime > 500 ||  */distance > 100) {
        this.parent.remove(this.item);
        this.parent.collapse();
      }
    }
  },

  // ----------  
  // Function: stop
  // Called in response to an <Item> draggable "stop" event.
  stop: function() {
    Trenches.hideGuides();
    this.item.isDragging = false;

    if (this.parent && !this.parent.locked.close && this.parent != this.item.parent 
        && this.parent.isEmpty()) {
      this.parent.close();
    }
     
    if (this.parent && this.parent.expanded)
      this.parent.arrange();
      
    if (this.item && !this.item.parent) {
      this.item.setZ(drag.zIndex);
      drag.zIndex++;
      
      this.item.pushAway();
    }
    
    Trenches.disactivate();
  }
};
