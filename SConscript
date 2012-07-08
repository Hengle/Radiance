# SConscript
# Radiance construction file
# Copyright (c) 2010 Sunside Inc., All Rights Reserved
# Author: Joe Riedel (joeriedel@hotmail.com)
# See Radiance/LICENSE for licensing terms.

import sys
import platform
import string
import traceback
from build import BuildPlatform
from build import BuildTarget
from build import Switches
from SCons.Script import *
from SCons.Errors import *

Import('build_switches_override', 'variant_override', 'architecture')
	
_architecture = architecture # stupid naming collision with switches

build = BuildPlatform()

if not variant_override:
	variant_override = './'
switches = build_switches_override

# http://www.scons.org/wiki/GoFastButton
SetOption('max_drift', 1)

if switches == None:

	if architecture == None:
	
		if build.allowStaticBuild():
			AddOption('--static',
				dest='BUILD_STATIC',
				action='count',
				help='static link to c-runtime')
				
		if build.allowDynamicBuild():
			AddOption('--dynamic',
				dest='BUILD_DYNAMIC',
				action='count',
				help='dynamic link to c-runtime')
				
		AddOption('--nosymbols',
			dest='BUILD_NO_DEBUG',
			action='count',
			help='compile without debug symbols')
			
		AddOption('--symbols',
			dest='BUILD_DEBUG_INFO',
			action='count',
			help='compile with debug symbols')
			
		AddOption('--opt',
			dest='BUILD_OPTIMIZED',
			action='count',
			help='enable compiler optimizations')
			
		AddOption('--noopt',
			dest='BUILD_NOT_OPTIMIZED',
			action='count',
			help='disable compiler optimizations')
			
		AddOption('--ltgc',
			dest='WHOLE_PROGRAM_OPT',
			action='count',
			help='whole program optimization')
				
		AddOption('--developer',
			dest='BUILD_DEVELOPER',
			action='count',
			help='developer build configuration')
			
		AddOption('--golden',
			dest='BUILD_GOLDEN',
			action='count',
			help='golden build configuration')
			
		AddOption('--glerrors',
			dest='GLERRORS',
			action='count',
			help='enable glError() checking (slow)')
			
		AddOption('--alerrors',
			dest='ALERRORS',
			action='count',
			help='enable alError() checking (slow)')
			
		AddOption('--analyze',
			dest='ANALYZE',
			action='count',
			help='static code analysis')
			
		AddOption('--iphone',
			dest='IPHONE',
			action='count',
			help='iphone build')
			
		AddOption('--ipad',
			dest='IPAD',
			action='count',
			help='ipad build')
			
		AddOption('--ios_device',
			dest='IOS_DEVICE',
			action='count',
			help='ios provisioned device build (osx only)')
			
		AddOption('--ios_simulator',
			dest='IOS_SIMULATOR',
			action='count',
			help='ios simulator build (osx only)')
			
		AddOption('--ios_universal_binary',
			dest='IOS_UNIVERSAL_BINARY',
			action='count',
			help='build armv6 & armv7 as universal binary (osx only)')
		
		AddOption('--nocom',
			dest='NO_COM',
			action='count',
			help='do not build a tools .com command line version (windows only)')
			
		AddOption('--nounittests',
			dest='NO_UNIT_TESTS',
			action='count',
			help='do not build unit test projects')
			
		AddOption('--verbose',
			dest='BUILD_VERBOSE',
			action='count',
			help='verbose build output')
			
		AddOption('--VSExp',
			dest='VS_EXP',
			action='count',
			help='Select VS Express Edition (2010)')
			
	# endif - architecture == None
			
	def safeGetInt(name):
		x = GetOption(name)
		if x == None: return 0
		return x
			
	class RadSw(Switches):
	
		def optimized(self):
			x = safeGetInt('BUILD_OPTIMIZED') + safeGetInt('BUILD_DEVELOPER') + safeGetInt('BUILD_GOLDEN')
			y = safeGetInt('BUILD_NOT_OPTIMIZED')
			return x > 0 and y == 0
			
		def wholeprogramopt(self):
			x = 0
			if self.optimized():
				x = safeGetInt('WHOLE_PROGRAM_OPT')
			return x > 0
			
		def dynamic(self):
			if build.allowStaticBuild():
				x = safeGetInt('BUILD_STATIC')
				return x == 0
			return True
			
		def dbginfo(self):
			x = safeGetInt('BUILD_NO_DEBUG') + safeGetInt('BUILD_DEVELOPER') + safeGetInt('BUILD_GOLDEN')
			y = safeGetInt('BUILD_DEBUG_INFO')
			return x == 0 or y > 0
			
		def verbose(self):
			x = safeGetInt('BUILD_VERBOSE')
			if x > 0: return True
			return False
			
		def buildType(self):
			x = safeGetInt('BUILD_GOLDEN')
			if x > 0: return 'GOLDEN'
			x = safeGetInt('BUILD_DEVELOPER')
			if x > 0: return 'DEVELOPER'
			return 'DEBUG'
			
		def glErrors(self):
			x = safeGetInt('GLERRORS')
			if x > 0: return True
			return False
			
		def alErrors(self):
			x = safeGetInt('ALERRORS')
			if x > 0: return True
			return False
			
		def analyze(self):
			x = safeGetInt('ANALYZE')
			if x > 0: return True
			return False
				
		def architecture(self):
			if self.armv6():
				return 'armv6'
			return 'armv7'
			
		def armv6(self):
			if not self.ios_device():
				raise UserError("ios only call!")
			return (_architecture == None and self.ios_iphone()) or _architecture == 'armv6'
			
		def armv7(self):
			if not self.ios_device():
				raise UserError("ios only call!")
			return not armv6 and (self.ios_ipad() or _architecture == 'armv7')
		
		def ios_universal_binary(self):
			if not self.ios_device():
				return False
			x = safeGetInt('IOS_UNIVERSAL_BINARY')
			if x > 0: return True
			return False
			
		def iphone(self):
			x = safeGetInt('IPHONE')
			if x > 0: return True
			return False
			
		def ipad(self):
			if self.iphone(): return False
			x = safeGetInt('IPAD')
			if x > 0: return True
			return False
			
		def ios(self):
			return (self.iphone() or self.ipad()) and (self.ios_simulator() or self.ios_device())
			
		def ios_simulator(self):
			if self.iphone() or self.ipad():
				x = safeGetInt('IOS_SIMULATOR')
				if x > 0: return True
			return False
			
		def ios_device(self):
			if self.ios_simulator(): return False
			if self.iphone() or self.ipad():
				x = safeGetInt('IOS_DEVICE')
				if x > 0: return True
			return False
			
		def ios_iphone(self):
			return self.ios() and self.iphone()
			
		def ios_ipad(self):
			return self.ios() and self.ipad()
			
		def ios_iphone_simulator(self):
			return self.ios_iphone() and self.ios_simulator()
			
		def ios_iphone_device(self):
			return self.ios_iphone() and self.ios_device()
			
		def ios_ipad_simulator(self):
			return self.ios_ipad() and self.ios_simulator()
			
		def ios_ipad_device(self):
			return self.ios_ipad() and self.ios_device()
			
		def no_com(self):
			x = safeGetInt('NO_COM')
			if x > 0: return True
			return False
			
		def no_unittests(self):
			x = safeGetInt('NO_UNIT_TESTS')
			if x > 0: return True
			return False
			
		def targetOverride(self):
			if self.ios():
				return 'ios'
			return None
			
		def VSExp(self):
			x = safeGetInt('VS_EXP')
			if x > 0: return True
			return False
			
		def targetName(self):
		
			name = 'golden'
			
			if self.golden() or self.developer():
				if self.developer():
					name = 'developer'
				if not self.optimized():
					name += '-noopt'
				if self.dbginfo():
					name += '-sym'
			else:
				name = 'debug'
				if self.optimized():
					name += '-opt'
				if not self.dbginfo():
					name += '-nosym'
			
			if self.ios_iphone_device():
				name = 'iphone-device-' + name
			elif self.ios_iphone_simulator():
				name = 'iphone-simulator-' + name
			elif self.iphone():
				name = 'iphone-' + name
			elif self.ios_ipad_device():
				name = 'ipad-device-' + name
			elif self.ios_ipad_simulator():
				name = 'ipad-simulator-' + name
			elif self.ipad():
				name = 'ipad-' + name
				
			return name

	switches = RadSw()
