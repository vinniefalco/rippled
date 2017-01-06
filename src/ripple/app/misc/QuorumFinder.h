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

#ifndef RIPPLE_APP_QUORUMFINDER_H_INCLUDED
#define RIPPLE_APP_QUORUMFINDER_H_INCLUDED

#include <ripple/beast/utility/Journal.h>
#include <ripple/json/json_value.h>
#include <beast/unit_test/dstream.hpp>
#include <selene.h>

namespace ripple {

class QuorumFinder
{
    beast::Journal j_;
    sel::State ss_;

public:
    static
    Json::Value
    makeAttestation()
    {
        Json::Value jv;
        jv["sequence"] = 1;
        auto& ja = jv["validators"];
        for(int i = 1; i <= 3; ++i)
        {
            auto& v = ja.append(Json::objectValue);
            v["validation_public_key"] = "vpk" + std::to_string(i);;
            switch(i)
            {
            default:
            case 1: v["jurisdiction"] = "usa"; break;
            case 2: v["jurisdiction"] = "china"; break;
            case 3: v["jurisdiction"] = "eu"; break;
            }
        }
    }

    explicit
    QuorumFinder(beast::Journal j)
        : j_(j)
    {
        ss_["print"] =
            [this](std::string const& s)
            {
            #if 1
                beast::unit_test::dstream dout{std::cout};
                dout << s << "\n";
            #else
                j_.info() << s;
            #endif
            };

        ss_.Load("D:\\policy.lua");

    }

    void
    onAttestation(Json::Value const& jv)
    {
    }
};

} // ripple

#endif
