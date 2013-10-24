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

#include "CallQueue.h"

namespace beast {

CallQueue::CallQueue (const String& name)
: m_name (name)
, m_threadID (0)
{
}

CallQueue::~CallQueue ()
{
  // Someone forget to close the queue.
  bassert (m_closed.isSignaled ());

  // TODO how to implement this assert?

  // Can't destroy queue with unprocessed calls.
  // SharedState::Access state (m_state);
  // bassert (state->handlers.empty())
}

bool CallQueue::isAssociatedWithCurrentThread () const
{
  return Thread::getCurrentThreadId () == m_threadID;
}

// Can still have pending calls, just can't put new ones in.
void CallQueue::close ()
{
  m_closed.signal ();

  stop();
}

void CallQueue::associateWithCurrentThread()
{
  m_threadID = Thread::getCurrentThreadId ();
}

}
