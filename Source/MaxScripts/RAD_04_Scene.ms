-- RAD_Scene.ms
-- Copyright (c) 2010 Sunside Inc., All Rights Reserved
-- Author: Joe Riedel
-- See Radiance/LICENSE for licensing terms.

-- [name, origin[3], id, subobjs[], ids[]]

global kSceneFile_Id =  (bit.shift (bit.charAsInt "N") 24) + (bit.shift (bit.charAsInt "C") 16) + (bit.shift (bit.charAsInt "S") 8) + (bit.charAsInt "R")
global kSceneFile_Version = 7
global kSceneFlag_HasMaterialFlag = 0x80000000
global kSceneFlag_HasMesh = 0x00100000
global kSceneFlag_HasAnim = 0x00200000
global kSceneFlag_CinematicObject = 0x00080000
global kSceneFlag_HideUntilRefed = 0x00040000
global kSceneFlag_HideWhenDone = 0x00010000
global kSceneFlag_SetBBox = 0x00008000
global kSceneFlag_ObjectIsWorld = 0x00004000
global kSceneFlag_ObjectIsObject = 0x00002000
global kSceneFlag_CastShadows = 0x00001000
global kSceneFlag_SkyActor = 0x00000800
global kLightFlag_Diffuse = 0x000000001
global kLightFlag_Specular = 0x00000002
global kLightFlag_CastShadows = 0x00000004
global kLightFlag_AffectWorld = 0x00000008
global kLightFlag_AffectPlayer = 0x00000010
global kLightFlag_AffectObjects = 0x00000020

