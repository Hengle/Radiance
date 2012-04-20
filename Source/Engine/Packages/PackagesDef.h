/*! \file PackagesDef.h
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup packages
*/

#pragma once

#include "../Types.h"
#include "../Assets/AssetTypes.h"
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

class Engine;

namespace pkg {

//! Packages memory zone.
RAD_ZONE_DEC(RADENG_API, ZPackages);

//! Defines an asset instancing zone id.
/*!
	When an asset is instantiated from a package a zone is specified which controls the 
	instancing semantics of the asset. An asset instance is unique to the zone id it was 
	instantiated in.

	This means that creating an asset instance multiple times with the same zone id will return
	a reference to the same instance of the asset, while creating instances in multiple different 
	zone id's would return multiple unique instances of the asset.

	An asset instance is a seperate self contained copy of the asset meaning it can be
	processed and operated upon without effecting the state of other instances, however
	meta-data is not part of an asset instance and is always shared.

	Typically zones are declared and used for seperate systems. Instances of assets created
	by those systems typically create them inside their own zone so as not to interfere with
	the state of other systems. For example, the Content Browser creates and processes asset
	instances in the pkg::Z_ContentBrowser zone, and instances of the game running inside the 
	engine use pkg::Z_Engine. Therefore resizing and previewing done inside the Content Browser 
	does not effect the assets loaded by the engine.
*/
enum Zone
{
	/*! \internal */
	Z_First,
	Z_Engine=Z_First, /*!< Engine Zone */
#if defined(RAD_OPT_PC_TOOLS)
	Z_ContentBrowser, /*!< Content Browser Zone */
	Z_Cooker, /*!< Cooker Zone */
#endif
	Z_Max, /*!< Max Zone, Not a valid zone id. */
	Z_All /*!< Special zone id meaning all zones, for functions that can operate on all assets. */
#if defined(RAD_OPT_PC_TOOLS)
	, Z_Unique /*!< Unique zone, any asset instance created with this zone id will always be unique and will never be shared. */
#endif
};

//! Common asset sink results. \sa file::Result
/*! Sink results are integers and can therfore take on any integer value.
	
	Sink result codes that are less than (<) pkg::SR_Success are error codes. The first 11 negative error
	codes -1 to -11 are file system error codes.

	Sink result codes that are greater than (>) pkg::SR_Pending are custom user return codes and should not
	be exposed to general engine systems as result codes as they will not be handled.
*/
enum SinkResult
{
	SR_Success = 0, /*!< Sink command completed successfully. */
	SR_Pending, /*!< Sink command is pending, pkg::Asset::Process() must be called until the command does not return pkg::SR_Pending. */
	SR_User, /*!< First valid user result code, for internal use by Sinks. */
	// -1 to -11 are file system error codes. See <Runtime/File/FileDef.h>
	SR_ErrorGeneric = -128, /*!< A generic (unidentified) error occurred. */
	SR_ParseError = -129, /*!< Invalid or erroneous input data. Common with scripts or structured text files. */
	SR_BadVersion = -130, /*!< Invalid version of a file. */
	SR_MetaError = -131, /*!< Invalid asset meta data. */
	SR_MissingFile = -132, /*!< File was not found or could not be opened. */
	SR_InvalidFormat = -133, /*!< Unsupported file format. */
	SR_CorruptFile = -134, /*!< Corrupt file data or unrecognized file type. */
	SR_IOError = -135, /*!< Generic error related to file system or IO. */
	SR_CompilerError = -136, /*!< Error occurred during compilation. */
	SR_ScriptError = -137 /*!< Syntax or logic error during script processing. */
};

//! Defines the relative order of stages of sink processing.
/*! Multiple sinks can be attached to an asset type. The order in which they run is
	defined by their stage. Stages can be any integer value, stages with a larger
	value run after stages with a smaller value.
*/
enum SinkStage
{
	SS_Parser  = 100, /*!< Default parsing stage. For stages that parse asset data into an intermediate representation. */
	SS_Load, /*!< Default loading stage. For stages that consume pkg::SS_Parser output and load the data. */
	SS_Process = 256 /*!< Default processing stage. For stages that consume pkg::SS_Load output and process it further. */
};

#if defined(RAD_OPT_TOOLS)
//! Cooked state of an asset.
/*! The package cooker will query each asset for its current state to determine
	if it needs to be rebuilt, which is communicated through one of these
	return codes.
*/
enum CookStatus
{
	CS_UpToDate, /*!< Cooked state of asset is up to date. */
	CS_Ignore, /*!< Asset should be ignored and not be cooked or packaged. */
	CS_NeedRebuild /*!< Cooked state of asset is out of date and needs to be rebuilt. */
};
#endif

//! Process flags.
/*! Process flags define and control the processing commands executing on an asset
	through the pkg::Asset::Process() method. They contain bit flags defining the operation
	to perform and the target platform the action should be done for.

	Typically the process flags are created using the #P_TARGET_FLAGS(flags) macro, which will set
	the target platform bits based on what the application was built for (for example
	if the application was built for iOS #P_TARGET_FLAGS(flags) would set the iOS target bits).

	In specialized cases, like cooking for example, the target platform may be specified
	directly.
*/
enum PFlags
{
	// Process flags

