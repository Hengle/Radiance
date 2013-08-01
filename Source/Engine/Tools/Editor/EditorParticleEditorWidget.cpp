/*! \file EditorParticleEditorWidget.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#include RADPCH
#include "EditorParticleEditorWidget.h"
#include "EditorMainWindow.h"
#include "EditorUtils.h"
#include "EditorGLNavWidget.h"
#include "../../Renderer/GL/GLState.h"
#include "../../Renderer/GL/GLTexture.h"
#include "../../Renderer/Shader.h"
#include "../../Renderer/Material.h"
#include "../../Assets/ParticleMaterialLoader.h"
#include <QtGui/QSplitter>
#include <QtGui/QWidget>
#include <QtGui/QGridLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QFormLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>
#include <QtGui/QLabel>
#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>
#include <QtGui/QPushButton>
#include <Runtime/Math.h>

using namespace r;

namespace tools {
namespace editor {

///////////////////////////////////////////////////////////////////////////////

ParticleEditorWidget::ParticleEditorWidget(
	const pkg::Asset::Ref &asset,
	bool editable,
	QWidget *parent
) : QWidget(parent), 
m_asset(asset), 
m_glw(0), 
m_parser(0), 
m_material(0),
m_spawnCount(0),
m_numParticles(0),
m_numParticlesLabel(0)
{
}

ParticleEditorWidget::~ParticleEditorWidget() {
}


void ParticleEditorWidget::resizeEvent(QResizeEvent *event) {

	if (m_glw)
		return;

	int left, right;
	right = (int)(this->width()*0.85f);
	left = this->width()-right;

	QSplitter *s = new (ZEditor) QSplitter(this);
	s->setOpaqueResize(false);
	s->setOrientation(Qt::Horizontal);

	QWidget *w = new (ZEditor) QWidget();

	QVBoxLayout *vbl = new (ZEditor) QVBoxLayout(w);

	QFormLayout *l = new (ZEditor) QFormLayout();
	
	QLineEdit *le = new (ZEditor) QLineEdit();
	le->setText("0 0 1");
	connect(le, SIGNAL(textEdited(const QString&)), SLOT(OnDirChanged(const QString&)));
	l->addRow("Direction", le);

	le = new (ZEditor) QLineEdit();
	le->setText("0.5");
	le->setValidator(new (ZEditor) QDoubleValidator(le));
	connect(le, SIGNAL(textEdited(const QString&)), SLOT(OnSpreadChanged(const QString&)));
	l->addRow("Spread", le);

	le = new (ZEditor) QLineEdit();
	le->setText("0 0 0");
	connect(le, SIGNAL(textEdited(const QString&)), SLOT(OnPosChanged(const QString&)));
	l->addRow("Position", le);

	le = new (ZEditor) QLineEdit();
	le->setText("10");
	le->setValidator(new (ZEditor) QDoubleValidator(le));
	connect(le, SIGNAL(textEdited(const QString&)), SLOT(OnPPSChanged(const QString&)));
	l->addRow("PPS", le);

	le = new (ZEditor) QLineEdit();
	le->setText("0 0 0");
	connect(le, SIGNAL(textEdited(const QString&)), SLOT(OnVolumeChanged(const QString&)));
	l->addRow("Volume", le);

	le = new (ZEditor) QLineEdit();
	le->setText("1000");
	le->setValidator(new (ZEditor) QDoubleValidator(le));
	connect(le, SIGNAL(textEdited(const QString&)), SLOT(OnMaxParticlesChanged(const QString&)));
	l->addRow("Max Particles", le);

	vbl->addLayout(l);

	QGroupBox *g = new (ZEditor) QGroupBox("Bulk Spawn");
	l = new (ZEditor) QFormLayout(g);

	m_spawnCount = new (ZEditor) QLineEdit();
	m_spawnCount->setText("50");
	m_spawnCount->setValidator(new (ZEditor) QIntValidator(le));
	l->addRow("Particle Count", m_spawnCount);

	QPushButton *b = new (ZEditor) QPushButton("Spawn");
	connect(b, SIGNAL(clicked()), SLOT(OnSpawnPressed()));
	l->addWidget(b);

	vbl->addWidget(g);
	
	m_numParticlesLabel = new (ZEditor) QLabel("Active Particles: 0");
	vbl->addWidget(m_numParticlesLabel);	
	
	s->addWidget(w);

	m_glw = new (ZEditor) GLNavWidget();
	RAD_VERIFY(connect(m_glw, SIGNAL(OnRenderGL(GLWidget&)), SLOT(OnRenderGL(GLWidget&))));
	
	m_glw->resize(right, m_glw->height());
	s->addWidget(m_glw);

	QGridLayout *gl = new (ZEditor) QGridLayout(this);
	gl->addWidget(s, 0, 0);
}

bool ParticleEditorWidget::Load() {
	
	if (m_asset->type != asset::AT_Particle)
		return false;

	m_glw->bindGL(true);

	m_emitterStyle.maxParticles = 1000;
	m_emitterStyle.pps = 10;
	m_emitterStyle.spread = 0.5;
	m_emitterStyle.volume = Vec3::Zero;

	memset(&m_particleStyle, 0, sizeof(m_particleStyle));
	
	m_emitter.reset(new (ZTools) r::ParticleEmitter(m_emitterStyle, m_particleStyle));
	
	if (!LoadAsset())
		return false;

	Bind(
		m_asset->entry->OnKeyChange,
		&ParticleEditorWidget::OnParticleDataChanged
	);

	m_glw->camera->pos = Vec3(300.f, 0.f, 0.f);
	m_glw->camera->LookAt(Vec3::Zero);
	m_glw->camera->fov = 70.f;
	m_glw->SetFreeMode();
	m_glw->kbSpeed = 800.f;
	m_glw->mouseSpeed = 0.3f;
	m_glw->camera->fov = 90.f;

	m_glw->unbindGL();

	return true;
}

bool ParticleEditorWidget::LoadAsset() {

	if (m_parser) { // already loaded
		m_parser = 0;
		m_material = 0;

		pkg::Package::Entry::Ref entry = m_asset->entry;
		pkg::Zone zone = m_asset->zone;

		// hang on to the material, if it didn't change
		// we don't have to reload it
		pkg::Asset::Ref material;
		asset::ParticleMaterialLoader *particleMaterialLoader = asset::ParticleMaterialLoader::Cast(m_asset);
		if (particleMaterialLoader)
			material = particleMaterialLoader->material;
		
		// unload
		m_asset.reset();

		// reload
		m_asset = entry->Asset(zone);
	}

	int fastFlags = (MainWindow::Get()->lowQualityPreview) ? pkg::P_FastCook : 0;

	int r = m_asset->Process(
		xtime::TimeSlice::Infinite,
		pkg::P_Load/*|pkg::P_FastPath*/|fastFlags
	);

	if (r != pkg::SR_Success) {
		QMessageBox::critical(
			this,
			"Error",
			QString("Failed to load ") + m_asset->path.get()
		);
		return false;
	}
	
	m_parser = asset::ParticleParser::Cast(m_asset);
	if (!m_parser)
		return false;

	asset::ParticleMaterialLoader *particleMaterialLoader = asset::ParticleMaterialLoader::Cast(m_asset);
	if (!particleMaterialLoader)
		return false;
	
	asset::MaterialParser *materialParser = asset::MaterialParser::Cast(particleMaterialLoader->material);
	if (!materialParser)
		return false;

	m_material = materialParser->material;
	m_loader = asset::MaterialLoader::Cast(particleMaterialLoader->material);

	if (!m_loader)
		return false;

	m_particleStyle = *m_parser->particleStyle;
	m_emitter->UpdateStyle(m_particleStyle);

	return true;
}

