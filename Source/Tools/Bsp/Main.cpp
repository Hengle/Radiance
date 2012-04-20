// Main.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Bsp.h"
#include "../Common/GLNavWindow.h"
#include "../Common/Files.h"
#include "../../Common/Tokenizer.h"
#include <Runtime/File/FileStream.h>

bool g_glDebug=false;
bool g_fast=false;
bool g_le=true;
int  g_plats=0;
char g_mapname[1024];

void LoadEntity(const char *filename, Tokenizer &script, Map &map)
{
}

void LoadScene(const char *filename, Tokenizer &script, Map &map)
{
	if (!script.IsNextToken("{"))
	{
		Error("Expected '{' on line %d of %s\n", script.GetLine(), filename);
	}

	std::string t;
	while (script.GetToken(t))
	{
		if (t == "}") break;
		
		{
			t += ".rscn";
			FILE *fp = fopen(t.c_str(), "rb");
			if (!fp)
			{
				Error("Failed to open %s\n", t.c_str());
			}
			file::stream::InputBuffer ib(fp);
			stream::InputStream is(ib);
			try
			{
				Log(LogNormal, "Loading %s...\n", t.c_str());
				LoadMaxScene(is, map);
				fclose(fp);
			}
			catch (std::exception &e)
			{
				fclose(fp);
				Error("Error loading rscn: %s\n", e.what());
			}
		}
	}
}

void LoadMapFile(const char *filename, Map &map)
{
	char path[1024];
	strcpy(path, filename);
	strcat(path, ".map");

	int len;
	char *data = (char *)SafeLoadFile(path, len);

	Tokenizer script;
	script.InitParsing(data, (int)len);
	FreeFileData(data);

	std::string t;
	while (script.GetToken(t))
	{
		if (t == "scenes") 
		{
			LoadScene(path, script, map);
		}
		else
		{
			Error("Unrecognized token '%s', line %d, file %s\n", t.c_str(), script.GetLine(), path);
		}
	}
}

void Usage()
{
	std::cout << "Usage: bsp filename [-gldebug][-fast][-el|-eb][-gl|-dx|-xbox360|-ps3]" << std::endl;
}

Map::Vec3 RandomColor()
{
	static Map::Vec3Vec s_colors;
	if (s_colors.empty())
	{
		for (int i = 0; i < 256; ++i)
		{
			s_colors.push_back(
				Map::Vec3(
					Map::Vec3::ValueType((((rand() & 0xf) + 1) & 0xf) / 16.0),
					Map::Vec3::ValueType((((rand() & 0xf) + 1) & 0xf) / 16.0),
					Map::Vec3::ValueType((((rand() & 0xf) + 1) & 0xf) / 16.0)
				)
			);
		}
	}
	return s_colors[rand() & 0xff];
}

void RandomizeVertColors(Map &m)
{
	for (Map::EntityVec::iterator e = m.ents.begin(); e != m.ents.end(); ++e)
	{
		for (Map::TriModelVec::iterator m = (*e)->models.begin(); m != (*e)->models.end(); ++m)
		{
			for (Map::TriVertVec::iterator v = (*m)->verts.begin(); v != (*m)->verts.end(); ++v)
			{
				(*v).color = RandomColor();
			}
		}
	}
}

class PaintHandler : public GLNavWindow::PaintHandler
{
public:
	PaintHandler (const Map &m) : m_map(m) {}
	virtual void OnPaint(GLNavWindow &w)
	{
		GLState &s = w.BeginFrame();
		w.Camera().Bind(s);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		s.SetState(DT_Less|CFM_Back|CFM_CCW|CWM_All|NoArrays, BM_Off);
		s.DisableAllTMUs();
		s.Commit();

		glEnable(GL_LIGHTING);
		glEnable(GL_COLOR_MATERIAL);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glShadeModel( GL_SMOOTH );

		float dir0[4]  = { 0.4f, 0.7f, 1.0f, 0.0f };
		float amb0[4]  = { 0.2f, 0.2f, 0.2f, 1.0f };
		float diff0[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		
		glLightfv( GL_LIGHT0, GL_POSITION, dir0 );
		glLightfv( GL_LIGHT0, GL_DIFFUSE, diff0 );
		glEnable(GL_LIGHT0);

		float dir1[4]  = { -0.4f, -0.7f, -1.0f, 0.0f };
		float diff1[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		
		glLightfv( GL_LIGHT1, GL_POSITION, dir1 );
		glLightfv( GL_LIGHT1, GL_DIFFUSE, diff1 );
		glEnable(GL_LIGHT1);

		glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE );

		float c[4] = {1, 1, 1, 1};
		glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, c );
		glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, c );

		glBegin(GL_TRIANGLES);