	RAD_FLAG(P_SAlloc),       /*!< Allocates Sink States. Not required as the system will allocate sink states as necessary. */
	RAD_FLAG(P_Info),         /*!< Parse asset for header information. Typically fulfilled by the pkg::SS_Parser sink. */
	RAD_FLAG(P_Parse),        /*!< Fully parse asset data. Typically fulfilled by the pkg::SS_Parser sink. */
	RAD_FLAG(P_Load),         /*!< Fully load the the asset data. Typically fulfilled by an pkg::SS_Parser, pkg::SS_Load, and the pkg::SS_Process sinks. */
	RAD_FLAG(P_Unformatted),  /*!< Do not reformat the data upon load. For example a texture asset which 
                                   specifies DXT compression but is loaded from a TGA file would be compressed
                                   by the pkg::SS_Parser sink unless this flag is specified. 
                              */
	RAD_FLAG(P_Unload),       /*!< Unload the asset and free all data. This process flag is specified automatically
                                   by the system when all references to an asset instance are released. It is therefore
                                   not necessary to do manually.
                              */
	RAD_FLAG(P_Trim),         /*!< Request that all asset sinks release any intermediate data. This should always be performed
                                   by any caller doing a pkg::P_Load command after the load operation completes. Typically this
                                   request will cause the pkg::SS_Parser sink to free intermediate data and therefore that stage
                                   would no longer have data available to consume. For example after executing a pkg::P_Load
                                   on a texture asset the pkg::SS_Parser stage would contain a loaded copy of the texture data
                                   and the pkg::SS_Load stage would have uploaded the pkg::SS_Parser data into the graphics backend.
                                   In most circumstances the data in the pkg::SS_Parser stage is now redundant and can be free'd,
                                   which is what the pkg::P_Trim command is used for.
                              */
	RAD_FLAG(P_VidReset),     /*!< A video reset has occurred and sinks should release any video related state. */
	RAD_FLAG(P_VidBind),      /*!< A video rebind has occurred and sinks should recreate any video related state. */
	RAD_FLAG(P_Cancel),       /*!< Cancel an outstanding command. */
	RAD_FLAG(P_FastPath),      /*!< Load data from cached data (on-the-fly cooking). (Does nothing in golden builds). */
	RAD_FLAG(P_TargetPC),     /*!< PC target. */
	RAD_FLAG(P_TargetIPhone), /*!< IPhone target. */
	RAD_FLAG(P_TargetIPad),   /*!< IPad target. */
	RAD_FLAG(P_TargetXBox360),/*!< XBox260 target. */
	RAD_FLAG(P_TargetPS3),    /*!< PS3 target. */

#if defined(RAD_OPT_TOOLS)

	RAD_FLAG(P_TargetDefault), /*!< When specified #P_TARGET_FLAGS(flags) will no longer specify a target if one is not provided.
                                    This will force sinks and cookers to load generic meta-data for an asset.
                               */
	RAD_FLAG(P_Clean),         /*!< Used during cooking to request that any output files be cleaned. */
	RAD_FLAG(P_ScriptsOnly),   /*!< Used during cooking to request that only scripts be cooked. */
	RAD_FLAG(P_NoDefaultMedia),/*!< Requests that sinks do not resolve pkg::SR_MissingFile by substituting a place-holder,
                                    for example do not load a missing texture image.
                               */
	RAD_FLAG(P_NoDefaultKey),  /*!< When requesting a key value from pkg::Package::Entry::KeyValue() or any other method
                                    do not use the default key value from the asset definition if the key cannot be found.
                               */
	RAD_FLAG(P_Exact),         /*!< Key path should be matched exactly. \sa pkg::Package::Entry::RemoveKey() */
	RAD_FLAG(P_Prefix),        /*!< Key path should be matched as a prefix. \sa pkg::Package::Entry::RemoveKey() */

#endif

	P_TargetIOS = P_TargetIPhone|P_TargetIPad, /*!< All iOS target bits. \sa pkg::P_TargetIPhone and pkg::P_TargetIPad */
	P_TargetConsole = P_TargetXBox360|P_TargetPS3, /*!< All Console target bits. \sa pkg::P_TargetXBox360 and pkg::P_TargetPS3 */
	P_AllTargets = P_TargetConsole|P_TargetPC|P_TargetIPhone|P_TargetIPad, /*!< All target bits set. \sa pkg::P_TargetPC, pkg::P_TargetIPhone, pkg::P_TargetIPad, pkg::P_TargetXBox360, and pkg::P_TargetPS3 */
	P_FirstTarget = P_TargetPC, /*!< First target bit. */
	P_LastTarget  = P_TargetPS3, /*!< Last target bit. */
	P_NumTargets  = 5 /*!< Number of targets. */

};

#if defined(RAD_OPT_TOOLS)

//! Key Styles
/*! Defines valid keyvalue styles. */
enum KFlags
{
	K_TypeMask = 0xf, /*!< A mask for keyvalue types. */
	K_Variant = 0, /*!< The key value is a variant without any special  */
	K_Import, /*!< The key value is a string which references another asset. */
	K_File, /*!< The key value is a path to a file and should use a file dialog in the editor. */
	K_CheckBoxes, /*!< The key value is a set of bitflags that should be represented as checkboxes in a list. */
	K_List, /*!< The key value is a value which should be selected from a list of predefined values. */
	K_Unsigned, /*!< The key value should be treated as unsigned. */
	K_MultiLine, /*!< The key should be edited with a multilined edit box. */
	K_Color, /*!< The key value should use the color picker. */

