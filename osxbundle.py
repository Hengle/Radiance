# Copyright (c) 2005 Joey Mukherjee
# Posted here: http://markmail.org/message/dduoolptfcgaenup#query:scons%20app%20bundle+page:1+mid:2wcow5ozagqoqha6+state:results
# To my knowledge this is free software. It has been modified to support Radiance, and Qt (I added the Qt framework install_name_tool stuff)
# Modified by Joe Riedel (joeriedel@hotmail.com)
# See Radiance/LICENSE for licensing terms.

from os.path import *
from os import listdir, stat
import re
import shutil
import glob

from SCons.Builder import *
from SCons.Defaults import SharedCheck, ProgScan
from SCons.Script.SConscript import SConsEnvironment
from SCons.Script import Copy
import SCons.Action
import SCons.Subst

# SCons is not replacing $TARGET in our Actions() for whatever reason, had to lift this from SCons code and do it manually. *sigh*
def spawn(env, cmd_line):
	escape = env.get('ESCAPE', lambda x: x) 
	_spawn  = env['SPAWN']
	shell = env['SHELL']
	escape_list = SCons.Subst.escape_list 
	cmd = escape_list([cmd_line], escape)
	ENV = SCons.Action.get_default_ENV(env)
	print cmd_line
	result = _spawn(shell, escape, cmd[0], cmd, ENV) 
	if result: 
		msg = "Error %s" % result
		return SCons.Errors.BuildError(errstr=msg,
			status=result,
			command=cmd_line) 
	return 0
			
def TOOL_BUNDLE(env):
	"""defines env.Bundle() for linking bundles on Darwin/OSX, and
	   env.InstallBundle() for installing a bundle into its dir.
	   A bundle has this structure: (filenames are case SENSITIVE)
	   sapphire.bundle/
		 Contents/
		   Info.plist (an XML key->value database; defined by BUNDLE_INFO_PLIST)
		   PkgInfo (trivially short; defined by value of BUNDLE_PKGINFO)
		   MacOS/
			 executable (the executable or shared lib, linked with Bundle())
	Resources/
		 """
	if 'BUNDLE' in env['TOOLS']: return
	if env['PLATFORM'] == 'darwin':
		env.Append(TOOLS = 'BUNDLE')
		# This is like the regular linker, but uses different vars.
		LinkBundle = SCons.Builder.Builder(action=[SharedCheck, "$BUNDLECOM"],
										   emitter="$SHLIBEMITTER",
										   prefix = '$BUNDLEPREFIX',
										   suffix = '$BUNDLESUFFIX',
										   target_scanner = ProgScan,
										   src_suffix = '$BUNDLESUFFIX',
										   src_builder = 'SharedObject')
		env['BUILDERS']['LinkBundle'] = LinkBundle
		env['BUNDLEEMITTER'] = None
		env['BUNDLEPREFIX'] = ''
		env['BUNDLESUFFIX'] = ''
		env['BUNDLEDIRSUFFIX'] = '.bundle'
		env['BUNDLE'] = '$SHLINK'
		env['BUNDLEFLAGS'] = ' -bundle'
		env['BUNDLECOM'] = '$BUNDLE $BUNDLEFLAGS -o ${TARGET} $SOURCES $_LIBDIRFLAGS $_LIBFLAGS $FRAMEWORKS'
		# This requires some other tools:
		TOOL_WRITE_VAL(env)
		TOOL_SUBST(env)
		IOS_UNIVERSAL_BIN(env)

		def ensureWritable(nodes):
			for node in nodes:
				if exists(node.path) and not (stat(node.path)[0] & 0200):
				   chmod(node.path, 0777)
			return nodes

