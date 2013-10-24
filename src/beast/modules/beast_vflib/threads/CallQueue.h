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

#ifndef BEAST_VFLIB_CALLQUEUE_H_INCLUDED
#define BEAST_VFLIB_CALLQUEUE_H_INCLUDED

namespace beast {

class CallQueue
: public BindableServiceQueue
{
public:
  explicit CallQueue (const String& name);

  ~CallQueue ();

  /** See if the caller is on the association thread.

      @return `true` if the calling thread of execution is associated with the
              queue.
  */
  bool isAssociatedWithCurrentThread () const;

protected:
  /** Synchronize the queue.

      A synchronize operation calls all functors in the queue.  If a functor
      causes additional functors to be added, they are eventually executed
      before synchronize() returns. Derived class call this when the queue is
      signaled, and optionally at any other time. Calling this function from
      more than one thread simultaneously is undefined.

      @return  true if any functors were executed.
  */
  virtual bool synchronize () = 0;

  /** Close the queue.

      Functors may not be added after this routine is called. This is used for
      diagnostics, to track down spurious calls during application shutdown
      or exit. Derived classes may call this if the appropriate time is known.

      The queue is synchronized after it is closed.
  */
  void close ();

  /** Call this to associate this queue with the callers thread. Subclasses
      must call this before calling synchronize.
  */
  void associateWithCurrentThread();

private:
  void doClose ();

  String m_name;
  Thread::ThreadID m_threadID;
  AtomicFlag m_closed;
};

}

#endif
