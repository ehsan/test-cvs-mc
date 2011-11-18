/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
 * The Original Code is Mozilla Corporation code.
 *
 * The Initial Developer of the Original Code is Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *     Matt Woodrow <mwoodrow@mozilla.com>
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

#include "DrawTargetSkia.h"
#include "SourceSurfaceSkia.h"
#include "ScaledFontSkia.h"
#include "skia/SkDevice.h"
#include "skia/SkTypeface.h"
#include "skia/SkGradientShader.h"
#include "skia/SkBlurDrawLooper.h"
#include "skia/SkBlurMaskFilter.h"
#include "skia/SkColorFilter.h"
#include "skia/SkLayerRasterizer.h"
#include "skia/SkLayerDrawLooper.h"
#include "skia/SkDashPathEffect.h"
#include "Logging.h"
#include "HelpersSkia.h"
#include "gfxImageSurface.h"
#include "Tools.h"
#include <algorithm>

namespace mozilla {
namespace gfx {

SkColor ColorToSkColor(const Color &color, Float aAlpha)
{
  return SkColorSetARGB(color.a*aAlpha*255.0, color.r*255.0, color.g*255.0, color.b*255.0);
}

class GradientStopsSkia : public GradientStops
{
public:
  GradientStopsSkia(const std::vector<GradientStop>& aStops, uint32_t aNumStops)
    : mCount(aNumStops)
  {
    if (mCount == 0) {
      return;
    }

    // Skia gradients always require a stop at 0.0 and 1.0, insert these if
    // we don't have them.
    uint32_t shift = 0;
    if (aStops[0].offset != 0) {
      mCount++;
      shift = 1;
    }
    if (aStops[aNumStops-1].offset != 1) {
      mCount++;
    }
    mColors.resize(mCount);
    mPositions.resize(mCount);
    if (aStops[0].offset != 0) {
      mColors[0] = ColorToSkColor(aStops[0].color, 1.0);
      mPositions[0] = 0;
    }
    for (uint32_t i = 0; i < aNumStops; i++) {
      mColors[i + shift] = ColorToSkColor(aStops[i].color, 1.0);
      mPositions[i + shift] = SkFloatToScalar(aStops[i].offset);
    }
    if (aStops[aNumStops-1].offset != 1) {
      mColors[mCount-1] = ColorToSkColor(aStops[aNumStops-1].color, 1.0);
      mPositions[mCount-1] = SK_Scalar1;
    }
  }

  BackendType GetBackendType() const { return BACKEND_SKIA; }

  std::vector<SkColor> mColors;
  std::vector<SkScalar> mPositions;
  int mCount;
};

SkXfermode::Mode
GfxOpToSkiaOp(CompositionOp op)
{
  switch (op)
  {
    case OP_OVER:
      return SkXfermode::kSrcOver_Mode;
    case OP_ADD:
      return SkXfermode::kPlus_Mode;
    case OP_ATOP:
      return SkXfermode::kSrcATop_Mode;
    case OP_OUT:
      return SkXfermode::kSrcOut_Mode;
    case OP_IN:
      return SkXfermode::kSrcIn_Mode;
    case OP_SOURCE:
      return SkXfermode::kSrc_Mode;
    case OP_DEST_IN:
      return SkXfermode::kDstIn_Mode;
    case OP_DEST_OUT:
      return SkXfermode::kDstOut_Mode;
    case OP_DEST_OVER:
      return SkXfermode::kDstOver_Mode;
    case OP_DEST_ATOP:
      return SkXfermode::kDstATop_Mode;
    case OP_XOR:
      return SkXfermode::kXor_Mode;
    case OP_COUNT:
      return SkXfermode::kSrcOver_Mode;
  }
  return SkXfermode::kSrcOver_Mode;
}


SkRect
RectToSkRect(const Rect& aRect)
{
  return SkRect::MakeXYWH(SkFloatToScalar(aRect.x), SkFloatToScalar(aRect.y), 
                          SkFloatToScalar(aRect.width), SkFloatToScalar(aRect.height));
}

SkRect
IntRectToSkRect(const IntRect& aRect)
{
  return SkRect::MakeXYWH(SkIntToScalar(aRect.x), SkIntToScalar(aRect.y), 
                          SkIntToScalar(aRect.width), SkIntToScalar(aRect.height));
}

SkIRect
RectToSkIRect(const Rect& aRect)
{
  return SkIRect::MakeXYWH(aRect.x, aRect.y, aRect.width, aRect.height);
}

SkIRect
IntRectToSkIRect(const IntRect& aRect)
{
  return SkIRect::MakeXYWH(aRect.x, aRect.y, aRect.width, aRect.height);
}


DrawTargetSkia::DrawTargetSkia()
{
}

DrawTargetSkia::~DrawTargetSkia()
{
  MarkChanged();
}

TemporaryRef<SourceSurface>
DrawTargetSkia::Snapshot()
{
  RefPtr<SourceSurfaceSkia> source = new SourceSurfaceSkia();
  if (!source->InitWithBitmap(mBitmap, mFormat, this)) {
    return NULL;
  }
  AppendSnapshot(source);
  return source;
}

SkShader::TileMode
ExtendModeToTileMode(ExtendMode aMode)
{
  switch (aMode)
  {
    case EXTEND_CLAMP:
      return SkShader::kClamp_TileMode;
    case EXTEND_WRAP:
      return SkShader::kRepeat_TileMode;
    case EXTEND_MIRROR:
      return SkShader::kMirror_TileMode;
  }
  return SkShader::kClamp_TileMode;
}

struct AutoPaintSetup {
  AutoPaintSetup(SkCanvas *aCanvas, const DrawOptions& aOptions, const Pattern& aPattern)
    : mNeedsRestore(false), mAlpha(1.0)
  {
    Init(aCanvas, aOptions);
    SetPattern(aPattern);
  }

