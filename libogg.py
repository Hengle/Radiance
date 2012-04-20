# libogg.py
# http://www.xiph.org/ogg/
# Copyright (c) 2010 Sunside Inc., All Rights Reserved
# Author: Joe Riedel
# See Radiance/LICENSE for licensing terms.

Import('radvars')
Import('variant_dir')
(build, source) = radvars

version = '1.2.2'
path = './Extern/libogg-' + version

x = build.libBuilder('libogg', build, source, path, variant_dir)

build.backend.addIncludePath(x.source, 
	[
		build.absPath(path + '/include')
	])
	

x.add('src/bitwise.c')
x.add('src/framing.c')

libogg = x.create()
Export('libogg')
