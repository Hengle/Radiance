-- RAD_Anim.ms
-- Copyright (c) 2010 Sunside Inc., All Rights Reserved
-- Author: Joe Riedel
-- See Radiance/LICENSE for licensing terms.

-- [name, start, end, looping]

global kScene_AnimTypeSkeletal = 0
global kScene_AnimTypeVertex = 1

Struct Animation
(
	name,
	start,
	end,
	loop,
	export
)

function RAD_FindBoneNum skel name = 
(
	numBones = skinops.GetNumberBones skel.skin
	for x = 1 to numBones do
	(
		n = skinops.GetBoneName skel.skin x 0
		if (n == name) then return x
	)
	
	0
)

-- [node, parentidx]
function RAD_IndexSkel skel = 
(
	select skel
	max modify mode
	numBones = skinops.GetNumberBones skel.skin
	
	l = #()
	
	for x = 1 to numBones do
	(
		name = skinops.GetBoneName skel.skin x 0
		b = GetNodeByName name
		
		parent = 0
		if (b.parent != undefined) then
		(
			parent = RAD_FindBoneNum skel b.parent.name
		)
		
		append l #(b, x, parent, b.parent)
	)
	
	l
)

function RAD_RestoreBindPose skel =
(
	select skel
	max modify mode
	
	numBones = skinops.GetNumberBones skel.skin
	for x = 1 to numBones do
	(
		n = getNodeByName (skinops.getbonename skel.skin x 0)
		n.transform = skinutils.GetBoneBindTM skel n
	)
)

function RAD_WriteSkel file skel =
(
	idx = RAD_IndexSkel skel
	WriteLong file idx.count
	for x in idx do
	(
		WriteString file x[1].name
		WriteLong file (x[3]-1) -- parent
		tm = skinUtils.GetBoneBindTM skel x[1]
		if (x[4] != undefined) then
		(
			bp = skinUtils.GetBoneBindTM skel x[4]
			tm = tm * (inverse bp) -- parent
		)
		RAD_WriteMat3 file tm
	)
)

function RAD_GetSkelRoot skel =
(
		select skel
		max modify mode
		numBones = skinops.GetNumberBones skel.skin
		if numBones == 0 then return undefined
		name = skinops.GetBoneName skel.skin 1 0
		obj = GetNodeByName name
		while (obj.parent != undefined) do (
			if (classof(obj.parent) == CATParent) then ( exit )
			obj = obj.parent
		)
		obj
)

function RAD_IsAnimated model =
(
	s = getUserProp model "$animations$"
	(s != undefined and s != "$empty$")
)

function RAD_FilterAnimated objs =
(
	anims = #()
	for x in objs do
	(
		if (RAD_IsAnimated x) then
		(
			append anims x
		)
	)
	
	anims
)

function RAD_FilterAnimatable objs =
(
	animatable = RAD_ConvertableFilter objs kRADMeshTypes
	RAD_Exclude animatable kRADHelperTypes -- filter helpers
)

function RAD_GetObjInitialAnim obj =
(
	s = getUserProp obj "$initial_animation$"
	if(s == undefined or s == "$empty$") then return undefined
	s
)

function RAD_SetObjInitialAnim obj anim =
(
	if anim == undefined then 
	(
		setUserProp obj "$initial_animation$" "$empty$"
	)
	else
	(
		setUserProp obj "$initial_animation$" anim
	)
)

function RAD_FilterInitialAnim list =
(
	s = undefined
	for x in list do
	(
		z = RAD_GetObjInitialAnim x
		if (z == undefined) then return undefined
		if (s != undefined and s != z) then return undefined
		s = z
	)
	
	s
)

function RAD_FindAnimIndex list name =
(
	c = 1
	for x in list do
	(
		if (x.name == name) then return c
		c = c + 1
	)
	
	return 0
)

function RAD_NewAnimName list =
(
	for c = 1 to 9999 do
	(
		str = "New Anim " + (c as string)
		if ((RAD_FindAnimIndex list str) == 0) then return str
	)
	
	"New Anim"
)

function RAD_ParseAnimString str = 
(
	str = filterString str ";"
	Animation str[1] (str[2] as Integer) (str[3] as Integer) (str[4] as BooleanClass) (str[5] as BooleanClass)
)

function RAD_BuildAnimString anim = 
(
	str = anim.name + ";" + (anim.start as String) + ";" + (anim.end as String) + ";" + (anim.loop as String) + ";" + (anim.export as String)
)

