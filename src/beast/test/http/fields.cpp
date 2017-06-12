//
// Copyright (c) 2013-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// Test that header file is self-contained.
#include <beast/http/fields.hpp>

#include <beast/test/test_allocator.hpp>
#include <beast/unit_test/suite.hpp>
#include <boost/lexical_cast.hpp>

namespace beast {
namespace http {

class fields_test : public beast::unit_test::suite
{
public:
    template<class Allocator>
    using fa_t = basic_fields<Allocator>;

    using f_t = fa_t<std::allocator<char>>;

    template<class Allocator>
    static
    void
    fill(std::size_t n, basic_fields<Allocator>& f)
    {
        for(std::size_t i = 1; i<= n; ++i)
            f.insert(boost::lexical_cast<std::string>(i), i);
    }

    template<class U, class V>
    static
    void
    self_assign(U& u, V&& v)
    {
        u = std::forward<V>(v);
    }

    template<class Alloc>
    static
    bool
    empty(basic_fields<Alloc> const& f)
    {
        return f.begin() == f.end();
    }

    template<class Alloc>
    static
    std::size_t
    size(basic_fields<Alloc> const& f)
    {
        return std::distance(f.begin(), f.end());
    }

    void
    testMembers()
    {
        using namespace test;

        // compare equal
        using equal_t = test::test_allocator<char,
            true, true, true, true, true>;

        // compare not equal
        using unequal_t = test::test_allocator<char,
            false, true, true, true, true>;

        // construction
        {
            {
                fields f;
                BEAST_EXPECT(f.begin() == f.end());
            }
            {
                unequal_t a1;
                basic_fields<unequal_t> f{a1};
                BEAST_EXPECT(f.get_allocator() == a1);
                BEAST_EXPECT(f.get_allocator() != unequal_t{});
            }
        }

        // move construction
        {
            {
                basic_fields<equal_t> f1;
                BEAST_EXPECT(f1.get_allocator()->nmove == 0);
                f1.insert("1", "1");
                BEAST_EXPECT(f1["1"] == "1");
                basic_fields<equal_t> f2{std::move(f1)};
                BEAST_EXPECT(f2.get_allocator()->nmove == 1);
                BEAST_EXPECT(f2["1"] == "1");
                BEAST_EXPECT(f1["1"] == "");
            }
            // allocators equal
            {
                basic_fields<equal_t> f1;
                f1.insert("1", "1");
                equal_t a;
                basic_fields<equal_t> f2{std::move(f1), a};
                BEAST_EXPECT(f2["1"] == "1");
                BEAST_EXPECT(f1["1"] == "");
            }
            {
                // allocators unequal
                basic_fields<unequal_t> f1;
                f1.insert("1", "1");
                unequal_t a;
                basic_fields<unequal_t> f2{std::move(f1), a};
                BEAST_EXPECT(f2["1"] == "1");
            }
        }

        // copy construction
        {
            {
                basic_fields<equal_t> f1;
                f1.insert("1", "1");
                basic_fields<equal_t> f2{f1};
                BEAST_EXPECT(f1.get_allocator() == f2.get_allocator());
                BEAST_EXPECT(f1["1"] == "1");
                BEAST_EXPECT(f2["1"] == "1");
            }
            {
                basic_fields<unequal_t> f1;
                f1.insert("1", "1");
                unequal_t a;
                basic_fields<unequal_t> f2(f1, a);
                BEAST_EXPECT(f1.get_allocator() != f2.get_allocator());
                BEAST_EXPECT(f1["1"] == "1");
                BEAST_EXPECT(f2["1"] == "1");
            }
            {
                basic_fields<equal_t> f1;
                f1.insert("1", "1");
                basic_fields<unequal_t> f2(f1);
                BEAST_EXPECT(f1["1"] == "1");
                BEAST_EXPECT(f2["1"] == "1");
            }
            {
                basic_fields<unequal_t> f1;
                f1.insert("1", "1");
                equal_t a;
                basic_fields<equal_t> f2(f1, a);
                BEAST_EXPECT(f2.get_allocator() == a);
                BEAST_EXPECT(f1["1"] == "1");
                BEAST_EXPECT(f2["1"] == "1");
            }
        }

        // move assignment
        {
            {
                fields f1;
                f1.insert("1", "1");
                fields f2;
                f2 = std::move(f1);
                BEAST_EXPECT(f1.begin() == f1.end());
                BEAST_EXPECT(f2["1"] == "1");
            }
            {
                // propagate_on_container_move_assignment : true
                using pocma_t = test::test_allocator<char,
                    true, true, true, true, true>;
                basic_fields<pocma_t> f1;
                f1.insert("1", "1");
                basic_fields<pocma_t> f2;
                f2 = std::move(f1);
                BEAST_EXPECT(f1.begin() == f1.end());
                BEAST_EXPECT(f2["1"] == "1");
            }
            {
                // propagate_on_container_move_assignment : false
                using pocma_t = test::test_allocator<char,
                    true, true, false, true, true>;
                basic_fields<pocma_t> f1;
                f1.insert("1", "1");
                basic_fields<pocma_t> f2;
                f2 = std::move(f1);
                BEAST_EXPECT(f1.begin() == f1.end());
                BEAST_EXPECT(f2["1"] == "1");
            }
        }

        // copy assignment
        {
            {
                fields f1;
                f1.insert("1", "1");
                fields f2;
                f2 = f1;
                BEAST_EXPECT(f1["1"] == "1");
                BEAST_EXPECT(f2["1"] == "1");
                basic_fields<equal_t> f3;
                f3 = f2;
                BEAST_EXPECT(f3["1"] == "1");
            }
            {
                // propagate_on_container_copy_assignment : true
                using pocca_t = test::test_allocator<char,
                    true, true, true, true, true>;
                basic_fields<pocca_t> f1;
                f1.insert("1", "1");
                basic_fields<pocca_t> f2;
                f2 = f1;
                BEAST_EXPECT(f2["1"] == "1");
            }
            {
                // propagate_on_container_copy_assignment : false
                using pocca_t = test::test_allocator<char,
                    true, false, true, true, true>;
                basic_fields<pocca_t> f1;
                f1.insert("1", "1");
                basic_fields<pocca_t> f2;
                f2 = f1;
                BEAST_EXPECT(f2["1"] == "1");
            }
        }

        // swap
        {
            {
                // propagate_on_container_swap : true
                using pocs_t = test::test_allocator<char,
                    false, true, true, true, true>;
                pocs_t a1, a2;
                BEAST_EXPECT(a1 != a2);
                basic_fields<pocs_t> f1{a1};
                f1.insert("1", "1");
                basic_fields<pocs_t> f2{a2};
                BEAST_EXPECT(f1.get_allocator() == a1);
                BEAST_EXPECT(f2.get_allocator() == a2);
                swap(f1, f2);
                BEAST_EXPECT(f1.get_allocator() == a2);
                BEAST_EXPECT(f2.get_allocator() == a1);
                BEAST_EXPECT(f1.begin() == f1.end());
                BEAST_EXPECT(f2["1"] == "1");
                swap(f1, f2);
                BEAST_EXPECT(f1.get_allocator() == a1);
                BEAST_EXPECT(f2.get_allocator() == a2);
                BEAST_EXPECT(f1["1"] == "1");
                BEAST_EXPECT(f2.begin() == f2.end());
            }
            {
                // propagate_on_container_swap : false
                using pocs_t = test::test_allocator<char,
                    true, true, true, false, true>;
                pocs_t a1, a2;
                BEAST_EXPECT(a1 == a2);
                BEAST_EXPECT(a1.id() != a2.id());
                basic_fields<pocs_t> f1{a1};
                f1.insert("1", "1");
                basic_fields<pocs_t> f2{a2};
                BEAST_EXPECT(f1.get_allocator() == a1);
                BEAST_EXPECT(f2.get_allocator() == a2);
                swap(f1, f2);
                BEAST_EXPECT(f1.get_allocator().id() == a1.id());
                BEAST_EXPECT(f2.get_allocator().id() == a2.id());
                BEAST_EXPECT(f1.begin() == f1.end());
                BEAST_EXPECT(f2["1"] == "1");
                swap(f1, f2);
                BEAST_EXPECT(f1.get_allocator().id() == a1.id());
                BEAST_EXPECT(f2.get_allocator().id() == a2.id());
                BEAST_EXPECT(f1["1"] == "1");
                BEAST_EXPECT(f2.begin() == f2.end());
            }
        }

        // operations
        {
            fields f;
            f.insert(field::user_agent, "x");
            BEAST_EXPECT(f.count(field::user_agent));
            BEAST_EXPECT(f.count(to_string(field::user_agent)));
            BEAST_EXPECT(f.count(field::user_agent) == 1);
            BEAST_EXPECT(f.count(to_string(field::user_agent)) == 1);
            f.insert(field::user_agent, "y");
            BEAST_EXPECT(f.count(field::user_agent) == 2);
        }
    }