# endif
	
build  = BuildTarget(switches, variant_override)

if build.win():
	if switches.VSExp():
		source = switches.setupEnv(Environment(MSVC_VERSION='10.0Exp', TARGET_ARCH='x86'), build)
	else:
		source = switches.setupEnv(Environment(MSVC_VERSION='10.0', TARGET_ARCH='x86'), build)

# add global include directories
build.backend.addIncludePath(source, 
	[
		build.absPath('./Extern/boost/1.49.0'), 
		build.absPath('./Source')
	])

if build.pctools():
	build.backend.addCgPaths(source)
	
# export build vars to other scripts
radvars = (build, source)
Export('radvars')

def build_boost():
	variant_dir = build.variantDir('boost')
	SConscript(
		'boost.py', 
		variant_dir=variant_dir, 
		duplicate=0,
		exports='variant_dir'
	)
	
def build_glsl_optimizer():
	if build.tools():
		variant_dir = build.variantDir('glsl-optimizer')
		SConscript(
			'glsl-optimizer.py', 
			variant_dir=variant_dir, 
			duplicate=0,
			exports='variant_dir'
		)
	else:
		glsl_optimizer = None
		Export('glsl_optimizer')
		
def build_freetype():
	variant_dir = build.variantDir('freetype')
	SConscript(
		'freetype.py',
		variant_dir=variant_dir,
		duplicate=0,
		exports='variant_dir'
	)

