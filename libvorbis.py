# libvorbis.py
# http://xiph.org/vorbis/
# Copyright (c) 2010 Sunside Inc., All Rights Reserved
# Author: Joe Riedel
# See Radiance/LICENSE for licensing terms.

Import('radvars')
Import('variant_dir')
(build, source) = radvars

version = '1.3.2'
path = './Extern/libvorbis-' + version

x = build.libBuilder('libvorbis', build, source, path, variant_dir)

build.backend.addIncludePath(x.source, 
	[
		build.absPath(path + '/include'),
		build.absPath(path + '/lib'),
		build.absPath('./Extern/libogg-1.2.2/include')
	])

if build.win():
	build.backend.addCCFlag(x.source, ['/wd4554', '/wd4244', '/wd4267', '/wd4305'])

x.add('lib/analysis.c')
x.add('lib/barkmel.c')
x.add('lib/bitrate.c')
x.add('lib/block.c')
x.add('lib/codebook.c')
x.add('lib/envelope.c')
x.add('lib/floor0.c')
x.add('lib/floor1.c')
x.add('lib/info.c')
x.add('lib/lookup.c')
x.add('lib/lpc.c')
x.add('lib/lsp.c')
x.add('lib/mapping0.c')
x.add('lib/mdct.c')
x.add('lib/psy.c')
x.add('lib/registry.c')
x.add('lib/res0.c')
x.add('lib/sharedbook.c')
x.add('lib/smallft.c')
x.add('lib/synthesis.c')
x.add('lib/vorbisenc.c')
x.add('lib/vorbisfile.c')
x.add('lib/window.c')

libvorbis = x.create()
Export('libvorbis')

