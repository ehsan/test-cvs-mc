/*
 * Copyright (c) 2009 The Chromium Authors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

description("Tests generation of synthetic and real GL errors");

var context = create3DContext();
var program = loadStandardProgram(context);

// Other tests in this directory like getActiveTest and
// incorrect-context-object-behaviour already test the raising of many
// synthetic GL errors. This test verifies the raising of certain
// known real GL errors, and contains a few regression tests for bugs
// discovered in the synthetic error generation and in the WebGL
// implementation itself.

shouldBe("context.getError()", "0");

debug("Testing getActiveAttrib");
// Synthetic OpenGL error
shouldBeNull("context.getActiveAttrib(null, 2)");
glErrorShouldBe(context, context.INVALID_OPERATION);
// Error state should be clear by this point
glErrorShouldBe(context, context.NO_ERROR);
// Real OpenGL error
shouldBeNull("context.getActiveAttrib(program, 2)");
glErrorShouldBe(context, context.INVALID_VALUE);
// Error state should be clear by this point
glErrorShouldBe(context, context.NO_ERROR);

debug("Testing getActiveUniform");
// Synthetic OpenGL error
shouldBeNull("context.getActiveUniform(null, 0)");
glErrorShouldBe(context, context.INVALID_OPERATION);
// Error state should be clear by this point
glErrorShouldBe(context, context.NO_ERROR);
// Real OpenGL error
shouldBeNull("context.getActiveUniform(program, 50)");
glErrorShouldBe(context, context.INVALID_VALUE);
// Error state should be clear by this point
glErrorShouldBe(context, context.NO_ERROR);

debug("Testing attempts to manipulate the default framebuffer");
shouldBeUndefined("context.bindFramebuffer(context.FRAMEBUFFER, 0)");
glErrorShouldBe(context, context.NO_ERROR);
shouldBeUndefined("context.framebufferRenderbuffer(context.FRAMEBUFFER, context.DEPTH_ATTACHMENT, context.RENDERBUFFER, 0)");
// Synthetic OpenGL error
glErrorShouldBe(context, context.INVALID_OPERATION);
// Error state should be clear by this point
glErrorShouldBe(context, context.NO_ERROR);
shouldBeUndefined("context.framebufferTexture2D(context.FRAMEBUFFER, context.COLOR_ATTACHMENT0, context.TEXTURE_2D, 0, 0)");
// Synthetic OpenGL error
glErrorShouldBe(context, context.INVALID_OPERATION);
// Error state should be clear by this point
glErrorShouldBe(context, context.NO_ERROR);

successfullyParsed = true;