    void testHeaders()
    {
        f_t f1;
        BEAST_EXPECT(empty(f1));
        fill(1, f1);
        BEAST_EXPECT(size(f1) == 1);
        f_t f2;
        f2 = f1;
        BEAST_EXPECT(size(f2) == 1);
        f2.insert("2", "2");
        BEAST_EXPECT(std::distance(f2.begin(), f2.end()) == 2);
        f1 = std::move(f2);
        BEAST_EXPECT(size(f1) == 2);
        BEAST_EXPECT(size(f2) == 0);
        f_t f3(std::move(f1));
        BEAST_EXPECT(size(f3) == 2);
        BEAST_EXPECT(size(f1) == 0);
        self_assign(f3, std::move(f3));
        BEAST_EXPECT(size(f3) == 2);
        BEAST_EXPECT(f2.erase("Not-Present") == 0);
    }

    void testRFC2616()
    {
        f_t f;
        f.insert("a", "w");
        f.insert("a", "x");
        f.insert("aa", "y");
        f.insert("f", "z");
        BEAST_EXPECT(f.count("a") == 2);
    }

    void testErase()
    {
        f_t f;
        f.insert("a", "w");
        f.insert("a", "x");
        f.insert("aa", "y");
        f.insert("f", "z");
        BEAST_EXPECT(size(f) == 4);
        f.erase("a");
        BEAST_EXPECT(size(f) == 2);
    }

