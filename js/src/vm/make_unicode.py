# -*- coding: utf-8 -*-
# Based upon makeunicodedata.py
# (http://hg.python.org/cpython/file/c8192197d23d/Tools/unicode/makeunicodedata.py)
# written by Fredrik Lundh (fredrik@pythonware.com)
#
#    Copyright (C) 2011 Tom Schuster <evilpies@gmail.com>
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

from __future__ import print_function
import csv
import sys

# ECMAScript 5 $ 7.2
whitespace = [
    # python doesn't support using control character names :(
    0x9, # CHARACTER TABULATION
    0xb, # LINE TABULATION
    0xc, # FORM FEED
    ord(u'\N{SPACE}'),
    ord(u'\N{NO-BREAK SPACE}'),
    ord(u'\N{ZERO WIDTH NO-BREAK SPACE}'), # also BOM
]

# $ 7.3
line_terminator = [
    0xa, # LINE FEED
    0xd, # CARRIAGE RETURN
    ord(u'\N{LINE SEPARATOR}'),
    ord(u'\N{PARAGRAPH SEPARATOR}'),
]

# These are also part of IdentifierPart $7.6
ZWNJ = ord(u'\N{ZERO WIDTH NON-JOINER}')
ZWJ = ord(u'\N{ZERO WIDTH JOINER}')

FLAG_SPACE = 1 << 0
FLAG_LETTER = 1 << 1
FLAG_IDENTIFIER_PART = 1 << 2
FLAG_NO_DELTA = 1 << 3
FLAG_ENCLOSING_MARK = 1 << 4
FLAG_COMBINING_SPACING_MARK = 1 << 5

MAX = 0xffff

public_domain = """
/*
 * Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/licenses/publicdomain/
 */
"""

def read_unicode_data(unicode_file):
    """
        If you want to understand how this wonderful file format works checkout
          Unicode Standard Annex #44 - Unicode Character Database
          http://www.unicode.org/reports/tr44/
    """
    
    reader = csv.reader(unicode_data, delimiter=';')

    while True:
        row = reader.next()
        name = row[1]

        # We need to expand the UAX #44 4.2.3 Code Point Range
        if name.startswith('<') and name.endswith('First>'): 
            next_row = reader.next()

            for i in range(int(row[0], 16), int(next_row[0], 16) + 1):
                row[0] = i
                row[1] = name[1:-8]

                yield row
        else:
            row[0] = int(row[0], 16)
            yield row

