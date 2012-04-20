# Runtime/Tools/Bsp.SConstruct
# Builds runtime library
# Copyright (c) 2009 Pyramind Labs LLC, All Rights Reserved
# Author: Joe Riedel (joe@joecodegood.net)
# See Radiance/LICENSE for licensing terms.

Import('radvars')
Import('boost_threads')
Import('radiance_runtime')
Import('variant_dir')
(build, source) = radvars

x = build.exeBuilder('bsp', build, source, '.', variant_dir, 'CON')

x.add('Common/Files.cpp')
x.add('Common/Texture.cpp')
x.add('Common/Tokenizer.cpp')
x.add('Common/MaxScene.cpp')
x.add('Common/RGL.cpp')
x.add('Common/GLNavWindow.cpp')
x.add('Common/GLCamera.cpp')
x.add('Common/Log.cpp')
x.add('Common/BSPFile.cpp')
x.add('Bsp/Main.cpp')
x.add('Bsp/Bsp.cpp')
x.add('Bsp/Portals.cpp')
x.add('Bsp/Flood.cpp')
x.add('Bsp/Sectors.cpp')
x.add('Bsp/Emit.cpp')
x.add(radiance_runtime, type='lib')
x.add(boost_threads, type='lib')

if build.win():
	x.add('Common/Win/WinGLWindow.cpp')
	build.backend.addLib(x.source, 'glu32')
	
x.create()
