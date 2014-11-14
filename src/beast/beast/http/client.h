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

#ifndef BEAST_HTTP_CLIENT_H_INCLUDED
#define BEAST_HTTP_CLIENT_H_INCLUDED

#include <beast/asio/placeholders.h>
#include <beast/asio/streambuf.h>
#include <beast/http/message.h>
#include <beast/http/parser.h>
#include <beast/http/URL.h>
#include <beast/utility/Journal.h>
#include <boost/asio.hpp>
#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/container/flat_map.hpp>
#include <condition_variable>
#include <chrono>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace beast {
namespace http {

namespace detail {

struct null_log
{
    template <class T>
    null_log&
    operator<< (T const&) const
    {
        return *this;
    }
};

}

/** A boost::asio based asynchronous HTTP client. */
template <
    class Log = detail::null_log,
    class Allocator = std::allocator<char>
>
class basic_client
{
private:
    using error_code = boost::system::error_code;
    using address_type = boost::asio::ip::address;
    using endpoint_type = boost::asio::ip::tcp::endpoint;
    using socket_type = boost::asio::ip::tcp::socket;
    using stream_type = boost::asio::ssl::stream<socket_type&>;
    using context_type = boost::asio::ssl::context;

    class basic_child
    {
    protected:
        basic_client& client_;

        explicit
        basic_child (basic_client& client)
            : client_(client)
        {
        }

    public:
        virtual ~basic_child()
        {
            client_.remove(*this);
        }

        virtual void stop() = 0;
    };

    //--------------------------------------------------------------------------

    template <class Impl>
    class connection : public basic_child
    {
    private:
        using resolver_type = boost::asio::ip::tcp::resolver;
        using iterator_type = typename resolver_type::iterator;
        using query_type = typename resolver_type::query;
        using timer_type = boost::asio::basic_waitable_timer<
            std::chrono::steady_clock>;

        static std::size_t const readBytes = 8192;

    protected:
        boost::asio::io_service::strand strand_;
        socket_type socket_;
        timer_type timer_;

    private:
        resolver_type resolver_;
        asio::basic_streambuf<Allocator> read_buf_;
        asio::basic_streambuf<Allocator> write_buf_;
        http::parser parser_;
        http::message response_;
        asio::basic_streambuf<Allocator> body_;

    public:
        connection(basic_client& client)
            : basic_child (client)
            , strand_ (client_.io_service_)
            , socket_ (client_.io_service_)
            , timer_ (client_.io_service_)
            , resolver_ (client_.io_service_)
            , parser_ (response_, false,
                [this](void const* data, std::size_t bytes)
                {
                    this->body_.commit(boost::asio::buffer_copy(body_.prepare(
                        bytes), boost::asio::buffer(data, bytes)));
                })
        {
        }

        void
        async_get (URL const& uri)
        {
            do_resolve(uri);
        }

    protected:
        void
        do_resolve (URL const& uri)
        {
            query_type query (uri.host(), uri.scheme());
            resolver_.async_resolve (query, strand_.wrap(std::bind(
                &connection::on_resolve, impl().shared_from_this(),
                    beast::asio::placeholders::error,
                        beast::asio::placeholders::iterator,
                            uri)));
        }

        void
        do_request (URL const& uri)
        {
            http::message message;
            message.request (true);
            std::string path = uri.path() + uri.query() +
                uri.fragment() + uri.userinfo();
            if (path.empty())
                path = "/";
            message.url (path);
            message.method (http::method_t::http_get);
            message.version (1, 0);
            //message.headers.append("Connection", "Close");
            message.headers.append ("Host", uri.host());
            message.headers.append ("Content-Length", "0");
            write(write_buf_, message);

            boost::asio::async_read(impl().stream(),
                read_buf_.prepare(readBytes), boost::asio::transfer_at_least(1),
                    strand_.wrap(std::bind(&connection::on_read,
                        impl().shared_from_this(),
                            beast::asio::placeholders::error,
                                beast::asio::placeholders::bytes_transferred)));

            set_timer();
            boost::asio::async_write(impl().stream(), write_buf_.data(),
                boost::asio::transfer_at_least(1), strand_.wrap(std::bind(
                    &connection::on_write, impl().shared_from_this(),
                        beast::asio::placeholders::error,
                            beast::asio::placeholders::bytes_transferred)));
        }

    private:
        Impl&
        impl()
        {
            return *static_cast<Impl*>(this);
        }

        void
        set_timer()
        {
            error_code ec;
            timer_.expires_from_now(
                std::chrono::seconds(3), ec);
            if (ec)
            {
                client_.log_ << "set_timer: " << ec.message();
                return;
            }
            timer_.async_wait(strand_.wrap(std::bind(
                &connection::on_timer, impl().shared_from_this(),
                    beast::asio::placeholders::error)));
        }

        void
        cancel_timer()
        {
            error_code ec;
            timer_.cancel(ec);
        }

        void
        close (error_code ec)
        {
        }

        void
        fail (error_code ec)
        {
            socket_.close(ec);
            //timer_.cancel(ec);
        }

        void
        on_timer (error_code ec)
        {
            if (ec == boost::asio::error::operation_aborted)
                return;
            if (ec)
            {
                client_.log_ << "on_timer: " << ec.message();
                return;
            }
            client_.log_ << "timed out";

        }

