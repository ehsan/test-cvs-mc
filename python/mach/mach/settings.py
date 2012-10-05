# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at http://mozilla.org/MPL/2.0/.

from __future__ import print_function, unicode_literals

from textwrap import TextWrapper

from mozbuild.base import MozbuildObject
from mach.base import CommandProvider
from mach.base import Command

@CommandProvider
class Settings(MozbuildObject):
    """Interact with settings for mach.

    Currently, we only provide functionality to view what settings are
    available. In the future, this module will be used to modify settings, help
    people create configs via a wizard, etc.
    """
    @Command('settings-list', help='Show available config settings.')
    def list_settings(self):
        """List available settings in a concise list."""
        for section in sorted(self.settings):
            for option in sorted(self.settings[section]):
                short, full = self.settings.option_help(section, option)
                print('%s.%s -- %s' % (section, option, short))

    @Command('settings-create',
        help='Print a new settings file with usage info.')
    def create(self):
        """Create an empty settings file with full documentation."""
        wrapper = TextWrapper(initial_indent='# ', subsequent_indent='# ')

        for section in sorted(self.settings):
            print('[%s]' % section)
            print('')

            for option in sorted(self.settings[section]):
                short, full = self.settings.option_help(section, option)

                print(wrapper.fill(full))
                print(';%s =' % option)
                print('')