def generate_unicode_stuff(unicode_data, data_file, test_mapping, test_space):
    dummy = (0, 0, 0)
    table = [dummy]
    cache = {dummy: 0}
    index = [0] * (MAX + 1)
    test_table = {}
    test_space_table = []

    for row in read_unicode_data(unicode_data):
        code = row[0]
        name = row[1]
        category = row[2]
        alias = row[-5]
        uppercase = row[-3]
        lowercase = row[-2]
        flags = 0

        if code > MAX:
            break

        # we combine whitespace and lineterminators because in pratice we don't need them separated
        if category == 'Zs' or code in whitespace or code in line_terminator:
            flags |= FLAG_SPACE
            test_space_table.append(code)
        if category in ['Lu', 'Ll', 'Lt', 'Lm', 'Lo', 'Nl']: # $ 7.6 (UnicodeLetter)
            flags |= FLAG_LETTER
        if category in ['Mn', 'Mc', 'Nd', 'Pc'] or code == ZWNJ or code == ZWJ: # $ 7.6 (IdentifierPart)
            flags |= FLAG_IDENTIFIER_PART
        if category == 'Me':
            flags |= FLAG_ENCLOSING_MARK
        if category == 'Mc':
            flags |= FLAG_COMBINING_SPACING_MARK

        if uppercase:
            upper = int(uppercase, 16)
        else:
            upper = code

        if lowercase:
            lower = int(lowercase, 16)
        else:
            lower = code

        test_table[code] = (upper, lower, name, alias)

        up_d = upper - code
        low_d = lower - code

        if -32768 <= up_d <= 32767 and -32768 <= low_d <= 32767:
            upper = up_d & 0xffff
            lower = low_d & 0xffff
        else:
            flags |= FLAG_NO_DELTA

        item = (upper, lower, flags)

        i = cache.get(item)
        if i is None:
            assert item not in table
            cache[item] = i = len(table)
            table.append(item)
        index[code] = i

    test_mapping.write('/* Generated by make_unicode.py DO NOT MODIFY */\n')
    test_mapping.write(public_domain)
    test_mapping.write('var mapping = [\n')
    for code in range(0, MAX + 1):
        entry = test_table.get(code)

        if entry:
            upper, lower, name, alias = entry
            test_mapping.write('  [' + hex(upper) + ', ' + hex(lower) + '], /* ' +
                       name + (' (' + alias + ')' if alias else '') + ' */\n')
        else:
            test_mapping.write('  [' + hex(code) + ', ' + hex(code) + '],\n')
    test_mapping.write('];')
    test_mapping.write("""
assertEq(mapping.length, 0x10000);
for (var i = 0; i <= 0xffff; i++) {
    var char = String.fromCharCode(i);
    var info = mapping[i];

    assertEq(char.toUpperCase().charCodeAt(0), info[0]);
    assertEq(char.toLowerCase().charCodeAt(0), info[1]);
}

if (typeof reportCompare === "function")
    reportCompare(true, true);
""")

    test_space.write('/* Generated by make_unicode.py DO NOT MODIFY */\n')
    test_space.write(public_domain)
    test_space.write('var onlySpace = String.fromCharCode(' +
                     ', '.join(map(lambda c: hex(c), test_space_table)) + ');\n')
    test_space.write("""
assertEq(onlySpace.trim(), "");
assertEq((onlySpace + 'aaaa').trim(), 'aaaa');
assertEq(('aaaa' + onlySpace).trim(), 'aaaa');
assertEq((onlySpace + 'aaaa' + onlySpace).trim(), 'aaaa');

if (typeof reportCompare === "function")
    reportCompare(true, true);
""")

    index1, index2, shift = splitbins(index)

    # Don't forget to update CharInfo in Unicode.cpp if you need to change this
    assert shift == 6

    # verify correctness
    for char in index:
        test = table[index[char]]

        idx = index1[char >> shift]
        idx = index2[(idx << shift) + (char & ((1 << shift) - 1))]

        assert test == table[idx]


    comment = """
/*
 * So how does indexing work?
 * First let's have a look at a jschar, 16-bits:
 *              [................]
 * Step 1:
 *  Extracting the upper 10 bits from the jschar.
 *   upper = char >> 6 ([**********......])
 * Step 2:
 *  Using these bits to get an reduced index from index1.
 *   index = index1[upper]
 * Step 3:
 *  Combining the index and the bottom 6 bits of the orginial jschar.
 *   real_index = index2[(index << 6) + (jschar & 0x3f)] ([..********++++++])
 *
 * The advantage here is that the bigest number in index1 doesn't need 10 bits,
 * but 8 and we save some memory.
 *
 * Step 4:
 *  Get the character informations by looking up real_index in js_charinfo.
 *
 * Pseudocode of generation:
 *
 * let table be the mapping of jschar => js_charinfo_index
 * let index1 be an empty array
 * let index2 be an empty array
 * let cache be a hash map
 *
 * while shift is less then maximal amount you can shift 0xffff before it's 0
 *  let chunks be table split in chunks of size 2**shift
 *
 *  for every chunk in chunks
 *   if chunk is in cache
 *    let index be cache[chunk]
 *   else
 *    let index be the max key of index2 + 1
 *    for element in chunk
 *     push element to index2
 *    put index as chunk in cache
 *
 *   push index >> shift to index1
 *
 *  increase shift
 *  stop if you found the best shift
 */
"""
    data_file.write('/* Generated by make_unicode.py DO NOT MODIFY */\n')
    data_file.write(public_domain)
    data_file.write('#include "Unicode.h"\n\n')
    data_file.write('namespace js {\n')
    data_file.write('namespace unicode {\n')
    data_file.write(comment)
    data_file.write('const CharacterInfo js_charinfo[] = {\n')
    for d in table:
        data_file.write('    {')
        data_file.write(', '.join((str(e) for e in d)))
        data_file.write('},\n')
    data_file.write('};\n')
    data_file.write('\n')

    def dump(data, name, file):
        file.write('const uint8_t ' + name + '[] = {\n')

        line = pad = ' ' * 4
        lines = []
        for entry in data:
            assert entry < 256
            s = str(entry)
            s = s.rjust(3)

            if len(line + s) + 5 > 99:
                lines.append(line.rstrip())
                line = pad + s + ', '
            else:
                line = line + s + ', '
        lines.append(line.rstrip())

        file.write('\n'.join(lines))
        file.write('\n};\n')

    dump(index1, 'index1', data_file)
    data_file.write('\n')
    dump(index2, 'index2', data_file)
    data_file.write('\n')

    data_file.write('} /* namespace unicode */\n')
    data_file.write('} /* namespace js */\n')
    data_file.write('\n')

