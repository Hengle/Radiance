// ALDriver.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "ALDriver.h"
#include <Runtime/StringBase.h>

#if defined(RAD_OPT_OSX)
#define alSpeedOfSound alDopplerVelocity
#endif

#undef MessageBox

///////////////////////////////////////////////////////////////////////////////

void ClearALErrors()
{
	if (!alcGetCurrentContext())
		return;

	while (alGetError() != AL_NO_ERROR) {
	}
}

bool CheckALErrors(const char *file, int line)
{
	if (!alcGetCurrentContext())
		return false;

#if defined(RAD_OPT_IOS)
	return alGetError() != AL_NO_ERROR;
#else
	WString str;
	bool found = false;
	int count = 0;

	for (ALenum err = alGetError(); err != AL_NO_ERROR; err = alGetError())
	{
		if (++count > 256)
			break;

		if (!found)
		{
			str.format(
				L"AL Errors (file: %s, line: %d):\n",
				string::Widen(file).c_str(),
				line
			);
			found = true;
		}

		switch (err)
		{
		case AL_INVALID_NAME:
			str += L"AL_INVALID_NAME\n";
			break;
		case AL_INVALID_ENUM:
			str += L"AL_INVALID_ENUM\n";
			break;
		case AL_INVALID_VALUE:
			str += L"AL_INVALID_VALUE\n";
			break;
		case AL_INVALID_OPERATION:
			str += L"AL_INVALID_OPERATION\n";
			break;
		case AL_OUT_OF_MEMORY:
			str += L"AL_OUT_OF_MEMORY\n";
			break;
		default:
			str += L"Unknown Error\n";
		};
	}

	ClearALErrors();
	if (found)
	{
		MessageBox(L"AL Errors Detected", str.c_str(), MBStyleOk);
	}

	return found;
#endif
}

///////////////////////////////////////////////////////////////////////////////

ALDriver::Callback::Callback(const ALDriver::Ref &driver) : m_driver(driver) {
}

ALDriver::Callback::~Callback() {
	ALDriver::Ref driver = m_driver.lock();
	if (driver)
		driver->removeCallback(*this);
}

///////////////////////////////////////////////////////////////////////////////

ALDriver::Ref ALDriver::create(ALDRIVER_PARAMS const char *deviceName) {
	ALDriver::Ref r(new (ZSound) ALDriver());

	r->Run();

	Command c(&fn_create);
	ALDRIVER_CMD_SIG(c);
	c.args.pvoid = (void*)deviceName;

	r->submit(c);
	r->sync_flush(ALDRIVER_VOID_SIG);

	if (!c.args.ival) // error
		r.reset();

	return r;
}

ALDriver::ALDriver() : 
m_head(0), m_tail(0), m_quit(false), m_suspended(false), m_alc(0), m_ald(0) {
	m_cmdPool.Create(ZSound, "alDriverCmds", 64);
}

ALDriver::~ALDriver() {
	m_quit = true;
	wake();
	Join();
}

int ALDriver::ThreadProc() {

	while (!m_quit) {
		m_sema.Get(33, true);

		Command *head;
		{
			Lock L(m_m);
			head = m_head;
			m_head = 0;
			m_tail = 0;
		}

		exec(head);
		doCallbacks();
	}

	alcMakeContextCurrent(0);

	if (m_alc)
		alcDestroyContext(m_alc);
	if (m_ald)
		alcCloseDevice(m_ald);

	return 0;
}

void ALDriver::exec(Command *head) {
	while (head) {
		Command *c = head;
		head = head->m_next;
		c->m_fn(*this, c);
	}
}

void ALDriver::doCallbacks() {
	Lock L(m_mcb);

	for (CallbackSet::const_iterator it = m_callbacks.begin(); it != m_callbacks.end(); ++it) {
		(*it)->tick(*this);
	}
}

void ALDriver::wake() {
	m_sema.Put();
}

void ALDriver::addCallback(Callback &callback) {
	Lock L(m_mcb);
	m_callbacks.insert(&callback);
}

void ALDriver::removeCallback(Callback &callback) {
	Lock L(m_mcb);
	m_callbacks.erase(&callback);
}

void ALDriver::sync_process(ALDRIVER_VOID_PARAMS) {
	Command c(&fn_process);
	ALDRIVER_CMD_SIG(c);
	submit(c);
	sync_flush(ALDRIVER_VOID_ARGS);
	m_suspended = false;
}

