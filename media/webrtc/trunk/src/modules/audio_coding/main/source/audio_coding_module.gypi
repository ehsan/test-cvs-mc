# Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

{
  'variables': {
    'audio_coding_dependencies': [
      'CNG',
      'NetEq',
      '<(webrtc_root)/common_audio/common_audio.gyp:resampler',
      '<(webrtc_root)/common_audio/common_audio.gyp:signal_processing',
      '<(webrtc_root)/common_audio/common_audio.gyp:vad',
      '<(webrtc_root)/system_wrappers/source/system_wrappers.gyp:system_wrappers',
    ],
    'audio_coding_defines': [],
  },
  'targets': [
    {
      'target_name': 'audio_coding_module',
      'type': '<(library)',
      'defines': [
        '<@(audio_coding_defines)',
      ],
      'dependencies': [
        '<@(audio_coding_dependencies)',
      ],
      'conditions': [
        ['codec_g711_enable!=0', {
          'dependencies': [
            'G711',
          ],
          'sources': [
            'acm_pcma.cc',
            'acm_pcma.h',
            'acm_pcmu.cc',
            'acm_pcmu.h',
          ],
        }],
        ['codec_g722_enable!=0', {
          'dependencies': [
            'G722',
          ],
          'sources': [
            'acm_g722.cc',
            'acm_g722.h',
            'acm_g7221.cc',
            'acm_g7221.h',
            'acm_g7221c.cc',
            'acm_g7221c.h',
          ],
        }],
        ['codec_ilbc_enable!=0', {
          'dependencies': [
            'iLBC',
          ],
          'sources': [
            'acm_ilbc.cc',
            'acm_ilbc.h',
          ],
        }],
        ['codec_isac_enable!=0', {
          'dependencies': [
            'iSAC',
            'iSACFix',
          ],
          'sources': [
            'acm_isac.cc',
            'acm_isac.h',
            'acm_isac_macros.h',
          ],
        }],
        ['codec_opus_enable!=0', {
          'dependencies': [
            'opus',
          ],
          'sources': [
            'acm_opus.cc',
            'acm_opus.h',
          ],
        }],
        ['codec_pcm16b_enable!=0', {
          'dependencies': [
            'PCM16B',
          ],
          'sources': [
            'acm_pcm16b.cc',
            'acm_pcm16b.h',
          ],
        }],
      ],
      'include_dirs': [
        '../interface',
        '../../../interface',
        '../../codecs/opus/interface',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
        '../interface',
        '../../../interface',
        ],
      },
      'sources': [
        '../interface/audio_coding_module.h',
        '../interface/audio_coding_module_typedefs.h',
        'acm_cng.cc',
        'acm_cng.h',
        'acm_codec_database.cc',
        'acm_codec_database.h',
        'acm_dtmf_detection.cc',
        'acm_dtmf_detection.h',
        'acm_dtmf_playout.cc',
        'acm_dtmf_playout.h',
        'acm_generic_codec.cc',
        'acm_generic_codec.h',
        'acm_neteq.cc',
        'acm_neteq.h',
        'acm_red.cc',
        'acm_red.h',
        'acm_resampler.cc',
        'acm_resampler.h',
        'audio_coding_module.cc',
        'audio_coding_module_impl.cc',
        'audio_coding_module_impl.h',
      ],
    },
  ],
  'conditions': [
    ['include_tests==1', {
      'targets': [
        {
          'target_name': 'audio_coding_module_test',
          'type': 'executable',
          'dependencies': [
            'audio_coding_module',
            '<(webrtc_root)/test/test.gyp:test_support_main',
            '<(DEPTH)/testing/gtest.gyp:gtest',
            '<(webrtc_root)/system_wrappers/source/system_wrappers.gyp:system_wrappers',
          ],
          'sources': [
             '../test/ACMTest.cc',
             '../test/APITest.cc',
             '../test/Channel.cc',
             '../test/EncodeDecodeTest.cc',
             '../test/iSACTest.cc',
             '../test/PCMFile.cc',
             '../test/RTPFile.cc',
             '../test/SpatialAudio.cc',
             '../test/TestAllCodecs.cc',
             '../test/Tester.cc',
             '../test/TestFEC.cc',
             '../test/TestStereo.cc',
             '../test/TestVADDTX.cc',
             '../test/TimedTrace.cc',
             '../test/TwoWayCommunication.cc',
             '../test/utility.cc',
          ],
        },
        {
          'target_name': 'audio_coding_unittests',
          'type': 'executable',
          'dependencies': [
            'audio_coding_module',
            'NetEq',
            '<(webrtc_root)/common_audio/common_audio.gyp:vad',
            '<(DEPTH)/testing/gtest.gyp:gtest',
            '<(webrtc_root)/test/test.gyp:test_support_main',
            '<(webrtc_root)/system_wrappers/source/system_wrappers.gyp:system_wrappers',
          ],
          'sources': [
             'acm_neteq_unittest.cc',
          ],
        }, # audio_coding_unittests
      ],
    }],
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
