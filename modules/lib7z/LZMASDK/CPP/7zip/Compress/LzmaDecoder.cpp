// LzmaDecoder.cpp

#include "StdAfx.h"

#include "../../../C/Alloc.h"

#include "../Common/StreamUtils.h"

#include "LzmaDecoder.h"

static HRESULT SResToHRESULT(SRes res)
{
  switch(res)
  {
    case SZ_OK: return S_OK;
    case SZ_ERROR_MEM: return E_OUTOFMEMORY;
    case SZ_ERROR_PARAM: return E_INVALIDARG;
    case SZ_ERROR_UNSUPPORTED: return E_NOTIMPL;
    case SZ_ERROR_DATA: return S_FALSE;
  }
  return E_FAIL;
}

namespace NCompress {
namespace NLzma {

static const UInt32 kInBufSize = 1 << 20;

CDecoder::CDecoder(): _inBuf(0), _propsWereSet(false), _outSizeDefined(false), FinishStream(false)
{
  _inSizeProcessed = 0;
  _inPos = _inSize = 0;
  LzmaDec_Construct(&_state);
}

static void *SzAlloc(void *p, size_t size) { p = p; return MyAlloc(size); }
static void SzFree(void *p, void *address) { p = p; MyFree(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

CDecoder::~CDecoder()
{
  LzmaDec_Free(&_state, &g_Alloc);
  MyFree(_inBuf);
}

HRESULT CDecoder::CreateInputBuffer()
{
  if (_inBuf == 0)
  {
    _inBuf = (Byte *)MyAlloc(kInBufSize);
    if (_inBuf == 0)
      return E_OUTOFMEMORY;
  }
  return S_OK;
}

STDMETHODIMP CDecoder::SetDecoderProperties2(const Byte *prop, UInt32 size)
{
  RINOK(SResToHRESULT(LzmaDec_Allocate(&_state, prop, size, &g_Alloc)));
  _propsWereSet = true;
  return CreateInputBuffer();
}

void CDecoder::SetOutStreamSizeResume(const UInt64 *outSize)
{
  _outSizeDefined = (outSize != NULL);
  if (_outSizeDefined)
    _outSize = *outSize;
  _outSizeProcessed = 0;
  LzmaDec_Init(&_state);
}

STDMETHODIMP CDecoder::SetOutStreamSize(const UInt64 *outSize)
{
  _inSizeProcessed = 0;
  _inPos = _inSize = 0;
  SetOutStreamSizeResume(outSize);
  return S_OK;
}

HRESULT CDecoder::CodeSpec(ISequentialInStream *inStream, ISequentialOutStream *outStream, ICompressProgressInfo *progress)
{
  if (_inBuf == 0 || !_propsWereSet)
    return S_FALSE;

  UInt64 startInProgress = _inSizeProcessed;

  for (;;)
  {
    if (_inPos == _inSize)
    {
      _inPos = _inSize = 0;
      RINOK(inStream->Read(_inBuf, kInBufSize, &_inSize));
    }

    SizeT dicPos = _state.dicPos;
    SizeT curSize = _state.dicBufSize - dicPos;
    const UInt32 kStepSize = ((UInt32)1 << 22);
    if (curSize > kStepSize)
      curSize = (SizeT)kStepSize;
    
    ELzmaFinishMode finishMode = LZMA_FINISH_ANY;
    if (_outSizeDefined)
    {
      const UInt64 rem = _outSize - _outSizeProcessed;
      if (rem < curSize)
      {
        curSize = (SizeT)rem;
        if (FinishStream)
          finishMode = LZMA_FINISH_END;
      }
    }

    SizeT inSizeProcessed = _inSize - _inPos;
    ELzmaStatus status;
    SRes res = LzmaDec_DecodeToDic(&_state, dicPos + curSize, _inBuf + _inPos, &inSizeProcessed, finishMode, &status);

    _inPos += (UInt32)inSizeProcessed;
    _inSizeProcessed += inSizeProcessed;
    SizeT outSizeProcessed = _state.dicPos - dicPos;
    _outSizeProcessed += outSizeProcessed;

    bool finished = (inSizeProcessed == 0 && outSizeProcessed == 0);
    bool stopDecoding = (_outSizeDefined && _outSizeProcessed >= _outSize);

    if (res != 0 || _state.dicPos == _state.dicBufSize || finished || stopDecoding)
    {
      HRESULT res2 = WriteStream(outStream, _state.dic, _state.dicPos);
      if (res != 0)
        return S_FALSE;
      RINOK(res2);
      if (stopDecoding)
        return S_OK;
      if (finished)
        return (status == LZMA_STATUS_FINISHED_WITH_MARK ? S_OK : S_FALSE);
    }
    if (_state.dicPos == _state.dicBufSize)
      _state.dicPos = 0;

    if (progress)
    {
      UInt64 inSize = _inSizeProcessed - startInProgress;
      RINOK(progress->SetRatioInfo(&inSize, &_outSizeProcessed));
    }
  }
}

STDMETHODIMP CDecoder::Code(ISequentialInStream *inStream, ISequentialOutStream *outStream,
    const UInt64 * /* inSize */, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  SetOutStreamSize(outSize);
  return CodeSpec(inStream, outStream, progress);
}

#ifndef NO_READ_FROM_CODER

STDMETHODIMP CDecoder::SetInStream(ISequentialInStream *inStream) { _inStream = inStream; return S_OK; }
STDMETHODIMP CDecoder::ReleaseInStream() { _inStream.Release(); return S_OK; }

STDMETHODIMP CDecoder::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize)
    *processedSize = 0;
  do
  {
    if (_inPos == _inSize)
    {
      _inPos = _inSize = 0;
      RINOK(_inStream->Read(_inBuf, kInBufSize, &_inSize));
    }
    {
      SizeT inProcessed = _inSize - _inPos;

      if (_outSizeDefined)
      {
        const UInt64 rem = _outSize - _outSizeProcessed;
        if (rem < size)
          size = (UInt32)rem;
      }

      SizeT outProcessed = size;
      ELzmaStatus status;
      SRes res = LzmaDec_DecodeToBuf(&_state, (Byte *)data, &outProcessed,
          _inBuf + _inPos, &inProcessed, LZMA_FINISH_ANY, &status);
      _inPos += (UInt32)inProcessed;
      _inSizeProcessed += inProcessed;
      _outSizeProcessed += outProcessed;
      size -= (UInt32)outProcessed;
      data = (Byte *)data + outProcessed;
      if (processedSize)
        *processedSize += (UInt32)outProcessed;
      RINOK(SResToHRESULT(res));
      if (inProcessed == 0 && outProcessed == 0)
        return S_OK;
    }
  }
  while (size != 0);
  return S_OK;
}

HRESULT CDecoder::CodeResume(ISequentialOutStream *outStream, const UInt64 *outSize, ICompressProgressInfo *progress)
{
  SetOutStreamSizeResume(outSize);
  return CodeSpec(_inStream, outStream, progress);
}

HRESULT CDecoder::ReadFromInputStream(void *data, UInt32 size, UInt32 *processedSize)
{
  RINOK(CreateInputBuffer());
  if (processedSize)
    *processedSize = 0;
  while (size > 0)
  {
    if (_inPos == _inSize)
    {
      _inPos = _inSize = 0;
      RINOK(_inStream->Read(_inBuf, kInBufSize, &_inSize));
      if (_inSize == 0)
        break;
    }
    {
      UInt32 curSize = _inSize - _inPos;
      if (curSize > size)
        curSize = size;
      memcpy(data, _inBuf + _inPos, curSize);
      _inPos += curSize;
      _inSizeProcessed += curSize;
      size -= curSize;
      data = (Byte *)data + curSize;
      if (processedSize)
        *processedSize += curSize;
    }
  }
  return S_OK;
}

#endif

}}
