/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

//
// vie_autotest_render.cc
//

#include "vie_autotest_defines.h"
#include "vie_autotest.h"
#include "engine_configurations.h"

#include "video_render.h"

#include "tb_interfaces.h"
#include "tb_video_channel.h"
#include "tb_capture_device.h"

#if defined(WIN32)
#include <windows.h>
#include <ddraw.h>
#include <tchar.h>
#elif defined(WEBRTC_LINUX)
    //From windgi.h
    #undef RGB
    #define RGB(r,g,b)          ((unsigned long)(((unsigned char)(r)|((unsigned short)((unsigned char)(g))<<8))|(((unsigned long)(unsigned char)(b))<<16)))
    //From ddraw.h
/*    typedef struct _DDCOLORKEY
 {
 DWORD       dwColorSpaceLowValue;   // low boundary of color space that is to
 DWORD       dwColorSpaceHighValue;  // high boundary of color space that is
 } DDCOLORKEY;*/
#elif defined(WEBRTC_MAC)
#endif

class ViEAutoTestExternalRenderer: public webrtc::ExternalRenderer
{
public:
    ViEAutoTestExternalRenderer() :
        _width(0),
        _height(0)
    {
    }
    virtual int FrameSizeChange(unsigned int width, unsigned int height,
                                unsigned int numberOfStreams)
    {
        _width = width;
        _height = height;
        return 0;
    }

    virtual int DeliverFrame(unsigned char* buffer, int bufferSize,
                             uint32_t time_stamp,
                             int64_t render_time)
    {
        if (bufferSize != _width * _height * 3 / 2)
        {
            ViETest::Log("incorrect render buffer received, of length = %d\n",
                         bufferSize);
            return 0;
        }
        return 0;
    }

public:
    virtual ~ViEAutoTestExternalRenderer()
    {
    }
private:
    int _width, _height;
};

void ViEAutoTest::ViERenderStandardTest()
{
    //***************************************************************
    //	Begin create/initialize WebRTC Video Engine for testing
    //***************************************************************
    int rtpPort = 6000;

    TbInterfaces ViE("ViERenderStandardTest");

    // Create a video channel
    TbVideoChannel tbChannel(ViE, webrtc::kVideoCodecVP8);
    TbCaptureDevice tbCapture(ViE); // Create a capture device
    tbCapture.ConnectTo(tbChannel.videoChannel);
    tbChannel.StartReceive(rtpPort);
    tbChannel.StartSend(rtpPort);

    EXPECT_EQ(0, ViE.render->RegisterVideoRenderModule(*_vrm1));
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbCapture.captureId, _window1, 0, 0.0, 0.0, 1.0, 1.0));
    EXPECT_EQ(0, ViE.render->StartRender(tbCapture.captureId));
    EXPECT_EQ(0, ViE.render->RegisterVideoRenderModule(*_vrm2));
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbChannel.videoChannel, _window2, 1, 0.0, 0.0, 1.0, 1.0));
    EXPECT_EQ(0, ViE.render->StartRender(tbChannel.videoChannel));

    ViETest::Log("\nCapture device is renderered in Window 1");
    ViETest::Log("Remote stream is renderered in Window 2");
    AutoTestSleep(KAutoTestSleepTimeMs);

    EXPECT_EQ(0, ViE.render->StopRender(tbCapture.captureId));
    EXPECT_EQ(0, ViE.render->RemoveRenderer(tbCapture.captureId));

    // PIP and full screen rendering is not supported on Android
