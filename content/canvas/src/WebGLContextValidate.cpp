/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 *   Mozilla Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Vladimir Vukicevic <vladimir@pobox.com> (original author)
 *   Mark Steele <mwsteele@gmail.com>
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

#include "WebGLContext.h"

using namespace mozilla;

/*
 * Pull all the data out of the program that will be used by validate later on
 */
PRBool
WebGLProgram::UpdateInfo(gl::GLContext *gl)
{
    gl->fGetProgramiv(mName, LOCAL_GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &mAttribMaxNameLength);
    gl->fGetProgramiv(mName, LOCAL_GL_ACTIVE_UNIFORM_MAX_LENGTH, &mUniformMaxNameLength);
    gl->fGetProgramiv(mName, LOCAL_GL_ACTIVE_UNIFORMS, &mUniformCount);
    gl->fGetProgramiv(mName, LOCAL_GL_ACTIVE_ATTRIBUTES, &mAttribCount);

    GLint numVertexAttribs;
    gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_ATTRIBS, &numVertexAttribs);
    mAttribsInUse.clear();
    mAttribsInUse.resize(numVertexAttribs);

    nsAutoArrayPtr<char> nameBuf(new char[mAttribMaxNameLength]);

    for (int i = 0; i < mAttribCount; ++i) {
        GLint attrnamelen;
        GLint attrsize;
        GLenum attrtype;
        gl->fGetActiveAttrib(mName, i, mAttribMaxNameLength, &attrnamelen, &attrsize, &attrtype, nameBuf);
        if (attrnamelen > 0) {
            GLint loc = gl->fGetAttribLocation(mName, nameBuf);
            mAttribsInUse[loc] = true;
        }
    }

    return PR_TRUE;
}

/*
 * Verify that we can read count consecutive elements from each bound VBO.
 */

PRBool
WebGLContext::ValidateBuffers(PRUint32 count)
{
    NS_ENSURE_TRUE(count > 0, PR_TRUE);

#ifdef DEBUG
    GLuint currentProgram = 0;
    MakeContextCurrent();
    gl->fGetIntegerv(LOCAL_GL_CURRENT_PROGRAM, (GLint*) &currentProgram);
    NS_ASSERTION(currentProgram == mCurrentProgram->GLName(),
                 "WebGL: current program doesn't agree with GL state");
    if (currentProgram != mCurrentProgram->GLName())
        return PR_FALSE;
#endif

    PRUint32 attribs = mAttribBuffers.Length();
    for (PRUint32 i = 0; i < attribs; ++i) {
        const WebGLVertexAttribData& vd = mAttribBuffers[i];

        // If the attrib array isn't enabled, there's nothing to check;
        // it's a static value.
        if (!vd.enabled)
            continue;

        if (vd.buf == nsnull) {
            LogMessage("No VBO bound to enabled attrib index %d!", i);
            return PR_FALSE;
        }

        // If the attrib is not in use, then we don't have to validate
        // it, just need to make sure that the binding is non-null.
        if (!mCurrentProgram->IsAttribInUse(i))
            continue;

        // compute the number of bytes we actually need
        WebGLuint needed = vd.byteOffset +     // the base offset
            vd.actualStride() * (count-1) +    // to stride to the start of the last element group
            vd.componentSize() * vd.size;      // and the number of bytes needed for these components

        if (vd.buf->ByteLength() < needed) {
            LogMessage("VBO too small for bound attrib index %d: need at least %d bytes, but have only %d", i, needed, vd.buf->ByteLength());
            return PR_FALSE;
        }
    }

    return PR_TRUE;
}

PRBool WebGLContext::ValidateCapabilityEnum(WebGLenum cap, const char *info)
{
    switch (cap) {
        case LOCAL_GL_BLEND:
        case LOCAL_GL_CULL_FACE:
        case LOCAL_GL_DEPTH_TEST:
        case LOCAL_GL_DITHER:
        case LOCAL_GL_POLYGON_OFFSET_FILL:
        case LOCAL_GL_SAMPLE_ALPHA_TO_COVERAGE:
        case LOCAL_GL_SAMPLE_COVERAGE:
        case LOCAL_GL_SCISSOR_TEST:
        case LOCAL_GL_STENCIL_TEST:
            return PR_TRUE;
        default:
            ErrorInvalidEnumInfo(info);
            return PR_FALSE;
    }
}

