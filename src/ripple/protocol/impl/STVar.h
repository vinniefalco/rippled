//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

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

#ifndef RIPPLE_PROTOCOL_STVAR_H_INCLUDED
#define RIPPLE_PROTOCOL_STVAR_H_INCLUDED

#include <ripple/protocol/STBase.h>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <typeinfo>

#include <mutex>
#include <unordered_map>
#include <beast/streams/debug_ostream.h>
#include <beast/utility/static_initializer.h>

namespace ripple {
namespace detail {

// "variant" that can hold any type of serialized object
// and includes a small-object allocation optimization.
class STVar
{
private:
    std::aligned_storage<72>::type d_;
    STBase* p_ = nullptr;

    struct Log
    {
        std::mutex mutex_;
        std::unordered_map<
            std::size_t, std::size_t> map_;

        ~Log()
        {
            beast::debug_ostream os;
            for(auto const& e : map_)
                os << e.first << "," << e.second;
        }

        void
        operator() (std::size_t n)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto const result = map_.emplace(n, 1);
            if (! result.second)
                ++result.first->second;
        }
    };

public:
    ~STVar()
    {
        destroy();
    }

    STVar (STVar const& other)
    {
        p_ = other.p_->copy(
            sizeof(d_), &d_);
    }

    STVar (STVar&& other)
    {
        p_ = other.p_->move(
            sizeof(d_), &d_);
    }

    STVar&
    operator= (STVar const& rhs)
    {
        destroy();
        p_ = rhs.p_->copy(
            sizeof(d_), &d_);
        return *this;
    }

    STVar&
    operator= (STVar&& rhs)
    {
        destroy();
        if (rhs.on_heap())
        {
            p_ = rhs.p_;
            rhs.p_ = nullptr;
        }
        else
        {
            p_ = rhs.p_->move(
                sizeof(d_), &d_);
        }
        return *this;
    }

    STVar (STBase&& t)
    {
        p_ = t.move(sizeof(d_), &d_);
    }

    STVar (STBase const& t)
    {
        p_ = t.copy(sizeof(d_), &d_);
    }

    STBase&
    get()
    {
        return *p_;
    }

    STBase const&
    get() const
    {
        return *p_;
    }

    STBase&
    operator*()
    {
        return get();
    }

    STBase const&
    operator*() const
    {
        return get();
    }

    STBase*
    operator->()
    {
        return &get();
    }

    STBase const*
    operator->() const
    {
        return &get();
    }

private:
    STVar() = default;

    bool
    on_heap() const
    {
        return static_cast<void const*>(p_) !=
            static_cast<void const*>(&d_);
    }

    void
    destroy()
    {
    #if 0
        // Turn this on to get a histogram on exit
        if (p_ != nullptr)
        {
            static beast::static_initializer<Log> log;
            (*log)(p_->size_of());
        }
    #endif

        if (on_heap())
            delete p_;
        else
            p_->~STBase();
    }
};

inline
bool
operator== (STVar const& lhs, STVar const& rhs)
{
    return lhs.get().isEquivalent(rhs.get());
}

inline
bool
operator!= (STVar const& lhs, STVar const& rhs)
{
    return ! (lhs == rhs);
}

} // detail
} // ripple

#endif
