// ALDriver.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "ALDriver.h"
#include "../COut.h"
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
	String str;
	bool found = false;
	int count = 0;

	for (ALenum err = alGetError(); err != AL_NO_ERROR; err = alGetError())
	{
		if (++count > 256)
			break;

		if (!found)
		{
			str.Printf(
				"AL Errors (file: %s, line: %d):\n",
				file,
				line
			);
			found = true;
		}

		switch (err)
		{
		case AL_INVALID_NAME:
			str += "AL_INVALID_NAME\n";
			break;
		case AL_INVALID_ENUM:
			str += "AL_INVALID_ENUM\n";
			break;
		case AL_INVALID_VALUE:
			str += "AL_INVALID_VALUE\n";
			break;
		case AL_INVALID_OPERATION:
			str += "AL_INVALID_OPERATION\n";
			break;
		case AL_OUT_OF_MEMORY:
			str += "AL_OUT_OF_MEMORY\n";
			break;
		default:
			str += "Unknown Error\n";
		};
	}

	ClearALErrors();
	if (found)
	{
		COut(C_Debug) << str.c_str.get() << std::endl;
	}

	return found;
#endif
}

///////////////////////////////////////////////////////////////////////////////

ALDriver::Callback::Callback(const ALDriver::Ref &driver) : m_driver(driver) {
	RAD_DEBUG_ONLY(m_registered = false);
}

ALDriver::Callback::~Callback() {
	RAD_ASSERT(!m_registered); // Derrived classes MUST call Unregister() in their destructor.
}

void ALDriver::Callback::Unregister() {
	ALDriver::Ref driver = m_driver.lock();
	if (driver)
		driver->RemoveCallback(*this);
	m_driver.reset();
}

///////////////////////////////////////////////////////////////////////////////

ALDriver::Ref ALDriver::New(ALDRIVER_PARAMS const char *deviceName) {
	ALDriver::Ref r(new (ZSound) ALDriver());

	r->Run();

	Command c(&fn_Create);
	ALDRIVER_CMD_SIG(c);
	c.args.pvoid = (void*)deviceName;

	r->Submit(c);
	r->SyncFlush(ALDRIVER_VOID_SIG);

	if (!c.args.ival) // error
		r.reset();

	return r;
}

ALDriver::ALDriver() : 
m_head(0), m_tail(0), m_quit(false), m_suspended(false), m_alc(0), m_ald(0), m_enabled(true, false) {
	m_cmdPool.Create(ZSound, "alDriverCmds", 64);
}

ALDriver::~ALDriver() {
	m_quit = true;
	Wake();
	Join();
}

void ALDriver::Enable(bool enabled) {
	if (enabled) {
		m_enabled.Open();
	} else {
		m_enabled.Close();
	}
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

		Exec(head);
		DoCallbacks();
		m_enabled.Wait();
	}

	alcMakeContextCurrent(0);

	if (m_alc)
		alcDestroyContext(m_alc);
	if (m_ald)
		alcCloseDevice(m_ald);

	return 0;
}

void ALDriver::Exec(Command *head) {
	while (head) {
		Command *c = head;
		head = head->m_next;
		c->m_fn(*this, c);
	}
}

void ALDriver::DoCallbacks() {
	Lock L(m_mcb);

	for (CallbackSet::const_iterator it = m_callbacks.begin(); it != m_callbacks.end(); ++it) {
		(*it)->Tick(*this);
	}
}

void ALDriver::Wake() {
	m_sema.Put();
}

void ALDriver::AddCallback(Callback &callback) {
	Lock L(m_mcb);
	m_callbacks.insert(&callback);
	RAD_DEBUG_ONLY(callback.m_registered = true);
}

void ALDriver::RemoveCallback(Callback &callback) {
	Lock L(m_mcb);
	m_callbacks.erase(&callback);
	RAD_DEBUG_ONLY(callback.m_registered = false);
}