    void
    testContainer()
    {
        {
            // group fields
            fields f;
            f.insert(field::age,   1);
            f.insert(field::body,  2);
            f.insert(field::close, 3);
            f.insert(field::body,  4);
            BEAST_EXPECT(std::next(f.begin(), 0)->name() == field::age);
            BEAST_EXPECT(std::next(f.begin(), 1)->name() == field::body);
            BEAST_EXPECT(std::next(f.begin(), 2)->name() == field::body);
            BEAST_EXPECT(std::next(f.begin(), 3)->name() == field::close);
            BEAST_EXPECT(std::next(f.begin(), 0)->name_string() == "Age");
            BEAST_EXPECT(std::next(f.begin(), 1)->name_string() == "Body");
            BEAST_EXPECT(std::next(f.begin(), 2)->name_string() == "Body");
            BEAST_EXPECT(std::next(f.begin(), 3)->name_string() == "Close");
            BEAST_EXPECT(std::next(f.begin(), 0)->value() == "1");
            BEAST_EXPECT(std::next(f.begin(), 1)->value() == "2");
            BEAST_EXPECT(std::next(f.begin(), 2)->value() == "4");
            BEAST_EXPECT(std::next(f.begin(), 3)->value() == "3");
            BEAST_EXPECT(f.erase(field::body) == 2);
            BEAST_EXPECT(std::next(f.begin(), 0)->name_string() == "Age");
            BEAST_EXPECT(std::next(f.begin(), 1)->name_string() == "Close");
        }
        {
            // group fields, case insensitive
            fields f;
            f.insert("a",  1);
            f.insert("ab", 2);
            f.insert("b",  3);
            f.insert("AB", 4);
            BEAST_EXPECT(std::next(f.begin(), 0)->name() == field::unknown);
            BEAST_EXPECT(std::next(f.begin(), 1)->name() == field::unknown);
            BEAST_EXPECT(std::next(f.begin(), 2)->name() == field::unknown);
            BEAST_EXPECT(std::next(f.begin(), 3)->name() == field::unknown);
            BEAST_EXPECT(std::next(f.begin(), 0)->name_string() == "a");
            BEAST_EXPECT(std::next(f.begin(), 1)->name_string() == "ab");
            BEAST_EXPECT(std::next(f.begin(), 2)->name_string() == "AB");
            BEAST_EXPECT(std::next(f.begin(), 3)->name_string() == "b");
            BEAST_EXPECT(std::next(f.begin(), 0)->value() == "1");
            BEAST_EXPECT(std::next(f.begin(), 1)->value() == "2");
            BEAST_EXPECT(std::next(f.begin(), 2)->value() == "4");
            BEAST_EXPECT(std::next(f.begin(), 3)->value() == "3");
            BEAST_EXPECT(f.erase("Ab") == 2);
            BEAST_EXPECT(std::next(f.begin(), 0)->name_string() == "a");
            BEAST_EXPECT(std::next(f.begin(), 1)->name_string() == "b");
        }
    }

    void run() override
    {
        testMembers();
        testHeaders();
        testRFC2616();
        testErase();
        testContainer();
    }
};

BEAST_DEFINE_TESTSUITE(fields,http,beast);

} // http
} // beast
