-- RAD_Scene.ms
-- Copyright (c) 2010 Pyramind Labs LLC, All Rights Reserved
-- Author: Joe Riedel (joeriedel@hotmail.com)
-- See Radiance/LICENSE for licensing terms.

-- [name, origin[3], id, subobjs[], ids[]]

function RAD_GatherEntities objs =
(
	ents = #()
	meshes = RAD_Exclude (RAD_ConvertableFilter objs #(Mesh)) Bone -- exclude bones
	ids = RAD_AssignIDs meshes 1
	
	if (meshes.count > 0) then
	(
		ents = append ents #("worldspawn", [0, 0, 0], 0, meshes, ids[2])
	)
	
	ents
)

function RAD_GatherCameras objs =
(
	return RAD_Filter objs #(Freecamera)
)

function RAD_ExportObjectsToSceneFile objs doMeshes doAnims fileName =
(
	file = fopen fileName "wb"
	if (file == undefined) then
	(
		messagebox ("cannot open " + fileName + " for write!")
		return undefined
	)
	
	cancel = false
	numtris = 0
	
	--try
	(
	
		progressStart "Exporting Scene..."
		
		-- Header
		
		id = (bit.shift (bit.charAsInt "N") 24) + (bit.shift (bit.charAsInt "C") 16) + (bit.shift (bit.charAsInt "S") 8) + (bit.charAsInt "R")
		version = 3
		
		WriteLong file id
		WriteLong file version
		
		-- Materials
		
		mats = sceneMaterials
		WriteLong file mats.count #unsigned
		for x in mats do
		(
			flags = 0
			WriteString file x.name
			if ((classof x) == MultiMaterial) then
			(
				flags = 1
				WriteLong file flags #unsigned
				WriteLong file x.materialList.count #unsigned
				c = 1
				for y in x.materialList do
				(
					WriteLong file x.materialIDList[c] #unsigned
					WriteString file y.name
					c = c + 1
				)
			)
			else
			(
				WriteLong file flags #unsigned
			)
		)
		
		ents = RAD_GatherEntities objs
		
		if (doAnims) then
		(
			cams = RAD_GatherCameras objs
			
			WriteLong file cams.count #unsigned
			
			for x in cams do
			(
				WriteString file x.name
				flags = 0
				cinematicObject = getUserProp x "CinematicObject"
				if (cinematicObject != undefined and cinematicObject == true) then (flags = bit.or flags 0x00080000)
				WriteLong file flags #unsigned
				RAD_ExportAnims file false true x
			)
		)
		else
		(
			WriteLong file 0 #unsigned
		)
		
		WriteLong file ents.count #unsigned
	
		c = 0
		for x in ents do
		(
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
			
			for y in skels[2] do
			(
				if (y[1] != 0) then
				(
					RAD_WriteSkel file y[3]
				)
			)
			
			WriteLong file x[4].count #unsigned -- # of objects
			
			meshNum = 1
			for m in x[4] do -- mesh
			(
				if (getProgressCancel()) then (throw "cancel")
				
				WriteLong file x[5][meshNum] #unsigned -- id
				
				flags = 0
				
				if (doMeshes) then (flags = bit.or flags 0x00100000) -- mesh
				if (doAnims) then (flags = bit.or flags 0x00200000) -- anims
					
				if (m.material != undefined) then ( flags = bit.or flags 0x80000000 ) -- has a material
				
				contents = getUserProp m "Contents"
				if (contents != undefined) then
				(
					if (contents == "detail") then
					(
						flags = bit.or flags 0x40000000
					)
					else if (contents == "areaportal") then
					(
						flags = bit.or flags 0x20000000
					)
				)
				
				collision = getUserProp m "Collision"
				if (collision == undefined or collision == true) then ( flags = bit.or flags 0x00800000 )
				
				visible = getUserProp m "Visible"
				if (visible == undefined or visible == true) then ( flags = bit.or flags 0x00400000 )
				
				cinematicObject = getUserProp m "CinematicObject"
				if (cinematicObject != undefined and cinematicObject == true) then (flags = bit.or flags 0x00080000)
				
				hideUntilRef = getUserProp m "HideUtilRef"
				if (hideUntilRef == undefined or hideUntilRef == true) then ( flags = bit.or flags 0x00040000 )
				
				hideWhenDone = getUserProp m "HideWhenDone"
				if (hideWhenDone == undefined or hideWhenDone == true) then ( flags = bit.or flags 0x00010000 )
				
				WriteLong file flags #unsigned
				-- write skel index
				WriteLong file (skels[2][meshNum][1]-1)
				
				if (m.material != undefined) then
				(
					idx = findItem sceneMaterials m.material
					if (idx == 0) then (throw "Invalid material index!")
					WriteLong file (idx-1) #unsigned
				)
				
				skinEnabled = false
				if (m.modifiers[#skin] != undefined) then
				(
					skinEnabled = m.skin.enabled
					m.skin.enabled = false
				)
				
				print ("Exporting " + m.name)
				trim = snapshotAsMesh m
				
				if (m.modifiers[#skin] != undefined) then m.skin.enabled = true
				
				numtris = numtris + trim.numfaces
				
				if (doMeshes) then
				(
					RAD_WriteTriMesh file trim
				)
				
				RAD_ExportAnims file doMeshes doAnims m
								
				if (not skinEnabled and (m.modifiers[#skin] != undefined)) then
				(
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
	if (cancel) then (messagebox "Export failed.")
	else (messagebox ("Exported " + (numtris as String) + " tri(s) in " + (objs.count as String) + " object(s)."))
)