PRBool WebGLContext::ValidateBlendEquationEnum(WebGLenum mode, const char *info)
{
    switch (mode) {
        case LOCAL_GL_FUNC_ADD:
        case LOCAL_GL_FUNC_SUBTRACT:
        case LOCAL_GL_FUNC_REVERSE_SUBTRACT:
            return PR_TRUE;
        default:
            ErrorInvalidEnumInfo(info);
            return PR_FALSE;
    }
}

PRBool WebGLContext::ValidateBlendFuncDstEnum(WebGLenum factor, const char *info)
{
    switch (factor) {
        case LOCAL_GL_ZERO:
        case LOCAL_GL_ONE:
        case LOCAL_GL_SRC_COLOR:
        case LOCAL_GL_ONE_MINUS_SRC_COLOR:
        case LOCAL_GL_DST_COLOR:
        case LOCAL_GL_ONE_MINUS_DST_COLOR:
        case LOCAL_GL_SRC_ALPHA:
        case LOCAL_GL_ONE_MINUS_SRC_ALPHA:
        case LOCAL_GL_DST_ALPHA:
        case LOCAL_GL_ONE_MINUS_DST_ALPHA:
        case LOCAL_GL_CONSTANT_COLOR:
        case LOCAL_GL_ONE_MINUS_CONSTANT_COLOR:
        case LOCAL_GL_CONSTANT_ALPHA:
        case LOCAL_GL_ONE_MINUS_CONSTANT_ALPHA:
            return PR_TRUE;
        default:
            ErrorInvalidEnumInfo(info);
            return PR_FALSE;
    }
}

PRBool WebGLContext::ValidateBlendFuncSrcEnum(WebGLenum factor, const char *info)
{
    if (factor == LOCAL_GL_SRC_ALPHA_SATURATE)
        return PR_TRUE;
    else
        return ValidateBlendFuncDstEnum(factor, info);
}

PRBool WebGLContext::ValidateTextureTargetEnum(WebGLenum target, const char *info)
{
    switch (target) {
        case LOCAL_GL_TEXTURE_2D:
        case LOCAL_GL_TEXTURE_CUBE_MAP:
            return PR_TRUE;
        default:
            ErrorInvalidEnumInfo(info);
            return PR_FALSE;
    }
}

PRBool WebGLContext::ValidateComparisonEnum(WebGLenum target, const char *info)
{
    switch (target) {
        case LOCAL_GL_NEVER:
        case LOCAL_GL_LESS:
        case LOCAL_GL_LEQUAL:
        case LOCAL_GL_GREATER:
        case LOCAL_GL_GEQUAL:
        case LOCAL_GL_EQUAL:
        case LOCAL_GL_NOTEQUAL:
        case LOCAL_GL_ALWAYS:
            return PR_TRUE;
        default:
            ErrorInvalidEnumInfo(info);
            return PR_FALSE;
    }
}

PRBool WebGLContext::ValidateStencilOpEnum(WebGLenum action, const char *info)
{
    switch (action) {
        case LOCAL_GL_KEEP:
        case LOCAL_GL_ZERO:
        case LOCAL_GL_REPLACE:
        case LOCAL_GL_INCR:
        case LOCAL_GL_INCR_WRAP:
        case LOCAL_GL_DECR:
        case LOCAL_GL_DECR_WRAP:
        case LOCAL_GL_INVERT:
            return PR_TRUE;
        default:
            ErrorInvalidEnumInfo(info);
            return PR_FALSE;
    }
}

PRBool WebGLContext::ValidateFaceEnum(WebGLenum target, const char *info)
{
    switch (target) {
        case LOCAL_GL_FRONT:
        case LOCAL_GL_BACK:
        case LOCAL_GL_FRONT_AND_BACK:
            return PR_TRUE;
        default:
            ErrorInvalidEnumInfo(info);
            return PR_FALSE;
    }
}

