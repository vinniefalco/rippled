//------------------------------------------------------------------------------
/*
	This file is part of Beast: https://github.com/vinniefalco/Beast
	Copyright 2013, Vinnie Falco <vinnie.falco@gmail.com>
    Copyright Patrick Dehne <patrick@mysonicweb.de> (www.sonicweb-radio.de)

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

#ifndef BEAST_VFLIB_CALLQUEUE_H_INCLUDED
#define BEAST_VFLIB_CALLQUEUE_H_INCLUDED

#include "beast/Threads.h"
#include "../functor/BindHelper.h"

namespace beast {

template <class Allocator = std::allocator <char> >
class CallQueueType
: public ServiceQueueType <Allocator>
{
public:
    explicit CallQueueType (const String& name,
                        int expectedConcurrency = 1,
                        Allocator alloc = Allocator())
    : ServiceQueueType<Allocator>(expectedConcurrency, alloc)
    , m_name (name)
    , queue(*this)
    , call(*this)
    {
    }
    
    ~CallQueueType ()
    {
        // Someone forget to close the queue.
        bassert (m_closed.isSignaled ());
        
        // Can't destroy queue with unprocessed calls.
        bassert (detail::ServiceQueueBase::empty ());
    }
    
    void enqueue (detail::ServiceQueueBase::Item* item)
    {
        // If this goes off someone added calls
        // after the queue has been closed.
        bassert (!m_closed.isSignaled ());
        
        ServiceQueueType <Allocator>::enqueue (item);
    }
    
    /** Close the queue.
     
     Functors may not be added after this routine is called. This is used for
     diagnostics, to track down spurious calls during application shutdown
     or exit. Derived classes may call this if the appropriate time is known.
     
     The queue is synchronized after it is closed.
     Can still have pending calls, just can't put new ones in.
     */
    void close ()
    {
        m_closed.signal ();
        
        ServiceQueueType <Allocator>::stop ();
    }
    
	struct BindHelperPost
	{
		CallQueueType<Allocator>& m_queue;
		explicit BindHelperPost (CallQueueType<Allocator>& queue)
		: m_queue (queue)
		{ }
		template <typename F>
		void operator() (F const& f) const
		{ m_queue.post ( F (f) ); }
	};
    
	struct BindHelperDispatch
	{
		CallQueueType<Allocator>& m_queue;
		explicit BindHelperDispatch (CallQueueType<Allocator>& queue)
		: m_queue (queue)
		{ }
		template <typename F>
		void operator() (F const& f) const
		{ m_queue.dispatch ( F (f) ); }
	};
    
	BindHelper <BindHelperPost> const queue;
	BindHelper <BindHelperDispatch> const call;
    
private:
    String m_name;
    AtomicFlag m_closed;
};

typedef CallQueueType <std::allocator <char> > CallQueue;

namespace detail
{
    
    //------------------------------------------------------------------------------
    
class CallQueueTests
: public UnitTest
{
public:
	struct CallTracker
    {
        int c0, c1, c2, c3, c4, c5;
        int q0, q1, q2, q3, q4, q5;
        
        void doQ0() { q0++; }
        
        void doQ1(const String& p1)
        {
            bassert(p1 == "p1");
            q1++;
        }
        
        void doQ2(const String& p1, const String& p2)
        {
            bassert(p1 == "p1");
            bassert(p2 == "p2");
            q2++;
        }
        
        void doQ3(const String& p1, const String& p2, const String& p3)
        {
            bassert(p1 == "p1");
            bassert(p2 == "p2");
            bassert(p3 == "p3");
            q3++;
        }
        
        void doQ4(const String& p1, const String& p2, const String& p3, const String& p4)
        {
            bassert(p1 == "p1");
            bassert(p2 == "p2");
            bassert(p3 == "p3");
            bassert(p4 == "p4");
            q4++;
        }
        
        void doQ5(const String& p1, const String& p2, const String& p3, const String& p4, const String& p5)
        {
            bassert(p1 == "p1");
            bassert(p2 == "p2");
            bassert(p3 == "p3");
            bassert(p4 == "p4");
            bassert(p5 == "p5");
            q5++;
        }
        
        void doC0() { c0++; }
        
        void doC1(const String& p1)
        {
            bassert(p1 == "p1");
            c1++;
        }
        
        void doC2(const String& p1, const String& p2)
        {
            bassert(p1 == "p1");
            bassert(p2 == "p2");
            c2++;
        }
        
        void doC3(const String& p1, const String& p2, const String& p3)
        {
            bassert(p1 == "p1");
            bassert(p2 == "p2");
            bassert(p3 == "p3");
            c3++;
        }
        
        void doC4(const String& p1, const String& p2, const String& p3, const String& p4)
        {
            bassert(p1 == "p1");
            bassert(p2 == "p2");
            bassert(p3 == "p3");
            bassert(p4 == "p4");
            c4++;
        }
        
        void doC5(const String& p1, const String& p2, const String& p3, const String& p4, const String& p5)
        {
            bassert(p1 == "p1");
            bassert(p2 == "p2");
            bassert(p3 == "p3");
            bassert(p4 == "p4");
            bassert(p5 == "p5");
            c5++;
        }
    };
    
    CallTracker m_callTracker;
    
    void testArities ()
	{
        beginTestCase("Arities");
        
        int calls = 0;
        
        m_queue.queue(&CallTracker::doQ0, &m_callTracker); calls++;
        m_queue.queue(&CallTracker::doQ1, &m_callTracker, "p1"); calls++;
        m_queue.queue(&CallTracker::doQ2, &m_callTracker, "p1", "p2"); calls++;
        m_queue.queue(&CallTracker::doQ3, &m_callTracker, "p1", "p2", "p3"); calls++;
        m_queue.queue(&CallTracker::doQ4, &m_callTracker, "p1", "p2", "p3", "p4"); calls++;
        m_queue.queue(&CallTracker::doQ5, &m_callTracker, "p1", "p2", "p3", "p4", "p5"); calls++;
        
        m_queue.queue(&CallTracker::doC0, &m_callTracker); calls++;
        m_queue.queue(&CallTracker::doC1, &m_callTracker, "p1"); calls++;
        m_queue.queue(&CallTracker::doC2, &m_callTracker, "p1", "p2"); calls++;
        m_queue.queue(&CallTracker::doC3, &m_callTracker, "p1", "p2", "p3"); calls++;
        m_queue.queue(&CallTracker::doC4, &m_callTracker, "p1", "p2", "p3", "p4"); calls++;
        m_queue.queue(&CallTracker::doC5, &m_callTracker, "p1", "p2", "p3", "p4", "p5"); calls++;
        
        std::size_t performedCalls = m_queue.poll();
        
        m_queue.close();
        
        expect (performedCalls == calls);
        
        expect (m_callTracker.c0 == 1);
        expect (m_callTracker.c1 == 1);
        expect (m_callTracker.c2 == 1);
        expect (m_callTracker.c3 == 1);
        expect (m_callTracker.c4 == 1);
        expect (m_callTracker.c5 == 1);
        
        expect (m_callTracker.q0 == 1);
        expect (m_callTracker.q1 == 1);
        expect (m_callTracker.q2 == 1);
        expect (m_callTracker.q3 == 1);
        expect (m_callTracker.q4 == 1);
        expect (m_callTracker.q5 == 1);
    }
    
    void runTest()
    {
        testArities();
    }
    
    CallQueueTests ()
    : UnitTest ("CallQueue", "beast")
    , m_queue("CallQueue Test Queue")
    {
    }
    
    CallQueue m_queue;
};

}

}

#endif
