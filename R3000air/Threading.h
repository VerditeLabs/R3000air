/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef _THREADING_H_
#define _THREADING_H_

#include <errno.h> // EBUSY
#include <semaphore.h>

#include "Pcsx2Defs.h"
#include "Exceptions.h"

namespace Threading
{
	///////////////////////////////////////////////////////////////
	// Define some useful object handles - wait events, mutexes.

	struct WaitEvent
	{
		pthread_cond_t cond;
		pthread_mutex_t mutex;

		WaitEvent();
		~WaitEvent();

		void Set();
		void Wait();
	};

	struct Semaphore
	{
		sem_t sema;

		Semaphore();
		~Semaphore();

		void Reset();
		void Post();
		void Post( int multiple );
		void Wait();
		int Count();
	};

	struct MutexLock
	{
		pthread_mutex_t mutex;

		MutexLock();
		~MutexLock();

		void Lock();
		void Unlock();
	};

	// Returns the number of available logical CPUs (cores plus hyperthreaded cpus)
	extern void CountLogicalCores( int LogicalCoresPerPhysicalCPU, int PhysicalCoresPerPhysicalCPU );

	// Releases a timeslice to other threads.
	extern void Timeslice();

	// For use in spin/wait loops.
	extern void SpinWait();
	
	// sleeps the current thread for the given number of milliseconds.
	extern void Sleep( int ms );

	//////////////////////////////////////////////////////////////////////////////////////////
	// Thread - Helper class for the basics of starting/managing simple threads.
	//
	// Use this as a base class for your threaded procedure, and implement the 'int Callback()'
	// method.  Use Start() and Close() to start and shutdown the thread, and use m_post_event
	// internally to post/receive events for the thread (make a public accessor for it in your
	// derived class if your thread utilizes the post).
	//
	// Notes:
	//  * To ensure thread safety against C++'s bizarre and not-thread-friendly object
	//    constructors and destructors, you *must* use Start() and Close().  There is a built-
	//    in Close() called on destruction, which should work for very simple threads (that
	//    do not have any special shutdown code of their own), but 
	// 
	//  * Constructing threads as static vars isn't recommended since it can potentially con-
	//    fuse w32pthreads, if the static initializers are executed out-of-order (C++ offers
	//    no dependency options for ensuring correct static var initializations).
	//
	class Thread : NoncopyableObject
	{
	protected:
		typedef int (*PlainJoeFP)();
		pthread_t m_thread;
		int m_returncode;		// value returned from the thread on close.
		bool m_terminated;		// set true after the thread has been closed.
		Semaphore m_post_event;	// general wait event that's needed by most threads.

	public:
		virtual ~Thread();
		Thread();

		virtual void Start();
		virtual void Close();

		// Gets the return code of the thread.
		// Throws std::logic_error if the thread has not terminated.
		int GetReturnCode() const;

	protected:
		// Used to dispatch the thread callback function.
		// (handles some thread cleanup on Win32, and is basically a typecast
		// on linux).
		static void* _internal_callback( void* func );

		// Implemented by derived class to handle threading actions!
		virtual int Callback()=0;
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// ScopedLock: Helper class for using Mutexes.
	// Using this class provides an exception-safe (and generally clean) method of locking
	// code inside a function or conditional block.
	//
	class ScopedLock : NoncopyableObject
	{
	protected:
		MutexLock& m_lock;

	public:
		virtual ~ScopedLock()
		{
			m_lock.Unlock();
		}