#ifndef WEBRTC_ANDROID
    EXPECT_EQ(0, ViE.render->DeRegisterVideoRenderModule(*_vrm1));
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbCapture.captureId, _window2, 0, 0.75, 0.75, 1.0, 1.0));
    EXPECT_EQ(0, ViE.render->StartRender(tbCapture.captureId));

    ViETest::Log("\nCapture device is now rendered in Window 2, PiP.");
    ViETest::Log("Switching to full screen rendering in %d seconds.\n",
                 KAutoTestSleepTimeMs / 1000);
    AutoTestSleep(KAutoTestSleepTimeMs);

    EXPECT_EQ(0, ViE.render->RemoveRenderer(tbCapture.captureId));
    EXPECT_EQ(0, ViE.render->RemoveRenderer(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.render->DeRegisterVideoRenderModule(*_vrm2));

    // Destroy render module and create new in full screen mode
    webrtc::VideoRender::DestroyVideoRender(_vrm1);
    _vrm1 = NULL;
    _vrm1 = webrtc::VideoRender::CreateVideoRender(
        4563, _window1, true, _renderType);
    EXPECT_TRUE(_vrm1 != NULL);

    EXPECT_EQ(0, ViE.render->RegisterVideoRenderModule(*_vrm1));
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbCapture.captureId, _window1, 0, 0.75f, 0.75f, 1.0f, 1.0f));
    EXPECT_EQ(0, ViE.render->StartRender(tbCapture.captureId));
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbChannel.videoChannel, _window1, 1, 0.0, 0.0, 1.0, 1.0));
    EXPECT_EQ(0, ViE.render->StartRender(tbChannel.videoChannel));

    AutoTestSleep(KAutoTestSleepTimeMs);

    EXPECT_EQ(0, ViE.render->RemoveRenderer(tbCapture.captureId));

    EXPECT_EQ(0, ViE.render->RemoveRenderer(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.render->DeRegisterVideoRenderModule(*_vrm1));

    // Destroy full screen render module and create new in normal mode
    webrtc::VideoRender::DestroyVideoRender(_vrm1);
    _vrm1 = NULL;
    _vrm1 = webrtc::VideoRender::CreateVideoRender(
        4561, _window1, false, _renderType);
    EXPECT_TRUE(_vrm1 != NULL);
#endif

    //***************************************************************
    //	Engine ready. Begin testing class
    //***************************************************************


    //***************************************************************
    //	Testing finished. Tear down Video Engine
    //***************************************************************
    tbCapture.Disconnect(tbChannel.videoChannel);
}

void ViEAutoTest::ViERenderExtendedTest()
{
    int rtpPort = 6000;

    TbInterfaces ViE("ViERenderExtendedTest");

    // Create a video channel
    TbVideoChannel tbChannel(ViE, webrtc::kVideoCodecVP8);
    TbCaptureDevice tbCapture(ViE); // Create a capture device
    tbCapture.ConnectTo(tbChannel.videoChannel);
    tbChannel.StartReceive(rtpPort);
    tbChannel.StartSend(rtpPort);

    EXPECT_EQ(0, ViE.render->RegisterVideoRenderModule(*_vrm1));
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbCapture.captureId, _window1, 0, 0.0, 0.0, 1.0, 1.0));
    EXPECT_EQ(0, ViE.render->StartRender(tbCapture.captureId));
    EXPECT_EQ(0, ViE.render->RegisterVideoRenderModule(*_vrm2));
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbChannel.videoChannel, _window2, 1, 0.0, 0.0, 1.0, 1.0));
    EXPECT_EQ(0, ViE.render->StartRender(tbChannel.videoChannel));

    ViETest::Log("\nCapture device is renderered in Window 1");
    ViETest::Log("Remote stream is renderered in Window 2");
    AutoTestSleep(KAutoTestSleepTimeMs);

#ifdef _WIN32
    ViETest::Log("\nConfiguring Window2");
    ViETest::Log("you will see video only in first quadrant");
    EXPECT_EQ(0, ViE.render->ConfigureRender(
        tbChannel.videoChannel, 0, 0.0f, 0.0f, 0.5f, 0.5f));
    AutoTestSleep(KAutoTestSleepTimeMs);

    ViETest::Log("you will see video only in fourth quadrant");
    EXPECT_EQ(0, ViE.render->ConfigureRender(
        tbChannel.videoChannel, 0, 0.5f, 0.5f, 1.0f, 1.0f));
    AutoTestSleep(KAutoTestSleepTimeMs);

    ViETest::Log("normal video on Window2");
    EXPECT_EQ(0, ViE.render->ConfigureRender(
        tbChannel.videoChannel, 0, 0.0f, 0.0f, 1.0f, 1.0f));
    AutoTestSleep(KAutoTestSleepTimeMs);
