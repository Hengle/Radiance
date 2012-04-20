# backend.py
# Copyright (c) 2010 Sunside Inc., All Rights Reserved
# Author: Joe Riedel
# See Radiance/LICENSE for licensing terms.

import platform
from SCons.Errors import UserError
from SCons.Script import Copy

class Backend:
	def __init__(self, build):
		self.build = build
		self.switches = build.switches
	def setupEnv(self, env, name, variant, type):
			
		if type != 'LIB' \
			and type != 'DLL' \
			and type != 'EXE' \
			and type != 'CON' \
			and type != 'COM' \
			and type != '3DSPLUGIN': 
				raise UserError('invalid target type')
		
		flags = self.ccFlags(name, type)
		defines = self.ccDefines(name, type)
		
		if self.switches.debug():
			defines.append('DEBUG')
			defines.append('_DEBUG')
		else:
			defines.append('NDEBUG')
			
		if self.build.pfiles():
			defines.append('RAD_OPT_POSIXFILES')
			if not (self.build.osx() or self.build.ios()): # osx/ios don't support AIO
				defines.append('RAD_OPT_POSIXAIO')
		if self.build.pthreads():
			defines.append('RAD_OPT_POSIXTHREADS')

		if type != '3DSPLUGIN':
			defines.append('UNICODE')
		
		defines.append('BOOST_ALL_NO_LIB')
		defines.append('RADMATH_OPT_NO_REFLECTION') # turn this off for now we don't need it.
		
		if self.switches.golden():
			defines.append('RAD_TARGET_GOLDEN')
		elif self.switches.developer():
			defines.append('RAD_TARGET_DEVELOPER')
			if self.build.win() or self.build.console():
				defines.append('RAD_OVERLOAD_STD_NEW')
		else:
			defines.append('RAD_TARGET_DEBUG')
			if self.build.win() or self.build.console():
				defines.append('RAD_OVERLOAD_STD_NEW')
				
		if self.switches.glErrors() or self.switches.debug():
			defines.append('RAD_OPT_GLERRORS')
		if self.switches.alErrors() or self.switches.debug():
			defines.append('RAD_OPT_ALERRORS')
						
		if self.build.tools():
			defines.append('RAD_OPT_TOOLS')
			if self.build.pc():
				defines.append('RAD_OPT_PC_TOOLS')
			
		if self.build.console():
			defines.append('RAD_OPT_CONSOLE')
		if self.build.pc():
			defines.append('RAD_OPT_PC')
			
		if not self.build.ios():
			defines.append('RAD_OPT_FIBERS')
			
		if self.build.gl():
			defines.append('RAD_OPT_GL')

		if self.build.ios():
			defines.append('RAD_OPT_IOS')
		if self.build.switches.ios_simulator():
			defines.append('RAD_OPT_IOS_SIMULATOR')
		if self.build.switches.ios_device():
			defines.append('RAD_OPT_IOS_DEVICE')

		if self.build.osx():
			defines.append('RAD_OPT_OSX')
			
		if self.switches.analyze():
			defines.append('RAD_OPT_CA_ENABLED')
			
		self.addCCFlag(env, flags)
		self.addDefine(env, defines)
		self.addLinkFlag(env, self.lnFlags(name, type))
		self.addLibFlag(env, self.lbFlags(name, type))
		self.addLib(env, self.libs(name, type))
		
		return env
	def qtPath(self, path):
		raise UserError("unimplemented")
	def qtBinPath(self):
		raise UserError("unimplemented")
	def setupQtApp(self, env, type):
		self.addDefine(env, 'QT_CORE_LIB')
		self.addDefine(env, 'QT_GUI_LIB')
		self.addDefine(env, 'QT_THREAD_SUPPORT')
				
		if not self.switches.debug():
			self.addDefine(env, 'QT_NO_DEBUG')

		if self.exeType(type) or self.dynamicLibType(type):
			self.addQtLibs(env)
		if self.exeType(type) or type == '3DSPLUGIN':
			self.addQtMain(env)

		return env
	def addQtMain(self, env):
		return
	def setupQtTools(self, env, type):
		
		def copy():
			return Copy("${SOURCE.dir}", "$TARGET") 
		
		env = env.Clone(
			QTDIR=self.qtPath(''), 
			QT4_BINPATH=self.qtBinPath(),
			tools=['default', 'qt4']
		)

		env['QT4_AUTOSCAN'] = 0
		env['QT4_MOCFROMHFLAGS'] = '-i'
		env['QT4_UICCOM'] = [env['QT4_UICCOM'], copy()]
		env['QT4_RCCCOM'] = [env['QT4_RCCCOM'], copy()]
		env['QT4_MOCFROMHCOM'] = [env['QT4_MOCFROMHCOM'], copy()]
		env['QT4_MOCFROMCXXCOM'] = env['QT4_MOCFROMCXXCOM'] + [copy()]
		
		return env

	def ccFlags(self, name, type):
		return []
	def ccDefines(self, name, type):
		return []
	def lnFlags(self, name, type):
		return []
	def libs(self, name, type):
		return []
	def addDefine(self, env, symbol):
		env.AppendUnique(CPPDEFINES = symbol)
		return env
	def addCCFlag(self, env, symbol):
		env.AppendUnique(CCFLAGS = symbol)
		return env
	def addLinkFlag(self, env, symbol):
		env.AppendUnique(LINKFLAGS = symbol)
		return env
	def addLibFlag(self, env, symbol):
		env.AppendUnique(ARFLAGS = symbol)
		return env
	def addIncludePath(self, env, path):
		env.AppendUnique(CPPPATH = path)
		return env
	def addLib(self, env, lib):
		env.AppendUnique(LIBS = lib)
		return env
	def addLibPath(self, env, path):
		env.AppendUnique(LIBPATH = path)
		return env
	def install(self, name, type, dir, env, p):
		env.Install(dir, p)
		return env
	def preActions(self, type, name, dir, env):
		return env
	def postActions(self, type, name, dir, env, p):
		return env
	def extractLibSource(self, lib):
		return lib
	def exeType(self, type):
		return type == 'EXE' or type == 'CON' or type == 'COM'
	def libType(self, type):
		return type == 'LIB' or type == 'DLL' or type == '3DSPLUGIN'
	def staticLibType(self, type):
		return type == 'LIB'
	def dynamicLibType(self, type):
		return type == 'DLL' or type == '3DSPLUGIN'
	def staticLibrary(self, type, name, objs, env):
		return env.StaticLibrary(name, objs)
	def sharedLibrary(self, type, name, objs, env):
		return env.SharedLibrary(name, objs)
	def program(self, type, name, objs, env):
		return env.Program(name, objs)
	def name(self):
		raise UserError("unimplemented")
	def target(self):
		return self.build.system() + '-' + self.name()
	def configLua(self, env):
		self.addIncludePath(env, self.build.absPath('./Extern/Lua/5.1.4'))
		if self.build.osx() or self.build.linux():
			self.addDefine(env, 'LUA_USE_LINUX')
		return env
