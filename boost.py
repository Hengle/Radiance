# boost.py
# builds boost
# Copyright (c) 2010 Sunside Inc., All Rights Reserved
# Author: Joe Riedel
# See Radiance/LICENSE for licensing terms.

Import('radvars')
Import('variant_dir')
(build, source) = radvars
x = build.libBuilder('boost_threads', build, source, './Extern/boost/1.49.0/libs/thread/src', variant_dir)

build.backend.addDefine(x.source, 'BOOST_THREAD_BUILD_LIB')
	
if build.pthreads():
	x.root += '/pthread'
	x.add('thread.cpp')
	x.add('once.cpp')
elif build.win():
	x.root += '/win32'
	x.add('thread.cpp')
	x.add('tss_dll.cpp')
	x.add('tss_pe.cpp')
else:
	raise UserError("invalid boost platform")

boost_threads = x.create()
Export('boost_threads')
