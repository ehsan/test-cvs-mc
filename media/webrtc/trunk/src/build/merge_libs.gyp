# Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
#
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file in the root of the source
# tree. An additional intellectual property rights grant can be found
# in the file PATENTS.  All contributing project authors may
# be found in the AUTHORS file in the root of the source tree.

{
  'includes': [ 'common.gypi', ],
  'targets': [
    {
      'target_name': 'no_op',
      'type': 'executable',
      'dependencies': [
        '../video_engine/video_engine.gyp:video_engine_core',
      ],
      'sources': [ 'no_op.cc', ],
    },
    {
      'target_name': 'merged_lib',
      'type': 'none',
      'dependencies': [
        'no_op',
      ],
      'actions': [
        {
          'variables': {
            'output_lib_name': 'webrtc',
            'output_lib': '<(PRODUCT_DIR)/<(STATIC_LIB_PREFIX)<(output_lib_name)_<(OS)_<(target_arch)_<(CONFIGURATION_NAME)<(STATIC_LIB_SUFFIX)',
          },
          'action_name': 'merge_libs',
          'inputs': ['<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)no_op<(EXECUTABLE_SUFFIX)'],
          'outputs': ['<(output_lib)'],
          'action': ['python',
                     'merge_libs.py',
                     '<(PRODUCT_DIR)',
                     '<(output_lib)',],
        },
      ],
    },
  ],
}