  AutoPaintSetup(SkCanvas *aCanvas, const DrawOptions& aOptions)
    : mNeedsRestore(false), mAlpha(1.0)
  {
    Init(aCanvas, aOptions);
  }

  ~AutoPaintSetup()
  {
    if (mNeedsRestore) {
      mCanvas->restore();
    }
  }

  void Init(SkCanvas *aCanvas, const DrawOptions& aOptions)
  {
    mPaint.setXfermodeMode(GfxOpToSkiaOp(aOptions.mCompositionOp));
    mCanvas = aCanvas;

    //TODO: Can we set greyscale somehow?
    if (aOptions.mAntialiasMode != AA_NONE) {
      mPaint.setAntiAlias(true);
    } else {
      mPaint.setAntiAlias(false);
    }

    NS_ASSERTION(aOptions.mSnapping == SNAP_NONE, "Pixel snapping not supported yet!");
    
    // TODO: We could skip the temporary for operator_source and just
    // clear the clip rect. The other operators would be harder
    // but could be worth it to skip pushing a group.
    if (!IsOperatorBoundByMask(aOptions.mCompositionOp)) {
      mPaint.setXfermodeMode(SkXfermode::kSrcOver_Mode);
      SkPaint temp;
      temp.setXfermodeMode(GfxOpToSkiaOp(aOptions.mCompositionOp));
      temp.setAlpha(aOptions.mAlpha*255);
      //TODO: Get a rect here
      mCanvas->saveLayer(NULL, &temp);
      mNeedsRestore = true;
    } else {
      mPaint.setAlpha(aOptions.mAlpha*255.0);
      mAlpha = aOptions.mAlpha;
    }
    mPaint.setFilterBitmap(true);
  }

