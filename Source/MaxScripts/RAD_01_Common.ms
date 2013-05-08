-- RAD_Common.ms
-- Copyright (c) 2010 Sunside Inc., All Rights Reserved
-- Author: Joe Riedel
-- See Radiance/LICENSE for licensing terms.

global kRADBoneTypes = #(BoneGeometry, CATBone, CATParent, HubObject)
global kRADHelperTypes = #(BoneGeometry, CATBone, CATParent, HubObject, SplineShape, Circle, Rectangle)
global kRADAnimatableTypes = #(Mesh, Targetcamera, Freecamera, Omnilight)
global kRADLightTypes = #(Omnilight)

function RAD_PrintProps obj = (

	z = getpropnames obj
	
	for p in z do (
		print p
	)
	
)

function RAD_GetObjectTransformKeyframeTimes obj = (
	pos = for x in obj.transform.controller.position.controller.keys collect x.time
	rot = for x in obj.transform.controller.rotation.controller.keys collect x.time
	scl = for x in obj.transform.controller.scale.controller.keys collect x.time
	
	keys = join pos rot
	keys = join keys scl
	keys = makeUniqueArray keys
	
	if (keys.count < 1) then (
		keys = undefined
	)
		
	keys
)

function RAD_GetLoftLineKeyframeTimes obj = (
	keyframes = undefined
	baseLine = obj.baseobject[#line][#master]
	props = getPropNames baseLine #dynamicOnly
	
	for p in props do (
		z = execute ("($'" + obj.name + "').baseobject[#line][#master]." + (p as string) + ".controller.keys")
		if (z.count > 1) then (
			w = for x in z collect x.time
			if (keyframes == undefined) then (
				keyframes = #()
			)
			join keyframes w
		)
	)
	
	xformkeys = RAD_GetObjectTransformKeyframeTimes obj
	if (xformkeys != undefined) then (
		if (keyframes != undefined) then (
			join keyframes xformkeys
		) else (
			keyframes = xformkeys
		)
	)
	
	if (keyframes != undefined) then (
		keyframes = makeUniqueArray keyframes
		sort keyframes
	)
	
	keyframes
)

function RAD_GetLoftKeyframeTimes obj = (
	if (obj.line != undefined) then (
		if (classof(obj.baseobject[#line][#master]) == SubAnim) then (
			return RAD_GetLoftLineKeyFrameTimes obj
		)
	)
	undefined
)

function RAD_GetEditableMeshKeyframeTimes obj = (
	props = getPropNames obj #dynamicOnly
	keyframes = undefined
	
	for p in props do (
		z = execute("($'" + obj.name + "')." + (p as string) + ".controller.keys")
		if (z.count > 1) then (
			w = for x in z collect x.time
			if (keyframes == undefined) then (
				keyframes = #()
			)
			join keyframes w
		)
	)
	
	xformkeys = RAD_GetObjectTransformKeyframeTimes obj
	if (xformkeys != undefined) then (
		if (keyframes != undefined) then (
			join keyframes xformkeys
		) else (
			keyframes = xformkeys
		)
	)
	
	if (keyframes != undefined) then (
		keyframes = makeUniqueArray keyframes
		sort keyframes
	)
	
	keyframes
)

function RAD_GetObjectKeyframeTimes obj = (
	c = classof(obj)
	if (c == Loft) then (
		return RAD_GetLoftKeyframeTimes(obj)
	) else if (c == Editable_Mesh) then (
		return RAD_GetEditableMeshKeyframeTimes(obj)
	)
	
	undefined
)

function RAD_GetKeyframeBounds start end keys = (
	z = #()
	append z start
	for k in keys do (
		if (k >= end) then ( exit )
		if (k > start) then (
			append z k
		)
	)
	append z end
	z = makeUniqueArray z
	sort z
	z
)

function RAD_WritePoint3 file p = (
	WriteFloat file p.x
	WriteFloat file p.y
	WriteFloat file p.z
)

function RAD_WritePoint2 file p = (
	WriteFloat file p.x
	WriteFloat file p.y
)

function RAD_WriteQuat file q = (
	q = inverse q
	WriteFloat file q.w
	WriteFloat file q.x
	WriteFloat file q.y
	WriteFloat file q.z
)

function RAD_WriteMat3 file m = (
	for x = 1 to 4 do (
		WriteFloat file m[x].x
		WriteFloat file m[x].y
		WriteFloat file m[x].z
	)
)

function RAD_FlipMatrix m = (
	r1 = m[1]
	r2 = m[2]
	r3 = m[3]
	r4 = m[4]
	matrix3 (point3 r1.x r1.z -r1.y) (point3 r2.x r2.z -r2.y) (point3 r3.x r3.z -r3.y) (point3 r4.x r4.z -r4.y)
)

function RAD_WriteBoneTM file m = (
	RAD_WriteMat3 file m
)

function RAD_FindSubMat mat id = (
	i = 1
	for x in mat.materialIDList do (
		if (x == id) then (
			return mat.materialList[i]
		)
	)
	undefined
)

function RAD_MaterialName mat id = (
	if ((classof mat) == MultiMaterial) then (
		mat = RAD_FindSubMat mat id
		if (mat != undefined) then ( return mat.name )
		return undefined
	)
	mat.name
)

function RAD_GatherMultiMaterials list mtl = (
	mats = #()
	for s = 1 to mtl.numsubs do (
		if (classof mtl.materialList[s] == MultiMaterial) then (
			mats = gatherMultiMaterials mats mtl.materialList[s]
		) else (
			mats = append mats mtl.materialList[s]
		)
	)
	mats + list
)

function RAD_GatherSceneMaterials = (
	mats = #()
	
	mtlib = sceneMaterials
	for i = 1 to mtlib.count do (
		mtl = mtlib[i]
		if classof mtl == MultiMaterial then (
			mats = gatherMultiMaterials mats mtl
		) else (
			mats = append mats mtl
		)
	)
	
	mats
)


function RAD_ConvertPath name = (
	ss = name as StringStream
	name = ""
	while (not eof ss) do (
		c = readChar ss
		if (c != " ") then name = name + c
	)
	close ss
	name
)

function RAD_GetObjectRoot obj = (
	while (obj.parent != undefined) do (obj = obj.parent)
	obj
)

function RAD_GetObjPath obj = (
	pn = obj.name
	while obj.parent != undefined do
	(
		obj = obj.parent
		pn = obj.name + "/" + pn
	)
	pn
)

function RAD_Exclude list class = (
	r = #()
	if ((classof class) != Array) then ( class = #(class) )
	
	for x in list do (
		skip = false
		
		for c in class do (
			if ((classof x) == c) then (
				skip = true
				exit
			)
		)
		
		if (skip == false) then append r x
	)
	r
)

function RAD_Filter list class = (
	r = #()
	if ((classof class) != Array) then ( class = #(class) )
	
	for x in list do (
		for c in class do (
			if ((classof x) == c) then (
				r = append r x
				exit
			)
		)
	)
	r
)

function RAD_ConvertableFilter list class = (
	r = #()
	if ((classof class) != Array) then ( class = #(class) )
	
	for x in list do (
		for c in class do (
			if (canConvertTo x c) then  ( 
				r = append r x
				exit
			)
		)
	)
	r
)

function RAD_FilterVisible objs val:true = (
	l = #()
	for x in objs do (
		if (x.isHiddenInVpt != val) then ( l = append l x )
	)
	l
)

function RAD_FilterSelected objs val:false = (
	l = #()
	for x in objs do (
		if (x.isSelected == val) then ( l = append l x )
	)
	l
)

function RAD_FilterVisibleSelected objs vis:true sel:true  = (
	l = #()
	for x in objs do (
		if (x.isSelected == sel and x.isHiddenInVpt != vis) then ( l = append l x )
	)
	l
)

function RAD_SplitTypes objs types = (
	l = #(objs)
	for t in types do (
		l = append l (RAD_Filter objs t)
	)
	l
)

function RAD_ConvertableSplitTypes objs types = (
	l = #(objs)
	for t in types do (
		l = append l (RAD_ConvertableFilter objs t)
	)
	l
)

-- This function gathers the requested property of all the indicated objects into an array
-- Return value = R[]
-- The first array element is a boolean indicating if all the properties are identical
-- R[2] contains an array for each object containing the 
-- FirstObject = R[2][1], SecondObject = R[2][2]... etc
-- FirstObject is an array, index [1] contains the object, [2] is an array of the property values
--
-- Therefore, if R[1] == true then all objects have identical properties, and R[2][1][2][1] will contain the
-- value of the first property.

function RAD_GatherObjectProps objs props defaults = (
	vals = #()
	aggr = #()
	same = true
	for x in defaults do (
		aggr = append aggr undefined
	)
	for x in objs do (
		c = 1
		objVals = #()
		for p in props do (
			v = getUserProp x p
			if (v == undefined) then (
				v = defaults[c]	
			)
			
			v = v as String
			
			if (aggr[c] == undefined) then (
				aggr[c] = v
			) else (
				same = same and (aggr[c] == v)
			)
			
			objVals = append objVals v
			c = c + 1
		)
		vals = append vals #(x, objVals)
	)
	#(same, vals)
)

function RAD_SetObjectProps objs props = (
	for x in objs do (
		for p in props do (
			setUserProp x p[1] p[2]
		)
	)
	objs
)

function RAD_FilterByProps objs props defaults values = (
	objs = RAD_GatherObjectProps objs props defaults
	l = #()
	for x in objs[2] do ( -- for each object
		c = 0
		for y in x[2] do ( -- for each propery in the object
			if (y != values[c+1]) then exit
			c = c + 1
		)
		if (c == values.count) then (l = append l x[1])
	)
	l
)

function RAD_FilterByModifiers list type = (
	out = #()
	
	for x in list do (
		if (x.modifiers[type] != undefined) then append out x
	)
	
	out
)

function RAD_AssignIDs list first = (
	ids = #()
	for x in list do (
		setUserProp x "id" first
		ids = append ids first
		first = first + 1
	)
	#(first, ids)
)

function RAD_GatherObjsByID objs id = (
	l = #()
	id = id as String
	for x in list do (
		z = getUserProp x "id"
		if (z == id) then ( l = append l x )
	)
	l
)

function RAD_GetObj name = (
	execute ("$" + (convertPath name))
)

function RAD_AddSelChangeHandler cmd name = (
	callbacks.addScript #selectionSetChanged cmd id:name
)

function RAD_RemoveSelChangeHandler name = (
	callbacks.removeScripts #selectionSetChanged id:name
)

function RAD_AddFileOpenHandler cmd name = (
	callbacks.addScript #filePostOpenProcess cmd id:name
	callbacks.addScript #postImport cmd id:name
)

function RAD_RemoveFileOpenHandler name = (
	callbacks.removeScripts #filePostOpenProcess id:name
	callbacks.removeScripts #postImport id:name
)
