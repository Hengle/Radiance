-- RAD_Mesh.ms
-- Copyright (c) 2010 Pyramind Labs LLC, All Rights Reserved
-- Author: Joe Riedel (joeriedel@hotmail.com)
-- See Radiance/LICENSE for licensing terms.

-- All meshes may reference a skeleton

function RAD_WriteTriMesh file mesh =
(
	nummaps = (meshop.getNumMaps mesh)-1
	c = 0
	for i=1 to nummaps do
	(
		if (meshop.getMapSupport mesh i) then ( c = c + 1 )
	)
	
	WriteLong file mesh.numverts #unsigned
	WriteLong file mesh.numfaces #unsigned
	WriteLong file c #unsigned
	
	print ("Num maps: " + (c as string))
	
	for i=1 to mesh.numverts do
	(
		v = getVert mesh i
--		z = point3 v.x v.z -v.y
		RAD_WritePoint3 file v
	)
	for i=1 to nummaps do
	(
		if (meshop.getMapSupport mesh i) then
		(
			z = meshop.getNumMapVerts mesh i
			WriteLong file z #unsigned
			for y=1 to z do
			(
				v = meshop.getMapVert mesh i y
				v.y = 1.0 - v.y
				RAD_WritePoint2 file v
			)
			z = meshop.getNumMapFaces mesh i
			if (z != mesh.numfaces) then ( throw "This shouldn't ever happen!" )
			for y=1 to z do
			(
				f = meshop.getMapFace mesh i y
				WriteLong file ((f.x as Integer)-1) #unsigned
				WriteLong file ((f.y as Integer)-1) #unsigned
				WriteLong file ((f.z as Integer)-1) #unsigned
			)
		)
	)
	for i=1 to mesh.numfaces do
	(
		f = getFace mesh i
		WriteLong file ((f.x as Integer)-1) #unsigned
		WriteLong file ((f.y as Integer)-1) #unsigned
		WriteLong file ((f.z as Integer)-1) #unsigned
		WriteLong file (getFaceSmoothGroup mesh i) #unsigned
		WriteLong file (getFaceMatId mesh i) #unsigned
	)
)