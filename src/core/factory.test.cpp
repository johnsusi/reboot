#include "factory.h"
#include "assert.h"

#include <array>
#include <iostream>
#include <memory>
#include <source_location>
#include <span>
#include <string>

using namespace std::string_literals;

void testManaged()
{

    struct X final : Managed<X>
    {
        X(Factory)
        {
        }
    };

    auto x = X::Create();
    assert::notNull(x);
}

void testSingleton()
{

    struct X final : Singleton<X>
    {
        X(Factory)
        {
        }
    };

    auto &x = X::Instance();
    auto &y = X::Instance();
    assert::same(&x, &y);
}

void testSharedInstance()
{

    struct X final : SharedInstance<X>
    {
        X(Factory)
        {
        }
    };

    auto x = X::Acquire();
    auto y = X::Acquire();
    assert::same(x, y);
}

int main()
{
    try
    {
        testManaged();
        testSingleton();
        testSharedInstance();
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