        void
        on_resolve (error_code ec, iterator_type iter, URL const& uri)
        {
            if (ec)
            {
                if (ec != boost::asio::error::operation_aborted)
                    client_.log_ << "on_resolve: " << ec.message();
                return impl().handler_(ec, response_, body_);
            }
            assert(iter != iterator_type{});
            set_timer();
            socket_.async_connect(iter->endpoint(),strand_.wrap(std::bind(
                &Impl::on_connect, impl().shared_from_this(),
                    beast::asio::placeholders::error, uri)));
        }

        void
        on_connect (error_code ec, URL const& uri)
        {
            cancel_timer();

            if (ec)
            {
                if (ec != boost::asio::error::operation_aborted)
                    client_.log_ << "on_connect: " << ec.message();
                return impl().handler_(ec, response_, body_);
            }

            impl().do_connect(uri);
        }

        void
        on_read (error_code ec, std::size_t bytes_transferred)
        {
            read_buf_.commit(bytes_transferred);

            bool const eof = ec == boost::asio::error::eof;
            if (! ec || eof)
            {
                do
                {
                    if (! eof)
                    {
                        std::size_t consumed;
                        std::tie(ec, consumed) =
                            parser_.write (read_buf_.data());
                        read_buf_.consume (consumed);
                        bytes_transferred -= consumed;
                    }
                    else
                    {
                        ec = parser_.write_eof();
                    }

                    if (! ec && parser_.complete())
                    {
                        impl().handler_(ec, response_, body_);
                        response_ = http::message{};
                        parser_ = http::parser(response_, false,
                            [this](void const* data, std::size_t bytes)
                            {
                                this->body_.commit(boost::asio::buffer_copy(
                                    body_.prepare(bytes), boost::asio::buffer(
                                        data, bytes)));
                            });
                    }
                }
                while (bytes_transferred > 0);
            }

            if (ec)
            {
                if (ec != boost::asio::error::operation_aborted)
                    client_.log_ << "on_read: " << ec.message();
                return impl().handler_(ec, response_, body_);
            }

            if (eof)
                return;

            boost::asio::async_read(impl().stream(),
                read_buf_.prepare(readBytes), boost::asio::transfer_at_least(1),
                    strand_.wrap(std::bind(&connection::on_read,
                        impl().shared_from_this(),
                            beast::asio::placeholders::error,
                                beast::asio::placeholders::bytes_transferred)));
        }

        void
        on_write (error_code ec, std::size_t bytes_transferred)
        {
            cancel_timer();
            write_buf_.consume (bytes_transferred);

            if (ec)
            {
                if (ec != boost::asio::error::operation_aborted)
                    client_.log_ << "on_write: " << ec.message();
                return impl().handler_(ec, response_, body_);
            }

            if (write_buf_.size() > 0)
            {
                set_timer();
                return boost::asio::async_write(impl().stream(), write_buf_.data(),
                    boost::asio::transfer_at_least(1), strand_.wrap(std::bind(&
                        connection::on_write, impl().shared_from_this(),
                            beast::asio::placeholders::error,
                                beast::asio::placeholders::bytes_transferred)));
            }
        }
    };

    //--------------------------------------------------------------------------

    template <class Handler>
    class plain_connection
        : public connection<
            plain_connection<Handler>>
        , public std::enable_shared_from_this<
            plain_connection<Handler>>
    {
    public:
        Handler handler_;

        plain_connection(Handler&& handler, basic_client& client)
            : connection<plain_connection<Handler>>(client)
            , handler_(std::forward<Handler>(handler))
        {
        }

        void
        stop() override
        {
        }

        socket_type&
        stream()
        {
            return socket_;
        }

        void
        do_connect (URL const& uri)
        {
            do_request (uri);
        }
    };

    //--------------------------------------------------------------------------

    Log log_;
    boost::asio::io_service& io_service_;
    boost::container::flat_map<basic_child*,
        std::weak_ptr<basic_child>> list_;
    std::mutex mutex_;
    std::condition_variable cond_;
    http::headers headers_;

public:
    using allocator_type = Allocator;

    basic_client (boost::asio::io_service& io_service,
        Log log = Log{})
        : log_(log)
        , io_service_(io_service)
    {
    }

    ~basic_client()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(! list_.empty())
            cond_.wait(lock);
    }

    http::headers&
    headers()
    {
        return headers_;
    }

    http::headers const&
    headers() const
    {
        return headers_;
    }

    template <class Handler>
    void
    async_get(std::string const& uri_string,
        Handler&& handler)
    {
        auto const parsed = parse_URL(uri_string);
        if (! parsed.first)
            throw std::invalid_argument("invalid uri");
        URL const& uri = parsed.second;
        if (uri.scheme() == "http")
        {
            auto const p = std::make_shared<plain_connection<Handler>>(
                std::forward<Handler>(handler), *this);
            add(p);
            return p->async_get(uri);
        }
        else if (uri.scheme() == "https")
        {
        }

        throw std::invalid_argument("invalid uri scheme");
    }

private:
    void
    add (std::shared_ptr<basic_child> const& child)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        list_.emplace(child.get(), child);
    }

    void
    stop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        for(auto& _ : list_)
            if (auto sp = _.second.lock())
                sp->stop();
    }

    void
    remove(basic_child& child)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        list_.erase(&child);
        if(list_.empty())
            cond_.notify_one();
    }
};

}
}

#endif
