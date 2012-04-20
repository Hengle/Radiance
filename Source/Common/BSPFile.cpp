// BSPFile.cpp
// Copyright (c) 2010 Pyramind Labs LLC, All Rights Reserved
// Author: Joe Riedel (joeriedel@hotmail.com)
// See Radiance/LICENSE for licensing terms.

#include "BSPFile.h"
#include "../Runtime/Utils.h"

using namespace stream;


namespace bsp {

namespace
{
	bool ReadBounds(InputStream &is, Bounds &bounds)
	{
		for (int i = 0; i < 3; ++i)
		{
			if (!is.Read(&bounds.mins[i])) return false;
		}
		for (int i = 0; i < 3; ++i)
		{
			if (!is.Read(&bounds.maxs[i])) return false;
		}
		return true;
	}

	bool WriteBounds(OutputStream &os, Bounds &bounds)
	{
		for (int i = 0; i < 3; ++i)
		{
			if (!os.Write(bounds.mins[i])) return false;
		}
		for (int i = 0; i < 3; ++i)
		{
			if (!os.Write(bounds.maxs[i])) return false;
		}
		return true;
	}
}

bool ReadHeader(InputStream &is, Header &header)
{
	bool b = is.Read(&header.id) &&
		is.Read(&header.version) &&
		is.Read(&header.api);
	if (b)
	{
		for (int i = 0; i < NumApis; ++i)
		{
			b = b && is.Read(&header.ofs[i]);
		}
	}
	return b;
}

bool WriteHeader(OutputStream &os, const Header &header)
{
	bool b = os.Write(header.id) &&
		os.Write(header.version) &&
		os.Write(header.api);
	if (b)
	{
		for (int i = 0; i < NumApis; ++i)
		{
			b = b && os.Write(header.ofs[i]);
		}
	}
	return b;
}

bool Read(InputStream &is, const Header &header, Data &bsp)
{
	memset(&bsp, 0, sizeof(Data));

	if (is.Read(&bsp.numShaders)) return false;
	if (bsp.numShaders)
	{
		bsp.shaders = (Shader*)safe_malloc(sizeof(Shader) * bsp.numShaders);
		for (int i = 0; i < bsp.numShaders; ++i)
		{
			if (!is.Read(bsp.shaders[i].name, 32, 0)) return false;
		}
	}

	if (!is.Read(&bsp.numPlanes)) return false;
	if (bsp.numPlanes)
	{
		bsp.planes = (Plane*)safe_malloc(sizeof(Plane) * bsp.numPlanes);
		for (int i = 0; i < bsp.numPlanes; ++i)
		{
			if (!is.Read(&bsp.planes[i].v[0])) return false;
			if (!is.Read(&bsp.planes[i].v[1])) return false;
			if (!is.Read(&bsp.planes[i].v[2])) return false;
			if (!is.Read(&bsp.planes[i].v[3])) return false;
		}
	}

	if (!is.Read(&bsp.numNodes)) return false;
	
	if (bsp.numNodes)
	{
		bsp.nodes = (Node*)safe_malloc(sizeof(Node) * bsp.numNodes);
		for (int i = 0; i < bsp.numNodes; ++i)
		{
			if (!ReadBounds(is, bsp.nodes[i].bounds)) return false;
			if (!is.Read(&bsp.nodes[i].children[0])) return false;
			if (!is.Read(&bsp.nodes[i].children[1])) return false;
			if (!is.Read(&bsp.nodes[i].parent)) return false;
			if (!is.Read(&bsp.nodes[i].planenum)) return false;
		}
	}

	if (!is.Read(&bsp.numLeafs)) return false;
	if (bsp.numLeafs)
	{
		bsp.leafs = (Leaf*)safe_malloc(sizeof(Leaf) * bsp.numLeafs);
		for (int i = 0; i < bsp.numLeafs; ++i)
		{
			if (!ReadBounds(is, bsp.leafs[i].bounds)) return false;
			if (!is.Read(&bsp.leafs[i].contents)) return false;
			if (!is.Read(&bsp.leafs[i].area)) return false;
		}
	}

	if (!is.Read(&bsp.numAreas)) return false;
	if (bsp.numAreas)
	{
		bsp.areas = (Area*)safe_malloc(sizeof(Area) * bsp.numAreas);
		for (int i = 0; i < bsp.numAreas; ++i)
		{
			if (!ReadBounds(is, bsp.areas[i].bounds)) return false;
			if (!is.Read(&bsp.areas[i].firstSector)) return false;
			if (!is.Read(&bsp.areas[i].numSectors)) return false;
			if (!is.Read(&bsp.areas[i].firstPortal)) return false;
			if (!is.Read(&bsp.areas[i].numPortals)) return false;
		}
	}

	if (!is.Read(&bsp.numSectors)) return false;
	if (bsp.numSectors)
	{
		bsp.sectors = (Sector*)safe_malloc(sizeof(Sector) * bsp.numSectors);
		for (int i = 0; i < bsp.numSectors; ++i)
		{
			if (!ReadBounds(is, bsp.sectors[i].bounds)) return false;
			if (!is.Read(&bsp.sectors[i].firstTriModel)) return false;
			if (!is.Read(&bsp.sectors[i].numTriModels)) return false;
		}
	}

	if (!is.Read(&bsp.numSModels)) return false;
	if (bsp.numSModels)
	{
		bsp.sModels = (STriModel*)safe_malloc(sizeof(STriModel) * bsp.numSModels);
		for (int i = 0; i < bsp.numSModels; ++i)
		{
			if (!is.Read(&bsp.sModels[i].shader)) return false;
			if (!is.Read(&bsp.sModels[i].firstTri)) return false;
			if (!is.Read(&bsp.sModels[i].numTris)) return false;
		}
	}

	if (!is.Read(&bsp.numPortals)) return false;
	if (bsp.numPortals)
	{
		bsp.portals = (Portal*)safe_malloc(sizeof(Portal) * bsp.numPortals);
		for (int i = 0; i < bsp.numPortals; ++i)
		{
			if (!is.Read(&bsp.portals[i].firstVert)) return false;
			if (!is.Read(&bsp.portals[i].numVerts)) return false;
		}
	}

	if (!is.Read(&bsp.numAreaPortals)) return false;
	if (bsp.numAreaPortals)
	{
		bsp.areaPortals = (AreaPortal*)safe_malloc(sizeof(AreaPortal) * bsp.numAreaPortals);
		for (int i = 0; i < bsp.numAreaPortals; ++i)
		{
			if (!is.Read(&bsp.areaPortals[i].ent)) return false;
			if (!is.Read(&bsp.areaPortals[i].areas[0])) return false;
			if (!is.Read(&bsp.areaPortals[i].areas[1])) return false;
			if (!is.Read(&bsp.areaPortals[i].firstPortal)) return false;
			if (!is.Read(&bsp.areaPortals[i].numPortals)) return false;
			if (!is.Read(&bsp.areaPortals[i].firstVert)) return false;
			if (!is.Read(&bsp.areaPortals[i].numVerts)) return false;
		}
	}

	if (!is.Read(&bsp.numVerts)) return false;
	if (bsp.numVerts)
	{
		bsp.verts = (FloatVal*)safe_malloc(sizeof(FloatVal) * bsp.numVerts);
		for (int i = 0; i < bsp.numVerts; ++i)
		{
			if (!is.Read(&bsp.verts[i*3+0])) return false;
			if (!is.Read(&bsp.verts[i*3+1])) return false;
			if (!is.Read(&bsp.verts[i*3+2])) return false;
		}
	}

	return true;
}

bool Write(OutputStream &os, const Data &bsp)
{
	if (os.Write(bsp.numShaders)) return false;
	if (bsp.numShaders)
	{
		for (int i = 0; i < bsp.numShaders; ++i)
		{
			if (!os.Write(bsp.shaders[i].name, 32, 0)) return false;
		}
	}

	if (!os.Write(bsp.numPlanes)) return false;
	if (bsp.numPlanes)
	{
		for (int i = 0; i < bsp.numPlanes; ++i)
		{
			if (!os.Write(bsp.planes[i].v[0])) return false;
			if (!os.Write(bsp.planes[i].v[1])) return false;
			if (!os.Write(bsp.planes[i].v[2])) return false;
			if (!os.Write(bsp.planes[i].v[3])) return false;
		}
	}

	if (!os.Write(bsp.numNodes)) return false;
	if (bsp.numNodes)
	{
		for (int i = 0; i < bsp.numNodes; ++i)
		{
			if (!WriteBounds(os, bsp.nodes[i].bounds)) return false;
			if (!os.Write(bsp.nodes[i].children[0])) return false;
			if (!os.Write(bsp.nodes[i].children[1])) return false;
			if (!os.Write(bsp.nodes[i].parent)) return false;
			if (!os.Write(bsp.nodes[i].planenum)) return false;
		}
	}

	if (!os.Write(bsp.numLeafs)) return false;
	if (bsp.numLeafs)
	{
		for (int i = 0; i < bsp.numLeafs; ++i)
		{
			if (!WriteBounds(os, bsp.leafs[i].bounds)) return false;
			if (!os.Write(bsp.leafs[i].contents)) return false;
			if (!os.Write(bsp.leafs[i].area)) return false;
		}
	}

	if (!os.Write(bsp.numAreas)) return false;
	if (bsp.numAreas)
	{
		for (int i = 0; i < bsp.numAreas; ++i)
		{
			if (!WriteBounds(os, bsp.areas[i].bounds)) return false;
			if (!os.Write(bsp.areas[i].firstSector)) return false;
			if (!os.Write(bsp.areas[i].numSectors)) return false;
			if (!os.Write(bsp.areas[i].firstPortal)) return false;
			if (!os.Write(bsp.areas[i].numPortals)) return false;
		}
	}

	if (!os.Write(bsp.numSectors)) return false;
	if (bsp.numSectors)
	{
		for (int i = 0; i < bsp.numSectors; ++i)
		{
			if (!WriteBounds(os, bsp.sectors[i].bounds)) return false;
			if (!os.Write(bsp.sectors[i].firstTriModel)) return false;
			if (!os.Write(bsp.sectors[i].numTriModels)) return false;
		}
	}

	if (!os.Write(bsp.numSModels)) return false;
	if (bsp.numSModels)
	{
		for (int i = 0; i < bsp.numSModels; ++i)
		{
			if (!os.Write(bsp.sModels[i].shader)) return false;
			if (!os.Write(bsp.sModels[i].firstTri)) return false;
			if (!os.Write(bsp.sModels[i].numTris)) return false;
		}
	}

	if (!os.Write(bsp.numPortals)) return false;
	if (bsp.numPortals)
	{
		for (int i = 0; i < bsp.numPortals; ++i)
		{
			if (!os.Write(bsp.portals[i].firstVert)) return false;
			if (!os.Write(bsp.portals[i].numVerts)) return false;
		}
	}

	if (!os.Write(bsp.numAreaPortals)) return false;
	if (bsp.numAreaPortals)
	{
		for (int i = 0; i < bsp.numAreaPortals; ++i)
		{
			if (!os.Write(bsp.areaPortals[i].ent)) return false;
			if (!os.Write(bsp.areaPortals[i].areas[0])) return false;
			if (!os.Write(bsp.areaPortals[i].areas[1])) return false;
			if (!os.Write(bsp.areaPortals[i].firstPortal)) return false;
			if (!os.Write(bsp.areaPortals[i].numPortals)) return false;
			if (!os.Write(bsp.areaPortals[i].firstVert)) return false;
			if (!os.Write(bsp.areaPortals[i].numVerts)) return false;
		}
	}

	if (!os.Write(bsp.numVerts)) return false;
	if (bsp.numVerts)
	{
		for (int i = 0; i < bsp.numVerts; ++i)
		{
			if (!os.Write(bsp.verts[i*3+0])) return false;
			if (!os.Write(bsp.verts[i*3+1])) return false;
			if (!os.Write(bsp.verts[i*3+2])) return false;
		}
	}

	return true;
}

void Free(Data &bsp)
{
	if (bsp.shaders) free(bsp.shaders);
	if (bsp.planes) free(bsp.planes);
	if (bsp.nodes) free(bsp.nodes);
	if (bsp.leafs) free(bsp.leafs);
	if (bsp.areas) free(bsp.areas);
	if (bsp.sectors) free(bsp.sectors);
	if (bsp.sModels) free(bsp.sModels);
	if (bsp.portals) free(bsp.portals);
	if (bsp.areaPortals) free(bsp.areaPortals);
	if (bsp.verts) free(bsp.verts);
	memset(&bsp, 0, sizeof(Data));
}

} // bsp
