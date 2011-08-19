// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

precision mediump float;

varying vec2 vTexcoord;
varying vec4 vColor;

void main()
{
   gl_FragColor = vec4(
     abs(vColor.xyz * 2.0 - vec3(1, 1, 1)),
     1);
}