void ALDriver::sync_suspend(ALDRIVER_VOID_PARAMS) {
	Command c(&fn_suspend);
	ALDRIVER_CMD_SIG(c);
	submit(c);
	sync_flush(ALDRIVER_VOID_ARGS);
	m_suspended = true;
}

void ALDriver::sync_flush(ALDRIVER_VOID_PARAMS) {
	MutexedCommand c(&fn_flush);
	ALDRIVER_CMD_SIG(c);
	thread::EventMutex::Sync S(c.m);
	submit(c);
	S.wait();
}

void ALDriver::dopplerFactor(ALDRIVER_PARAMS ALfloat factor) {
	Command *c = createCommand(&fn_dopplerFactor);
	ALDRIVER_CMD_SIG(*c);
	c->args.fval = factor;
	submit(*c);
}

void ALDriver::speedOfSound(ALDRIVER_PARAMS ALfloat speed) {
	Command *c = createCommand(&fn_speedOfSound);
	ALDRIVER_CMD_SIG(*c);
	c->args.fval = speed;
	submit(*c);
}

void ALDriver::distanceModel(ALDRIVER_PARAMS ALenum value) {
	Command *c = createCommand(&fn_distanceModel);
	ALDRIVER_CMD_SIG(*c);
	c->param = value;
	submit(*c);
}

void ALDriver::listenerfv(ALDRIVER_PARAMS ALenum param, ALfloat *values) {
	RAD_ASSERT(param == AL_ORIENTATION);
	Command *c = createCommand(&fn_listenerfv);
	ALDRIVER_CMD_SIG(*c);
	c->param = param;
	for (int i = 0; i < 6; ++i)
		c->args.fvvals[i] = values[i];
	submit(*c);
}

void ALDriver::listenerf(ALDRIVER_PARAMS ALenum param, ALfloat value) {
	Command *c = createCommand(&fn_listenerf);
	ALDRIVER_CMD_SIG(*c);
	c->param = param;
	c->args.fval = value;
	submit(*c);
}

void ALDriver::listener3f(ALDRIVER_PARAMS ALenum param, ALfloat x, ALfloat y, ALfloat z) {
	Command *c = createCommand(&fn_listener3f);
	ALDRIVER_CMD_SIG(*c);
	c->param = param;
	c->args.fvvals[0] = x;
	c->args.fvvals[1] = y;
	c->args.fvvals[2] = z;
	submit(*c);
}

bool ALDriver::sync_genBuffers(ALDRIVER_PARAMS ALsizei n, ALuint *buffers) {
	GenCommand c(&fn_genBuffers);
	ALDRIVER_CMD_SIG(c);
	c.num = n;
	c.args.puint = buffers;
	submit(c);
	sync_flush(ALDRIVER_VOID_ARGS);
	return c.args.puint != 0;
}

void ALDriver::sync_deleteBuffers(ALDRIVER_PARAMS ALsizei n, ALuint *buffers) {
	GenCommand c(&fn_deleteBuffers);
	ALDRIVER_CMD_SIG(c);
	c.num = n;
	c.args.puint = buffers;
	submit(c);
	sync_flush(ALDRIVER_VOID_ARGS);
}

bool ALDriver::sync_bufferData(
	ALDRIVER_PARAMS 
	ALuint buffer, 
	ALenum format, 
	const ALvoid *data, 
	ALsizei size,
	ALsizei freq
) {
	BufferDataCommand c(&fn_bufferData);
	ALDRIVER_CMD_SIG(c);
	c.handle = buffer;
	c.format = format;
	c.args.pvoid = (void*)data;
	c.size = size;
	c.freq = freq;
	submit(c);
	sync_flush(ALDRIVER_VOID_ARGS);
	return c.args.pvoid != 0;
}


bool ALDriver::sync_genSources(ALDRIVER_PARAMS ALsizei n, ALuint *sources) {
	GenCommand c(&fn_genSources);
	ALDRIVER_CMD_SIG(c);
	c.num = n;
	c.args.puint = sources;
	submit(c);
	sync_flush(ALDRIVER_VOID_ARGS);
	return c.args.puint != 0;
}

void ALDriver::sync_deleteSources(ALDRIVER_PARAMS ALsizei n, ALuint *sources) {
	GenCommand c(&fn_deleteSources);
	ALDRIVER_CMD_SIG(c);
	c.num = n;
	c.args.puint = sources;
	submit(c);
	sync_flush(ALDRIVER_VOID_ARGS);
}

