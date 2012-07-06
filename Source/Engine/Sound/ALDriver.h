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
		
		virtual void tick(ALDriver &driver) = 0;

	private:

		ALDriver::WRef m_driver;
	};

	~ALDriver();

	//! Creates an alDriver object.
	static Ref create(ALDRIVER_PARAMS const char *deviceName);

	//! Adds a processing callback.
	void addCallback(Callback &callback);
	
	//! alcProcessContext
	void sync_process(ALDRIVER_VOID_PARAMS);
	//! alcSuspectContext
	void sync_suspend(ALDRIVER_VOID_PARAMS);
	//! Flushes the driver command buffer.
	void sync_flush(ALDRIVER_VOID_PARAMS);

	//! alDopplerFactor
	void dopplerFactor(ALDRIVER_PARAMS ALfloat factor);
	//! alSpeedOfSound
	void speedOfSound(ALDRIVER_PARAMS ALfloat speed);
	//! alDistanceModel
	void distanceModel(ALDRIVER_PARAMS ALenum value);

	//! alListenerfv
	void listenerfv(ALDRIVER_PARAMS ALenum param, ALfloat *values);
	//! alListenerf
	void listenerf(ALDRIVER_PARAMS ALenum param, ALfloat value);
	//! alListener3f
	void listener3f(ALDRIVER_PARAMS ALenum param, ALfloat x, ALfloat y, ALfloat z);

	//! alGenBuffers
	bool sync_genBuffers(ALDRIVER_PARAMS ALsizei n, ALuint *buffers);
	//! alDeleteBuffers
	void sync_deleteBuffers(ALDRIVER_PARAMS ALsizei n, ALuint *buffers);
	//! alBufferData
	bool sync_bufferData(
		ALDRIVER_PARAMS 
		ALuint buffer, 
		ALenum format, 
		const ALvoid *data, 
		ALsizei size,
		ALsizei freq
	);
	
	//! alGenSources
	bool sync_genSources(ALDRIVER_PARAMS ALsizei n, ALuint *sources);
	//! alDeleteSources
	void sync_deleteSources(ALDRIVER_PARAMS ALsizei n, ALuint *sources);
	//! alSourcePlay
	void sourcePlay(ALDRIVER_PARAMS ALuint source);
	//! alSourcePause
	void sourcePause(ALDRIVER_PARAMS ALuint source);
	//! alSourceStop
	void sourceStop(ALDRIVER_PARAMS ALuint source);
	//! alSourcei
	void sourcei(ALDRIVER_PARAMS ALuint source, ALenum param, ALint value);
	//! alSourcef
	void sourcef(ALDRIVER_PARAMS ALuint source, ALenum param, ALfloat value);
	//! alSource3f
	void source3f(ALDRIVER_PARAMS ALuint source, ALenum param, ALfloat x, ALfloat y, ALfloat z);
	//! alSourceRewind
	void sourceRewind(ALDRIVER_PARAMS ALuint source);

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
	Command *createCommand(Command::FN fn);
	//! Destroys a command object and returns it to its object pool.
	void destroyCommand(Command *cmd);

	//! Submits a command. The command is responsible for releasing itself after execution.
	void submit(Command &command);

	//! Wakes the driver to do processing.
	void wake();

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
	void removeCallback(Callback &callback);
	void exec(Command *head);
	void doCallbacks();

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

	static void fn_create(ALDriver &driver, Command *cmd);
	static void fn_process(ALDriver &driver, Command *cmd);
	static void fn_suspend(ALDriver &driver, Command *cmd);
	static void fn_flush(ALDriver &driver, Command *cmd);
	static void fn_dopplerFactor(ALDriver &driver, Command *cmd);
	static void fn_speedOfSound(ALDriver &driver, Command *cmd);
	static void fn_distanceModel(ALDriver &driver, Command *cmd);
	static void fn_listenerfv(ALDriver &driver, Command *cmd);
	static void fn_listenerf(ALDriver &driver, Command *cmd);
	static void fn_listener3f(ALDriver &driver, Command *cmd);
	static void fn_genBuffers(ALDriver &driver, Command *cmd);
	static void fn_deleteBuffers(ALDriver &driver, Command *cmd);
	static void fn_bufferData(ALDriver &driver, Command *cmd);
	static void fn_genSources(ALDriver &driver, Command *cmd);
	static void fn_deleteSources(ALDriver &driver, Command *cmd);
	static void fn_sourcePlay(ALDriver &driver, Command *cmd);
	static void fn_sourcePause(ALDriver &driver, Command *cmd);
	static void fn_sourceStop(ALDriver &driver, Command *cmd);
	static void fn_sourcei(ALDriver &driver, Command *cmd);
	static void fn_sourcef(ALDriver &driver, Command *cmd);
	static void fn_source3f(ALDriver &driver, Command *cmd);
	static void fn_sourceRewind(ALDriver &driver, Command *cmd);
};
