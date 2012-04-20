// QtUpdate.h
// Copyright (c) 2009 Pyramind Labs LLC, All Rights Reserved

#pragma once

#include "../../Types.h"
#include <QtCore/QObject>

class GameBase;

namespace tools {
namespace editor {

class RADENG_CLASS Update : public QObject
{
	Q_OBJECT
public:
	Update(GameBase &g) : m_g(g) {}

public slots:

	void DoUpdate();
	
protected:
	
	virtual void timerEvent(QTimerEvent*) { DoUpdate(); }

private:

	GameBase &m_g;
};

} // editor
} // tools
