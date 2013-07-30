/*! \file EditorParticleEditorWidget.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#pragma once

#include "../../Types.h"
#include <QtGui/QWidget>
#include <QtCore/QPoint>
#include "EditorGLNavWidget.h"
#include "../../Renderer/Particles.h"
#include "../../Renderer/Material.h"
#include "../../Assets/ParticleParser.h"
#include "../../Assets/MaterialParser.h"
#include <Runtime/PushPack.h>

class QLineEdit;
class QPushButton;
class QLabel;

namespace tools {
namespace editor {

class RADENG_CLASS ParticleEditorWidget : public QWidget {
	Q_OBJECT
	RAD_EVENT_CLASS(EventNoAccess)
public:

	ParticleEditorWidget(
		const pkg::Asset::Ref &asset,
		bool editable,
		QWidget *parent = 0
	);
	
	bool Load();
	void Tick(float dt);

protected:

	virtual void resizeEvent(QResizeEvent *event);

private slots:

	void OnRenderGL(GLWidget &src);
	void OnDirChanged(const QString &str);
	void OnPosChanged(const QString &str);
	void OnPPSChanged(const QString &str);
	void OnSpreadChanged(const QString &str);
	void OnVolumeChanged(const QString &str);
	void OnMaxParticlesChanged(const QString &str);
	void OnSpawnPressed();

private:

	ParticleEditorWidget();
	virtual ~ParticleEditorWidget();

	void OnParticleDataChanged(const pkg::Package::Entry::KeyChangedEventData &data);
	bool LoadAsset();

	r::ParticleEmitterStyle m_emitterStyle;
	r::ParticleStyle m_particleStyle;
	r::ParticleEmitter::Ref m_emitter;
	pkg::Asset::Ref m_asset;
	asset::ParticleParser *m_parser;
	asset::MaterialLoader *m_loader;
	r::Material *m_material;
	QLineEdit *m_spawnCount;
	QLabel *m_numParticlesLabel;
	int m_numParticles;

	GLNavWidget *m_glw;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
