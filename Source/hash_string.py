# hash_string.py
# Copyright (c) 2013 Sunside Inc., All Rights Reserved
# Author: Joe Riedel
# See Radiance/LICENSE for licensing terms.
# Generates hashed string representation and writes to a file
# meant to be run interactively

import io
import sys
import random

random.seed()

stringToHash = raw_input("Enter string to hash: ")

s = io.open('hash_out.txt', 'w')
s.write(u'// original string: \'' + unicode(stringToHash) + '\'\n')
s.write(u'unsigned int _hashed[] = {\n\t')

first = True

for c in stringToHash:
#000fe000

	r = random.getrandbits(32)
	x = ~ord(c) & 0xffffffff
	x = x << 13
	x = (x & 0x000fe000) | (r & ~0x000fe000)
	x = hex(x)[:-1]

	if (first):
		s.write(unicode(x))
	else:
		s.write(u', ' + unicode(x))

	first = False

s.write(u', 0\n}')
s.close()
