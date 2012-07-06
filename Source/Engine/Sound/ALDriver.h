// ALDriver.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "ALDriverDef.h"
#include <Runtime/Thread.h>
#include <Runtime/Thread/Locks.h>
#include <Runtime/Base/ObjectPool.h>
#include <Runtime/Container/ZoneSet.h>
#include <OpenAL/al.h>
#include <OpenAL/alc.h>


#if defined(RAD_OPT_ALERRORS)

#define ALDRIVER_SIG __FILE__, __LINE__,
#define ALDRIVER_VOID_SIG __FILE__, __LINE__
#define ALDRIVER_PARAMS const char *__file, int __line,
#define ALDRIVER_VOID_PARAMS const char *__file, int __line
#define ALDRIVER_ARGS __file, __line,
#define ALDRIVER_VOID_ARGS __file, __line
#define ALDRIVER_ERRORS
#define ALDRIVER_CMD_SIG(c) \
	(c).file = __file; \
	(c).line = __line
#define ALDRIVER_CMD_SIG_SRC2(c, _x, _y) \
	(c).file = _x; \
	(c).line = _y
#define ALDRIVER_CMD_SIG_SRC(c) ALDRIVER_CMD_SIG_SRC2(c, __FILE__, __LINE__)
#define CHECK_AL_ERRORS(c) CheckALErrors((c).file, (c).line)
#define CLEAR_AL_ERRORS() ClearALErrors()

#else

#define ALDRIVER_SIG
#define ALDRIVER_VOID_SIG
#define ALDRIVER_PARAMS
#define ALDRIVER_VOID_PARAMS
#define ALDRIVER_ARGS
#define ALDRIVER_VOID_ARGS
#define ALDRIVER_CMD_SIG(c)
#define ALDRIVER_CMD_SIG_SRC(c)
#define CHECK_AL_ERRORS(c) CLEAR_AL_ERRORS()
#define CLEAR_AL_ERRORS() ClearALErrors()

#endif

void ClearALErrors();
bool CheckALErrors(const char *file, int line);


//! Multithreaded OpenAL driver command stream.
/*! This class is designed to minimize the locking and thread contention during music
	streaming. Streaming sounds must be decoded and queued on a seperate thread forcing
	all calls into OpenAL to be synchronized. The ALDriver is an AL command stream that
	runs on its own thread where all AL commands are submitted and music streaming occurs.
*/
class ALDriver : protected thread::Thread {
public:
	typedef ALDriverRef Ref;
	typedef ALDriverWRef WRef;

	//! Processing callback.
	/*! Callbacks are issued each time through the driver command stream. You are free
		to execute direct OpenAL calls. This is a time sensative operation, performing
		long operations may cause hickups in sound playback. 
		
		\note Callback objects are automatically removed from the alDriver object when
		their last shared_ptr is destroyed. 

		A null device can be bound. Check the context to see if it is non-null before
		calling OpenAL functions.
	*/
	class Callback {
	public:
		typedef boost::shared_ptr<Callback> Ref;

		Callback(const ALDriver::Ref &driver);
		~Callback();
		
		virtual void Tick(ALDriver &driver) = 0;

	private:

		ALDriver::WRef m_driver;
	};

	~ALDriver();

	//! Creates an alDriver object.
	static Ref New(ALDRIVER_PARAMS const char *deviceName);

	//! Adds a processing callback.
	void AddCallback(Callback &callback);
	
	//! alcProcessContext
	void SyncProcess(ALDRIVER_VOID_PARAMS);
	//! alcSuspectContext
	void SyncSuspend(ALDRIVER_VOID_PARAMS);
	//! Flushes the driver command buffer.
	void SyncFlush(ALDRIVER_VOID_PARAMS);

	//! alDopplerFactor
	void DopplerFactor(ALDRIVER_PARAMS ALfloat factor);
	//! alSpeedOfSound
	void SpeedOfSound(ALDRIVER_PARAMS ALfloat speed);
	//! alDistanceModel
	void DistanceModel(ALDRIVER_PARAMS ALenum value);

	//! alListenerfv
	void Listenerfv(ALDRIVER_PARAMS ALenum param, ALfloat *values);
	//! alListenerf
	void Listenerf(ALDRIVER_PARAMS ALenum param, ALfloat value);
	//! alListener3f
	void Listener3f(ALDRIVER_PARAMS ALenum param, ALfloat x, ALfloat y, ALfloat z);

	//! alGenBuffers
	bool SyncGenBuffers(ALDRIVER_PARAMS ALsizei n, ALuint *buffers);
	//! alDeleteBuffers
	void SyncDeleteBuffers(ALDRIVER_PARAMS ALsizei n, ALuint *buffers);
	//! alBufferData
	bool SyncBufferData(
		ALDRIVER_PARAMS 
		ALuint buffer, 
		ALenum format, 
		const ALvoid *data, 
		ALsizei size,
		ALsizei freq
	);
	