function RAD_LoadObjAnims obj =
(
	s = getUserProp obj "$animations$"
	if (s == undefined or s == "$empty$" or ((not (canConvertTo obj Mesh)) and (classof(obj) != Freecamera) and (classof(obj) != Targetcamera))) then
	(
		return #()
	)
	
	s = filterString s ":"
	anims = #()
	
	for x in s do
	(
		z = RAD_ParseAnimString x
		if (z != undefined) then append anims z
	)
	
	if anims.count == 0 then
	(
		return undefined
	)
	
	anims
)

function RAD_SaveObjAnims obj anims = 
(
	s = undefined
	for a in anims do
	(
		if (s != undefined) then
		(
			s = s + ":" + (RAD_BuildAnimString a)
		)
		else
		(
			s = RAD_BuildAnimString a
		)
	)
	
	if (s == undefined) then
	(
		setUserProp obj "$animations$" "$empty$"
	)
	else
	(
		setUserProp obj "$animations$" s
	)
)

function RAD_SetObjAnim obj anim = 
(
	anims = RAD_LoadObjAnims obj
	found = false

	for x in anims do
	(
		if (x.name == anim.name) then
		(
			if (anim.start != undefined) then x.start = anim.start
			if (anim.end != undefined) then x.end = anim.end
			if (anim.loop != undefined) then x.loop = anim.loop
			if (anim.export != undefined) then x.export = anim.export
			found = true
			exit
		)
	)
	
	if not found then 
	(
		append anims anim
	)
	
	RAD_SaveObjAnims obj anims	
)

function RAD_DeleteObjAnim obj name =
(
	anims = RAD_LoadObjAnims obj
	idx = RAD_FindAnimIndex anims name
	if (idx > 0) then
	(	
		deleteItem anims idx
		RAD_SaveObjAnims obj anims
	)
)

function RAD_RenameObjAnim obj oldName newName =
(
	anims = RAD_LoadObjAnims obj
	idx = RAD_FindAnimIndex anims oldName
	if (idx > 0) then
	(	
		anims[idx].name = newName
		RAD_SaveObjAnims obj anims
	)
)

function RAD_FindAnimName anims name =
(
	for x in anims do
	(
		if x.name == name then return x
	)
	undefined
)

function RAD_GatherAnims objs =
(
	r = #()
	anims = #()
	
	for x in objs do
	(
		a = RAD_LoadObjAnims x
		if (a != undefined) then append anims a
	)
	
	for x in anims do
	(
		for y in x do
		(
			found = 1
			anim = copy y
			
			for z in anims do
			(
				if (z == x) then continue
				
				for w in z do
				(
					if (w.name == y.name) then
					(
						found = found + 1
						if (w.start != y.start) then anim.start = undefined
						if (w.end != y.end) then anim.end = undefined
						if (w.loop != y.loop) then anim.loop = undefined
						if (w.export != y.export) then anim.export = undefined
						exit
					)
				)
			)
			
			if found == anims.count then append r anim
		)
	)
	
	r
)

