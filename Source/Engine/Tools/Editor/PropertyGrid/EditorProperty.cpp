// EditorProperty.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "EditorProperty.h"
#include <QtCore/QSignalMapper>

namespace tools {
namespace editor {

Property::Property(const QString &name, const UserContext::Ref &context, QWidget &widget, QObject *parent)
: QObject(parent), m_name(name), m_context(context), m_widget(&widget)
{
}

Property::~Property()
{
}

void Property::SignalChanged()
{
	emit Changed();
}

} // editor
} // tools

#include "moc_EditorProperty.cc"
