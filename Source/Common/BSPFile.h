// BSPFile.h
// Copyright (c) 2010 Pyramind Labs LLC, All Rights Reserved
// Author: Joe Riedel (joeriedel@hotmail.com)
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Runtime/Stream.h"
#include "../Runtime/Math/Math.h"
#include "../Runtime/Math/Vector.h"
#include "../Runtime/Math/Plane.h"
#include "../Runtime/Math/Quaternion.h"
#include "../Runtime/Math/Matrix.h"
#include "../Runtime/Math/ConvexPolygon.h"
#include "../Runtime/Math/AABB.h"


namespace bsp {

enum
{
	UVShader,
	UVLightmap,
	NumUVChannels,

	RAD_FLAG_BIT(ContentsAreaportal, 0),
	RAD_FLAG(ContentsSolid),
	RAD_FLAG(ContentsNoClip),

	Id = RAD_FOURCC('R', 'B', 'S', 'P'),
	Version = 1,
	RAD_FLAG_BIT(ApiGL, 0),
	RAD_FLAG(ApiDX),
	RAD_FLAG(ApiXBox360),
	RAD_FLAG(ApiPS3),
	NumApis = 4
};

struct Header
{
	int id;
	int version;
	int api;
	int ofs[NumApis];
};

typedef float FloatVal;

struct Plane
{
	FloatVal v[4];
};

struct Bounds
{
	FloatVal mins[3];
	FloatVal maxs[3];
};

struct Node
{
	Bounds bounds;
	int children[2];
	int parent;
	unsigned int planenum;
};

struct Leaf
{
	Bounds bounds;
	unsigned int contents;
	int area;
};

struct Shader
{
	char name[32];
};

struct STriModel
{
	int shader;
	unsigned int firstTri;
	unsigned int numTris;
};

struct Sector
{
	Bounds bounds;
	unsigned int firstTriModel;
	unsigned int numTriModels;
};

struct Portal
{
	unsigned int firstVert;
	unsigned int numVerts;
};

struct Area
{
	Bounds bounds;
	unsigned int firstSector;
	unsigned int numSectors;
	unsigned int firstPortal;
	unsigned int numPortals;
};

struct AreaPortal
{
	unsigned int ent;
	unsigned int areas[2];
	unsigned int firstPortal;
	unsigned int numPortals;
	unsigned int firstVert;
	unsigned int numVerts;
};

struct Data
{
	Shader *shaders;
	Plane *planes;
	Node *nodes;
	Leaf *leafs;
	Area *areas;
	Sector *sectors;
	STriModel *sModels;
	Portal *portals;
	AreaPortal *areaPortals;
	FloatVal *verts;

	int numShaders;
	int numPlanes;
	int numNodes;
	int numLeafs;
	int numAreas;
	int numSectors;
	int numSModels;
	int numPortals;
	int numAreaPortals;
	int numVerts;
};

bool ReadHeader(stream::InputStream &is, Header &header);
bool WriteHeader(stream::OutputStream &os, const Header &header);
bool Read(stream::InputStream &is, const Header &header, Data &bsp);
bool Write(stream::OutputStream &os, const Data &bsp);
void Free(Data &bsp);

} // bsp