	RAD_FLAG_BIT(K_EditorOnly, 8), /*!< Key is only for use in the editor, it does not need to be cooked. */
	RAD_FLAG(K_Hidden), /*!< Key should not be displayed in the editor. */
	RAD_FLAG(K_ReadOnly), /*!< Key value is read only. */
	RAD_FLAG(K_Global) /*!< Key value is global to all platforms and should not be instanced per platform. */
};

#endif

//! Target platform flag.
/*! \def P_TARGET_PLATFORM.
	Defined to be the target platform during compile.
	\sa pkg::P_TargetPC, pkg::P_TargetIPhone, pkg::P_TargetIPad, pkg::P_TargetXBox360, and pkg::P_TargetPS3
*/
#if defined(RAD_OPT_PC)
	#define P_TARGET_PLATFORM (pkg::P_TargetPC)
#elif defined(RAD_OPT_IOS)
#if defined(RAD_OPT_IPHONE)
	#define P_TARGET_PLATFORM (pkg::P_TargetIPhone)
#else
	#define P_TARGET_PLATFORM (pkg::P_TargetIPad)
#endif
#else
#error RAD_ERROR_UNSUP_PLAT
#endif

//! Target platform conditional.
/*! \def P_TARGET_FLAGS(_flags)
	This macro should be used to ensure that a target platform is defined when requesting a key value from
	pkg::Package::Entry::KeyValue() or other key-value functions with take PFlags.

	If P_TargetDefault is specified then #P_TARGET_FLAGS(flags) will allow no target bit to be set to enable
	generic key values to be accessed.
*/
#if defined(RAD_OPT_TOOLS)
#define P_TARGET_FLAGS(_flags) (((_flags)&(pkg::P_AllTargets|pkg::P_TargetDefault)) ? (_flags&pkg::P_AllTargets) : P_TARGET_PLATFORM)
#else
#define P_TARGET_FLAGS(_flags) (((_flags)&(pkg::P_AllTargets)) ? (_flags&pkg::P_AllTargets) : P_TARGET_PLATFORM)
#endif

//! Seperator character between package and asset name.
/*! \def RAD_PACKAGE_SEP_STR */
#define RAD_PACKAGE_SEP_STR "/"
//! Seperator character between package and asset name.
/*! \def RAD_PACKAGE_SEP_STR */
#define RAD_PACKAGE_SEP_CHAR '/'

class Package;
class PackageMan;
class Asset;
class Binding;
class SinkBase;

#if defined(RAD_OPT_PC_TOOLS)
class Cooker;
class BinFile;
class CookerFactory;
typedef boost::shared_ptr<Cooker> CookerRef;
typedef boost::shared_ptr<CookerFactory> CookerFactoryRef;
typedef boost::shared_ptr<BinFile> CookerFileRef;
#endif

typedef boost::shared_ptr<PackageMan> PackageManRef;
typedef boost::weak_ptr<PackageMan> PackageManWRef;
typedef boost::shared_ptr<Package> PackageRef;
typedef boost::weak_ptr<Package> PackageWRef;
typedef zone_map<String, PackageRef, ZPackagesT>::type PackageMap;
typedef zone_map<int, PackageWRef, ZPackagesT>::type IdPackageWMap;
typedef zone_vector<PackageRef, ZPackagesT>::type PackageVec;
typedef zone_vector<int, ZPackagesT>::type IdVec;
typedef boost::shared_ptr<Asset> AssetRef;
typedef boost::weak_ptr<Asset> AssetWRef;
typedef zone_map<String, AssetRef, ZPackagesT>::type AssetMap;
typedef zone_map<String, AssetWRef, ZPackagesT>::type AssetWMap;
typedef zone_map<int, AssetRef, ZPackagesT>::type AssetIdMap;
typedef zone_map<int, AssetWRef, ZPackagesT>::type AssetIdWMap;
typedef zone_vector<AssetRef, ZPackagesT>::type AssetVec;
typedef zone_map<String, int, ZPackagesT>::type StringIdMap;
typedef zone_set<String, ZPackagesT>::type StringSet;
typedef boost::shared_ptr<SinkBase> SinkBaseRef;

#if defined(RAD_OPT_TOOLS)
struct KeyVal;
struct KeyDef;
typedef boost::shared_ptr<KeyDef> KeyDefRef;
#endif

} // pkg

#include <Runtime/PopPack.h>
