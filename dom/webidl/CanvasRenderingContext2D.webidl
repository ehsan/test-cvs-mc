/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * The origin of this IDL file is
 * http://www.whatwg.org/specs/web-apps/current-work/
 *
 * © Copyright 2004-2011 Apple Computer, Inc., Mozilla Foundation, and
 * Opera Software ASA. You are granted a license to use, reproduce
 * and create derivative works of this document.
 */

interface CanvasGradient;
interface CanvasPattern;
interface HitRegionOptions;
interface HTMLCanvasElement;
interface HTMLImageElement;
interface HTMLVideoElement;
interface TextMetrics;
interface Window;
interface XULElement;

interface CanvasRenderingContext2D {

  // back-reference to the canvas.  Might be null if we're not
  // associated with a canvas.
  readonly attribute HTMLCanvasElement? canvas;

  // state
  void save(); // push state on state stack
  void restore(); // pop state stack and restore state

  // transformations (default transform is the identity matrix)
// NOT IMPLEMENTED           attribute SVGMatrix currentTransform;
  [Throws]
  void scale(double x, double y);
  [Throws]
  void rotate(double angle);
  [Throws]
  void translate(double x, double y);
  [Throws]
  void transform(double a, double b, double c, double d, double e, double f);
  [Throws]
  void setTransform(double a, double b, double c, double d, double e, double f);
// NOT IMPLEMENTED  void resetTransform();

  // compositing
           attribute double globalAlpha; // (default 1.0)
           [Throws]
           attribute DOMString globalCompositeOperation; // (default source-over)

  // colors and styles (see also the CanvasDrawingStyles interface)
           [GetterThrows]
           attribute any strokeStyle; // (default black)
           [GetterThrows]
           attribute any fillStyle; // (default black)
  [Throws]
  CanvasGradient createLinearGradient(double x0, double y0, double x1, double y1);
  [Throws]
  CanvasGradient createRadialGradient(double x0, double y0, double r0, double x1, double y1, double r1);
  [Throws]
  CanvasPattern createPattern((HTMLImageElement or HTMLCanvasElement or HTMLVideoElement) image, [TreatNullAs=EmptyString] DOMString repetition);

  // shadows
           attribute double shadowOffsetX; // (default 0)
           attribute double shadowOffsetY; // (default 0)
           attribute double shadowBlur; // (default 0)
           attribute DOMString shadowColor; // (default transparent black)

  // rects
  void clearRect(double x, double y, double w, double h);
  void fillRect(double x, double y, double w, double h);
  void strokeRect(double x, double y, double w, double h);

  // path API (see also CanvasPathMethods)
  void beginPath();
  void fill();
// NOT IMPLEMENTED  void fill(Path path);
  void stroke();
// NOT IMPLEMENTED  void stroke(Path path);
// NOT IMPLEMENTED  void drawSystemFocusRing(Element element);
// NOT IMPLEMENTED  void drawSystemFocusRing(Path path, Element element);
// NOT IMPLEMENTED  boolean drawCustomFocusRing(Element element);
// NOT IMPLEMENTED  boolean drawCustomFocusRing(Path path, Element element);
// NOT IMPLEMENTED  void scrollPathIntoView();
// NOT IMPLEMENTED  void scrollPathIntoView(Path path);
  void clip();
// NOT IMPLEMENTED  void clip(Path path);
// NOT IMPLEMENTED  void resetClip();
  boolean isPointInPath(double x, double y);
// NOT IMPLEMENTED  boolean isPointInPath(Path path, double x, double y);

  // text (see also the CanvasDrawingStyles interface)
  [Throws]
  void fillText(DOMString text, double x, double y, optional double maxWidth);
  [Throws]
  void strokeText(DOMString text, double x, double y, optional double maxWidth);
  [Throws]
  TextMetrics measureText(DOMString text);

  // drawing images
// NOT IMPLEMENTED           attribute boolean imageSmoothingEnabled; // (default true)
  [Throws]
  void drawImage((HTMLImageElement or HTMLCanvasElement or HTMLVideoElement) image, double dx, double dy);
  [Throws]
  void drawImage((HTMLImageElement or HTMLCanvasElement or HTMLVideoElement) image, double dx, double dy, double dw, double dh);
  [Throws]
  void drawImage((HTMLImageElement or HTMLCanvasElement or HTMLVideoElement) image, double sx, double sy, double sw, double sh, double dx, double dy, double dw, double dh);

  // hit regions
// NOT IMPLEMENTED  void addHitRegion(HitRegionOptions options);

  // pixel manipulation
  [Creator, Throws]
  ImageData createImageData(double sw, double sh);
  [Creator, Throws]
  ImageData createImageData(ImageData imagedata);
  [Creator, Throws]
  ImageData getImageData(double sx, double sy, double sw, double sh);
  [Throws]
  void putImageData(ImageData imagedata, double dx, double dy);
  [Throws]
  void putImageData(ImageData imagedata, double dx, double dy, double dirtyX, double dirtyY, double dirtyWidth, double dirtyHeight);