# Copy given patterns from inDir to outDir

		def DFS(root, skip_symlinks = 1):
			"""Depth first search traversal of directory structure.  Children
			are visited in alphabetical order."""
			stack = [root]
			visited = {}
			while stack:
				d = stack.pop()
				if d not in visited:  ## just to prevent any possible recursive
									  ## loops
					visited[d] = 1
					yield d
				stack.extend(subdirs(d, skip_symlinks))


		def subdirs(root, skip_symlinks = 1):
			"""Given a root directory, returns the first-level subdirectories."""
			try:
				dirs = [join(root, x) for x in listdir(root)]
				dirs = filter(isdir, dirs)
				if skip_symlinks:
					dirs = filter(lambda x: not islink(x), dirs)
				dirs.sort()
				return dirs
			except OSError, IOError: return []

		def copyFiles (env, outDir, inDir):
			inDirNode = env.Dir(inDir)
			outDirNode = env.Dir(outDir)
			subdirs = DFS (inDirNode.abspath)
			files = []
			for subdir in subdirs:
				files += glob.glob (join (subdir, '*'))
			outputs = []
			for f in files:
				if isfile (f):
#				   print inDirNode.abspath
#				   print f[len(inDirNode.abspath):]
				   outputs += ensureWritable (env.InstallAs (outDirNode.abspath + '/' + f[len(dirname(inDirNode.abspath))+1:], env.File (f)))
			return outputs
        
		def copySDLdylibs(preAction, env, bundledir):
			oalDir = env['OALDIR'] + '/libs/'
			files = ['libopenal.1.14.0.dylib']
			for f in files:
				postAction = env.Install(bundledir, oalDir + f)
				env.AddPostAction(preAction, postAction)

		def openALFrameworkSrc(framework):
			return framework + '.framework/Versions/A/' + framework

		def openALFrameworkDst(framework):
			return '@executable_path/../Frameworks/' + framework + '.framework/Versions/A/' + framework

		def openALFrameworkBundlePath(framework, bundledir):
			return bundledir + '/Contents/Frameworks/' + framework + '.framework/Versions/A'

		def openALFrameworkBundleFile(framework, bundledir):
			return bundledir + '/Contents/Frameworks/' + openALFrameworkSrc(framework)

		def cgFrameworkSrc(framework):
			return framework + '.framework/Versions/1.0/' + framework

		def cgFrameworkDst(framework):
			return '@executable_path/../Frameworks/' + framework + '.framework/Versions/1.0/' + framework

		def cgFrameworkBundlePath(framework, bundledir):
			return bundledir + '/Contents/Frameworks/' + framework + '.framework/Versions/1.0'

		def cgFrameworkBundleFile(framework, bundledir):
			return bundledir + '/Contents/Frameworks/' + cgFrameworkSrc(framework)

		def sdlFrameworkSrc(framework):
			return framework + '.framework/Versions/A/' + framework

		def sdlFrameworkDst(framework):
			return '@executable_path/../Frameworks/' + framework + '.framework/Versions/A/' + framework

		def sdlFrameworkBundlePath(framework, bundledir):
			return bundledir + '/Contents/Frameworks/' + framework + '.framework/Versions/A'

		def sdlFrameworkBundleFile(framework, bundledir):
			return bundledir + '/Contents/Frameworks/' + sdlFrameworkSrc(framework)

		def qtFrameworkSrc(framework):
			 return framework + '.framework/Versions/4/' + framework

		def qtFrameworkDst(framework):
			return '@executable_path/../Frameworks/' + framework + '.framework/Versions/4/' + framework

		def qtFrameworkBundlePath(framework, bundledir):
			return bundledir + '/Contents/Frameworks/' + framework + '.framework/Versions/4'

		def qtFrameworkBundleFile(framework, bundledir):
			return bundledir + '/Contents/Frameworks/' + qtFrameworkSrc(framework)

		def nullPrint(target, source, env): return

       	 	def copyAndFixupOpenALFramework(root, env, bundledir, framework, fixups=None):
			cgDir = env['OALDIR']
			x = env.Install(openALFrameworkBundlePath(framework, bundledir), cgDir + '/' + openALFrameworkSrc(framework))
			def installId():
				def action(target, source, env):
					#print str(target[0])
					cmd = 'install_name_tool -id %s %s' % (openALFrameworkDst(framework), str(target[0]))
					return spawn(env, cmd)
				return env.Action(action, nullPrint)
			def installChange(z):
				def action(target, source, env):
					#print str(target[0])
					cmd = 'install_name_tool -change %s %s %s' % (openALFrameworkSrc(z), openALFrameworkDst(z), str(target[0]))
					return spawn(env, cmd)
				return env.Action(action, nullPrint)
			x = env.AddPostAction(x, installId())
			outputs = [x]
			if fixups:
				for z in fixups:
					outputs += env.AddPostAction(x, installChange(z))
			return outputs

		def copyAndFixupCgFramework(root, env, bundledir, framework, fixups=None):
			cgDir = env['CGDIR']
			x = env.Install(cgFrameworkBundlePath(framework, bundledir), cgDir + '/' + cgFrameworkSrc(framework))
			def installId():
				def action(target, source, env):
					#print str(target[0])
					cmd = 'install_name_tool -id %s %s' % (cgFrameworkDst(framework), str(target[0]))
					return spawn(env, cmd)
				return env.Action(action, nullPrint)
			def installChange(z):
				def action(target, source, env):
					#print str(target[0])
					cmd = 'install_name_tool -change %s %s %s' % (cgFrameworkSrc(z), cgFrameworkDst(z), str(target[0]))
					return spawn(env, cmd)
				return env.Action(action, nullPrint)
			x = env.AddPostAction(x, installId())
			outputs = [x]
			if fixups:
				for z in fixups:
					outputs += env.AddPostAction(x, installChange(z))
			return outputs

		def copyAndFixupSDLFramework(root, env, bundledir, framework, fixups=None):
			sdlDir = env['SDLDIR']
			x = env.Install(sdlFrameworkBundlePath(framework, bundledir), sdlDir + '/' + sdlFrameworkSrc(framework))
			def installId():
				def action(target, source, env):
					#print str(target[0])
					cmd = 'install_name_tool -id %s %s' % (sdlFrameworkDst(framework), str(target[0]))
					return spawn(env, cmd)
				return env.Action(action, nullPrint)
			def installChange(z):
				def action(target, source, env):
					#print str(target[0])
					cmd = 'install_name_tool -change %s %s %s' % (sdlFrameworkSrc(z), sdlFrameworkDst(z), str(target[0]))
					return spawn(env, cmd)
				return env.Action(action, nullPrint)
			x = env.AddPostAction(x, installId())
			outputs = [x]
			if fixups:
				for z in fixups:
					outputs += env.AddPostAction(x, installChange(z))
			return outputs

		def copyAndFixupQtFramework(root, env, bundledir, framework, fixups=None):
			qtDir = env['QTDIR']
			x = env.Install(qtFrameworkBundlePath(framework, bundledir), qtDir + '/' + qtFrameworkSrc(framework))
			def installId():
				def action(target, source, env):
					#print str(target[0])
					cmd = 'install_name_tool -id %s %s' % (qtFrameworkDst(framework), str(target[0]))
					return spawn(env, cmd)
				return env.Action(action, nullPrint)
			def installChange(z):
				def action(target, source, env):
					#print str(target[0])
					cmd = 'install_name_tool -change %s %s %s' % (qtFrameworkSrc(z), qtFrameworkDst(z), str(target[0]))
					return spawn(env, cmd)
				return env.Action(action, nullPrint)
			x = env.AddPostAction(x, installId())
			outputs = [x]
			if fixups:
				for z in fixups:
					outputs += env.AddPostAction(x, installChange(z))
			return outputs

		def OpenALFrameworkDepends(framework):
			return None

		def CgFrameworkDepends(framework):
			return None

		def SDLFrameworkDepends(framework):
			return None

		def qtFrameworkDepends(framework):
			if framework == 'QtGui': return ['QtCore']
			if framework == 'QtOpenGL': return ['QtCore', 'QtGui']
			return None

		def copyOpenALdylib(root, env, bundledir):
			return copyOpenALdylibs(root, env, bundledir)

		def copyOpenALFrameworks(root, env, bundledir, frameworks):
			outputs = []
			for z in frameworks:
				if z in env['FRAMEWORKS']:
					outputs += copyAndFixupOpenALFramework(root, env, bundledir, z, OpenALFrameworkDepends(z))
			return outputs

		def copyCgFrameworks(root, env, bundledir, frameworks):
			outputs = []
			for z in frameworks:
				if z in env['FRAMEWORKS']:
					outputs += copyAndFixupCgFramework(root, env, bundledir, z, CgFrameworkDepends(z))
			return outputs

		def copySDLFrameworks(root, env, bundledir, frameworks):
			outputs = []
			for z in frameworks:
				if z in env['FRAMEWORKS']:
					outputs += copyAndFixupSDLFramework(root, env, bundledir, z, SDLFrameworkDepends(z))
			return outputs

		def copyQtFrameworks(root, env, bundledir, frameworks):
			outputs = []
			for z in frameworks:
				if z in env['FRAMEWORKS']:
					outputs += copyAndFixupQtFramework(root, env, bundledir, z, qtFrameworkDepends(z))
			return outputs

		def fixupOpenALRefs(env, bundledir, app, frameworks):
			def customAction(framework):
				cmd = 'install_name_tool -change %s %s' % (openALFrameworkSrc(framework), openALFrameworkDst(framework))
				def action(target, source, env):
					return spawn(env, cmd + ' ' + str(target[0]))
				return env.Action(action, nullPrint)
			x = [app]
			for z in frameworks:
				if z in env['FRAMEWORKS']:
					x += env.AddPostAction(app, customAction(z))
			return x

		def fixupCgRefs(env, bundledir, app, frameworks):
			def customAction(framework):
				cmd = 'install_name_tool -change %s %s' % (cgFrameworkSrc(framework), cgFrameworkDst(framework))
				def action(target, source, env):
					return spawn(env, cmd + ' ' + str(target[0]))
				return env.Action(action, nullPrint)
			x = [app]
			for z in frameworks:
				if z in env['FRAMEWORKS']:
					x += env.AddPostAction(app, customAction(z))
			return x
		
		def fixupSDLRefs(env, bundledir, app, frameworks):
			def customAction(framework):
				cmd = 'install_name_tool -change %s %s' % (sdlFrameworkSrc(framework), sdlFrameworkDst(framework))
				def action(target, source, env):
					return spawn(env, cmd + ' ' + str(target[0]))
				return env.Action(action, nullPrint)
			x = [app]
			for z in frameworks:
				if z in env['FRAMEWORKS']:
					x += env.AddPostAction(app, customAction(z))
			return x

		def fixupQtRefs(env, bundledir, app, frameworks):
			def customAction(framework):
				cmd = 'install_name_tool -change %s %s' % (qtFrameworkSrc(framework), qtFrameworkDst(framework))
				def action(target, source, env):
					return spawn(env, cmd + ' ' + str(target[0]))
				return env.Action(action, nullPrint)
			x = [app]
			for z in frameworks:
				if z in env['FRAMEWORKS']:
					x += env.AddPostAction(app, customAction(z))
			return x
			
		def InstallBundle (env, target_dir, bundle):
			"""Move a Mac OS-X bundle to its final destination"""

			if exists(target_dir) and not isdir (target_dir):
			   raise SCons.Errors.UserError, "InstallBundle: %s needs to be a directory!"%(target_dir)

			bundledirs = env.arg2nodes (bundle, env.fs.Dir)
			outputs = []
			for bundledir in bundledirs:
				suffix = bundledir.name [bundledir.name.rfind ('.'):]
				if (exists(bundledir.name) and not isdir (bundledir.name)) or suffix != '.app':
				   raise SCons.Errors.UserError, "InstallBundle: %s needs to be a directory with a .app suffix!"%(bundledir.name)

			# copy all of them to the target dir

				outputs += env.copyFiles (target_dir, bundledir)
								
			return outputs

		# Common type codes are BNDL for generic bundle and APPL for application.
		def MakeOSXBundle(
			env, 
			bundledir, 
			app,
			creator,
			qtFrameworks = ['QtCore', 'QtGui', 'QtOpenGL'],
			sdlFrameworks = ['SDL'],
			cgFrameworks = ['Cg'],
			openALFrameworks = ['OpenAL'],
			resources=None):
			"""Install a bundle into its dir, in the proper format"""
			resources = resources or []
			
			if SCons.Util.is_List(app):
				app = app[0]

			if SCons.Util.is_String(app):
				app = env.subst(app)
				appbase = basename(app)
			else:
				appbase = basename(str(app))
			if not ('.' in bundledir):
				bundledir += '.$BUNDLEDIRSUFFIX'
			bundledir = env.subst(bundledir) # substitute again
			env.SideEffect (bundledir, app)
			
			inst = env.Install(bundledir+'/Contents/MacOS', app)
        
			copySDLdylibs(inst, env, bundledir+'/Contents/Frameworks')
			
			if (qtFrameworks != None):
				a = fixupQtRefs(env, bundledir, app, qtFrameworks)
				a = env.AddPostAction(a, inst)
				copyQtFrameworks(a, env, bundledir, qtFrameworks)

			if (sdlFrameworks != None):
				a = fixupSDLRefs(env, bundledir, app, sdlFrameworks)
				a = env.AddPostAction(a, inst)
				copySDLFrameworks(a, env, bundledir, sdlFrameworks)

			if (cgFrameworks != None):
				a = fixupCgRefs(env, bundledir, app, cgFrameworks)
				a = env.AddPostAction(a, inst)
				copyCgFrameworks(a, env, bundledir, cgFrameworks)

			env.Install(bundledir+'/Contents', 'OSX/Info.plist')