	//! alGenSources
	bool SyncGenSources(ALDRIVER_PARAMS ALsizei n, ALuint *sources);
	//! alDeleteSources
	void SyncDeleteSources(ALDRIVER_PARAMS ALsizei n, ALuint *sources);
	//! alSourcePlay
	void SourcePlay(ALDRIVER_PARAMS ALuint source);
	//! alSourcePause
	void SourcePause(ALDRIVER_PARAMS ALuint source);
	//! alSourceStop
	void SourceStop(ALDRIVER_PARAMS ALuint source);
	//! alSourcei
	void Sourcei(ALDRIVER_PARAMS ALuint source, ALenum param, ALint value);
	//! alSourcef
	void Sourcef(ALDRIVER_PARAMS ALuint source, ALenum param, ALfloat value);
	//! alSource3f
	void Source3f(ALDRIVER_PARAMS ALuint source, ALenum param, ALfloat x, ALfloat y, ALfloat z);
	//! alSourceRewind
	void SourceRewind(ALDRIVER_PARAMS ALuint source);

	//! Command class.
	/*! Driver commands are executed on the OpenAL thread. They should call OpenAL API methods
		directly. You can write custom commands to insert into the command stream.

		\note A null device can be bound. Check the context to see if it is non-null before
		calling OpenAL functions.
	*/
	class Command {
	public:
		typedef void (*FN) (ALDriver &driver, Command *cmd);

		Command(FN fn) : m_fn(fn) {}

#if defined(ALDRIVER_ERRORS)
		const char *file;
		int line;
#endif

		ALuint handle;
		ALenum param;

		union {
			ALint ival;
			ALuint *puint;
			ALfloat fval;
			ALfloat fvvals[6];
			void *pvoid;
		} args;

	private:

		friend class ALDriver;

		Command *m_next;
		FN m_fn;
	};

	//! Creates a command object from an object pool.
	Command *CreateCommand(Command::FN fn);
	//! Destroys a command object and returns it to its object pool.
	void DestroyCommand(Command *cmd);

	//! Submits a command. The command is responsible for releasing itself after execution.
	void Submit(Command &command);

	//! Wakes the driver to do processing.
	void Wake();

	RAD_DECLARE_READONLY_PROPERTY(ALDriver, device, ALCdevice*);
	RAD_DECLARE_READONLY_PROPERTY(ALDriver, context, ALCcontext*);
	RAD_DECLARE_READONLY_PROPERTY(ALDriver, suspended, bool);

private:

	friend class Callback;

	virtual int ThreadProc();

	typedef boost::mutex Mutex;
	typedef boost::lock_guard<Mutex> Lock;
	typedef ThreadSafeObjectPool<Command> CommandPool;
	typedef zone_set<Callback*, ZSoundT>::type CallbackSet;

	class MutexedCommand : public Command {
	public:
		MutexedCommand(Command::FN fn) : Command(fn) {}

		thread::EventMutex m;
	};

	class GenCommand : public Command {
	public:
		GenCommand(Command::FN fn) : Command(fn) {}

		ALsizei num;
	};

	class BufferDataCommand : public Command {
	public:
		BufferDataCommand(Command::FN fn) : Command(fn) {}

		ALenum format;
		ALsizei size;
		ALsizei freq;
	};

	ALDriver();
	void RemoveCallback(Callback &callback);
	void Exec(Command *head);
	void DoCallbacks();

	RAD_DECLARE_GET(device, ALCdevice*) { return m_ald; }
	RAD_DECLARE_GET(context, ALCcontext*) { return m_alc; }
	RAD_DECLARE_GET(suspended, bool) { return m_suspended; }

	CommandPool m_cmdPool;
	CallbackSet m_callbacks;
	Mutex m_m, m_mcb;
	thread::Semaphore m_sema;
	Command *m_head;
	Command *m_tail;
	ALCdevice *m_ald;
	ALCcontext *m_alc;
	bool m_quit;
	bool m_suspended;

	static void fn_Create(ALDriver &driver, Command *cmd);
	static void fn_Process(ALDriver &driver, Command *cmd);
	static void fn_Suspend(ALDriver &driver, Command *cmd);
	static void fn_Flush(ALDriver &driver, Command *cmd);
	static void fn_DopplerFactor(ALDriver &driver, Command *cmd);
	static void fn_SpeedOfSound(ALDriver &driver, Command *cmd);
	static void fn_DistanceModel(ALDriver &driver, Command *cmd);
	static void fn_Listenerfv(ALDriver &driver, Command *cmd);
	static void fn_Listenerf(ALDriver &driver, Command *cmd);
	static void fn_Listener3f(ALDriver &driver, Command *cmd);
	static void fn_GenBuffers(ALDriver &driver, Command *cmd);
	static void fn_DeleteBuffers(ALDriver &driver, Command *cmd);
	static void fn_BufferData(ALDriver &driver, Command *cmd);
	static void fn_GenSources(ALDriver &driver, Command *cmd);
	static void fn_DeleteSources(ALDriver &driver, Command *cmd);
	static void fn_SourcePlay(ALDriver &driver, Command *cmd);
	static void fn_SourcePause(ALDriver &driver, Command *cmd);
	static void fn_SourceStop(ALDriver &driver, Command *cmd);
	static void fn_Sourcei(ALDriver &driver, Command *cmd);
	static void fn_Sourcef(ALDriver &driver, Command *cmd);
	static void fn_Source3f(ALDriver &driver, Command *cmd);
	static void fn_SourceRewind(ALDriver &driver, Command *cmd);
};