  // Mozilla-specific stuff
  // FIXME Bug 768048 mozCurrentTransform/mozCurrentTransformInverse should return a WebIDL array.
  [Throws]
  attribute object mozCurrentTransform; // [ m11, m12, m21, m22, dx, dy ], i.e. row major
  [Throws]
  attribute object mozCurrentTransformInverse;

  attribute DOMString mozFillRule; /* "evenodd", "nonzero" (default) */

  [Throws]
  attribute any mozDash; /* default |null| */

  attribute double mozDashOffset; /* default 0.0 */

  [SetterThrows]
  attribute DOMString mozTextStyle;

  // image smoothing mode -- if disabled, images won't be smoothed
  // if scaled.
  attribute boolean mozImageSmoothingEnabled;

  // Show the caret if appropriate when drawing
  [ChromeOnly]
  const unsigned long DRAWWINDOW_DRAW_CARET   = 0x01;
  // Don't flush pending layout notifications that could otherwise
  // be batched up
  [ChromeOnly]
  const unsigned long DRAWWINDOW_DO_NOT_FLUSH = 0x02;
  // Draw scrollbars and scroll the viewport if they are present
  [ChromeOnly]
  const unsigned long DRAWWINDOW_DRAW_VIEW    = 0x04;
  // Use the widget layer manager if available. This means hardware
  // acceleration may be used, but it might actually be slower or
  // lower quality than normal. It will however more accurately reflect
  // the pixels rendered to the screen.
  [ChromeOnly]
  const unsigned long DRAWWINDOW_USE_WIDGET_LAYERS = 0x08;
  // Don't synchronously decode images - draw what we have
  [ChromeOnly]
  const unsigned long DRAWWINDOW_ASYNC_DECODE_IMAGES = 0x10;

  /**
   * Renders a region of a window into the canvas.  The contents of
   * the window's viewport are rendered, ignoring viewport clipping
   * and scrolling.
   *
   * @param x
   * @param y
   * @param w
   * @param h specify the area of the window to render, in CSS
   * pixels.
   *
   * @param backgroundColor the canvas is filled with this color
   * before we render the window into it. This color may be
   * transparent/translucent. It is given as a CSS color string
   * (e.g., rgb() or rgba()).
   *
   * @param flags Used to better control the drawWindow call.
   * Flags can be ORed together.
   *
   * Of course, the rendering obeys the current scale, transform and
   * globalAlpha values.
   *
   * Hints:
   * -- If 'rgba(0,0,0,0)' is used for the background color, the
   * drawing will be transparent wherever the window is transparent.
   * -- Top-level browsed documents are usually not transparent
   * because the user's background-color preference is applied,
   * but IFRAMEs are transparent if the page doesn't set a background.
   * -- If an opaque color is used for the background color, rendering
   * will be faster because we won't have to compute the window's
   * transparency.
   *
   * This API cannot currently be used by Web content. It is chrome
   * only.
   */
  [Throws, ChromeOnly]
  void drawWindow(Window window, double x, double y, double w, double h,
                  DOMString bgColor, optional unsigned long flags = 0);
  [Throws, ChromeOnly]
  void asyncDrawXULElement(XULElement elem, double x, double y, double w,
                           double h, DOMString bgColor,
                           optional unsigned long flags = 0);
};
CanvasRenderingContext2D implements CanvasDrawingStyles;
CanvasRenderingContext2D implements CanvasPathMethods;

[NoInterfaceObject]
interface CanvasDrawingStyles {
  // line caps/joins
           attribute double lineWidth; // (default 1)
           attribute DOMString lineCap; // "butt", "round", "square" (default "butt")
           [GetterThrows]
           attribute DOMString lineJoin; // "round", "bevel", "miter" (default "miter")
           attribute double miterLimit; // (default 10)

  // dashed lines
// NOT IMPLEMENTED    void setLineDash(sequence<double> segments); // default empty
// NOT IMPLEMENTED    sequence<double> getLineDash();
// NOT IMPLEMENTED             attribute double lineDashOffset;

  // text
           [SetterThrows]
           attribute DOMString font; // (default 10px sans-serif)
           attribute DOMString textAlign; // "start", "end", "left", "right", "center" (default: "start")
           attribute DOMString textBaseline; // "top", "hanging", "middle", "alphabetic", "ideographic", "bottom" (default: "alphabetic")
};

[NoInterfaceObject]
interface CanvasPathMethods {
  // shared path API methods
  void closePath();
  void moveTo(double x, double y);
  void lineTo(double x, double y);
  void quadraticCurveTo(double cpx, double cpy, double x, double y);

  void bezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y);

  [Throws]
  void arcTo(double x1, double y1, double x2, double y2, double radius); 
// NOT IMPLEMENTED  void arcTo(double x1, double y1, double x2, double y2, double radiusX, double radiusY, double rotation);

  void rect(double x, double y, double w, double h);

  [Throws]
  void arc(double x, double y, double radius, double startAngle, double endAngle, optional boolean anticlockwise = false); 
// NOT IMPLEMENTED  void ellipse(double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle, boolean anticlockwise);
};