void ALDriver::sourcePlay(ALDRIVER_PARAMS ALuint source) {
	Command *c = createCommand(&fn_sourcePlay);
	ALDRIVER_CMD_SIG(*c);
	c->handle = source;
	submit(*c);
}

void ALDriver::sourcePause(ALDRIVER_PARAMS ALuint source) {
	Command *c = createCommand(&fn_sourcePause);
	ALDRIVER_CMD_SIG(*c);
	c->handle = source;
	submit(*c);
}

void ALDriver::sourceStop(ALDRIVER_PARAMS ALuint source) {
	Command *c = createCommand(&fn_sourceStop);
	ALDRIVER_CMD_SIG(*c);
	c->handle = source;
	submit(*c);
}

void ALDriver::sourcei(ALDRIVER_PARAMS ALuint source, ALenum param, ALint value) {
	Command *c = createCommand(&fn_sourcei);
	ALDRIVER_CMD_SIG(*c);
	c->handle = source;
	c->param = param;
	c->args.ival = value;
	submit(*c);
}

void ALDriver::sourcef(ALDRIVER_PARAMS ALuint source, ALenum param, ALfloat value) {
	Command *c = createCommand(&fn_sourcef);
	ALDRIVER_CMD_SIG(*c);
	c->handle = source;
	c->param = param;
	c->args.fval = value;
	submit(*c);
}

void ALDriver::source3f(ALDRIVER_PARAMS ALuint source, ALenum param, ALfloat x, ALfloat y, ALfloat z) {
	Command *c = createCommand(&fn_source3f);
	ALDRIVER_CMD_SIG(*c);
	c->handle = source;
	c->param = param;
	c->args.fvvals[0] = x;
	c->args.fvvals[1] = y;
	c->args.fvvals[2] = z;
	submit(*c);
}

void ALDriver::sourceRewind(ALDRIVER_PARAMS ALuint source) {
	Command *c = createCommand(&fn_sourceRewind);
	ALDRIVER_CMD_SIG(*c);
	c->handle = source;
	submit(*c);
}

ALDriver::Command *ALDriver::createCommand(Command::FN fn) {
	return m_cmdPool.Construct(fn);
}

void ALDriver::destroyCommand(Command *cmd) {
	m_cmdPool.Destroy(cmd);
}

void ALDriver::submit(Command &command) {
	{
		Lock L(m_m);
		
		if (!m_head)
			m_head = &command;
		if (m_tail)
			m_tail->m_next = &command;

		command.m_next = 0;
		m_tail = &command;
	}

	wake();
}

void ALDriver::fn_create(ALDriver &driver, Command *cmd) {

	if (!cmd->args.pvoid || string::cmp((const char*)cmd->args.pvoid, "null")) {
		driver.m_ald = alcOpenDevice((const char*)cmd->args.pvoid);
		if (driver.m_ald) {
			driver.m_alc = alcCreateContext(driver.m_ald, 0);
			if (driver.m_alc) {
				alcMakeContextCurrent(driver.m_alc);
			} else {
				alcCloseDevice(driver.m_ald);
				driver.m_ald = 0;
			}
		}

		cmd->args.ival = (driver.m_ald&&driver.m_alc) ? 1 : 0;
	} else {
		cmd->args.ival = 1; // null device always succeeds.
	}
}

void ALDriver::fn_process(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alcProcessContext(driver.m_alc);
	CHECK_AL_ERRORS(*cmd);
}

void ALDriver::fn_suspend(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alcSuspendContext(driver.m_alc);
	CHECK_AL_ERRORS(*cmd);
}

void ALDriver::fn_flush(ALDriver &driver, Command *cmd) {
	MutexedCommand *m = static_cast<MutexedCommand*>(cmd);
	m->m.notifyAll();
}

void ALDriver::fn_dopplerFactor(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alDopplerFactor(cmd->args.fval);
	CHECK_AL_ERRORS(*cmd);
	driver.destroyCommand(cmd);
}

void ALDriver::fn_speedOfSound(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alSpeedOfSound(cmd->args.fval);
	CHECK_AL_ERRORS(*cmd);
	driver.destroyCommand(cmd);
}

void ALDriver::fn_distanceModel(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alDistanceModel(cmd->param);
	CHECK_AL_ERRORS(*cmd);
	driver.destroyCommand(cmd);
}

