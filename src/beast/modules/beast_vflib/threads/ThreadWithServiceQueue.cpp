//------------------------------------------------------------------------------
/*
	This file is part of Beast: https://github.com/vinniefalco/Beast
	Copyright 2013, Vinnie Falco <vinnie.falco@gmail.com>

	Permission to use, copy, modify, and/or distribute this software for any
	purpose  with  or without fee is hereby granted, provided that the above
	copyright notice and this permission notice appear in all copies.

	THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
	WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
	MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
	ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
	WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
	ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
	OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
//==============================================================================

#include "ThreadWithServiceQueue.h"

namespace beast {

ThreadWithServiceQueue::ThreadWithServiceQueue (const String& name)
: CallQueue(name)
, m_thread(name, this)
, m_entryPoints(nullptr)
, m_calledStart(false)
, m_calledStop(false)
{
}

ThreadWithServiceQueue::~ThreadWithServiceQueue ()
{
	stop(true);
}

void ThreadWithServiceQueue::start (EntryPoints* const entryPoints)
{
	// start() MUST be called.
	bassert (!m_calledStart);
	m_calledStart = true;
	
	m_entryPoints = entryPoints;
	
	m_thread.startThread ();
}

void ThreadWithServiceQueue::stop (bool const wait)
{
	// start() MUST be called.
	bassert (m_calledStart);
	
	if (!m_calledStop)
	{
		m_calledStop = true;
		
		queue (&Thread::signalThreadShouldExit, &m_thread);
		
		// something could slip in here
		
		close ();
	}
	
	if (wait)
		m_thread.waitForThreadToExit ();
}

bool  ThreadWithServiceQueue::synchronize ()
{
	bassert (isAssociatedWithCurrentThread ());

	bool didSomething = false;

    while (! m_thread.threadShouldExit())
	{
		if(run_one() > 0)
			didSomething = true;
	}

	return didSomething;
}

void ThreadWithServiceQueue::runThread ()
{
	associateWithCurrentThread();

	m_entryPoints->threadInit();
	
	while (! m_thread.threadShouldExit())
		synchronize ();
	
	m_entryPoints->threadExit();
}

//------------------------------------------------------------------------------

namespace detail
{

//------------------------------------------------------------------------------

class BindableServiceQueueTests
: public UnitTest
{
public:
	
	struct BindableServiceQueueRunner
	: public ThreadWithServiceQueue::EntryPoints
	{
		ThreadWithServiceQueue m_worker;
		int cCallCount, c1CallCount;
		int initCalled, exitCalled;
		
		BindableServiceQueueRunner()
		: m_worker("BindableServiceQueueRunner")
		, cCallCount(0)
		, c1CallCount(0)
		, initCalled(0)
		, exitCalled(0)
		{
		}
		
		void start()
		{
			m_worker.start(this);
		}
		
		void stop()
		{
			m_worker.stop(true);
		}
		
		void c()
		{
			m_worker.queue(&BindableServiceQueueRunner::cImpl, this);
		}
		
		void cImpl()
		{
			cCallCount++;
		}
		
		void c1(int p1)
		{
			m_worker.queue(&BindableServiceQueueRunner::c1Impl, this, p1);
		}
		
		void c1Impl(int p1)
		{
			c1CallCount++;
		}

		void threadInit ()
		{
			initCalled++;
		}
					
		void threadExit ()
		{
			exitCalled++;
		}
	};
		
	static int const calls = 10000;
	
	void performCalls()
	{
		Random r;
		r.setSeedRandomly();
		
		BindableServiceQueueRunner runner;
		
		beginTestCase("Calls and interruptions");
								
		runner.start();
		
		for(std::size_t i=0; i<calls; i++)
		{
			int wait = r.nextLargeNumber(10).toInteger();
			
			if(wait % 2)
				runner.c();
			else
				runner.c1Impl(wait);
		}
		
		runner.stop();
		
		expect ((runner.cCallCount + runner.c1CallCount) == calls);
		expect (runner.initCalled == 1);
		expect (runner.exitCalled == 1);
	}
	
	void runTest()
	{
		performCalls ();
	}
	
	BindableServiceQueueTests () : UnitTest ("BindableServiceQueue", "beast")
	{
	}
};

static BindableServiceQueueTests bindableServiceQueueTests;
	
}

}
