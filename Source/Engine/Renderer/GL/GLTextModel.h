// GLTextModel.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../TextModel.h"
#include "../Mesh.h"
#include <Runtime/PushPack.h>

namespace r {

class RADENG_CLASS GLTextModel : public TextModel {
public:
	typedef boost::shared_ptr<GLTextModel> Ref;

	GLTextModel() : m_streamSize(0), m_streamId(-1) {}

protected:

	virtual VertexType *LockVerts(int num);
	virtual void UnlockVerts();
	virtual void ReserveVerts(int num);
	virtual void BindStates(const r::Material &material, int ofs, int count, font::IGlyphPage &page);
	virtual void DrawVerts(const r::Material &material, int ofs, int count);

private:

	Mesh::Ref m_mesh;
	GLVertexBuffer::Ptr::Ref m_lock;
	int m_streamSize;
	int m_streamId;
};

} // r

#include <Runtime/PopPack.h>
