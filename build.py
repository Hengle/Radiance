# build.py
# Copyright (c) 2010 Sunside Inc., All Rights Reserved
# Author: Joe Riedel
# See Radiance/LICENSE for licensing terms.

import os
import platform
from SCons.Script import *
from SCons.Environment import *
from SCons.Errors import *

def importCode(name, path, code, addToSystem=0):
	import sys, imp
	m = imp.new_module(name)
	m.__file__ = path
	exec code in m.__dict__
	if addToSystem:
		sys.modules[name] = m
	return m

#
# Node generator.
#
# NOTE: the root here is used for python file IO, specifically when auto building directories using addDir().
# This is not a magic root directory, add()'ed files must still contain the full path relative to the SConstruct
# directory.
#
class NodeGen:
	def __init__(self, name, build, source, root, variant, type, objs, setupEnv=True):
		self.uic = None
		self.name = name
		self.build = build
		self.type = type
		self.uic_depends = []
		if setupEnv:
			self.source = build.backend.setupEnv(source, name, variant, type)
		else:
			self.source = source
		self.root = root
		self.variant = variant
		self.objs = objs
				
	def add(self, file, type='file'):
		if isinstance(file, list):
			for z in file:
				self.add(z, type)
		else:
			if type=='dir': 
				self.addDir(file)
			elif type=='file':
				obj = self.source.Object(self.root + '/' + file)
				self.objs.append(obj)
				return obj
			elif type=='lib':
				obj = self.build.backend.extractLibSource(file);
				self.objs.append(obj)
				return obj
			elif type=='obj':
				self.objs.append(file)
				return file
			else:
				raise UserError('invalid object type')
				
	def moc(self, src, header=None):
		obj = None
		cpp = src
		h   = header
		
		if not header:
			cpp = src+'.cpp'
			h   = src+'.h'
			
		if cpp:
			obj = self.add(cpp, 'file')
			
		moc = self.build.uicMocBuilder(self.build, self.source, '.', self.variant)
		moc.defType = 'moc'
		moc.add(h)
		moc = moc.create()
		self.uic_depends.append(moc)
		if obj:
			Depends(obj, moc)
		return obj
		
	def addQtSrc(self, src):
		obj = self.add(src, 'file')
		self.uic_depends.append(obj)
		return obj
		
	def addDir(self, dir):
		x = NodeGen(self.name, self.build, self.source, '.', self.variant, self.type, self.objs, False)
		SConscript(dir + '/SConstruct', duplicate=0, exports='x')
		
	def __addUicMocDepends__(self, p):
		if len(self.uic_depends) < 1:
			return
			
		uic = None
		if self.uic:
			uic = self.uic.create()
							
		if uic:
			for z in self.uic_depends:
				Depends(z, uic)

#
# Builds a dll
#
class DLLBuilder(NodeGen):
	
	def __init__(self, name, build, source, root, variant, type, objs):
		NodeGen.__init__(self, name, build, source.Clone(), root, variant, type, objs)
			
	def create(self):
		self.build.backend.preActions(self.typ, self.namee, self.build.absTargetDir(), self.source)
		p = self.build.backend.sharedLibrary(self.type, self.name, self.objs, self.source)
		self.__addUicMocDepends__(p)
		self.build.backend.postActions(self.type, self.name, self.build.absTargetDir(), self.source, p)
		self.build.backend.install(self.type, self.name, self.build.absTargetDir(), self.source, p)
		return 'dll', p
				
#
# Builds a library
#
class LibraryBuilder(NodeGen):
	
	def __init__(self, name, build, source, root, variant, type, objs):
		NodeGen.__init__(self, name, build, source.Clone(), root, variant, type, objs)
			
	def create(self):
		self.build.backend.preActions(self.type, self.name, self.build.absTargetDir(), self.source)
		p = self.build.backend.staticLibrary(self.type, self.name, self.objs, self.source)
		self.__addUicMocDepends__(p)
		self.build.backend.postActions(self.type, self.name, self.build.absTargetDir(), self.source, p)
		self.build.backend.install(self.type, self.name, self.build.absTargetDir(), self.source, p)
		return 'lib', p
	
#
# Builds an executable
#
class ProgramBuilder(NodeGen):
	
	def __init__(self, name, build, source, root, variant, type, objs):
		if not build.win() and type == 'COM':
			type = 'EXE'
		source = source.Clone()
		NodeGen.__init__(self, name, build, source, root, variant, type, objs)
			
	def create(self):
		
		self.build.backend.preActions(self.type, self.name, self.build.absTargetDir(), self.source)
		p = self.build.backend.program(self.type, self.name, self.objs, self.source)
		self.__addUicMocDepends__(p)
		self.build.backend.postActions(self.type, self.name, self.build.absTargetDir(), self.source, p)
		self.build.backend.install(self.type, self.name, self.build.absTargetDir(), self.source, p)
		return p
		
#
# UIC/MOC Builder
#
class UicMocBuilder(NodeGen):
	
	def __init__(self, name, build, source, root, variant, type, objs):
		self.defType = None
		source = build.backend.setupQtTools(source, type)
		NodeGen.__init__(self, name, build, source, root, variant, type, objs)
			
	def create(self):
		return self.objs
		
	def add(self, file, type=None):
		if type == None: type = self.defType
		if isinstance(file, list):
			NodeGen.add(self, file, type)
		else:
			if type=='moc':
				self.objs.append(self.source.Moc4(self.root + '/' + file))
			elif type=='uic':
				self.objs.append(self.source.Uic4(self.root + '/' + file))
			elif type=='qrc':
				self.objs.append(self.source.Qrc(self.root + '/' + file))
			else:
				raise UserError('invalid object type for uic/moc')
		