		ScopedLock( MutexLock& locker ) :
		m_lock( locker )
		{
			m_lock.Lock();
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// BaseTaskThread - an abstract base class which provides simple parallel execution of
	// single tasks.
	//
	// Implementation:
	//   To use this class your derived class will need to implement its own Task() function
	//   and also a "StartTask( parameters )" function which suits the need of your task, along
	//   with any local variables your task needs to do its job.  You may additionally want to
	//   implement a "GetResult()" function, which would be a combination of WaitForResult()
	//   and a return value of the computational result.
	//
	// Thread Safety:
	//   If operating on local variables, you must execute WaitForResult() before leaving the
	//   variable scope -- or alternatively have your StartTask() implementation make full
	//   copies of dependent data.  Also, by default PostTask() always assumes the previous
	//   task has completed.  If your system can post a new task before the previous one has
	//   completed, then it needs to explicitly call WaitForResult() or provide a mechanism
	//   to cancel the previous task (which is probably more work than it's worth).
	//
	// Performance notes:
	//  * Remember that thread creation is generally slow, so you should make your object
	//    instance once early and then feed it tasks repeatedly over the course of program
	//    execution.
	//
	//  * For threading to be a successful speedup, the task being performed should be as lock
	//    free as possible.  For example using STL containers in parallel usually fails to
	//    yield any speedup due to the gratuitous amount of locking that the STL performs
	//    internally.
	//
	//  * The best application of tasking threads is to divide a large loop over a linear array
	//    into smaller sections.  For example, if you have 20,000 items to process, the task
	//    can be divided into two threads of 10,000 items each.
	// 
	class BaseTaskThread : public Thread
	{
	protected:
		volatile bool m_done;
		volatile bool m_TaskComplete;

	public:
		virtual ~BaseTaskThread() {}
		BaseTaskThread() :
			m_done( false )
		,	m_TaskComplete( false )
		{
		}

		// Tells the thread to exit and then waits for thread termination.
		// To force-terminate the thread without "nicely" waiting for the task to complete,
		// explicitly use the Thread::Close parent implementation instead.
		void Close()
		{
			if( m_terminated ) return;
			m_done = true;
			m_post_event.Post();
			pthread_join( m_thread, NULL );
		}
		
		// Initiates the new task.  This should be called after your own StartTask has
		// initialized internal variables / preparations for task execution.
		void PostTask()
		{
			jASSUME( !m_terminated );
			m_TaskComplete = false;
			m_post_event.Post();
		}

		// Blocks current thread execution pending the completion of the parallel task.
		void WaitForResult() const
		{
			if( m_terminated ) return;
			while( !m_TaskComplete )
			{
				Timeslice();
				SpinWait();
			}
		}
		
	protected:
		// Abstract method run when a task has been posted.  Implementing classes should do
		// all your necessary processing work here.
		virtual void Task()=0;

		int Callback()
		{
			do
			{
				// Wait for a job!
				m_post_event.Wait();

				if( m_done ) break;
				Task();
				m_TaskComplete = true;
			} while( !m_done );
			
			return 0;
		}
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	// Our fundamental interlocking functions.  All other useful interlocks can be derived
	// from these little beasties!

	extern long pcsx2_InterlockedExchange(volatile long* Target, long srcval);
	extern long pcsx2_InterlockedCompareExchange( volatile long* target, long srcval, long comp );
	extern long pcsx2_InterlockedExchangeAdd( volatile long* target, long addval );

	extern void AtomicExchange( volatile u32& Target, u32 value );
	extern void AtomicExchangeAdd( volatile u32& Target, u32 value );
	extern void AtomicIncrement( volatile u32& Target );
	extern void AtomicDecrement( volatile u32& Target );
	extern void AtomicExchange( volatile s32& Target, s32 value );
	extern void AtomicExchangeAdd( volatile s32& Target, u32 value );
	extern void AtomicIncrement( volatile s32& Target );
	extern void AtomicDecrement( volatile s32& Target );

	extern void _AtomicExchangePointer( const void ** target, const void* value );
	extern void _AtomicCompareExchangePointer( const void ** target, const void* value, const void* comparand );

	#define AtomicExchangePointer( target, value ) \
		_AtomicExchangePointer( (const void**)(&target), (const void*)(value) )

	#define AtomicCompareExchangePointer( target, value, comparand ) \
		_AtomicCompareExchangePointer( (const void**)(&target), (const void*)(value), (const void*)(comparand) )

}

#endif