#			f=env.SubstInFile(bundledir+'/Contents/Info.plist', info_plist,
#							SUBST_DICT=subst_dict)
#			env.Depends(f, SCons.Node.Python.Value(key+creator+typecode))#+env['VERSION_NUM']+env['VERSION_NAME']))
			env.WriteVal(target=bundledir+'/Contents/PkgInfo',
						 source=SCons.Node.Python.Value('AAPL'+creator))
			for r in resources:
				if SCons.Util.is_List(r):
					env.InstallAs(join(bundledir+'/Contents/Resources', r[1]), r[0])
				else:
					env.Install(bundledir+'/Contents/Resources', r)
			return [ SCons.Node.FS.default_fs.Dir(bundledir) ]
			
		# Common type codes are BNDL for generic bundle and APPL for application.
		def MakeIOSBundle(
			env, 
			type, 
			bundledir, 
			app,
			root='',
			resources=None):
			"""Install a bundle into its dir, in the proper format"""
			resources = resources or []
			
			if SCons.Util.is_List(app):
				app = app[0]

			if SCons.Util.is_String(app):
				app = env.subst(app)
				appbase = basename(app)
			else:
				appbase = basename(str(app))
			if not ('.' in bundledir):
				bundledir += '.$BUNDLEDIRSUFFIX'
			bundledir = env.subst(bundledir) # substitute again
			env.SideEffect (bundledir, app)
			
			env.Install(bundledir, app)
			env.Install(bundledir, root + 'XCode/IOS/' + type + '/Info.plist')
			env.Install(bundledir, root + 'XCode/IOS/' + type + '/MainWindow.nib')
			env.Install(bundledir, root + 'XCode/IOS/' + type + '/PkgInfo')
			
			if type == 'iPhone.device':
				env.Install(bundledir, root + 'XCode/IOS/' + type + '/embedded.mobileprovision')
				env.Install(bundledir, root + 'XCode/IOS/' + type + '/ResourceRules.plist')
			
			for r in resources:
				if SCons.Util.is_List(r):
					env.InstallAs(join(bundledir, r[1]), r[0])
				else:
					env.Install(bundledir, r)
			
			return [ SCons.Node.FS.default_fs.Dir(bundledir) ]

		# This is not a regular Builder; it's a wrapper function.
		# So just make it available as a method of Environment.
		SConsEnvironment.MakeOSXBundle = MakeOSXBundle
		SConsEnvironment.MakeIOSBundle = MakeIOSBundle
		SConsEnvironment.InstallBundle = InstallBundle
		SConsEnvironment.copyFiles = copyFiles