void ALDriver::SyncProcess(ALDRIVER_VOID_PARAMS) {
	Command c(&fn_Process);
	ALDRIVER_CMD_SIG(c);
	Submit(c);
	SyncFlush(ALDRIVER_VOID_ARGS);
	m_suspended = false;
}

void ALDriver::SyncSuspend(ALDRIVER_VOID_PARAMS) {
	Command c(&fn_Suspend);
	ALDRIVER_CMD_SIG(c);
	Submit(c);
	SyncFlush(ALDRIVER_VOID_ARGS);
	m_suspended = true;
}

void ALDriver::SyncFlush(ALDRIVER_VOID_PARAMS) {
	MutexedCommand c(&fn_Flush);
	ALDRIVER_CMD_SIG(c);
	thread::EventMutex::Sync S(c.m);
	Submit(c);
	S.Wait();
}

void ALDriver::DopplerFactor(ALDRIVER_PARAMS ALfloat factor) {
	Command *c = CreateCommand(&fn_DopplerFactor);
	ALDRIVER_CMD_SIG(*c);
	c->args.fval = factor;
	Submit(*c);
}

void ALDriver::SpeedOfSound(ALDRIVER_PARAMS ALfloat speed) {
	Command *c = CreateCommand(&fn_SpeedOfSound);
	ALDRIVER_CMD_SIG(*c);
	c->args.fval = speed;
	Submit(*c);
}

void ALDriver::DistanceModel(ALDRIVER_PARAMS ALenum value) {
	Command *c = CreateCommand(&fn_DistanceModel);
	ALDRIVER_CMD_SIG(*c);
	c->param = value;
	Submit(*c);
}

void ALDriver::Listenerfv(ALDRIVER_PARAMS ALenum param, ALfloat *values) {
	RAD_ASSERT(param == AL_ORIENTATION);
	Command *c = CreateCommand(&fn_Listenerfv);
	ALDRIVER_CMD_SIG(*c);
	c->param = param;
	for (int i = 0; i < 6; ++i)
		c->args.fvvals[i] = values[i];
	Submit(*c);
}

void ALDriver::Listenerf(ALDRIVER_PARAMS ALenum param, ALfloat value) {
	Command *c = CreateCommand(&fn_Listenerf);
	ALDRIVER_CMD_SIG(*c);
	c->param = param;
	c->args.fval = value;
	Submit(*c);
}

void ALDriver::Listener3f(ALDRIVER_PARAMS ALenum param, ALfloat x, ALfloat y, ALfloat z) {
	Command *c = CreateCommand(&fn_Listener3f);
	ALDRIVER_CMD_SIG(*c);
	c->param = param;
	c->args.fvvals[0] = x;
	c->args.fvvals[1] = y;
	c->args.fvvals[2] = z;
	Submit(*c);
}

bool ALDriver::SyncGenBuffers(ALDRIVER_PARAMS ALsizei n, ALuint *buffers) {
	GenCommand c(&fn_GenBuffers);
	ALDRIVER_CMD_SIG(c);
	c.num = n;
	c.args.puint = buffers;
	Submit(c);
	SyncFlush(ALDRIVER_VOID_ARGS);
	return c.args.puint != 0;
}

void ALDriver::SyncDeleteBuffers(ALDRIVER_PARAMS ALsizei n, ALuint *buffers) {
	GenCommand c(&fn_DeleteBuffers);
	ALDRIVER_CMD_SIG(c);
	c.num = n;
	c.args.puint = buffers;
	Submit(c);
	SyncFlush(ALDRIVER_VOID_ARGS);
}

bool ALDriver::SyncBufferData(
	ALDRIVER_PARAMS 
	ALuint buffer, 
	ALenum format, 
	const ALvoid *data, 
	ALsizei size,
	ALsizei freq
) {
	BufferDataCommand c(&fn_BufferData);
	ALDRIVER_CMD_SIG(c);
	c.handle = buffer;
	c.format = format;
	c.args.pvoid = (void*)data;
	c.size = size;
	c.freq = freq;
	Submit(c);
	SyncFlush(ALDRIVER_VOID_ARGS);
	return c.args.pvoid != 0;
}