#endif

    ViETest::Log("Mirroring Local Preview (Window1) Left-Right");
    EXPECT_EQ(0, ViE.render->MirrorRenderStream(
        tbCapture.captureId, true, false, true));
    AutoTestSleep(KAutoTestSleepTimeMs);

    ViETest::Log("\nMirroring Local Preview (Window1) Left-Right and Up-Down");
    EXPECT_EQ(0, ViE.render->MirrorRenderStream(
        tbCapture.captureId, true, true, true));
    AutoTestSleep(KAutoTestSleepTimeMs);

    ViETest::Log("\nMirroring Remote Window(Window2) Up-Down");
    EXPECT_EQ(0, ViE.render->MirrorRenderStream(
        tbChannel.videoChannel, true, true, false));
    AutoTestSleep(KAutoTestSleepTimeMs);

    ViETest::Log("Disabling Mirroing on Window1 and Window2");
    EXPECT_EQ(0, ViE.render->MirrorRenderStream(
        tbCapture.captureId, false, false, false));
    AutoTestSleep(KAutoTestSleepTimeMs);
    EXPECT_EQ(0, ViE.render->MirrorRenderStream(
        tbChannel.videoChannel, false, false, false));
    AutoTestSleep(KAutoTestSleepTimeMs);

    ViETest::Log("\nEnabling Full Screen render in 5 sec");

    EXPECT_EQ(0, ViE.render->RemoveRenderer(tbCapture.captureId));
    EXPECT_EQ(0, ViE.render->DeRegisterVideoRenderModule(*_vrm1));
    EXPECT_EQ(0, ViE.render->RemoveRenderer(tbChannel.videoChannel));
    EXPECT_EQ(0, ViE.render->DeRegisterVideoRenderModule(*_vrm2));

    // Destroy render module and create new in full screen mode
    webrtc::VideoRender::DestroyVideoRender(_vrm1);
    _vrm1 = NULL;
    _vrm1 = webrtc::VideoRender::CreateVideoRender(
        4563, _window1, true, _renderType);
    EXPECT_TRUE(_vrm1 != NULL);

    EXPECT_EQ(0, ViE.render->RegisterVideoRenderModule(*_vrm1));
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbCapture.captureId, _window1, 0, 0.0f, 0.0f, 1.0f, 1.0f));
    EXPECT_EQ(0, ViE.render->StartRender(tbCapture.captureId));
    AutoTestSleep(KAutoTestSleepTimeMs);

    ViETest::Log("\nStop renderer");
    EXPECT_EQ(0, ViE.render->StopRender(tbCapture.captureId));
    ViETest::Log("\nRemove renderer");
    EXPECT_EQ(0, ViE.render->RemoveRenderer(tbCapture.captureId));

    EXPECT_EQ(0, ViE.render->DeRegisterVideoRenderModule(*_vrm1));

    // Destroy full screen render module and create new for external rendering
    webrtc::VideoRender::DestroyVideoRender(_vrm1);
    _vrm1 = NULL;
    _vrm1 = webrtc::VideoRender::CreateVideoRender(4564, NULL, false,
                                                   _renderType);
    EXPECT_TRUE(_vrm1 != NULL);

    EXPECT_EQ(0, ViE.render->RegisterVideoRenderModule(*_vrm1));

    ViETest::Log("\nExternal Render Test");
    ViEAutoTestExternalRenderer externalRenderObj;
    EXPECT_EQ(0, ViE.render->AddRenderer(
        tbCapture.captureId, webrtc::kVideoI420, &externalRenderObj));
    EXPECT_EQ(0, ViE.render->StartRender(tbCapture.captureId));
    AutoTestSleep(KAutoTestSleepTimeMs);

    EXPECT_EQ(0, ViE.render->StopRender(tbCapture.captureId));
    EXPECT_EQ(0, ViE.render->RemoveRenderer(tbCapture.captureId));
    EXPECT_EQ(0, ViE.render->DeRegisterVideoRenderModule(*_vrm1));

    // Destroy render module for external rendering and create new in normal
    // mode
    webrtc::VideoRender::DestroyVideoRender(_vrm1);
    _vrm1 = NULL;
    _vrm1 = webrtc::VideoRender::CreateVideoRender(
        4561, _window1, false, _renderType);
    EXPECT_TRUE(_vrm1 != NULL);
    tbCapture.Disconnect(tbChannel.videoChannel);
}

void ViEAutoTest::ViERenderAPITest()
{
    // TODO(unknown): add the real tests cases
}