  void SetPattern(const Pattern& aPattern)
  {
    if (aPattern.GetType() == PATTERN_COLOR) {
      Color color = static_cast<const ColorPattern&>(aPattern).mColor;
      mPaint.setColor(ColorToSkColor(color, mAlpha));
    } else if (aPattern.GetType() == PATTERN_LINEAR_GRADIENT) {
      const LinearGradientPattern& pat = static_cast<const LinearGradientPattern&>(aPattern);
      GradientStopsSkia *stops = static_cast<GradientStopsSkia*>(pat.mStops.get());

      if (stops->mCount >= 2) {
        SkPoint points[2];
        points[0] = SkPoint::Make(SkFloatToScalar(pat.mBegin.x), SkFloatToScalar(pat.mBegin.y));
        points[1] = SkPoint::Make(SkFloatToScalar(pat.mEnd.x), SkFloatToScalar(pat.mEnd.y));

        SkShader* shader = SkGradientShader::CreateLinear(points, 
                                                          &stops->mColors.front(), 
                                                          &stops->mPositions.front(), 
                                                          stops->mCount, 
                                                          SkShader::kClamp_TileMode);
        SkSafeUnref(mPaint.setShader(shader));
      } else {
        mPaint.setColor(SkColorSetARGB(0, 0, 0, 0));
      }
    } else if (aPattern.GetType() == PATTERN_RADIAL_GRADIENT) {
      const RadialGradientPattern& pat = static_cast<const RadialGradientPattern&>(aPattern);
      GradientStopsSkia *stops = static_cast<GradientStopsSkia*>(pat.mStops.get());

      if (stops->mCount >= 2) {
        SkPoint points[2];
        points[0] = SkPoint::Make(SkFloatToScalar(pat.mCenter1.x), SkFloatToScalar(pat.mCenter1.y));
        points[1] = SkPoint::Make(SkFloatToScalar(pat.mCenter2.x), SkFloatToScalar(pat.mCenter2.y));

        SkShader* shader = SkGradientShader::CreateTwoPointRadial(points[0], 
                                                                  SkFloatToScalar(pat.mRadius1),
                                                                  points[1], 
                                                                  SkFloatToScalar(pat.mRadius2),
                                                                  &stops->mColors.front(), 
                                                                  &stops->mPositions.front(), 
                                                                  stops->mCount, 
                                                                  SkShader::kClamp_TileMode);
        SkSafeUnref(mPaint.setShader(shader));
      } else {
        mPaint.setColor(SkColorSetARGB(0, 0, 0, 0));
      }
    } else {
      const SurfacePattern& pat = static_cast<const SurfacePattern&>(aPattern);
      const SkBitmap& bitmap = static_cast<SourceSurfaceSkia*>(pat.mSurface.get())->GetBitmap();

      SkShader::TileMode mode = ExtendModeToTileMode(pat.mExtendMode);
      SkShader* shader = SkShader::CreateBitmapShader(bitmap, mode, mode);
      SkSafeUnref(mPaint.setShader(shader));
    }
  }

