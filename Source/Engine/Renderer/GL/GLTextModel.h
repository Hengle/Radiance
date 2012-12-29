// GLTextModel.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../TextModel.h"
#include "GLVertexBuffer.h"
#include <Runtime/PushPack.h>

namespace r {

class RADENG_CLASS GLTextModel : public TextModel {
public:
	typedef boost::shared_ptr<GLTextModel> Ref;

protected:

	virtual VertexType *LockVerts(int num);
	virtual void UnlockVerts();
	virtual void BindStates(int ofs, int count, font::IGlyphPage &page);
	virtual void DrawVerts(int ofs, int count);

private:

	GLVertexBuffer::Ref m_vb;
	GLVertexBuffer::Ptr::Ref m_lock;
};

} // r

#include <Runtime/PopPack.h>