def build_libogg():
	variant_dir = build.variantDir('libogg')
	SConscript(
		'libogg.py',
		variant_dir=variant_dir,
		duplicate=0,
		exports='variant_dir'
	)
	
def build_libvorbis():
	variant_dir = build.variantDir('libvorbis')
	SConscript(
		'libvorbis.py',
		variant_dir=variant_dir,
		duplicate=0,
		exports='variant_dir'
	)
	
def build_lua():
	variant_dir = build.variantDir('Lua')
	SConscript(
		'lua.py', 
		variant_dir=variant_dir, 
		duplicate=0,
		exports='variant_dir'
	)

def build_runtime():
	variant_dir = build.variantDir('Runtime')
	SConscript(
		'Source/Runtime/SConstruct', 
		variant_dir=variant_dir, 
		duplicate=0,
		exports='variant_dir'
	)
	if not build.console() and not build.switches.no_unittests():
		variant_dir = build.variantDir('UnitTests/Runtime')
		SConscript(
			'Source/UnitTests/Runtime/SConstruct',
			variant_dir=variant_dir,
			duplicate=0,
			exports='variant_dir'
		)

def build_engine():
	variant_dir = build.variantDir('Engine')
	SConscript(
		'Source/Engine/SConstruct', 
		variant_dir=variant_dir, 
		duplicate=0,
		exports='variant_dir'
	)
	if not build.console() and not build.switches.no_unittests():
		variant_dir = build.variantDir('UnitTests/Engine')
		SConscript(
			'Source/UnitTests/Engine/SConstruct',
			variant_dir=variant_dir,
			duplicate=0,
			exports='variant_dir'
		)
	
def build_main():
	variant_dir = build.variantDir('Main')
	SConscript(
		'Source/Main/SConstruct', 
		variant_dir=variant_dir, 
		duplicate=0,
		exports='variant_dir'
	)

def build_tools():
	variant_dir = build.variantDir('Tools')
	SConscript(
		'Source/Tools/SConstruct', 
		variant_dir=variant_dir, 
		duplicate=0,
		exports='variant_dir'
	)

build_boost()
build_lua()
build_glsl_optimizer()
build_freetype()
build_libogg()
build_libvorbis()
build_runtime()
build_engine()
build_main()

#if not build.console():
#	tools()
