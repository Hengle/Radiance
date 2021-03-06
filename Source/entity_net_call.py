# entity_net_call.py
# Copyright (c) 2010 Sunside Inc., All Rights Reserved
# Author: Joe Riedel (joeriedel@hotmail.com)
# See Radiance/LICENSE for licensing terms.
# Generates Engine/World/EntityNetCall.inl file

import io
import template_args as z

print "generating Engine/World/EntityNetCall.inl..."

s = io.open('Engine/World/EntityNetCall.inl', 'w')
s.write(u'// EntityNetCall.inl\n')
s.write(u'// Copyright (c) 2010 Sunside Inc., All Rights Reserved\n')
s.write(u'// auto generated by Radiance/Source/entity_net_call.py\n\n')
s.write(u'// See Radiance/LICENSE for licensing terms.\n\n')
s.write(u'#include <boost/type_traits/function_traits.hpp>\n\n')
s.write(u'namespace world {\nnamespace details {\n\n')

s.write(u'template <typename T>\nclass NetReadFMsg;\n')
s.write(u'template <typename T>\nclass NetWriteFMsg;\n\n')

def genRead(n):

	# template<A1...AN>
	s.write(u'template <typename R')
	for x in range(1, n):
		s.write(u', typename A%d' % x)
	s.write(u'>\n')
	
	# class NetFMsg<R (A1...AN)>
	# {
	# public:
	
	s.write(u'class NetReadFMsg<R (')
	for x in range(1, n):
		if x > 1:
			s.write(u', A%d' % x)
		else:
			s.write(u'A1')
	s.write(u')>\n{\npublic:\n')
	
	# typedef boost::function_traits<void (A1...AN)> Traits;
	
	s.write(u'\ttypedef boost::function_traits<R (')
	for x in range(1, n):
		if x > 1:
			s.write(u', A%d' % x)
		else:
			s.write(u'A1')
	s.write(u')> Traits;\n')
	
	# bool Read(stream::InputStream &args)
		
	s.write(u'\tbool Read(stream::InputStream &args)\n\t{\n')
	for x in range(1, n):
		s.write(u'\t\tif (!args.Read(&m_arg%d))\n\t\t\treturn false;\n' % x)
	s.write(u'\t\treturn true;\n\t}\n\n')
	
	# template <typename C, typename F>
	# void Call(this *, func*)
	
	s.write(u'\ttemplate <typename C>\n')
	s.write(u'\tvoid Call(C *c, R (C::*f)(')
	for x in range(1, n):
		if x > 1:
			s.write(u', A%d' % x)
		else:
			s.write(u'A1')
	s.write(u'))\n\t{\n')
	s.write(u'\t\t(c->*f)(')
	for x in range(1, n):
		if x > 1:
			s.write(u', m_arg%d' % x)	
		else:
			s.write(u'm_arg1')
	s.write(u');\n\t}\n')
	
	s.write(u'private:\n')
	for x in range(1, n):
		s.write(u'\ttypename Traits::arg%d_type m_arg%d;\n' % (x, x))
	s.write(u'};\n\n')
	
def genWrite(n):

	# template<A1...AN>
	s.write(u'template <typename R')
	for x in range(1, n):
		s.write(u', typename A%d' % x)
	s.write(u'>\n')
	
	# class NetWriteFMsg<R (A1...AN)>
	# {
	# public:
	
	s.write(u'class NetWriteFMsg<R (')
	for x in range(1, n):
		if x > 1:
			s.write(u', A%d' % x)
		else:
			s.write(u'A1')
	s.write(u')>\n{\npublic:\n')
	
	# typedef boost::function_traits<R (A1...AN)> Traits;
	
	s.write(u'\ttypedef boost::function_traits<R (')
	for x in range(1, n):
		if x > 1:
			s.write(u', A%d' % x)
		else:
			s.write(u'A1')
	s.write(u')> Traits;\n')
	
	# Constructor
	s.write(u'\tNetWriteFMsg(NetCmd id, stream::OutputStream &net) : m_id(id), m_net(&net) {}\n\n')
	
	# bool Write(A1...AN)
	s.write(u'\tbool Write(')
	for x in range(1, n):
		if x > 1:
			s.write(u', const A%d &a%d' % (x, x))	
		else:
			s.write(u'const A1 &a1')
			
	s.write(u')\n\t{\n')
	
	s.write(u'\t\tif (!m_net->Write(m_id))\n\t\t\treturn false;\n')
	for x in range(1, n):
		if x > 1:
			s.write(u'\t\tif (!m_net->Write(a%d))\n\t\t\treturn false;\n' % x)
		else:
			s.write(u'\t\tif (!m_net->Write(a1))\n\t\t\treturn false;\n')
	s.write(u'\t\treturn true;\n\t}\n\n')
	
	s.write(u'private:\n')
	s.write(u'\tNetCmd m_id;\n\tstream::OutputStream *m_net;\n};\n\n')

for x in range(1, 22):
	genRead(x)
	genWrite(x)

s.write(u'} // details\n} // world\n\n')
s.close()
