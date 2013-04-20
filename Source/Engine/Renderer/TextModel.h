/*! \file TextModel.h
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup renderer
*/

#pragma once

#include "RendererDef.h"
#include "Material.h"
#include "../Packages/PackagesDef.h"
#include <Runtime/Font/Font.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

namespace r {

class RADENG_CLASS TextModel {
public:

	typedef boost::shared_ptr<TextModel> Ref;
	
	// implemented by RB
	static Ref New();

	class RADENG_CLASS String {
	public:

		String() {}
		String(
			const char *utf8String, 
			float x = 0.0f, 
			float y = 0.0f, 
			float z = 0.0f, 
			bool kern = true, 
			float kernScale = 1.0f, 
			float scaleX = 1.0f, 
			float scaleY = 1.0f
		) {
			this->utf8String = utf8String;
			this->x         = x;
			this->y         = y;
			this->z         = z;
			this->kern      = kern;
			this->kernScale = kernScale;
			this->scaleX    = scaleX;
			this->scaleY    = scaleY;
		}

		void SetText(
			const char *utf8String, 
			float x, 
			float y, 
			float z,
			bool kern, 
			float kernScale = 1.0f, 
			float scaleX = 1.0f, 
			float scaleY = 1.0f
		) {
			this->utf8String = utf8String;
			this->x         = x;
			this->y         = y;
			this->z         = z;
			this->kern      = kern;
			this->kernScale = kernScale;
			this->scaleX    = scaleX;
			this->scaleY    = scaleY;
		}

		const char *utf8String;
		float x, y ,z;
		bool kern;
		float kernScale;
		float scaleX;
		float scaleY;
	};

	virtual ~TextModel();

	font::GlyphCache &GlyphCache() const {
		return m_fontInstance->cache;
	}

	void SetFont(const pkg::AssetRef &font, int fontWidth=0, int fontHeight=0);
	void SetSize(int fontWidth, int fontHeight);
	
	void Clear();

	void Dimensions(float &x, float &y, float &w, float &h);

	void SetText(
		const String &string
	);

	void SetText(
		const String *strings,
		int numStrings
	);

	void SetText(
		const char *utf8String, 
		float x = 0.0f, 
		float y = 0.0f, 
		float z = 0.0f, 
		bool kern = true, 
		float kernScale = 1.0f, 
		float scaleX = 1.0f, 
		float scaleY = 1.0f
	);

	RAD_DECLARE_READONLY_PROPERTY(TextModel, numPasses, int);

	// Draw with material.
	// Assumes screen xform set
	void Draw(
		r::Material &material, 
		bool sampleMaterialColor = true, 
		const Vec4 &rgba = Vec4(1, 1, 1, 1), 
		int flags=0, 
		int blends=0
	);
	
	// BatchDraw does not do a material.BindStates(), which
	// only needs to be done once per material, so this allows
	// more efficient rendering of multiple text models using
	// the same material.
	void BatchDraw(
		r::Material &material, 
		bool sampleMaterialColor = true,
		const Vec4 &rgba = Vec4(1, 1, 1, 1)
	);

	// This call is optional, and will allocate vertex memory for 
	// the specified text strings.
	void AllocateText(
		const char **utf8Strings,
		int numStrings
	);

protected:

	TextModel();
	TextModel(const pkg::AssetRef &font, int fontWidth, int fontHeight);

	struct VertexType {
		float x, y;
		float s, t;
	};

	virtual VertexType *LockVerts(int num) = 0;
	virtual void UnlockVerts() = 0;
	virtual void ReserveVerts(int num) = 0;
	virtual void BindStates(int ofs, int count, font::IGlyphPage &page) = 0;
	virtual void DrawVerts(int ofs, int count) = 0;

	// implemented by RB
	static font::IGlyphPage::Ref AllocatePage(int width, int height);

private:

	struct Pass {
		int ofs;
		int num;
		font::IGlyphPage *page;
	};

	typedef zone_vector<Pass, ZFontsT>::type PassVec;

	struct FontKey {
		FontKey(int _id, int _width, int _height)
			: id(_id), width(_width), height(_height) {
		}

		int id, width, height;

		bool operator < (const FontKey &key) const {
			if (width < key.width) {
				return true;
			} else if (width == key.width && height < key.height) {
				return true;
			} else if (width == key.width && height == key.height && id < key.id) {
				return true;
			}

			return false;
		}

		bool operator == (const FontKey &key) {
			return (width == key.width) && 
				(height == key.height) &&
				(id == key.id);
		}
	};

	struct FontInstance;
	typedef boost::shared_ptr<FontInstance> FontInstanceRef;
	typedef boost::weak_ptr<FontInstance> FontInstanceWRef;

	typedef zone_map<FontKey, FontInstanceWRef, ZFontsT>::type FontInstanceHash;

	struct FontInstance {
		typedef FontInstanceRef Ref;

		~FontInstance();
		
		pkg::AssetRef font;
		font::GlyphCache cache;
		FontInstanceHash::iterator it;
	};

	struct GlyphPageFactory : public font::IGlyphPageFactory {
		virtual font::IGlyphPage::Ref AllocatePage(int width, int height);
		static void NoopDelete(font::IGlyphPageFactory *f);
	};

	void BuildTextVerts(const String *strings, int numStrings);
	void BindFont(int fontWidth, int fontHeight);

	void BeginPass(int i);
	void Draw();
	void EndPass();

	RAD_DECLARE_GET(numPasses, int) { return (int)m_passes.size(); }

	PassVec m_passes;
	FontInstance::Ref m_fontInstance;
	pkg::AssetRef m_font;
	Pass *m_curPass;
	float m_orgX, m_orgY;
	float m_width, m_height;
	int m_fontWidth, m_fontHeight;

	static GlyphPageFactory s_factory;
	static FontInstanceHash s_fontHash;
};

} // r

#include <Runtime/PopPack.h>
