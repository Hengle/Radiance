# build_gcc.py
# Copyright (c) 2010 Sunside Inc., All Rights Reserved
# Author: Joe Riedel
# See Radiance/LICENSE for licensing terms.

from backend import Backend
from SCons.Environment import *
from SCons.Script import *
from SCons.Builder import *
import os
import osxbundle

class GCC4(Backend):
	
	def __init__(self, build):
		Backend.__init__(self, build)
		# this shouldn't be necessary, but it's fixing a crash i think that's due to our on the fly python module loading.
		self.base = Backend
		self.osxbundle = osxbundle
		self.SCons = SCons
		self.os = os

	def setupEnv(self, env, name, variant, type):
		
		if self.switches.ios_iphone_simulator():
			self.ios_path = '/Developer/Platforms/iPhoneSimulator.platform'
			self.ios_sdk  = '/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator4.0.sdk'
		if self.switches.ios_iphone_device():
			self.ios_path = '/Developer/Platforms/iPhoneOS.platform'
			self.ios_sdk  = '/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS4.0.sdk'
			
		if self.switches.ios_ipad_simulator():
			self.ios_path = '/Developer/Platforms/iPhoneSimulator.platform'
			self.ios_sdk  = '/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator3.2.sdk'
		if self.switches.ios_ipad_device():
			self.ios_path = '/Developer/Platforms/iPhoneOS.platform'
			self.ios_sdk  = '/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS3.2.sdk'
		
		if self.build.ios():
			env['ARFLAGS'] = []
			
		env = self.base.setupEnv(self, env, name, variant, type)
	
		if not self.switches.verbose():
			env['CCCOMSTR'] = '$SOURCE'
			env['CXXCOMSTR'] = '$SOURCE'
			env['ARCOMSTR'] = 'Linking $TARGET'
			env['LINKCOMSTR'] = 'Linking $TARGET'
		
		if self.build.ios():
			env['FRAMEWORKS'] = []
			env['CXX'] = self.ios_path + '/Developer/usr/bin/gcc-4.2'
			env['CC'] = self.ios_path + '/Developer/usr/bin/gcc'
			env['AR'] = self.ios_path + '/Developer/usr/bin/libtool'
			env['ARCOM'] = '$AR $ARFLAGS $SOURCES $_FRAMEWORKPATH $_FRAMEWORKS $FRAMEWORKSFLAGS -o $TARGET'
			env['RANLIBCOM'] = ""
			env['ENV']['IPHONEOS_DEPLOYMENT_TARGET'] = "4.0"
			env['ENV']['LANG'] = "en_US.US-ASCII"
			
		#print env.Dump()
		return env
		
	def ccFlags(self, name, type):
		
		f = [				
			#'-xc++', # c++ language
			#'-std=gnu++98', #gnu++98 dialect
			'-ffor-scope', #for scoping
			'-fno-operator-names', #Do not treat the operator name keywords and, bitand, bitor, compl, not, or and xor as synonyms as keywords.
			'-fvisibility-inlines-hidden',
			'-fno-strict-aliasing',
			#'-fvisibility=hidden',
			#'-Wall', # all extra warnings
			'-Wno-multichar'
		]
		
		p = []
		
		if not self.build.ios():
			p.append('-fPIC')

		# we mix c++ and objc and some of the command line parms
		# are not valid for objc so we disable this so we don't die
		# on the warnings		
		if not (self.build.osx() or self.build.ios()):
			p.append('-Werror') # treat warnings as errors

		if self.switches.dbginfo():
			if self.build.ios():
				p.append('-gdwarf-2')
			else:
				p.append('-ggdb3') # gdb debug info, level 3
				
		if self.switches.optimized():
			p.append('-fno-enforce-eh-specs') # don't check for exception throwing runtime violation.
			p.append('-O3') # max optimization level
		else:
			p.append('-O0') # no optimizations

		if self.build.osx():
			p.append(['-arch', 'i386', '-mmacosx-version-min=10.4'])
		if self.build.ios():
			p.append(['-isysroot', self.ios_sdk])
		if self.build.switches.ios_iphone_simulator():
			p.append(['-arch', 'i386', '-mmacosx-version-min=10.6'])
		if self.build.switches.ios_iphone_device():
			p.append(['-arch', self.switches.architecture(), '-miphoneos-version-min=4.0'])
		if self.build.switches.ios_ipad_simulator():
			p.append(['-arch', 'i386', '-mmacosx-version-min=10.5'])
		if self.build.switches.ios_ipad_device():
			p.append(['-arch', self.switches.architecture(), '-miphoneos-version-min=3.2'])
			
		return f + p
		
	def ccDefines(self, name, type):
					
		d = [ 
		]
		
		p = [] # platform specific defines

		if self.build.osx():
			p.append('_XOPEN_SOURCE')
		if self.build.ios():
			p.append('_XOPEN_SOURCE')
		if self.build.switches.ios_simulator():
			p.append('__IPHONE_OS_VERSION_MIN_REQUIRED=30200')
			
		return d + p
		
	def lnFlags(self, name, type):
	
		f = []		
		p = []
		
		if not self.build.ios():
			p.append('-fPIC')
			p.append('-shared-libgcc')

		if self.build.osx():
			p.append(['-arch', 'i386', '-mmacosx-version-min=10.4', '-headerpad_max_install_names'])
		if self.build.ios():
			p.append(['-isysroot', self.ios_sdk])
		if self.build.switches.ios_simulator():
			p.append(['-fobjc-abi-version=2', '-fobjc-legacy-dispatch'])
		if self.build.switches.ios_device():
			p.append(['-dead_strip'])
		if self.build.switches.ios_iphone_simulator():
			p.append(['-arch', 'i386', '-mmacosx-version-min=10.6'])
		if self.build.switches.ios_iphone_device():
			p.append(['-arch', self.switches.architecture(), '-miphoneos-version-min=4.0'])
		if self.build.switches.ios_ipad_simulator():
			p.append(['-arch', 'i386', '-mmacosx-version-min=10.5'])
		if self.build.switches.ios_ipad_device():
			p.append(['-arch', self.switches.architecture(), '-miphoneos-version-min=3.2'])
				
		return f + p

	def lbFlags(self, name, type): # used when builing a library
	
		f = [
		]
		
		p = []
		
		if self.build.switches.ios_device():
			p.append(['-static', '-arch_only', self.switches.architecture()])
			p.append(['-syslibroot', self.ios_sdk])
			
		
		return f + p

	def preActions(self, name, type, dir, env):
		# link against RT (realtime library) for AIO
		if not (self.build.osx() or self.build.ios()):
			self.addLib(self.addLib(env, 'rt'), 'aio')
		if self.build.pthreads(): 
			self.addLib(env, 'pthread')
		if self.build.osx():
			env.AppendUnique(FRAMEWORKS=['Carbon', 'System', 'CoreFoundation', 'AppKit'])
		if self.build.ios():
			env.AppendUnique(FRAMEWORKS=['Foundation', 'UIKit', 'OpenGLES', 'QuartzCore'])
		if self.build.switches.ios_device():
			self.addLib(env, 'stdc++.6.0.9')
		return env

	def extractLibSource(self, lib):
		return lib[1]
	
	def qtPath(self, path):
		if self.build.osx():
			return self.build.absPath('./Extern/Mac/Qt/4.5.2') + path
		return self.build.absPath('./Extern/Linux/Qt/4.5.2' + path)

	def qtLibPath(self):
		if self.build.osx():
			return self.build.qtPath('')
		return self.qtPath('/lib')

	def qtBinPath(self):
		if self.build.osx():
			return self.qtPath('')
		return self.qtPath('/bin')

	def addQtFramework(self, env):
		env.AppendUnique(FRAMEWORKPATH=[self.qtPath('')])
		env.AppendUnique(FRAMEWORKS=['QtCore', 'QtGui'])
		if self.build.tools():
			env.AppendUnique(FRAMEWORKS=['QtOpenGL'])
		return env

	def addQtLibs(self, env):
		if self.build.osx():
			return self.addQtFramework(env)
		self.addLibPath(env, [self.qtLibPath()])
		self.addLib(env, 
			[
				'QtCore',
				'QtGui'
			])
		if self.build.tools():
			self.addLib(env, ['QtOpenGL'])
		return env

	def setupQtApp(self, env, type):
		env = self.base.setupQtApp(self, env, type)

		if self.build.osx():
			return self.addQtFramework(env)

		self.addIncludePath(env, 
			[
				self.qtPath('/include')
			])
		return env

	def addQtMain(self, env):
		# no qtmain needed on linux
		return
	
	def cgPath(self, path):
		if self.switches.static():
			raise UserError('cg cannot be used in static builds!')
		if self.build.osx():
			return self.build.absPath('./Extern/Mac/Cg/2.2' + path)
		return self.build.absPath('./Extern/Linux/Cg/2.2' + path)

	def cgLibPath(self):
		return self.cgPath('/lib')
		
	def addCgLibs(self, env):
		if self.build.osx():
			return self.addCgFramework(env)
		self.addLibPath(env, [self.cgLibPath()])
		self.addLib(env,
			[
				'Cg',
				'CgGL'
			])
		return self.addCgPaths(env, type)
		
	def addCgPaths(self, env):
		if self.build.osx():
			return self.addCgFramework(env)
		return self.addIncludePath(env, self.cgPath(''))

	def macSDLPath(self, path):
		return self.build.absPath('./Extern/Mac/SDL/1.2') + path

	def addSDLFramework(self, env):
		env.AppendUnique(FRAMEWORKPATH=[self.macSDLPath('')])
		env.AppendUnique(FRAMEWORKS=['SDL', 'OpenGL', 'Cocoa'])
		return env

	def addCgFramework(self, env):
		env.AppendUnique(FRAMEWORKPATH=[self.cgPath('')])
		env.AppendUnique(FRAMEWORKS=['Cg', 'OpenGL', 'Cocoa'])
		return env

	def addSDLLibs(self, env):
		if self.build.osx():
			return self.addSDLFramework(env)
		self.addLib(env, ['SDL', 'GL', 'GLU'])
		return env

	def addSDLPaths(self, env):
		# On OSX/Linux these are only libs, not headers so we add our portable headers here.
		return self.addIncludePath(env, self.build.absPath('./Extern/SDL/1.2'))
	
	def addSDLMain(self, env):
		return env

	def openALPath(self, path):
		if self.build.osx():
			return self.build.absPath('./Extern/Mac/OpenAL') + path
		return None

	def addOpenALPaths(self, env):
		if self.build.osx():
			return self.addIncludePath(env, self.openALPath('/include'))
		return env

	def addOpenALLibs(self, env):
		if self.build.osx():
			self.addLibPath(env, [self.openALPath('/libs')])
			self.addLib(env, ['openal.1.14.0'])
			return env
		raise UserError("Unsupported OpenAL target")

	def pvrLibPath(self, path):
		return self.build.absPath('./Extern/Mac/PVRTexLib' + path)
		
	def addPVRLibs(self, env):
		self.addLibPath(env, [self.pvrLibPath('')])
		self.addLib(env, ['PVRTexLib'])
		return env

	def program(self, type, name, objs, env):
		p = self.base.program(self, type, name, objs, env)

		if (not self.build.osx() and not self.build.ios()) or type != 'EXE':
			return p
		
		# osx/ios bundles
		
		self.osxbundle.TOOL_BUNDLE(env)
		
		if self.build.ios():
			return env.MakeIOSBundle(self.__iosType__(), name + '.app', p)

		env.AppendUnique(QTDIR=self.qtPath(''))
		env.AppendUnique(SDLDIR=self.macSDLPath(''))
		env.AppendUnique(CGDIR=self.cgPath(''))
		env.AppendUnique(OALDIR=self.openALPath(''))
		
		return env.MakeOSXBundle(name + '.app', p, name)
	
	def __iosType__(self):
		type = 'iPhone.device'
		if self.build.switches.ios_iphone_simulator():
			type = 'iPhone.simulator'
		if self.build.switches.ios_ipad_device():
			type = 'iPad.device'
		if self.build.switches.ios_ipad_simulator():
			type = 'iPad.simulator'
		return type
		
	def makeIOSUniversalBinary(self, source, name, tags, dir, variant, sources):
		self.osxbundle.TOOL_BUNDLE(source)
		source['IOSUB_ROOT'] = dir
		p = source.BuildUniversalBinary(variant + '/' + name, [sources[0][1], sources[1][1]])
		return p
	
	def install(self, type, name, dir, env, p):
		if self.build.osx() and type == 'EXE': #bundle
			env.InstallBundle(dir, p)
		elif self.build.ios() and type == 'EXE': #bundle
			if self.switches.ios_simulator():
				if self.switches.debug():
					env.InstallBundle(dir+'/Debug-iphonesimulator', p)
				else:
					env.InstallBundle(dir+'/Release-iphonesimulator', p)
			else:
				if self.switches.debug():
					env.InstallBundle(dir+'/Debug-iphoneos', p)
				else:
					env.InstallBundle(dir+'/Releaes-iphoneos', p)
		else:
			self.base.install(self, type, name, dir, env, p)
			
	def name(self):
		return "gcc4"

def create(build): return GCC4(build)