  // TODO: Maybe add an operator overload to access this easier?
  SkPaint mPaint;
  bool mNeedsRestore;
  SkCanvas* mCanvas;
  Float mAlpha;
};

void
DrawTargetSkia::Flush()
{
}

void
DrawTargetSkia::DrawSurface(SourceSurface *aSurface,
                            const Rect &aDest,
                            const Rect &aSource,
                            const DrawSurfaceOptions &aSurfOptions,
                            const DrawOptions &aOptions)
{
  if (aSurface->GetType() != SURFACE_SKIA) {
    return;
  }

  if (aSource.IsEmpty()) {
    return;
  }

  MarkChanged();

  SkRect destRect = RectToSkRect(aDest);
  SkRect sourceRect = RectToSkRect(aSource);

  SkMatrix matrix;
  matrix.setRectToRect(sourceRect, destRect, SkMatrix::kFill_ScaleToFit);
  
  const SkBitmap& bitmap = static_cast<SourceSurfaceSkia*>(aSurface)->GetBitmap();
 
  AutoPaintSetup paint(mCanvas.get(), aOptions);
  SkShader *shader = SkShader::CreateBitmapShader(bitmap, SkShader::kClamp_TileMode, SkShader::kClamp_TileMode);
  shader->setLocalMatrix(matrix);
  SkSafeUnref(paint.mPaint.setShader(shader));
  if (aSurfOptions.mFilter != FILTER_LINEAR) {
    paint.mPaint.setFilterBitmap(false);
  }
  mCanvas->drawRect(destRect, paint.mPaint);
}

void
DrawTargetSkia::DrawSurfaceWithShadow(SourceSurface *aSurface,
                                      const Point &aDest,
                                      const Color &aColor,
                                      const Point &aOffset,
                                      Float aSigma,
                                      CompositionOp aOperator)
{
  MarkChanged();
  mCanvas->save(SkCanvas::kMatrix_SaveFlag);
  mCanvas->resetMatrix();

  uint32_t blurFlags = SkBlurMaskFilter::kHighQuality_BlurFlag |
                       SkBlurMaskFilter::kIgnoreTransform_BlurFlag;
  const SkBitmap& bitmap = static_cast<SourceSurfaceSkia*>(aSurface)->GetBitmap();
  SkShader* shader = SkShader::CreateBitmapShader(bitmap, SkShader::kClamp_TileMode, SkShader::kClamp_TileMode);
  SkMatrix matrix;
  matrix.reset();
  matrix.setTranslateX(SkFloatToScalar(aDest.x));
  matrix.setTranslateY(SkFloatToScalar(aDest.y));
  shader->setLocalMatrix(matrix);
  SkLayerDrawLooper* dl = new SkLayerDrawLooper;
  SkLayerDrawLooper::LayerInfo info;
  info.fPaintBits |= SkLayerDrawLooper::kShader_Bit;
  SkPaint *layerPaint = dl->addLayer(info);
  layerPaint->setShader(shader);

  info.fPaintBits = 0;
  info.fPaintBits |= SkLayerDrawLooper::kMaskFilter_Bit;
  info.fPaintBits |= SkLayerDrawLooper::kColorFilter_Bit;
  info.fColorMode = SkXfermode::kDst_Mode;
  info.fOffset.set(SkFloatToScalar(aOffset.x), SkFloatToScalar(aOffset.y));
  info.fPostTranslate = true;

  SkMaskFilter* mf = SkBlurMaskFilter::Create(aSigma, SkBlurMaskFilter::kNormal_BlurStyle, blurFlags);
  SkColor color = ColorToSkColor(aColor, 1);
  SkColorFilter* cf = SkColorFilter::CreateModeFilter(color, SkXfermode::kSrcIn_Mode);


  layerPaint = dl->addLayer(info);
  SkSafeUnref(layerPaint->setMaskFilter(mf));
  SkSafeUnref(layerPaint->setColorFilter(cf));
  layerPaint->setColor(color);
  
  // TODO: This is using the rasterizer to calculate an alpha mask
  // on both the shadow and normal layers. We should fix this
  // properly so it only happens for the shadow layer
  SkLayerRasterizer *raster = new SkLayerRasterizer();
  SkPaint maskPaint;
  SkSafeUnref(maskPaint.setShader(shader));
  raster->addLayer(maskPaint, 0, 0);
  
  SkPaint paint;
  paint.setAntiAlias(true);
  SkSafeUnref(paint.setRasterizer(raster));
  paint.setXfermodeMode(GfxOpToSkiaOp(aOperator));
  SkSafeUnref(paint.setLooper(dl));

  SkRect rect = RectToSkRect(Rect(aDest.x, aDest.y, bitmap.width(), bitmap.height()));
  mCanvas->drawRect(rect, paint);
  mCanvas->restore();
}

void
DrawTargetSkia::FillRect(const Rect &aRect,
                         const Pattern &aPattern,
                         const DrawOptions &aOptions)
{
  MarkChanged();
  SkRect rect = RectToSkRect(aRect);
  AutoPaintSetup paint(mCanvas.get(), aOptions, aPattern);

  mCanvas->drawRect(rect, paint.mPaint);
}

void
DrawTargetSkia::Stroke(const Path *aPath,
                       const Pattern &aPattern,
                       const StrokeOptions &aStrokeOptions,
                       const DrawOptions &aOptions)
{
  MarkChanged();
  if (aPath->GetBackendType() != BACKEND_SKIA) {
    return;
  }

  const PathSkia *skiaPath = static_cast<const PathSkia*>(aPath);


  AutoPaintSetup paint(mCanvas.get(), aOptions, aPattern);
  if (!StrokeOptionsToPaint(paint.mPaint, aStrokeOptions)) {
    return;
  }

  mCanvas->drawPath(skiaPath->GetPath(), paint.mPaint);
}

void
DrawTargetSkia::StrokeRect(const Rect &aRect,
                           const Pattern &aPattern,
                           const StrokeOptions &aStrokeOptions,
                           const DrawOptions &aOptions)
{
  MarkChanged();
  AutoPaintSetup paint(mCanvas.get(), aOptions, aPattern);
  if (!StrokeOptionsToPaint(paint.mPaint, aStrokeOptions)) {
    return;
  }

  mCanvas->drawRect(RectToSkRect(aRect), paint.mPaint);
}

void 
DrawTargetSkia::StrokeLine(const Point &aStart,
                           const Point &aEnd,
                           const Pattern &aPattern,
                           const StrokeOptions &aStrokeOptions,
                           const DrawOptions &aOptions)
{
  MarkChanged();
  AutoPaintSetup paint(mCanvas.get(), aOptions, aPattern);
  if (!StrokeOptionsToPaint(paint.mPaint, aStrokeOptions)) {
    return;
  }

  mCanvas->drawLine(SkFloatToScalar(aStart.x), SkFloatToScalar(aStart.y), 
                    SkFloatToScalar(aEnd.x), SkFloatToScalar(aEnd.y), 
                    paint.mPaint);
}

void
DrawTargetSkia::Fill(const Path *aPath,
                    const Pattern &aPattern,
                    const DrawOptions &aOptions)
{
  MarkChanged();
  if (aPath->GetBackendType() != BACKEND_SKIA) {
    return;
  }

  const PathSkia *skiaPath = static_cast<const PathSkia*>(aPath);

  AutoPaintSetup paint(mCanvas.get(), aOptions, aPattern);

  mCanvas->drawPath(skiaPath->GetPath(), paint.mPaint);
}

void
DrawTargetSkia::FillGlyphs(ScaledFont *aFont,
                           const GlyphBuffer &aBuffer,
                           const Pattern &aPattern,
                           const DrawOptions &aOptions)
{
  if (aFont->GetType() != FONT_MAC && aFont->GetType() != FONT_SKIA) {
    return;
  }

  MarkChanged();

  ScaledFontSkia* skiaFont = static_cast<ScaledFontSkia*>(aFont);

  AutoPaintSetup paint(mCanvas.get(), aOptions, aPattern);
  paint.mPaint.setTypeface(skiaFont->mTypeface);
  paint.mPaint.setTextSize(SkFloatToScalar(skiaFont->mSize));
  paint.mPaint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
  
  std::vector<uint16_t> indices;
  std::vector<SkPoint> offsets;
  indices.resize(aBuffer.mNumGlyphs);
  offsets.resize(aBuffer.mNumGlyphs);

  for (unsigned int i = 0; i < aBuffer.mNumGlyphs; i++) {
    indices[i] = aBuffer.mGlyphs[i].mIndex;
    offsets[i].fX = SkFloatToScalar(aBuffer.mGlyphs[i].mPosition.x);
    offsets[i].fY = SkFloatToScalar(aBuffer.mGlyphs[i].mPosition.y);
  }

  mCanvas->drawPosText(&indices.front(), aBuffer.mNumGlyphs*2, &offsets.front(), paint.mPaint);
}

TemporaryRef<SourceSurface>
DrawTargetSkia::CreateSourceSurfaceFromData(unsigned char *aData,
                                             const IntSize &aSize,
                                             int32_t aStride,
                                             SurfaceFormat aFormat) const
{
  RefPtr<SourceSurfaceSkia> newSurf = new SourceSurfaceSkia();

  if (!newSurf->InitFromData(aData, aSize, aStride, aFormat)) {
    gfxDebug() << *this << ": Failure to create source surface from data. Size: " << aSize;
    return NULL;
  }
    
  return newSurf;
}

TemporaryRef<DrawTarget>
DrawTargetSkia::CreateSimilarDrawTarget(const IntSize &aSize, SurfaceFormat aFormat) const
{
  RefPtr<DrawTargetSkia> target = new DrawTargetSkia();
  if (!target->Init(aSize, aFormat)) {
    return NULL;
  }
  return target;
}

TemporaryRef<SourceSurface>
DrawTargetSkia::OptimizeSourceSurface(SourceSurface *aSurface) const
{
  return NULL;
}

TemporaryRef<SourceSurface>
DrawTargetSkia::CreateSourceSurfaceFromNativeSurface(const NativeSurface &aSurface) const
{
  return NULL;
}

void
DrawTargetSkia::CopySurface(SourceSurface *aSurface,
                            const IntRect& aSourceRect,
                            const IntPoint &aDestination)
{
  //TODO: We could just use writePixels() here if the sourceRect is the entire source
  
    if (aSurface->GetType() != SURFACE_SKIA) {
    return;
  }

  MarkChanged();
  
  const SkBitmap& bitmap = static_cast<SourceSurfaceSkia*>(aSurface)->GetBitmap();

  mCanvas->save();
  mCanvas->resetMatrix();
  SkRect dest = IntRectToSkRect(IntRect(aDestination.x, aDestination.y, aSourceRect.width, aSourceRect.height)); 
  SkIRect source = IntRectToSkIRect(aSourceRect);
  mCanvas->clipRect(dest, SkRegion::kReplace_Op);
  SkPaint paint;
  paint.setXfermodeMode(GfxOpToSkiaOp(OP_SOURCE));
  mCanvas->drawBitmapRect(bitmap, &source, dest, &paint);
  mCanvas->restore();
}

bool
DrawTargetSkia::Init(const IntSize &aSize, SurfaceFormat aFormat)
{
  mBitmap.setConfig(GfxFormatToSkiaConfig(aFormat), aSize.width, aSize.height);
  if (!mBitmap.allocPixels()) {
    return false;
  }
  mBitmap.eraseARGB(0, 0, 0, 0);
  SkAutoTUnref<SkDevice> device(new SkDevice(mBitmap));
  SkAutoTUnref<SkCanvas> canvas(new SkCanvas(device.get()));
  mSize = aSize;

  mDevice = device.get();
  mCanvas = canvas.get();
  mFormat = aFormat;
  return true;
}

void
DrawTargetSkia::SetTransform(const Matrix& aTransform)
{
  SkMatrix mat;
  GfxMatrixToSkiaMatrix(aTransform, mat);
  mCanvas->setMatrix(mat);
  mTransform = aTransform;
}

TemporaryRef<PathBuilder> 
DrawTargetSkia::CreatePathBuilder(FillRule aFillRule) const
{
  RefPtr<PathBuilderSkia> pb = new PathBuilderSkia(aFillRule);
  return pb;
}

void
DrawTargetSkia::ClearRect(const Rect &aRect)
{
  MarkChanged();
  SkPaint paint;
  mCanvas->save();
  mCanvas->clipRect(RectToSkRect(aRect), SkRegion::kIntersect_Op);
  paint.setColor(SkColorSetARGB(0, 0, 0, 0));
  paint.setXfermodeMode(SkXfermode::kSrc_Mode);
  mCanvas->drawPaint(paint);
  mCanvas->restore();
}

void
DrawTargetSkia::PushClip(const Path *aPath)
{
  if (aPath->GetBackendType() != BACKEND_SKIA) {
    return;
  }

  const PathSkia *skiaPath = static_cast<const PathSkia*>(aPath);
  mCanvas->save(SkCanvas::kClip_SaveFlag);
  mCanvas->clipPath(skiaPath->GetPath());
}

void
DrawTargetSkia::PopClip()
{
  mCanvas->restore();
}

TemporaryRef<GradientStops>
DrawTargetSkia::CreateGradientStops(GradientStop *aStops, uint32_t aNumStops) const
{
  std::vector<GradientStop> stops;
  stops.resize(aNumStops);
  for (uint32_t i = 0; i < aNumStops; i++) {
    stops[i] = aStops[i];
  }
  std::stable_sort(stops.begin(), stops.end());
  
  return new GradientStopsSkia(stops, aNumStops);
}

void
DrawTargetSkia::AppendSnapshot(SourceSurfaceSkia* aSnapshot)
{
  mSnapshots.push_back(aSnapshot);
}

void
DrawTargetSkia::RemoveSnapshot(SourceSurfaceSkia* aSnapshot)
{
  std::vector<SourceSurfaceSkia*>::iterator iter = std::find(mSnapshots.begin(), mSnapshots.end(), aSnapshot);
  if (iter != mSnapshots.end()) {
    mSnapshots.erase(iter);
  }
}

void
DrawTargetSkia::MarkChanged()
{
  if (mSnapshots.size()) {
    for (std::vector<SourceSurfaceSkia*>::iterator iter = mSnapshots.begin();
         iter != mSnapshots.end(); iter++) {
      (*iter)->DrawTargetWillChange();
    }
    // All snapshots will now have copied data.
    mSnapshots.clear();
  }
}

}
}
