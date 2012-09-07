--- RAD_UtilMainms
-- Copyright (c) 2010 Pyramind Labs LLC, All Rights Reserved
-- Author: Joe Riedel (joeriedel@hotmail.com)
-- See Radiance/LICENSE for licensing terms.

utility RadianceUtil "Radiance Tools" width:166 height:643
(

	GroupBox grp1 "Selection" pos:[6,7] width:155 height:546
	radiobuttons modelTypeRadio "" pos:[19,27] width:71 height:48 labels:#("Solid", "Detail", "Areaportal") default:0 columns:1
	checkbox noCollisionCheckBox "No Clip" pos:[96,25] width:60 height:18
	GroupBox grp4 "Animation" pos:[15,82] width:138 height:270
	dropdownList animList "" pos:[18,100] width:130 height:21
	edittext animName "Name:" pos:[18,128] width:130 height:17
	GroupBox grp8 "Export" pos:[6,558] width:154 height:79
	checkbox selectionOnlyCheckbox "Selection Only" pos:[69,573] width:87 height:14
	checkbox exportHiddenCheckbox "Hidden" pos:[11,573] width:55 height:14
	button exportSceneButton "Scene..." pos:[11,592] width:65 height:17
	button exportAnimButton "Anim..." pos:[89,614] width:65 height:17
	button exportMeshButton "Mesh..." pos:[89,591] width:65 height:17
	button exportSkelButton "Skel..." pos:[11,614] width:65 height:17
	button selectOtherLikeThisButton "Select Others Like This" pos:[16,416] width:134 height:21
	button selectSolidButton "Select Solid" pos:[16,441] width:134 height:21
	button selectDetailButton "Select Detail" pos:[16,469] width:134 height:21
	button selectNoCollisionButton "Select No Collision" pos:[16,497] width:134 height:21
	button selectAreaportalButton "Select Areaportal" pos:[16,525] width:134 height:21
	checkbox nodrawCheckbox "No Draw" pos:[96,41] width:60 height:18
	edittext animStart "Start:" pos:[18,152] width:66 height:17
	edittext animEnd "End:" pos:[86,152] width:66 height:17
	checkbox animLoop "Looping" pos:[21,231] width:63 height:18
	dropdownList animInitialState "" pos:[22,271] width:123 height:21
	GroupBox grp32 "Initial State" pos:[19,253] width:129 height:64
	button animSetStart "Set Start" pos:[20,176] width:61 height:20
	button animSetEnd "Set End" pos:[87,176] width:61 height:20
	button animTest "Test" pos:[52,204] width:61 height:20
	checkbox animExport "Export" pos:[91,231] width:53 height:18
	button addAnim "Add" pos:[24,328] width:53 height:18
	button deleteAnim "Delete" pos:[87,328] width:53 height:18
	button noDefault "None" pos:[53,296] width:54 height:16
	checkbox cinematicObject "Cinematic Object" pos:[21,359] width:108 height:17
	checkbox hideUntilRef "Hide Until Referenced" pos:[21,376] width:129 height:17
	checkbox hideWhenDone "Hide When Done" pos:[21,394] width:129 height:17
	
	local animSel = #() -- selected objects that have animations
	local animSet = #() -- selected set of animations [struct Animation]
	
	persistent global RadianceUtil_lastSceneFile
	
	function exportScene doMeshes doAnims =
	(
		hidden  = exportHiddenCheckbox.checked
		objs = objects;
		oldSelection = selection
		oldRange = animationRange
		oldTime = sliderTime
		if (selectionOnlyCheckbox.checked) then ( objs = selection )
		objs = RAD_FilterVisible objs vis:(not hidden)		
		objs = RAD_Filter objs #(Editable_Mesh, Freecamera)
		objs = RAD_Exclude objs Bone -- exclude bones
		
		if (objs.count == 0) then
		(
			messagebox "No objects to export!"
			return undefined
		)
		
		if (RadianceUtil_lastSceneFile == undefined) then ( RadianceUtil_lastSceneFile = "Scene.3dx" )
		file = getSaveFileName caption:"Scene Export..." filename:RadianceUtil_lastSceneFile types:"3DX Files (*.3dx)|*.3dx|All Files (*.*)|*.*"
		if (file == undefined) then return undefined
		RadianceUtil_lastSceneFile = file
		RAD_ExportObjectsToSceneFile objs doMeshes doAnims file
		clearSelection()
		select oldSelection
		max utility mode
		animationRange = oldRange
		sliderTime = oldTime
	)
	
	function enableMeshControls val =
	(
		exportMeshButton.enabled =val 
		exportSkelButton.enabled = false 
		modelTypeRadio.enabled = val
		noCollisionCheckBox.enabled = val
		selectOtherLikeThisButton.enabled = val
		nodrawCheckbox.enabled = val
	)
	
	function enableAnimControls val =
	(
		animList.enabled = val
		animStart.enabled = val
		animEnd.enabled = val
		animName.enabled = val
		animSetStart.enabled = val
		animSetEnd.enabled = val
		animTest.enabled = val
		animInitialState.enabled = val
		animLoop.enabled = val
		animExport.enabled = val
		addAnim.enabled = val
		deleteAnim.enabled = val
		noDefault.enabled = val
		exportAnimButton.enabled = val 
	)
	
	function enableAnimObjControls val =
	(
		cinematicObject.enabled = val
		hideUntilRef.enabled = val
		hideWhenDone.enabled = val
	)
	
	function disableAll =
	(
		enableMeshControls false
		enableAnimControls false
		enableAnimObjControls false
	)
	
	function selectedMeshes = 
	(
		meshes = (RAD_ConvertableFilter (getCurrentSelection()) #(Mesh))
		RAD_Exclude meshes #(Bone) -- filter bones
	)
	
	function selectedAnimatable =
	(
		RAD_FilterAnimatable (getCurrentSelection())
	)
	
	function setSelectionType type =
	(
		meshes = selectedMeshes()
		if (type == #Solid) then (RAD_SetObjectProps meshes #(#("Contents", "solid")))
		if (type == #Detail) then (RAD_SetObjectProps meshes #(#("Contents", "detail")))
		if (type == #Areaportal) then (RAD_SetObjectProps meshes #(#("Contents", "areaportal")))
	)
	
	function setSelectionCollision val =
	(
		RAD_SetObjectProps (selectedMeshes()) #(#("Collision", val))
	)
	
	function setSelectionVisible val =
	(
		RAD_SetObjectProps (selectedMeshes()) #(#("Visible", val))
	)
	
	function saveAnims =
	(
		for x in animSel do
		(
			for y in animSet do
			(
				RAD_SetObjAnim x y
			)
		)
		
		for x in animSel do
		(
			RAD_SetObjInitialAnim x animInitialState.selected
		)
	)
	
	function setupUIForSelAnim =
	(
		anim = animSet[animList.selection]
		animName.text = anim.name
		
		if (anim.start != undefined) then
		(
			animStart.text = anim.start as string
		)
		else
		(
			animStart.text = ""
		)
		
		if (anim.end != undefined) then
		(
			animEnd.text = anim.end as string
		)
		else
		(
			animEnd.text = ""
		)
		
		if (anim.loop != undefined) then
		(
			animLoop.checked = anim.loop
		)
		else
		(
			animLoop.triState = 2
		)
		
		if (anim.export != undefined) then
		(
			animExport.checked = anim.export
		)
		else
		(
			animExport.triState = 2
		)
	)
	
	function deleteSelAnim =
	(
		anim = animSet[animList.selection]
		initial = animInitialState.selected
		
		if (anim.name == initial) then 
		(
			for x in animSel do
			(
				RAD_SetObjInitialAnim x undefined
			)
		)
		
		for x in animSel do
		(
			RAD_DeleteObjAnim x animList.selected
		)
		
		deleteItem animSet animList.selection
		items = (for x in animSet collect x.name)
		animList.items = items
		animInitialState.items = items
		
		if (animSet.count > 0) then
		(
			animList.selection = 1
			animInitialState.selection = findItem animInitialState.items (RAD_FilterInitialAnim animSel)
			setupUIForSelAnim()
		)
		else
		(
			animName.text = ""
			animStart.text = ""
			animEnd.text = ""
			animLoop.checked = false
			animExport.checked = false
			enableAnimControls false
			addAnim.enabled = true
		)
	)
	
	function addNewAnim =
	(
		anim = Animation (RAD_NewAnimName animSet) 0 0 false true
		append animSet anim
		items = (for x in animSet collect x.name)
		animList.items = items
		animList.selection = 1
		animInitialState.items = items
		animInitialState.selection = findItem animInitialState.items (RAD_FilterInitialAnim animSel)
		enableAnimControls true
		setupUIForSelAnim()
		saveAnims()
	)
	
	function renameSelAnim name =
	(
		if (name == "") then
		(
			messagebox "Invalid animation name!"
			return undefined
		)
		
		c = animList.selection
		print c
		if (name != animSet[c].name) then
		(
			print c
			if ((RAD_FindAnimIndex animSet name) > 0) then
			(
				messagebox "An animation with that name already exists!"
				return undefined
			)
			
			print c
			
			for x in animSel do
			(
				print c
				print animSet.count
				print animSet[c].name
				print name
				RAD_RenameObjAnim x animSet[c].name name
			)
			
			animSet[c].name = name
			animList.items[c] = name
			animList.selected = name
			
			animInitialState.items[c] = name
			if (animInitialState.selection == c) then animInitialState.selected = name
		)
	)
	
	function setupUIForAnims objs =
	(
		-- get animatables in the selection
		animSel = RAD_FilterAnimatable objs
		animSet = RAD_GatherAnims animSel
		
		if (animSet.count > 0) then
		(
			items = (for x in animSet collect x.name)
			animList.items = items
			animList.selection = 1
			animInitialState.items = items
			animInitialState.selection = findItem animInitialState.items (RAD_FilterInitialAnim animSel)
			enableAnimControls true
			setupUIForSelAnim()
		)
		else if (animSel.count > 0) then
		(
			animList.items = #()
			animList.enabled = false
			animInitialState.items = #()
			animInitialState.enabled = false
			animStart.enabled = false
			animEnd.enabled = false
			animSetStart.enabled = false
			animSetEnd.enabled = false
			animLoop.enabled = false
			animExport.enabled = false
			noDefault.enabled = false
			addAnim.enabled = true
			deleteAnim.enabled = false
		)
	)
	
	function setupUIForAnimatable objs = 
	(
		setupUIForAnims objs
		
		if (animSel.count > 0) then
		(
			enableAnimObjControls true
			
			vals = RAD_GatherObjectProps objs #("CinematicObject") #("false")
			if (vals[1]) then
			(
				if (vals[2][1][2][1] as BooleanClass) then
				(
					cinematicObject.triState = 1 -- checked
				)
				else
				(
					cinematicObject.triState = 0
				)
			)
			else
			(
				cinematicObject.state = 2 -- undefined
			)
			
			vals = RAD_GatherObjectProps objs #("HideUntilRef") #("true")
			if (vals[1]) then
			(
				if (vals[2][1][2][1] as BooleanClass) then
				(
					hideUntilRef.triState = 1 -- checked
				)
				else
				(
					hideUntilRef.triState = 0
				)
			)
			else
			(
				hideUntilRef.state = 2 -- undefined
			)
			
			vals = RAD_GatherObjectProps objs #("HideWhenDone") #("true")
			if (vals[1]) then
			(
				if (vals[2][1][2][1] as BooleanClass) then
				(
					hideWhenDone.triState = 1 -- checked
				)
				else
				(
					hideWhenDone.triState = 0
				)
			)
			else
			(
				hideWhenDone.state = 2 -- undefined
			)
		)
		else
		(
			enableAnimObjControls false
		)
	)
	
	function setupUIForObjects objs =
	(
		meshes = RAD_ConvertableFilter objs #(Mesh)
		
		if meshes.count > 0 then
		(
			enableMeshControls true
					
			vals = RAD_GatherObjectProps meshes #("Contents") #("solid")
			if (vals[1]) then -- all where identical
			(
				vals = vals[2][1][2][1] -- pull first one off
				if (vals == "solid") then
				(
					modelTypeRadio.state = 1 -- detail objects
				)
				else if (vals == "detail") then
				(
					modelTypeRadio.state = 2 -- solid
				)
				else
				(
					modelTypeRadio.state = 3
				)
			)
			else
			(
				modelTypeRadio.state = 0 -- undefined
			)
			
			vals = RAD_GatherObjectProps meshes #("Collision") #("true")
			if (vals[1]) then
			(
				vals = vals[2][1][2][1]
				if (vals as BooleanClass) then
				(
					noCollisionCheckBox.triState = 0
				)
				else
				(
					noCollisionCheckBox.triState = 1
				)
			)
			else
			(
				noCollisionCheckBox.triState = 2
			)
			
			vals = RAD_GatherObjectProps meshes #("Visible") #("true")
			if (vals[1]) then
			(
				vals = vals[2][1][2][1]
				if (vals as BooleanClass) then
				(
					nodrawCheckbox.triState = 0
				)
				else
				(
					nodrawCheckbox.triState = 1
				)
			)
			else
			(
				nodrawCheckbox.triState = 2
			)
		)
		
		setupUIForAnimatable objs
	)
			
	function onSelChange =
	(
		sel = RAD_Exclude (RAD_ConvertableFilter selection #(Mesh, Freecamera)) Bone
		if (sel.count > 0) then
		(
			setupUIForObjects sel
		)
		else
		(
			animSel = #()
			animSet = #()
			animList.items = #()
			animInitialState.items = #()
			animName.text = ""
			animStart.text = ""
			animEnd.text = ""
			animLoop.checked = false
			animExport.checked = false
			disableAll()
		)
	)
	
	function fileOpen =
	(
	)
	
	on RadianceUtil open do
	(
		RadianceUtil.disableAll()
		RadianceUtil.onSelChange()
		RAD_AddSelChangeHandler "RadianceUtil.onSelChange()" #RadianceUtil_SELCHANGE
		RAD_AddFileOpenHandler "RadianceUtil.fileOpen()" #RadianceUtil_FILEOPEN
	)
	on RadianceUtil close do
	(
		RAD_RemoveSelChangeHandler #RadianceUtil_SELCHANGE
		RAD_RemoveFileOpenHandler #RadianceUtil_FILEOPEN
	)
	on modelTypeRadio changed state do
	(
		val = #Solid
		if (state == 2) then
		(
			val = #Detail
		)
		else if (state == 3) then
		(
			val = #Areaportal
		)
		setSelectionType val
	)
	on noCollisionCheckBox changed state do
	(
		setSelectionCollision (not state)
	)
	on cinematicObject changed state do
	(
		if (state) then
		(
			RAD_SetObjectProps (selectedAnimatable()) #(#("CinematicObject", "true"))
		)
		else
		(
			RAD_SetObjectProps (selectedAnimatable()) #(#("CinematicObject", "false"))
		)
	)
	
	on hideUntilRef changed state do
	(
		if (state) then
		(
			RAD_SetObjectProps (selectedAnimatable()) #(#("HideUntilRef", "true"))
		)
		else
		(
			RAD_SetObjectProps (selectedAnimatable()) #(#("HideUntilRef", "false"))
		)
	)

	on hideWhenDone changed state do
	(
		if (state) then
		(
			RAD_SetObjectProps (selectedAnimatable()) #(#("HideWhenDone", "true"))
		)
		else
		(
			RAD_SetObjectProps (selectedAnimatable()) #(#("HideWhenDone", "false"))
		)
	)
	
	on animList selected idx do
	(
		animName.text = animSet[animList.selection].name
		animStart.text = animSet[animList.selection].start as string
		animEnd.text = animSet[animList.selection].end as string
		animExport.checked = animSet[animList.selection].export
		animLoop.checked = animSet[animList.selection].loop
	)
	on animName entered newVal do
	(
		renameSelAnim newVal
	)
	on exportSceneButton pressed do
	(
		exportScene true true
	)
	on exportAnimButton pressed do
	(
		exportScene false true
	)
	on exportMeshButton pressed do
	(
		exportScene true false
	)
	on exportSkelButton pressed do
	(
		exportScene false false
	)
	on selectOtherLikeThisButton pressed do
	(
		p = #("Contents", "Collision", "Visible")
		d = #("solid", "true", "true")
		m = (selectedMeshes())[1]
		v = RAD_GatherObjectProps #(m) p d
		v = v[2][1][2] -- pull out mesh properies
		-- filter scene
		selectMore (RAD_FilterByProps (RAD_Exclude (RAD_ConvertableFilter (RAD_FilterVisibleSelected objects  vis:true sel:false) Mesh) Bone) p d v)
	)
	on selectSolidButton pressed do
	(
		p = #("Contents")
		d = #("solid")
		-- filter scene
		selectMore (RAD_FilterByProps (RAD_Exclude (RAD_ConvertableFilter (RAD_FilterVisibleSelected objects  vis:true sel:false) Mesh) Bone) p d d)
	)
	on selectDetailButton pressed do
	(
		p = #("Contents")
		v = #("detail")
		d = #("solid")
		-- filter scene
		selectMore (RAD_FilterByProps (RAD_Exclude (RAD_ConvertableFilter (RAD_FilterVisibleSelected objects  vis:true sel:false) Mesh) Bone) p d v)
	)
	on selectNoCollisionButton pressed do
	(
		p = #("Collision")
		v = #("false")
		d = #("true")
		-- filter scene
		selectMore (RAD_FilterByProps (RAD_Exclude (RAD_ConvertableFilter (RAD_FilterVisibleSelected objects  vis:true sel:false) Mesh) Bone) p d v)
	)
	on selectAreaportalButton pressed do
	(
		p = #("Contents")
		v = #("areaportal")
		d = #("solid")
		-- filter scene
		selectMore (RAD_FilterByProps (RAD_Exclude (RAD_ConvertableFilter (RAD_FilterVisibleSelected objects  vis:true sel:false) Mesh) Bone) p d v)
	)
	on nodrawCheckbox changed state do
	(
		setSelectionVisible (not state)
	)
	on animStart entered newVal do
	(
		if (newVal == "") then
		(
			messagebox "Please enter a number for the animation start frame."
			return undefined
		)
		
		animSet[animList.selection].start = (newVal as integer)
		saveAnims()
	)
	on animEnd entered newVal do
	(
		if (newVal == "") then
		(
			messagebox "Please enter a number for the animation end frame."
			return undefined
		)
		
		animSet[animList.selection].end = (newVal as integer)
		saveAnims()
	)
	on animLoop changed state do
	(
		animSet[animList.selection].loop = state
		saveAnims()
	)
	on animInitialState selected idx do
	(
		saveAnims()
	)
	on animSetStart pressed do
		(
	)
	on animSetEnd pressed do
		(
	)
	on animTest pressed do
		(
	)
	on animExport changed state do
	(
		animSet[animList.selection].export = state
		saveAnims()
	)
	on addAnim pressed do
	(
		addNewAnim()
	)
	on deleteAnim pressed do
	(
		deleteSelAnim()
	)
	on noDefault pressed do
	(
		animInitialState.selection = -1
		animInitialState.items = (for x in animSet collect x.name)
				
		for x in animSel do
		(
			RAD_SetObjInitialAnim x undefined
		)
	)
)