def getsize(data):
    """ return smallest possible integer size for the given array """
    maxdata = max(data)
    assert maxdata < 2**32

    if maxdata < 256:
        return 1
    elif maxdata < 65536:
        return 2
    else:
        return 4

def splitbins(t):
    """t -> (t1, t2, shift).  Split a table to save space.

    t is a sequence of ints.  This function can be useful to save space if
    many of the ints are the same.  t1 and t2 are lists of ints, and shift
    is an int, chosen to minimize the combined size of t1 and t2 (in C
    code), and where for each i in range(len(t)),
        t[i] == t2[(t1[i >> shift] << shift) + (i & mask)]
    where mask is a bitmask isolating the last "shift" bits.
    """

    def dump(t1, t2, shift, bytes):
        print("%d+%d bins at shift %d; %d bytes" % (
            len(t1), len(t2), shift, bytes), file=sys.stderr)
        print("Size of original table:", len(t)*getsize(t), \
            "bytes", file=sys.stderr)
    n = len(t)-1    # last valid index
    maxshift = 0    # the most we can shift n and still have something left
    if n > 0:
        while n >> 1:
            n >>= 1
            maxshift += 1
    del n
    bytes = sys.maxsize  # smallest total size so far
    t = tuple(t)    # so slices can be dict keys
    for shift in range(maxshift + 1):
        t1 = []
        t2 = []
        size = 2**shift
        bincache = {}

        for i in range(0, len(t), size):
            bin = t[i:i + size]

            index = bincache.get(bin)
            if index is None:
                index = len(t2)
                bincache[bin] = index
                t2.extend(bin)
            t1.append(index >> shift)

        # determine memory size
        b = len(t1) * getsize(t1) + len(t2) * getsize(t2)
        if b < bytes:
            best = t1, t2, shift
            bytes = b
    t1, t2, shift = best

    print("Best:", end=' ', file=sys.stderr)
    dump(t1, t2, shift, bytes)

    # exhaustively verify that the decomposition is correct
    mask = 2**shift - 1
    for i in range(len(t)):
        assert t[i] == t2[(t1[i >> shift] << shift) + (i & mask)]
    return best

if __name__ == '__main__':
    import urllib2

    if len(sys.argv) > 1:
        print('Always make sure you have the newest UnicodeData.txt!')
        unicode_data = open(sys.argv[1], 'r')
    else:
        print('Downloading...')
        reader = urllib2.urlopen('http://unicode.org/Public/UNIDATA/UnicodeData.txt')
        data = reader.read()
        reader.close()
        unicode_data = open('UnicodeData.txt', 'w+')
        unicode_data.write(data)
        unicode_data.seek(0)

    print('Generating...')
    generate_unicode_stuff(unicode_data,
        open('Unicode.cpp', 'w'),
        open('../tests/ecma_5/String/string-upper-lower-mapping.js', 'w'),
        open('../tests/ecma_5/String/string-space-trim.js', 'w'))