#
# Basic build related functions used by everything.
# Provides helpers for telling what platform the build is for,
# getting absolute paths to files, and dynamically loading python
# modules.
#
# This build object contains a 'switches' object which defines various build options
# like debug info, optimizations, etc. It also contains a 'backend' which represents
# the target compiler / tools suite with a standard interface for defining build paths
# etc.
#
# The typical way the build is done is this object is created (which automatically loads the
# appropriate compiler backend), and then is exported to other modules along with a source
# environment, which is cloned and altered by the various parts of the build as necessary.
#
class BuildPlatform:

	def __init__(self, target=None):
		self.sysOverride = target
		
	def system(self):
		if self.sysOverride != None:
			return self.sysOverride
		s = platform.system().lower()
		if s == 'windows': s = 'win'
		if s == 'darwin': s = 'osx'
		return s
		
	def osx(self):
		return self.system() == 'osx'

	def linux(self):
		return self.system() == 'linux'
		
	def pthreads(self):
		return self.linux() or self.osx()

	def pfiles(self):
		return self.linux() or self.osx()

	def win(self):
		return self.system() == 'win'

	def xbox360(self):
		return False

	def ps3(self):
		return False
	
	def console(self):
		return self.xbox360() or self.ps3()
		
	def pc(self):
		return not self.console()
		
	def allowDynamicBuild(self):
		return True
		
	def allowStaticBuild(self):
		return True
			
	def libBuilder(self, name, build, source, root, variant=None, type='LIB'):
		if variant == None: variant = build.variantDir(name)
		return LibraryBuilder(name, build, source, root, variant, type, [])
		
	def dllBuilder(self, name, build, source, root, variant=None, type='DLL'):
		if variant == None: variant = build.variantDir(name)
		return DLLBuilder(name, build, source, root, variant, type, [])
		
	def exeBuilder(self, name, build, source, root, variant=None, type='EXE'):
		if variant == None: variant = build.variantDir(name)
		return ProgramBuilder(name, build, source, root, variant, type, [])
		
	def qtBuilder(self, name, build, source, root, variant=None, type='EXE'):
		if not build.win() and type == 'COM':
			type = 'EXE'
		if variant == None: variant = build.variantDir(name)
		b = None
		uic = self.uicMocBuilder(build, source, '.', variant)
		uic.defType = 'uic'
		if build.backend.exeType(type):
			b = ProgramBuilder(name, build, source, root, variant, type, [])
		elif build.backend.dynamicLibType(type):
			b = DLLBuilder(name, build, source, root, variant, type, [])
		else:
			b = LibraryBuilder(name, build, source, root, variant, type, [])
		b.uic = uic
		b.source = b.build.backend.setupQtApp(b.source, type)
		return b
		
	def uicMocBuilder(self, build, source, root, variant=None, type='EXE'):
		if variant == None: variant = build.variantDir(name)
		b = UicMocBuilder('qtmocbuilder', build, source, root, variant, type, [])
		return b
		
	def compiler(self, env=None):
		if env is None: 
			env = Environment(MSVC_VERSION='10.0')
		if (env['CC'] == 'cl'): 
			return 'msvc10'
		return env['CC'].lower()
	
	def importModule(self, name, path, relative=None, addToSystem=0):
		path = path + '.py'
		if relative: path = os.path.abspath(os.path.abspath(relative) + '/' + path)
		try:
			f = open(path, 'r')
			#print 'importModule: ' + path
		except IOError:
			return None
		return importCode(name, path, f, addToSystem)
		
class BuildTarget(BuildPlatform):

	def __init__(self, switches, root='./'):
		BuildPlatform.__init__(self, switches.targetOverride())
		self.root = root
		self.switches = switches
		self.backend = "build_" + self.system()
		self.backend = self.importModule(self.backend, self.backend, self.absPath('.'))

		if self.backend is None:
			self.backend = "build_" + self.compiler()
			self.backend = self.importModule(self.backend, self.backend, self.absPath('.'))

		if self.backend is None: raise UserError('unable to find target backend for: ' + self.system() + '-' + self.compiler())

		self.backend = self.backend.create(self)
		print('target: ' + self.backend.target() + '-' + switches.targetName())
	
	def tools(self):
		return not self.switches.golden()
		
	def pctools(self):
		return self.pc() and self.tools()
		
	def gl(self):
		return not self.xbox360()
						
	def variantDir(self, dir):
		return self.root + 'Bin/Intermediate/' + dir + self.targetDir()
	
	def __targetDir__(self):
		return '/' + self.backend.target() + '-' + self.switches.targetName()
		
	def targetDir(self):
		return self.__targetDir__();

	def absPath(self, p):
		path = os.path.dirname(__file__)
		if len(p) == 0: return path
		if p[0] == '/': return os.path.abspath(path + p)
		return os.path.abspath(path + '/' + p)
	
	def absBuildDir(self):
		return self.absPath(self.root + 'Bin')
		
	def __absTargetDir__(self):
		return self.absBuildDir() + self.__targetDir__()
		
	def absTargetDir(self):
		return self.absBuildDir() + self.targetDir()

class Switches:
	def static(self):
		return not self.dynamic()
	def debug(self):
		return self.buildType() == 'DEBUG'
	def developer(self):
		return self.buildType() == 'DEVELOPER'
	def golden(self):
		return self.buildType() == 'GOLDEN'
	def targetName(self):
		raise UserError("unimplemented")
	def setupEnv(self, source, build):
		# http://www.scons.org/wiki/GoFastButton
		# as of SCons 0.98, you can set the Decider function on an environment. 
		# MD5-timestamp says if the timestamp matches, don't bother re-MD5ing the file. 
		# This can give huge speedups.
		source.Decider('MD5-timestamp')
		#source.Decider('timestamp-newer');
		return source
		