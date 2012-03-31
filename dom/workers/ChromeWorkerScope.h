/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_workers_chromeworkerscope_h__
#define mozilla_dom_workers_chromeworkerscope_h__

#include "Workers.h"

BEGIN_WORKERS_NAMESPACE

bool
DefineChromeWorkerFunctions(JSContext* aCx, JSObject* aGlobal);

END_WORKERS_NAMESPACE

#endif // mozilla_dom_workers_chromeworkerscope_h__