bool ALDriver::SyncGenSources(ALDRIVER_PARAMS ALsizei n, ALuint *sources) {
	GenCommand c(&fn_GenSources);
	ALDRIVER_CMD_SIG(c);
	c.num = n;
	c.args.puint = sources;
	Submit(c);
	SyncFlush(ALDRIVER_VOID_ARGS);
	return c.args.puint != 0;
}

void ALDriver::SyncDeleteSources(ALDRIVER_PARAMS ALsizei n, ALuint *sources) {
	GenCommand c(&fn_DeleteSources);
	ALDRIVER_CMD_SIG(c);
	c.num = n;
	c.args.puint = sources;
	Submit(c);
	SyncFlush(ALDRIVER_VOID_ARGS);
}

void ALDriver::SourcePlay(ALDRIVER_PARAMS ALuint source) {
	Command *c = CreateCommand(&fn_SourcePlay);
	ALDRIVER_CMD_SIG(*c);
	c->handle = source;
	Submit(*c);
}

void ALDriver::SourcePause(ALDRIVER_PARAMS ALuint source) {
	Command *c = CreateCommand(&fn_SourcePause);
	ALDRIVER_CMD_SIG(*c);
	c->handle = source;
	Submit(*c);
}

void ALDriver::SourceStop(ALDRIVER_PARAMS ALuint source) {
	Command *c = CreateCommand(&fn_SourceStop);
	ALDRIVER_CMD_SIG(*c);
	c->handle = source;
	Submit(*c);
}

void ALDriver::Sourcei(ALDRIVER_PARAMS ALuint source, ALenum param, ALint value) {
	Command *c = CreateCommand(&fn_Sourcei);
	ALDRIVER_CMD_SIG(*c);
	c->handle = source;
	c->param = param;
	c->args.ival = value;
	Submit(*c);
}

void ALDriver::Sourcef(ALDRIVER_PARAMS ALuint source, ALenum param, ALfloat value) {
	Command *c = CreateCommand(&fn_Sourcef);
	ALDRIVER_CMD_SIG(*c);
	c->handle = source;
	c->param = param;
	c->args.fval = value;
	Submit(*c);
}

void ALDriver::Source3f(ALDRIVER_PARAMS ALuint source, ALenum param, ALfloat x, ALfloat y, ALfloat z) {
	Command *c = CreateCommand(&fn_Source3f);
	ALDRIVER_CMD_SIG(*c);
	c->handle = source;
	c->param = param;
	c->args.fvvals[0] = x;
	c->args.fvvals[1] = y;
	c->args.fvvals[2] = z;
	Submit(*c);
}

void ALDriver::SourceRewind(ALDRIVER_PARAMS ALuint source) {
	Command *c = CreateCommand(&fn_SourceRewind);
	ALDRIVER_CMD_SIG(*c);
	c->handle = source;
	Submit(*c);
}

ALDriver::Command *ALDriver::CreateCommand(Command::FN fn) {
	return m_cmdPool.Construct(fn);
}

void ALDriver::DestroyCommand(Command *cmd) {
	m_cmdPool.Destroy(cmd);
}

void ALDriver::Submit(Command &command) {
	{
		Lock L(m_m);
		
		if (!m_head)
			m_head = &command;
		if (m_tail)
			m_tail->m_next = &command;

		command.m_next = 0;
		m_tail = &command;
	}

	Wake();
}

