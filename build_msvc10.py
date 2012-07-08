# build_msvc10.py
# Copyright (c) 2010 Sunside Inc., All Rights Reserved
# Author: Joe Riedel
# See Radiance/LICENSE for licensing terms.

from backend import Backend
from SCons.Environment import *
from SCons.Script import *
import os

class MSVC(Backend):
	
	def __init__(self, build):
		#print Backend.__dict__
		Backend.__init__(self, build)
		# this shouldn't be necessary, but it's fixing a crash i think that's due to our on the fly python module loading.
		self.base = Backend
		self.os = os
		self.abspath = os.path.abspath(os.path.dirname(__file__))
				
	def printCmdLine(self, s, target, src, env):
		if not s == '-': print s
		return
		
	def setupEnv(self, env, name, variant, type):
		env['LINKFLAGS'] = None
		env['QT_LIB'] = None
		env['TARGET_ARCH'] = 'x86'
		
		if type == 'COM':
			env['PROGSUFFIX'] = '.com'		
		
		env = self.base.setupEnv(self, env, name, variant, type)
	
		if self.switches.dbginfo():
			# all this horseplay is an attempt to get /Zi working, and it does, however
			# the linker always complains that the .pdb file does not contain debug info for all
			# the object files, which I think is due to scons messing with the .pdb timestamp before
			# the link stage... but I really can't be sure. All i know is we're giving MSVC tools
			# the exact same command line switches as other non-scons projects have which work, and
			# we get this issue...
			#
			# for now the default /Z7 is left in place.
			#
			#print env.variant_dir
			if type == 'COM':
				env['PDB'] = name + '.com.pdb'
			else:
				env['PDB'] = name + '.pdb'
			env['VARIANT'] = variant
			#print self.abspath + variant + name
			#print env['PDB']
			#env['CCPDBFLAGS'] = ['${(PDB and "/Zi /Fd%s" % PDB) or ""}']
			#env['CCPDBFLAGS'] = '/Zi /Fd' + variant + '/' + env['PDB']
		
		# the ms cl tool already prints the name of the file out for us so we turn that off.
		# we also eliminate some output verboseness here...
		# comment out the follow env[] lines to restore full verbose output showing all command line switches
		# going to the compiler/linker
		
		if not self.switches.verbose():
			env['CCCOMSTR'] = '-'
			env['CXXCOMSTR'] = '-'
			env['ARCOMSTR'] = 'Linking $TARGET'
			env['LINKCOMSTR'] = 'Linking $TARGET'
			env['PRINT_CMD_LINE_FUNC'] = self.printCmdLine
		
		self.addLib(env, [ 'Advapi32', 'Gdi32', 'user32', 'Winspool', 'Imm32', 'shell32', 'winmm', 'opengl32', 'glu32', 'Ws2_32' ])
		
		if not self.switches.golden():
			self.addIncludePath(env, self.build.absPath('./Extern/Win/VLD/1.9h'))
			if self.exeType(type):
				self.addLib(env, self.build.absPath('./Extern/Win/VLD/1.9h/VS8/lib/vld.lib'));
					
		return env
	
	def postActions(self, type, name, dir, env, p):
	
		if self.build.switches.dynamic() and (self.dynamicLibType(type) or self.exeType(type)):
			if self.dynamicLibType(type):
				type = ';2'
			else:
				type = ';1'
			env.AddPostAction(p, 'mt.exe -nologo -manifest ${TARGET}.manifest -outputresource:$TARGET' + type)
		return env
	
	def extractLibSource(self, lib):
		#print lib[0]
		#print lib[1]
		#print lib[2]
		#print lib[0]
		#print lib[1]
		#print len(lib)
		#for x in lib: print x
		if lib[0] == 'lib': return lib[1][0]
		#if self.switches.static(): return lib[0]
		if self.switches.dbginfo(): return lib[1][2]
		return lib[1][1]
		
	def ccFlags(self, name, type):
		
		f = [				
			'/W3',         # warning level 3
			#'/Wp64',      # detect 64bit port problems
			'/WX',         # warnings as errors
			'/GF',         # string pooling
			'/EHsc',       # c++ exceptions
			'/fp:precise', # precise floating point
			'/Gd',         # __cdecl
			#'/TP',        # compile as c++ code
			'/Zc:wchar_t-' # treat wchar_t as native type
			#'/bigobj',    # bigobj object format (for large object file support)
		]
		
		p = []
		
		if self.switches.debug():
			if not self.switches.optimized(): 
				p.append('/RTC1') # basic runtime checks
			if self.switches.dynamic():
				p.append('/MDd') # multithreaded debug dll
			else:
				p.append('/MTd') # multithreaded debug static
		else:
			if self.switches.dynamic():
				p.append('/MD') # multithreaded dll
			else:
				p.append('/MT') # multithreaded static
		
		if self.switches.optimized():
			p.append('/GS-') # no buffer overrun check
			p.append('/Ox') # full optimizations
			p.append('/Ob2') # any suitable inline expansion
			if self.switches.golden() or self.switches.wholeprogramopt():
				p.append('/GL') # enable link time code generation (Whole Program Optimization)			
		else:
			p.append('/Od') # no optimizations
			
		if self.switches.analyze():
			p.append("/analyze:stacksize98304")
			
		return f + p
		
	def ccDefines(self, name, type):
					
		d = [ # included in all platforms
			'_CRT_SECURE_NO_WARNINGS',
			'_CRT_SECURE_NO_DEPRECATE',
			'_WIN32_WINNT=0x501'
		]
		
		p = [] # platform specific defines
		
		if type != '3DSPLUGIN':
			p.append('_UNICODE')
						
		if self.build.win():
			p = ['WIN32', '_WINDOWS']
		else:
			raise UserError('invalid msvc target platform')
			
		if type == 'CON':
			p.append('_CONSOLE')
		
		return d + p
		
	def lnFlags(self, name, type):
	
		f = [
			'/nologo',
			'/INCREMENTAL:NO',
			'/MACHINE:X86',
			'/MANIFEST',
			'/ManifestFile:"${TARGET}.manifest"',
			'/ALLOWISOLATION',
			'/MANIFESTUAC:level=\'asInvoker\' uiAccess=\'false\'',
			'/DYNAMICBASE',
			'/NXCOMPAT'
		]
		
		p = []
		
		if (self.switches.optimized() and self.switches.wholeprogramopt()):
			p.append('/LTCG')
			
		if type == 'CON' or type == 'COM':
			p.append('/SUBSYSTEM:CONSOLE')
		elif type == 'EXE' or type == '3DSPLUGIN':
			p.append('/SUBSYSTEM:WINDOWS')

		return f + p
		
	def lbFlags(self, name, type): # used when builing a library
	
		f = [
			'/MACHINE:X86'
		]
		
		p = []
		
		if self.switches.optimized():
			p.append('/LTCG')
			
		return f + p
	
	def install(self, type, name, dir, env, p):
		self.base.install(self, type, name, dir, env, p)
		
		# overloaded to install .pdb file to target directory, which scons does not do for us.	
		# this dies on libs because C7 (default /Z7 switch) doesn't produce a pdb file for .libs
		# need to get /Zi working but it's a pain...
		if self.switches.dbginfo() and type != 'LIB':
			env.Install(dir, self.os.path.abspath(self.abspath + '/' + env['VARIANT'] + '/' + env['PDB']))
	
	def qtPath(self, path):
		return self.build.absPath('./Extern/Win/Qt/4.8.1' + path)
		
	def qtBinPath(self):
		return self.build.absPath('./Extern/Win/Qt/4.8.1/bin')

	def qtLibPath(self, name):
		if name != 'qtmain':
			if self.switches.debug():
				return self.qtPath('/lib/' + name + 'd4.lib')
			return self.qtPath('/lib/' + name + '4.lib')
		if self.switches.debug():
			return self.qtPath('/lib/' + name + 'd.lib')
		return self.qtPath('/lib/' + name + '.lib')
				
	def addSDLLibs(self, env):
		self.addLib(env,
			[
				self.build.absPath('./Extern/Win/SDL/1.2.15/SDL.lib')
			])
		return env
	
	def addSDLMain(self, env):
		self.addLib(env,
			[
				self.build.absPath('./Extern/Win/SDL/1.2.15/SDLmain.lib')
			])
		return env

	def addSDLPaths(self, env):
		self.addIncludePath(env, self.build.absPath('./Extern/SDL/1.2.15'))
		return env
		
	def addQtMain(self, env):
		self.addLib(env, self.qtLibPath('qtmain'))
		return env
		
	def addQtLibs(self, env):
		self.addLib(env, 
			[
				self.qtLibPath('QtCore'),
				self.qtLibPath('QtGui')
			])
		if self.build.tools():
			self.addLib(env, [self.qtLibPath('QtOpenGL')])
		return env

	def setupQtApp(self, env, type):
		env = self.base.setupQtApp(self, env, type)

		self.addIncludePath(env, 
			[
				self.qtPath('/include')
			])
		return env
		
	def cgPath(self, path):
		if self.switches.static():
			raise UserError('cg cannot be used in static builds!')
		return self.build.absPath('./Extern/Win/Cg/2.2' + path)
		
	def addCgLibs(self, env):
		self.addLib(env,
			[
				self.cgPath('/lib/cg.lib'),
				self.cgPath('/lib/cgGL.lib')
			])
		return self.addCgPaths(env)
		
	def addCgPaths(self, env):
		return self.addIncludePath(env, self.cgPath(''))
		
	def openALPath(self, path):
		return self.build.absPath('./Extern/Win/OpenAL' + path)
		
	def addOpenALPaths(self, env):
		return self.addIncludePath(env, self.openALPath(''))
		
	def addOpenALLibs(self, env):
		self.addLib(env,
			[
				self.openALPath('/lib/OpenAL32.lib')
			])
		return self.addOpenALPaths(env)
		
	def pvrLibPath(self, path):
		return self.build.absPath('./Extern/Win/PVRTexLib' + path)
		
	def addPVRLibs(self, env):
		self.addLib(env,
			[
				self.pvrLibPath('/x32/PVRTexLib.lib')
			])
		return env
		
	def name(self):
		return "msvc10"

def create(build): return MSVC(build)