PRBool WebGLContext::ValidateTexFormatAndType(WebGLenum format, WebGLenum type,
                                                PRUint32 *texelSize, const char *info)
{
    if (type == LOCAL_GL_UNSIGNED_BYTE)
    {
        switch (format) {
            case LOCAL_GL_RED:
            case LOCAL_GL_GREEN:
            case LOCAL_GL_BLUE:
            case LOCAL_GL_ALPHA:
            case LOCAL_GL_LUMINANCE:
                *texelSize = 1;
                return PR_TRUE;
            case LOCAL_GL_LUMINANCE_ALPHA:
                *texelSize = 2;
                return PR_TRUE;
            case LOCAL_GL_RGB:
                *texelSize = 3;
                return PR_TRUE;
            case LOCAL_GL_RGBA:
                *texelSize = 4;
                return PR_TRUE;
            default:
                ErrorInvalidEnum("%s: invalid format", info);
                return PR_FALSE;
        }
    } else {
        switch (type) {
            case LOCAL_GL_UNSIGNED_SHORT_4_4_4_4:
            case LOCAL_GL_UNSIGNED_SHORT_5_5_5_1:
                if (format == LOCAL_GL_RGBA) {
                    *texelSize = 2;
                    return PR_TRUE;
                } else {
                    ErrorInvalidOperation("%s: mutually incompatible format and type", info);
                    return PR_FALSE;
                }
            case LOCAL_GL_UNSIGNED_SHORT_5_6_5:
                if (format == LOCAL_GL_RGB) {
                    *texelSize = 2;
                    return PR_TRUE;
                } else {
                    ErrorInvalidOperation("%s: mutually incompatible format and type", info);
                    return PR_FALSE;
                }
            default:
                ErrorInvalidEnum("%s: invalid type", info);
                return PR_FALSE;
        }
    }
}

PRBool
WebGLContext::InitAndValidateGL()
{
    if (!gl) return PR_FALSE;

    mActiveTexture = 0;
    mSynthesizedGLError = LOCAL_GL_NO_ERROR;

    mAttribBuffers.Clear();

    mUniformTextures.Clear();
    mBound2DTextures.Clear();
    mBoundCubeMapTextures.Clear();

    mBoundArrayBuffer = nsnull;
    mBoundElementArrayBuffer = nsnull;
    mCurrentProgram = nsnull;

    mFramebufferColorAttachments.Clear();
    mFramebufferDepthAttachment = nsnull;
    mFramebufferStencilAttachment = nsnull;

    mBoundFramebuffer = nsnull;
    mBoundRenderbuffer = nsnull;

    mMapTextures.Clear();
    mMapBuffers.Clear();
    mMapPrograms.Clear();
    mMapShaders.Clear();
    mMapFramebuffers.Clear();
    mMapRenderbuffers.Clear();

    // make sure that the opengl stuff that we need is supported
    GLint val = 0;

    // XXX this exposes some strange latent bug; what's going on?
    //MakeContextCurrent();

    gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_ATTRIBS, &val);
    if (val == 0) {
        LogMessage("GL_MAX_VERTEX_ATTRIBS is 0!");
        return PR_FALSE;
    }

    mAttribBuffers.SetLength(val);

    //fprintf(stderr, "GL_MAX_VERTEX_ATTRIBS: %d\n", val);

    // Note: GL_MAX_TEXTURE_UNITS is fixed at 4 for most desktop hardware,
    // even though the hardware supports much more.  The
    // GL_MAX_{COMBINED_}TEXTURE_IMAGE_UNITS value is the accurate
    // value.  For GLES2, GL_MAX_TEXTURE_UNITS is still correct.
    gl->fGetIntegerv(LOCAL_GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &val);
    if (val == 0) {
        LogMessage("GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS is 0!");
        return PR_FALSE;
    }

    mBound2DTextures.SetLength(val);
    mBoundCubeMapTextures.SetLength(val);

    //fprintf(stderr, "GL_MAX_TEXTURE_UNITS: %d\n", val);

    gl->fGetIntegerv(LOCAL_GL_MAX_COLOR_ATTACHMENTS, &val);
    mFramebufferColorAttachments.SetLength(val);

#if defined(DEBUG_vladimir) && defined(USE_GLES2)
    gl->fGetIntegerv(LOCAL_GL_IMPLEMENTATION_COLOR_READ_FORMAT, &val);
    fprintf(stderr, "GL_IMPLEMENTATION_COLOR_READ_FORMAT: 0x%04x\n", val);

    gl->fGetIntegerv(LOCAL_GL_IMPLEMENTATION_COLOR_READ_TYPE, &val);
    fprintf(stderr, "GL_IMPLEMENTATION_COLOR_READ_TYPE: 0x%04x\n", val);
#endif

#ifndef USE_GLES2
    // gl_PointSize is always available in ES2 GLSL, but has to be
    // specifically enabled on desktop GLSL.
    gl->fEnable(LOCAL_GL_VERTEX_PROGRAM_POINT_SIZE);
#endif

    return PR_TRUE;
}
