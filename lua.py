# lua.py
# builds lua
# Copyright (c) 2010 Sunside Inc., All Rights Reserved
# Author: Joe Riedel
# See Radiance/LICENSE for licensing terms.

Import('radvars')
Import('variant_dir')
(build, source) = radvars
x = build.libBuilder('Lua', build, source, './Extern/Lua/5.1.4/Lua', variant_dir)
build.backend.configLua(x.source)

x.add('lapi.cpp')
x.add('lauxlib.cpp')
x.add('lbaselib.cpp')
x.add('lcode.cpp')
x.add('ldblib.cpp')
x.add('ldebug.cpp')
x.add('ldo.cpp')
x.add('ldump.cpp')
x.add('lfunc.cpp')
x.add('lgc.cpp')
x.add('linit.cpp')
x.add('liolib.cpp')
x.add('llex.cpp')
x.add('lmathlib.cpp')
x.add('lmem.cpp')
x.add('loadlib.cpp')
x.add('lobject.cpp')
x.add('lopcodes.cpp')
x.add('loslib.cpp')
x.add('lparser.cpp')
x.add('lstate.cpp')
x.add('lstring.cpp')
x.add('lstrlib.cpp')
x.add('ltable.cpp')
x.add('ltablib.cpp')
x.add('ltm.cpp')
x.add('lundump.cpp')
x.add('lvm.cpp')
x.add('lzio.cpp')
x.add('bit.cpp') # bitop library

lua = x.create()
Export('lua')