void ALDriver::fn_listenerfv(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alListenerfv(cmd->param, cmd->args.fvvals);
	CHECK_AL_ERRORS(*cmd);
	driver.destroyCommand(cmd);
}

void ALDriver::fn_listenerf(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alListenerf(cmd->param, cmd->args.fval);
	CHECK_AL_ERRORS(*cmd);
	driver.destroyCommand(cmd);
}

void ALDriver::fn_listener3f(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alListener3f(cmd->param, cmd->args.fvvals[0], cmd->args.fvvals[1], cmd->args.fvvals[2]);
	CHECK_AL_ERRORS(*cmd);
	driver.destroyCommand(cmd);
}

void ALDriver::fn_genBuffers(ALDriver &driver, Command *cmd) {
	GenCommand *gc = static_cast<GenCommand*>(cmd);
	CLEAR_AL_ERRORS();
	if (driver.m_alc) {
		alGenBuffers(gc->num, gc->args.puint);
		if (alGetError() != AL_NO_ERROR) {
			memset(gc->args.puint, 0, gc->num*sizeof(ALuint));
			gc->args.puint = 0; // flag error.
		}
	} else {
		memset(gc->args.puint, 1, gc->num*sizeof(ALuint));
	}
	CLEAR_AL_ERRORS();
}

void ALDriver::fn_deleteBuffers(ALDriver &driver, Command *cmd) {
	GenCommand *gc = static_cast<GenCommand*>(cmd);
	if (driver.m_alc)
		alDeleteBuffers(gc->num, gc->args.puint);
	CHECK_AL_ERRORS(*cmd);
}

void ALDriver::fn_bufferData(ALDriver &driver, Command *cmd) {
	BufferDataCommand *bdc = static_cast<BufferDataCommand*>(cmd);
	CLEAR_AL_ERRORS();
	if (driver.m_alc) {
		alBufferData(bdc->handle, bdc->format, bdc->args.pvoid, bdc->size, bdc->freq);
		if (alGetError() != AL_NO_ERROR) {
			bdc->args.pvoid = 0; // flag error.
		}
	}
	CLEAR_AL_ERRORS();
}

void ALDriver::fn_genSources(ALDriver &driver, Command *cmd) {
	GenCommand *gc = static_cast<GenCommand*>(cmd);
	CLEAR_AL_ERRORS();
	if (driver.m_alc) {
		alGenSources(gc->num, gc->args.puint);
		if (alGetError() != AL_NO_ERROR) {
			memset(gc->args.puint, 0, gc->num*sizeof(ALuint));
			gc->args.puint = 0; // flag error.
		}
	} else {
		memset(gc->args.puint, 1, gc->num*sizeof(ALuint));
	}
	CLEAR_AL_ERRORS();
}

void ALDriver::fn_deleteSources(ALDriver &driver, Command *cmd) {
	GenCommand *gc = static_cast<GenCommand*>(cmd);
	if (driver.m_alc)
		alDeleteSources(gc->num, gc->args.puint);
	CHECK_AL_ERRORS(*cmd);
}

void ALDriver::fn_sourcePlay(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alSourcePlay(cmd->handle);
	CHECK_AL_ERRORS(*cmd);
	driver.destroyCommand(cmd);
}

void ALDriver::fn_sourcePause(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alSourcePause(cmd->handle);
	CHECK_AL_ERRORS(*cmd);
	driver.destroyCommand(cmd);
}

void ALDriver::fn_sourceStop(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alSourceStop(cmd->handle);
	CHECK_AL_ERRORS(*cmd);
	driver.destroyCommand(cmd);
}

void ALDriver::fn_sourcei(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alSourcei(cmd->handle, cmd->param, cmd->args.ival);
	CHECK_AL_ERRORS(*cmd);
	driver.destroyCommand(cmd);
}

void ALDriver::fn_sourcef(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alSourcef(cmd->handle, cmd->param, cmd->args.fval);
	CHECK_AL_ERRORS(*cmd);
	driver.destroyCommand(cmd);
}

void ALDriver::fn_source3f(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alSource3f(cmd->handle, cmd->param, cmd->args.fvvals[0], cmd->args.fvvals[1], cmd->args.fvvals[2]);
	CHECK_AL_ERRORS(*cmd);
	driver.destroyCommand(cmd);
}

void ALDriver::fn_sourceRewind(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alSourceRewind(cmd->handle);
	CHECK_AL_ERRORS(*cmd);
	driver.destroyCommand(cmd);
}