def IOS_UNIVERSAL_BIN(env):

	env.Append(TOOLS='IOSUB')
	
	def unibin_action(target, source, env):
		root = env['IOSUB_ROOT']
		cmd = '/Developer/Platforms/iPhoneOS.platform/Developer/usr/bin/libtool -static "%s" "%s" -o "%s"' % \
			(root + '/' + str(source[0]), root + '/' + str(source[1]), root + '/' + str(target[0]))
		return spawn(env, cmd)
					
	def unibin_print(target, source, env):
		"""This is what gets printed on the console."""
		return 'creating universal binary...\n'

	def unibin_emitter(target, source, env):
		return target, source
		
	build_action=SCons.Action.Action(unibin_action, unibin_print)
	env['BUILDERS']['BuildUniversalBinary'] = Builder(action=build_action, emitter=unibin_emitter)
	
def TOOL_SUBST(env):
	"""Adds SubstInFile builder, which substitutes the keys->values of SUBST_DICT
	from the source to the target.
	The values of SUBST_DICT first have any construction variables expanded
	(its keys are not expanded).
	If a value of SUBST_DICT is a python callable function, it is called and
	the result is expanded as the value.
	If there's more than one source and more than one target, each target gets
	substituted from the corresponding source.
	"""
	env.Append(TOOLS = 'SUBST')
	def do_subst_in_file(targetfile, sourcefile, dict):
		"""Replace all instances of the keys of dict with their values.
		For example, if dict is {'%VERSION%': '1.2345', '%BASE%': 'MyProg'},
		then all instances of %VERSION% in the file will be replaced with 1.2345 etc.
		"""
		try:
			f = open(sourcefile, 'rb')
			contents = f.read()
			f.close()
		except:
			raise SCons.Errors.UserError, "Can't read source file %s"%sourcefile
		for (k,v) in dict.items():
			contents = re.sub(k, v, contents)
		try:
			f = open(targetfile, 'wb')
			f.write(contents)
			f.close()
		except:
			raise SCons.Errors.UserError, "Can't write target file %s"%targetfile
		return 0 # success

	def subst_in_file(target, source, env):
		if not env.has_key('SUBST_DICT'):
			raise SCons.Errors.UserError, "SubstInFile requires SUBST_DICT to be set."
		d = dict(env['SUBST_DICT']) # copy it
		for (k,v) in d.items():
			if callable(v):
				d[k] = env.subst(v())
			elif SCons.Util.is_String(v):
				d[k]=env.subst(v)
			else:
				raise SCons.Errors.UserError, "SubstInFile: key %s: %s must be a string or callable"%(k, repr(v))
		for (t,s) in zip(target, source):
			return do_subst_in_file(str(t), str(s), d)

	def subst_in_file_string(target, source, env):
		"""This is what gets printed on the console."""
		return '\n'.join(['Substituting vars from %s into %s'%(str(s), str(t))
						  for (t,s) in zip(target, source)])

	def subst_emitter(target, source, env):
		"""Add dependency from substituted SUBST_DICT to target.
		Returns original target, source tuple unchanged.
		"""
		d = env['SUBST_DICT'].copy() # copy it
		for (k,v) in d.items():
			if callable(v):
				d[k] = env.subst(v())
			elif SCons.Util.is_String(v):
				d[k]=env.subst(v)
		env.Depends(target, SCons.Node.Python.Value(d))
		# Depends(target, source) # this doesn't help the install-sapphire-linux.sh problem
		return target, source

	subst_action=SCons.Action.Action(subst_in_file, subst_in_file_string)
	env['BUILDERS']['SubstInFile'] = Builder(action=subst_action, emitter=subst_emitter)

def TOOL_WRITE_VAL(env):
	env.Append(TOOLS = 'WRITE_VAL')
	def write_val(target, source, env):
		"""Write the contents of the first source into the target.
		source is usually a Value() node, but could be a file."""
		f = open(str(target[0]), 'wb')
		f.write(source[0].get_contents())
		f.close()
	env['BUILDERS']['WriteVal'] = Builder(action=write_val)


def osx_copy( dest, source, env ):
	from macostools import copy
	copy( source, dest )
	shutil.copymode(source, dest)