function RAD_GatherEntities objs = (
	ents = #()
	meshes = RAD_Exclude (RAD_ConvertableFilter objs #(Mesh)) kRADHelperTypes -- exclude helper types
	ids = RAD_AssignIDs meshes 1
	
	if (meshes.count > 0) then (
		ents = append ents #("worldspawn", [0, 0, 0], 0, meshes, ids[2])
	)
	
	ents
)

function RAD_GatherCameras objs = (
	return RAD_Filter objs #(Targetcamera, Freecamera)
)

function RAD_GatherEnabledLights objs = (
	x = RAD_Filter objs kRADLightTypes
	z = #()
	
	for l in x do (
		if (l.on) then (
			append z l
		)
	)
	
	z
)

function RAD_ExportObjectsToSceneFile objs doMeshes doAnims doLights suppressDialogs fileName = (
	file = fopen fileName "wb"
	if (file == undefined) then (
		messagebox ("cannot open " + fileName + " for write!")
		return undefined
	)
	
	cancel = false
	numtris = 0
	
	--try
	(
	
		progressStart "Exporting Scene..."
		
		-- Header
		
		WriteLong file kSceneFile_Id
		WriteLong file kSceneFile_Version
		
		-- Materials
		
		mats = sceneMaterials
		WriteLong file mats.count #unsigned
		for x in mats do (
			flags = 0
			WriteString file x.name
			if ((classof x) == MultiMaterial) then (
				flags = 1
				WriteLong file flags #unsigned
				WriteLong file x.materialList.count #unsigned
				c = 1
				for y in x.materialList do (
					WriteLong file x.materialIDList[c] #unsigned
					WriteString file y.name
					c = c + 1
				)
			) else (
				WriteLong file flags #unsigned
			)
		)
		
		ents = RAD_GatherEntities objs
		
		if (doAnims) then (
			cams = RAD_GatherCameras objs
			
			WriteLong file cams.count #unsigned
			
			for x in cams do (
				WriteString file x.name
				flags = 0
				cinematicObject = getUserProp x "CinematicObject"
				if (cinematicObject != undefined and cinematicObject == true) then (flags = bit.or flags kSceneFlag_CinematicObject)
				WriteLong file flags #unsigned
				RAD_ExportAnims file false true undefined x
			)
		) else (
			WriteLong file 0 #unsigned
		)
		
		if (doLights) then (
			_lights = RAD_GatherEnabledLights objs
			
			WriteLong file _lights.count #unsigned
			
			for x in _lights do (
				
				print ("Exporting light " + x.name)
				WriteString file x.name
				WriteFloat file x.pos.x
				WriteFloat file x.pos.y
				WriteFloat file x.pos.z
								
				flags = 0
				if (x.affectDiffuse) then (flags = bit.or flags kLightFlag_Diffuse)
				if (x.affectSpecular) then (flags = bit.or flags kLightFlag_Specular)
				if (x.baseObject.castShadows) then (flags = bit.or flags kLightFlag_CastShadows)
				
				affect = getUserProp x "AffectWorld"
				if ((affect == undefined) or (affect == true)) then (flags = bit.or flags kLightFlag_AffectWorld)
				affect = getUserProp x "AffectPlayer"
				if ((affect == undefined) or (affect == true)) then (flags = bit.or flags kLightFlag_AffectPlayer)
				affect = getUserProp x "AffectObjects"
				if ((affect == undefined) or (affect == true)) then (flags = bit.or flags kLightFlag_AffectObjects)
				
				WriteLong file flags #unsigned
				
				WriteFloat file (x.color.red/255.0)
				WriteFloat file (x.color.green/255.0)
				WriteFloat file (x.color.blue/255.0)
				WriteFloat file (x.shadowcolor.red/255.0)
				WriteFloat file (x.shadowcolor.green/255.0)
				WriteFloat file (x.shadowcolor.blue/255.0)
				
				WriteFloat file (x.multiplier)
				WriteFloat file (x.farattenend)
				
			)
			
		) else (	
			WriteLong file 0 #unsigned
		)
		
		WriteLong file ents.count #unsigned
	
		c = 0
		for x in ents do (
			WriteString file x[1] -- name
			WriteLong file x[3] -- id
			org = x[2]
			WriteFloat file org.x
			WriteFloat file org.y
			WriteFloat file org.z
						
			skels = RAD_GatherSkels x[4]
			WriteLong file skels[1] #unsigned -- num skels
			
			-- note there is a skel[] per mesh in the worlspawn
			-- meshes that have no skel[] have a 0 for their skel index.
			
			for y in skels[2] do (
				if (y[1] != 0) then (
					RAD_WriteSkel file y[3]
				)
			)
			
			WriteLong file x[4].count #unsigned -- # of objects
			
			print ("Exporting " + (x[4].count as string) + " object(s) in entity " + x[1])
			
			meshNum = 1
			for m in x[4] do ( -- mesh
				if (getProgressCancel()) then (throw "cancel")
				
				WriteLong file x[5][meshNum] #unsigned -- id
				WriteString file m.name -- name
				
				flags = 0
				
				if (doMeshes or (RAD_IsVertexAnimated m)) then (-- mesh
					flags = bit.or flags kSceneFlag_HasMesh
				)
				
				if (doAnims) then (-- anims
					flags = bit.or flags kSceneFlag_HasAnim
				) 
					
				if (m.material != undefined) then ( -- has a material
					flags = bit.or flags kSceneFlag_HasMaterialFlag 
				) 
				
				cinematicObject = getUserProp m "CinematicObject"
				if (cinematicObject != undefined and cinematicObject == true) then (
					flags = bit.or flags kSceneFlag_CinematicObject
				)
				
				hideUntilRef = getUserProp m "HideUtilRef"
				if (hideUntilRef == undefined or hideUntilRef == true) then (
					flags = bit.or flags kSceneFlag_HideUntilRefed
				)
				
				hideWhenDone = getUserProp m "HideWhenDone"
				if (hideWhenDone == undefined or hideWhenDone == true) then (
					flags = bit.or flags kSceneFlag_HideWhenDone
				)
				
				skyActor = getUserProp m "SkyActor"
				if ((skyActor != undefined) and (skyActor == true)) then (
					flags = bit.or flags kSceneFlag_SkyActor
				)
				
				worldLights = getUserProp m "WorldLights"
				if (worldLights == undefined or worldLights == true) then (
					flags = bit.or flags kSceneFlag_ObjectIsWorld
				)
				
				objectLights = getUserProp m "ObjectLights"
				if (objectLights == undefined or objectLights == true) then (
					flags = bit.or flags kSceneFlag_ObjectIsObject
				)
				
				castShadows = getUserProp m "CastShadows"
				if (castShadows == undefined or castShadows == true) then (
					flags = bit.or flags kSceneFlag_CastShadows
				)
				
				setBBox = getUserProp m "SetBBox"
				if (setBBox != undefined and setBBox == true) then (
					flags = bit.or flags kSceneFlag_SetBBox
				)
				
				WriteLong file flags #unsigned
				-- write skel index
				print (m.name + " has skel index " + (skels[2][meshNum][1] as string))
				WriteLong file (skels[2][meshNum][1]-1)
				
				if (m.material != undefined) then (
					idx = findItem sceneMaterials m.material
					if (idx == 0) then (throw "Invalid material index!")
					WriteLong file (idx-1) #unsigned
				)
				
				if (setBBox != undefined and setBBox == true) then (
					bboxSize = getUserProp m "BBox"
					if (bboxSize == undefined) then (
						bboxSize = "-24 -24 -24 24 24 24" -- default
					)
					WriteString file bboxSize
				)
				
				skinEnabled = false
				if (m.modifiers[#skin] != undefined) then (
					skinEnabled = m.skin.enabled
					m.skin.enabled = false
				)
				
				print ("Exporting " + m.name)
				trim = snapshotAsMesh m
				
				if (m.modifiers[#skin] != undefined) then (
					m.skin.enabled = true
				)
				
				numtris = numtris + trim.numfaces
				
				RAD_ExportAnims file doMeshes doAnims trim m
								
				if (not skinEnabled and (m.modifiers[#skin] != undefined)) then (
					m.skin.enabled = false
				)
				
				c = c + 1
				meshNum = meshNum + 1
				progressUpdate ((c as Float) / (objs.count as Float))
			)
		)
	)
	--catch
	(
	--	cancel = true
	--	throw()
	)

	fflush file		
	fclose file
	progressEnd()
	
	if (not suppressDialogs) then (
		if (cancel) then (messagebox "Export failed.")
		else (messagebox ("Exported " + (numtris as String) + " tri(s) in " + (objs.count as String) + " object(s)."))
	)
)

function RAD_ExportScene selectionOnly exportHidden doMeshes doAnims doLights suppressDialogs filename = (

	objs = objects;
	oldSelection = selection
	oldRange = animationRange
	oldTime = sliderTime
	if (selectionOnly) then ( objs = selection )
	objs = RAD_FilterVisible objs vis:(not exportHidden)		
	objs = RAD_ConvertableFilter objs kRADAnimatableTypes
	objs = RAD_Exclude objs kRADHelperTypes -- exclude helper types
	
	if (objs.count == 0) then (
		messagebox "No objects to export!"
		return undefined
	)
	
	disableSceneRedraw()
	RAD_ExportObjectsToSceneFile objs doMeshes doAnims doLights suppressDialogs filename
	enableSceneRedraw()
	clearSelection()
	select oldSelection
	max utility mode
	animationRange = oldRange
	sliderTime = oldTime
	
)

function RAD_ExportSelectedMesh filename = (
	RAD_ExportScene true (selection[1].ishiddeninvpt) true false false true filename
)

function RAD_ExportSelectedAnims filename = (
	RAD_ExportScene true (selection[1].ishiddeninvpt) false true false true filename
)