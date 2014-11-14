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

#include <beast/http/client.h>
#include <beast/unit_test/suite.h>
#include <functional>

namespace beast {
namespace http {

class client_test : public unit_test::suite
{
public:
    using error_code = boost::system::error_code;

    void
    on_get(error_code ec, beast::http::message& message,
        beast::asio::streambuf& body)
    {
        if(ec)
        {
            log << "on_get: " << ec.message();
        }
        else
        {
            log << to_string(message);
            log << to_string(body);
        }
    }

    void
    run()
    {
        boost::asio::io_service ios;
        {
            basic_client<log_type> client(ios, log);

            //client.async_get("http://ripplelabs.com",
            client.async_get("http://zerohedge.com",
            //client.async_get("http://limewire.com",
                std::bind(&client_test::on_get, this, std::placeholders::_1,
                    std::placeholders::_2, std::placeholders::_3));
            ios.run();
        }

        pass();
    }
};

BEAST_DEFINE_TESTSUITE(client,http,beast);

}
}
