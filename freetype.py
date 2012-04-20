# freetype.py
# http://www.freetype.org/
# Copyright (c) 2010 Sunside Inc., All Rights Reserved
# Author: Joe Riedel
# See Radiance/LICENSE for licensing terms.

Import('radvars')
Import('variant_dir')
(build, source) = radvars

version = '2.4.4'
path = './Extern/freetype-' + version

x = build.libBuilder('freetype', build, source, path, variant_dir)

build.backend.addIncludePath(x.source, 
	[
		build.absPath(path + '/include')
	])
	
build.backend.addDefine(x.source, ['FT2_BUILD_LIBRARY'])

if build.win():
	build.backend.addCCFlag(x.source, ['/wd4267', '/wd4244'])
	
x.add('src/autofit/autofit.c')
x.add('src/base/ftbase.c')
x.add('src/base/ftbbox.c')
x.add('src/base/ftbitmap.c')
x.add('src/base/ftdebug.c')
x.add('src/base/ftglyph.c')
x.add('src/base/ftinit.c')
x.add('src/base/ftmm.c')
x.add('src/base/ftpfr.c')
x.add('src/base/ftstroke.c')
x.add('src/base/ftsynth.c')
x.add('src/base/ftsystem.c')
x.add('src/base/fttype1.c')
x.add('src/base/ftwinfnt.c')
x.add('src/bdf/bdf.c')
x.add('src/cff/cff.c')
x.add('src/cid/type1cid.c')
x.add('src/gzip/ftgzip.c')
x.add('src/lzw/ftlzw.c')
x.add('src/pcf/pcf.c')
x.add('src/pfr/pfr.c')
x.add('src/psaux/psaux.c')
x.add('src/pshinter/pshinter.c')
x.add('src/psnames/psmodule.c')
x.add('src/raster/raster.c')
x.add('src/sfnt/sfnt.c')
x.add('src/smooth/smooth.c')
x.add('src/truetype/truetype.c')
x.add('src/type1/type1.c')
x.add('src/type42/type42.c')
x.add('src/winfonts/winfnt.c')

freetype = x.create()
Export('freetype')