void ALDriver::fn_Create(ALDriver &driver, Command *cmd) {

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

void ALDriver::fn_Process(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alcProcessContext(driver.m_alc);
	CHECK_AL_ERRORS(*cmd);
}

void ALDriver::fn_Suspend(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alcSuspendContext(driver.m_alc);
	CHECK_AL_ERRORS(*cmd);
}

void ALDriver::fn_Flush(ALDriver &driver, Command *cmd) {
	MutexedCommand *m = static_cast<MutexedCommand*>(cmd);
	m->m.NotifyAll();
}

void ALDriver::fn_DopplerFactor(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alDopplerFactor(cmd->args.fval);
	CHECK_AL_ERRORS(*cmd);
	driver.DestroyCommand(cmd);
}

void ALDriver::fn_SpeedOfSound(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alSpeedOfSound(cmd->args.fval);
	CHECK_AL_ERRORS(*cmd);
	driver.DestroyCommand(cmd);
}

void ALDriver::fn_DistanceModel(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alDistanceModel(cmd->param);
	CHECK_AL_ERRORS(*cmd);
	driver.DestroyCommand(cmd);
}

void ALDriver::fn_Listenerfv(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alListenerfv(cmd->param, cmd->args.fvvals);
	CHECK_AL_ERRORS(*cmd);
	driver.DestroyCommand(cmd);
}

void ALDriver::fn_Listenerf(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alListenerf(cmd->param, cmd->args.fval);
	CHECK_AL_ERRORS(*cmd);
	driver.DestroyCommand(cmd);
}

void ALDriver::fn_Listener3f(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alListener3f(cmd->param, cmd->args.fvvals[0], cmd->args.fvvals[1], cmd->args.fvvals[2]);
	CHECK_AL_ERRORS(*cmd);
	driver.DestroyCommand(cmd);
}

void ALDriver::fn_GenBuffers(ALDriver &driver, Command *cmd) {
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

void ALDriver::fn_DeleteBuffers(ALDriver &driver, Command *cmd) {
	GenCommand *gc = static_cast<GenCommand*>(cmd);
	if (driver.m_alc)
		alDeleteBuffers(gc->num, gc->args.puint);
	CHECK_AL_ERRORS(*cmd);
}

void ALDriver::fn_BufferData(ALDriver &driver, Command *cmd) {
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

void ALDriver::fn_GenSources(ALDriver &driver, Command *cmd) {
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

void ALDriver::fn_DeleteSources(ALDriver &driver, Command *cmd) {
	GenCommand *gc = static_cast<GenCommand*>(cmd);
	if (driver.m_alc)
		alDeleteSources(gc->num, gc->args.puint);
	CHECK_AL_ERRORS(*cmd);
}

void ALDriver::fn_SourcePlay(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alSourcePlay(cmd->handle);
	CHECK_AL_ERRORS(*cmd);
	driver.DestroyCommand(cmd);
}

void ALDriver::fn_SourcePause(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alSourcePause(cmd->handle);
	CHECK_AL_ERRORS(*cmd);
	driver.DestroyCommand(cmd);
}

void ALDriver::fn_SourceStop(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alSourceStop(cmd->handle);
	CHECK_AL_ERRORS(*cmd);
	driver.DestroyCommand(cmd);
}

void ALDriver::fn_Sourcei(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alSourcei(cmd->handle, cmd->param, cmd->args.ival);
	CHECK_AL_ERRORS(*cmd);
	driver.DestroyCommand(cmd);
}

void ALDriver::fn_Sourcef(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alSourcef(cmd->handle, cmd->param, cmd->args.fval);
	CHECK_AL_ERRORS(*cmd);
	driver.DestroyCommand(cmd);
}

void ALDriver::fn_Source3f(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alSource3f(cmd->handle, cmd->param, cmd->args.fvvals[0], cmd->args.fvvals[1], cmd->args.fvvals[2]);
	CHECK_AL_ERRORS(*cmd);
	driver.DestroyCommand(cmd);
}

void ALDriver::fn_SourceRewind(ALDriver &driver, Command *cmd) {
	if (driver.m_alc)
		alSourceRewind(cmd->handle);
	CHECK_AL_ERRORS(*cmd);
	driver.DestroyCommand(cmd);
}
