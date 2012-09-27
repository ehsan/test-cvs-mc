/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsMediaOmxDecoder.h"
#include "nsMediaOmxReader.h"
#include "nsBuiltinDecoderStateMachine.h"

nsMediaOmxDecoder::nsMediaOmxDecoder() :
  nsBuiltinDecoder()
{
}

nsMediaDecoder* nsMediaOmxDecoder::Clone()
{
  return new nsMediaOmxDecoder();
}

nsDecoderStateMachine* nsMediaOmxDecoder::CreateStateMachine()
{
  return new nsBuiltinDecoderStateMachine(this, new nsMediaOmxReader(this));
}

nsMediaOmxDecoder::~nsMediaOmxDecoder()
{
}