		for (Map::TriModelVec::const_iterator m = m_map.worldspawn->models.begin(); m != m_map.worldspawn->models.end(); ++m)
		{
			for (Map::TriFaceVec::const_iterator t = (*m)->tris.begin(); t != (*m)->tris.end(); ++t)
			{
				const Map::TriFace &f = *t;
				for (int i = 0; i < 3; ++i)
				{
					const Map::TriVert &v = (*m)->verts[f.v[i]];
					glColor3d(v.color[0], v.color[1], v.color[2]);
					glNormal3d(v.normal[0], v.normal[1], v.normal[2]);
					glVertex3d(v.pos[0], v.pos[1], v.pos[2]);
				}
			}
		}

		glEnd();

		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
		glDisable(GL_LIGHT1);
		glDisable(GL_COLOR_MATERIAL);
#if 1
		s.SetState(DT_Disable, 0);
		s.Commit();

		glBegin(GL_LINES);
		glColor3f(1, 1, 1);

		for (Map::TriModelVec::const_iterator m = m_map.worldspawn->models.begin(); m != m_map.worldspawn->models.end(); ++m)
		{
			for (Map::TriFaceVec::const_iterator t = (*m)->tris.begin(); t != (*m)->tris.end(); ++t)
			{
				const Map::TriFace &f = *t;
				for (int i = 0; i < 3; ++i)
				{
					const Map::TriVert &v = (*m)->verts[f.v[i]];
					glVertex3d(v.pos[0], v.pos[1], v.pos[2]);
					Map::Vec3 z = v.pos + (v.normal * 10.0);
					glVertex3d(z[0], z[1], z[2]);
				}
			}
		}

		glEnd();
#endif

		w.EndFrame();
	}

private:

	const Map m_map;
};

void DisplayMap(const Map &m)
{
	GLNavWindow win;
	GLCamera &c = win.Camera();
	Map::EntityRef e = m.EntForName("sp_player_start");
	if (e)
	{
		c.SetPos(
			GLVec3(
				GLVec3::ValueType(e->origin.X()), 
				GLVec3::ValueType(e->origin.Y()), 
				GLVec3::ValueType(e->origin.Z())
			)
		);
	}
	else
	{
		c.SetPos(GLVec3::Zero);
	}
	PaintHandler p(m);
	win.SetPaintHandler(&p);
	win.Open(L"BSP", -1, -1, 1280, 1024, true);
	win.WaitForClose();
}

int main(int argc, const char **argv)
{
	std::cout << "Radiance BSP Compiler 1.0: " << __DATE__ << " " << __TIME__ << std::endl;
	std::cout << "Copyright (c) 2010 Pyramind Labs, LLC" << std::endl;
	if (argc < 2)
	{
		Usage();
		return 1;
	}

	strcpy(g_mapname, argv[1]);

	for (int i = 2; i < argc; ++i)
	{
		if (!stricmp(argv[i], "-gldebug"))
		{
			g_glDebug = true;
		}
		else if(!stricmp(argv[i], "-fast"))
		{
			g_fast = true;
		}
		else if (!stricmp(argv[i], "-el"))
		{
			g_le = true;
		}
		else if (!stricmp(argv[i], "-eb"))
		{
			g_le = false;
		}
		else if (!stricmp(argv[i], "-gl"))
		{
			g_plats |= bsp::ApiGL;
		}
		else if(!stricmp(argv[i], "-dx"))
		{
			g_plats |= bsp::ApiDX;
		}
		else if (!stricmp(argv[i], "-xbox360"))
		{
			g_plats |= bsp::ApiXBox360;
		}
		else if (!stricmp(argv[i], "-ps3"))
		{
			g_plats |= bsp::ApiPS3;
		}
		else
		{
			std::cout << "ERROR: urecognized switch '" << argv[i] << "'." << std::endl;
			Usage();
			return 1;
		}
	}

	if (!g_plats)
	{
		std::cout << "ERROR: must specify target platform!" << std::endl;
		Usage();
		return 1;
	}

	std::cout << "Config: ";
	if (g_glDebug) std::cout << "gldebug;";
	if (g_fast) std::cout << "fast;";
	if (g_le) std::cout << "little_endian;";
	if (!g_le) std::cout << "big_endian;";
	if (g_plats&bsp::ApiGL) std::cout << "gl;";
	if (g_plats&bsp::ApiDX) std::cout << "dx;";
	if (g_plats&bsp::ApiXBox360) std::cout << "xbox360;";
	if (g_plats&bsp::ApiPS3) std::cout << "ps3;";
	std::cout << std::endl;

	Log(LogNormal, "Entering %s.map\n", argv[1]);
	Map map;
	LoadMapFile(argv[1], map);
	if (!map.worldspawn)
	{
		Error ("Missing worldspawn.\n");
	}

	if (g_glDebug)
	{
		RandomizeVertColors(map);
		DisplayMap(map);
	}

	BSP tree(map);
	tree.Build();
	return 0;
}