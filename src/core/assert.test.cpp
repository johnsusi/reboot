#include "assert.h"

#include <array>
#include <iostream>
#include <memory>
#include <source_location>
#include <span>
#include <string>

using namespace std::string_literals;

void shouldFail(auto &&func, std::source_location loc = std::source_location::current())
{
    try
    {
        func();
    }
    catch (const assert::AssertionFailure &)
    {
        return;
    }
    throw assert::AssertionFailure(loc);
}

void testFail()
{
    shouldFail([] { assert::fail("should fail"); });
}

void testEqual()
{
    assert::equal(10, 10);
    assert::equal(true, true);
    assert::equal(nullptr, nullptr);
    assert::equal("hello"s, "hello"s);
    assert::equal(std::array<int, 3>{1, 2, 3}, std::array<int, 3>{1, 2, 3});
    assert::equal(std::make_unique<int>(10), std::make_unique<int>(10));
    assert::equal(std::make_shared<int>(10), std::make_shared<int>(10));
    shouldFail([] { assert::equal(10, 11); });
    shouldFail([] { assert::equal(10, 10.1); });
    shouldFail([] { assert::equal("hello"s, "world"s); });
}

void testSequenceEqual()
{
    assert::sequenceEqual(std::array<int, 3>{1, 2, 3}, std::array<int, 3>{1, 2, 3});
    shouldFail([] { assert::sequenceEqual(std::array<int, 3>{1, 2, 3}, std::array<int, 3>{3, 2, 1}); });
}

void testSame()
{

    int i = 10;
    // assert::same(&i, &i);
}

int main()
{
    try
    {
        testFail();
        testEqual();
        testSequenceEqual();
        testSame();
        return 0;
    }
    catch (const std::exception &err)
    {
        std::cerr << err.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        return -1;
    }
}