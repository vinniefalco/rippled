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

#ifndef BEAST_VFLIB_GUISERVICEQUEUE_H_INCLUDED
#define BEAST_VFLIB_GUISERVICEQUEUE_H_INCLUDED

#include "AppConfig.h"
#include "modules/juce_core/juce_core.h"
#include "modules/juce_events/juce_events.h"

#include "CallQueue.h"

namespace beast {

class GuiServiceQueue
: public CallQueue
, private juce::AsyncUpdater
, private ThreadWithServiceQueue::EntryPoints
{
public:
	explicit GuiServiceQueue (const String& name)
    : CallQueue(name)
    , m_thread(name)
    {
        bassert (juce::MessageManager::getInstance()->isThisTheMessageThread());
        
        m_thread.start (this);
    }
    
    void close ()
    {
        m_thread.stop (true);
        
        CallQueue::close ();
    }
    
    void enqueue (detail::ServiceQueueBase::Item* item)
    {
        CallQueue::enqueue (item);
        
        m_thread.call (&juce::AsyncUpdater::triggerAsyncUpdate, (AsyncUpdater*)this);
    }
    
    void handleAsyncUpdate()
    {
        poll();
    }
    
private:
    ThreadWithServiceQueue m_thread;
};

}

#endif