function RAD_IsVertexAnimated obj = (
	if ((classof(obj) == Freecamera) or (classof(obj) == Targetcamera)) then (
		return false
	)
	
	if (obj.modifiers[#skin] != undefined) then (
		return false
	)
	
	z = RAD_LoadObjAnims obj
	z.count > 0
)

-- [num, [(skelnum, root, obj)]]
function RAD_GatherSkels objs =
(
	c = 0
	l = #()
	
	for x in objs do
	(
		if ((not RAD_IsVertexAnimated x) and (RAD_IsAnimated x)) then
		(
			root = RAD_GetSkelRoot x
			if (root != undefined) then
			(
				append l #(c+1, root, x)
				c = c + 1
				continue
			)
		)
		
		append l #(0, undefined, x)
	)
	
	#(c, l)
)


function RAD_GetNodeKeyFrameTag obj frame =
(
	for i = 1 to (numNoteTracks obj) do
	(
		ntrack = getNoteTrack obj i
		for k = 1 to ntrack.keys.count do
		(
			note = ntrack.keys[k]
			if note.time == frame then
			(
				return note.value
			)
		)
	)
	
	""
)

function RAD_GetBoneTM b =
(
	cl = classof b
	
	if (cl == CATParent) then (
		return (matrix3 1)
	)
	
	if ((cl == CATBone) or (cl == HubObject)) then
	(
		return b.objecttransform
	)
	
	b.transform
)

function RAD_ExportSkAnims file doMeshes doAnims isCamera anims obj = (
	select obj
	
	max modify mode
	numBones = skinops.GetNumberBones obj.skin
	numVerts = skinops.GetNumberVertices obj.skin
		
	if (doMeshes) then
	(
		for x = 1 to numVerts do
		(
			numWeights = skinops.GetVertexWeightCount obj.skin x
			WriteLong file numWeights #unsigned
			for w = 1 to numWeights do
			(
				weight = skinops.GetVertexWeight obj.skin x w
				boneid = skinops.GetVertexWeightBoneID obj.skin x w
				WriteFloat file weight
				WriteLong file (boneid-1)
			)
		)
	)
	
	if (not doAnims) then (return undefined)
	
	for x in anims do
	(
		print ("Exporting " + obj.name + " : skeletal animation '" + x.name + "'")
		
		flags = 0
		if (x.loop) then flags = 1
		WriteString file x.name
		WriteLong file flags #unsigned
		WriteLong file x.start
		WriteLong file ((x.end-x.start)+1) #unsigned
		
		animationRange = interval x.start (x.end+1)
		
		for f = x.start to x.end do
		(
			sliderTime = f
			
			if (isCamera) then
			(
				WriteFloat file obj.fov
				x = RAD_GetBoneTM obj
				x = (rotateXMatrix -90) * (rotateYMatrix 90) * x
				RAD_WriteBoneTM file x
				WriteString file (RAD_GetNodeKeyFrameTag obj f)
			)
			else
			(
				for bn =1 to numBones do
				(
					b = getNodeByName (skinops.GetBoneName obj.skin bn 0)
					
					tm = RAD_GetBoneTM b
					
					if ((b.parent != undefined) and (classof(b.parent) != CATParent)) then 
					(
						tm = tm * (inverse (RAD_GetBoneTM b.parent))
					)
					
					RAD_WriteBoneTM file tm
					WriteString file (RAD_GetNodeKeyFrameTag b f)
				)
			)
		)
	)
)

function RAD_ExportVertexAnims file anims obj = (

	keyframes = RAD_GetObjectKeyframeTimes obj
	if (keyframes == undefined) then (
		keyframes = #()
	)
	
	for x in anims do (
		
		flags = 0
		if (x.loop) then flags = 1
		WriteString file x.name
		WriteLong file flags #unsigned
		WriteLong file x.start
		WriteLong file ((x.end-x.start)+1) #unsigned
		
		boundedFrames = RAD_GetKeyframeBounds (x.start as time) (x.end as time) keyframes
		WriteLong file (boundedFrames.count) #unsigned
		
		s = "("
		for i = 1 to boundedFrames.count do (
			if (i > 1) then (s = s + ", ")
			s = s + (boundedFrames[i] as string)
		)
		
		s = s + ")"
		
		print ("Exporting " + obj.name + " : vertex animation '" + x.name + "' with keyframes " + s)
		
		animationRange = interval x.start (x.end+1)
		
		for f in boundedFrames do (
			sliderTime = f
			WriteLong file f #unsigned
			trim = snapshotAsMesh obj
			RAD_WriteTriMeshVerts file trim
		)
	)
)

function RAD_ExportAnims file doMeshes doAnims trim obj =
(
	isCamera = (classof(obj) == Freecamera) or (classof(obj) == Targetcamera)
	if ((not doAnims) and isCamera) then throw "Cannot export cameras without animation!"
	
	z = RAD_LoadObjAnims obj
	anims = #()
	for x in z do
	(
		if (x.export and x.end >= x.start) then append anims x
	)
	
	if (obj.modifiers[#skin] != undefined) then (
		if (doMeshes) then (
			RAD_WriteTriMesh file trim
		)
	) else if (z.count > 0) then ( -- it's vertex animated, always write full tri-mesh
		if (not isCamera) then (
			RAD_WriteTriMesh file trim
		)
	) else (
		if (doMeshes) then (
			RAD_WriteTriMesh file trim
		)
		WriteLong file -1 -- not animated
		return undefined
	)
	
	if (doAnims) then
	(
		WriteLong file anims.count
		WriteLong file frameRate #unsigned
	)
	else
	(
		WriteLong file 0
		WriteLong file frameRate #unsigned
	)
	
	if (isCamera or (obj.modifiers[#skin] != undefined)) then (
		WriteLong file kScene_AnimTypeSkeletal
		return RAD_ExportSkAnims file doMeshes doAnims isCamera anims obj
	)
	
	WriteLong file kScene_AnimTypeVertex
	RAD_ExportVertexAnims file anims obj
)