void ParticleEditorWidget::OnRenderGL(GLWidget &src) {
	
	glClearColor(0.3f, 0.3f, 0.3f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	if (!m_emitter || !m_material)
		return;

	float vpw = src.width();
	float vph = src.height();

	gl.RotateForCamera(
		m_glw->camera->pos,
		Mat4::Rotation(m_glw->camera->rot.get()),
		1.f,
		16384.f,
		m_glw->camera->fov,
		vph/vpw
	);

	if (m_emitter->numParticles > 0) {
		m_emitter->Skin();

		m_material->BindTextures(m_loader);
		m_material->BindStates();
		m_material->shader->Begin(Shader::kPass_Default, *m_material);

		for (int i = 0; i < m_emitter->numBatches; ++i) {
			r::SpriteBatch &batch = m_emitter->Batch(i);
			if (batch.numSprites > 0) {
				batch.mesh->BindAll(0);
				m_material->shader->BindStates();
				gls.Commit();
				batch.Draw();
			}
		}

		m_material->shader->End();
	}

	if (m_numParticles != m_emitter->numParticles) {
		m_numParticles = m_emitter->numParticles;
		m_numParticlesLabel->setText(QString("Num Particles: %1").arg(m_numParticles));
	}
	
	gls.Set(kDepthWriteMask_Enable, -1); // for glClear()
	gls.Commit();
}

void ParticleEditorWidget::Tick(float dt) {
	
	if (m_material)
		m_material->Sample(App::Get()->time, dt);

	if (m_emitter)
		m_emitter->Tick(dt);
		
	m_glw->TickCamera(dt);
	m_glw->updateGL();
}

void ParticleEditorWidget::OnDirChanged(const QString &str) {
	
	Vec3 dir(0.f, 0.f, 1.f);

	sscanf(
		str.toAscii().constData(), 
		"%f %f %f", 
		&dir[0], 
		&dir[1], 
		&dir[2]
	);
	
	if (m_emitter)
		m_emitter->dir = dir;
}

void ParticleEditorWidget::OnPosChanged(const QString &str) {
	Vec3 pos(Vec3::Zero);

	sscanf(
		str.toAscii().constData(), 
		"%f %f %f", 
		&pos[0], 
		&pos[1], 
		&pos[2]
	);
	
	if (m_emitter)
		m_emitter->pos = pos;
}

void ParticleEditorWidget::OnPPSChanged(const QString &str) {
	sscanf(str.toAscii().constData(), "%f", &m_emitterStyle.pps);

	if (m_emitter)
		m_emitter->UpdateStyle(m_emitterStyle);
}

void ParticleEditorWidget::OnSpreadChanged(const QString &str) {
	sscanf(str.toAscii().constData(), "%f", &m_emitterStyle.spread);

	if (m_emitter)
		m_emitter->UpdateStyle(m_emitterStyle);
}

void ParticleEditorWidget::OnVolumeChanged(const QString &str) {
	
	sscanf(
		str.toAscii().constData(), 
		"%f %f %f", 
		&m_emitterStyle.volume[0], 
		&m_emitterStyle.volume[1], 
		&m_emitterStyle.volume[2]
	);
	
	if (m_emitter)
		m_emitter->UpdateStyle(m_emitterStyle);
}

void ParticleEditorWidget::OnMaxParticlesChanged(const QString &str) {
	
	sscanf(str.toAscii().constData(), "%d", &m_emitterStyle.maxParticles);

	if (m_emitter)
		m_emitter->UpdateStyle(m_emitterStyle);
}

void ParticleEditorWidget::OnSpawnPressed() {
	int spawn = 0;
	sscanf(m_spawnCount->text().toAscii().constData(), "%d", &spawn);

	if ((spawn > 0) && m_emitter)
		m_emitter->Spawn(spawn);
}

void ParticleEditorWidget::OnParticleDataChanged(const pkg::Package::Entry::KeyChangedEventData &data) {
	LoadAsset();
}

} // editor
} // tools

#include "moc_EditorParticleEditorWidget.cc"

