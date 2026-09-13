#pragma once

#include <expected>
#include <format>
#include <functional>
#include <memory>
#include <source_location>
#include <stdexcept>
#include <string_view>

#if defined(__cpp_lib_stacktrace)
#include <stacktrace>
#else
#endif

namespace assert
{

class AssertionFailure : public std::runtime_error
{
  public:
#if defined(__cpp_lib_stacktrace)
    AssertionFailure(std::string_view what, std::source_location loc = std::source_location::current(),
                     std::stacktrace stacktrace = std::stacktrace::current());
    AssertionFailure(std::source_location loc = std::source_location::current(),
                     std::stacktrace stacktrace = std::stacktrace::current());
#else
    AssertionFailure(std::string_view what, std::source_location loc = std::source_location::current());
    AssertionFailure(std::source_location loc = std::source_location::current());
#endif
  private:
    std::source_location _loc;
};

[[noreturn]] void fail(std::string_view msg, const std::source_location &loc = std::source_location::current());

template <typename Expected, typename Actual>
void equal(const std::shared_ptr<Expected> &expected, const std::shared_ptr<Actual> &actual,
           std::source_location loc = std::source_location::current())
{
    if (!(*expected == *actual))
    {
        fail(std::format("expected {} but got {}", *expected, *actual), loc);
    }
}

template <typename Expected, typename Actual>
void equal(const std::unique_ptr<Expected> &expected, const std::unique_ptr<Actual> &actual,
           std::source_location loc = std::source_location::current())
{
    if (!(*expected == *actual))
    {
        fail(std::format("expected {} but got {}", *expected, *actual), loc);
    }
}

template <typename Expected, typename Actual>
void equal(const Expected &expected, const Actual &actual, std::source_location loc = std::source_location::current())
{
    if (!(expected == actual))
    {
        fail(std::format("expected {} but got {}", expected, actual), loc);
    }
}

template <typename Expected, typename Actual, typename Error>
void equal(const Expected &expected, std::expected<Actual, Error> actual,
           std::source_location loc = std::source_location::current())
{

    if (!actual)
    {
        fail(std::format("expected {} but got an error {}", expected, actual.error()));
    }
    if (!actual || !(expected == actual.value()))
    {
        fail(std::format("expected {} but got {}", expected, actual.value()), loc);
    }
}

template <typename Expected, typename Actual>
void notEqual(const std::shared_ptr<Expected> &expected, const std::shared_ptr<Actual> &actual,
              std::source_location loc = std::source_location::current())
{
    if (*expected == *actual)
    {
        fail("should not be equal", loc);
    }
}

template <typename Expected, typename Actual>
void notEqual(const std::unique_ptr<Expected> &expected, const std::unique_ptr<Actual> &actual,
              std::source_location loc = std::source_location::current())
{
    if (*expected == *actual)
    {
        fail("should not be equal", loc);
    }
}

template <typename Expected, typename Actual>
void notEqual(const Expected &expected, const Actual &actual,
              std::source_location loc = std::source_location::current())
{
    if (expected == actual)
    {
        fail("should not be equal", loc);
    }
}

// template <typename Expected, typename Actual>
// void equal(const Expected &expected, const Actual &actual, std::source_location loc =
// std::source_location::current())
// {
//     if (!(expected == actual))
//     {
//         fail("not equal", loc);
//     }
// }

template <std::ranges::input_range Expected, std::ranges::input_range Actual>
void sequenceEqual(const Expected &expected, const Actual &actual,
                   std::source_location loc = std::source_location::current())
{
    auto it = std::ranges::begin(expected);
    auto jt = std::ranges::begin(actual);
    const auto iend = std::ranges::end(expected);
    const auto jend = std::ranges::end(actual);

    std::size_t index = 0;
    for (; it != iend && jt != jend; ++it, ++jt, ++index)
    {
        if (!(*it == *jt))
        {
            fail(std::format("sequences differ at index {}: expected {} but got {}", index, *it, *jt), loc);
        }
    }
}

template <typename Expected, typename Actual>
void same(const std::shared_ptr<Expected> &expected, const std::shared_ptr<Actual> &actual,
          std::source_location loc = std::source_location::current())
{
    if (expected.get() != actual.get())
    {
        fail("Expected to be same", loc);
    }
}

template <typename Expected, typename Actual>
void same(const Expected &expected, const Actual &actual, std::source_location loc = std::source_location::current())
{
    if (&expected != &actual)
    {
        fail("Expected to be same", loc);
    }
}

template <typename Expected, typename Actual>
void notSame(const std::shared_ptr<Expected> &expected, const std::shared_ptr<Actual> &actual,
             std::source_location loc = std::source_location::current())
{
    if (expected.get() == actual.get())
    {
        fail("Expected to not be same", loc);
    }
}

template <typename T> void isTrue(T actual, std::source_location loc = std::source_location::current())
{
    if (!static_cast<bool>(std::forward<T>(actual)))
        fail("expected true", loc);
}

template <typename T> void isFalse(T actual, std::source_location loc = std::source_location::current())
{
    if (static_cast<bool>(std::forward<T>(actual)))
        fail("expected true", loc);
}

template <typename T> void isNull(T actual, std::source_location loc = std::source_location::current())
{
    equal(nullptr, actual, loc);
}

template <typename T> void notNull(T actual, std::source_location loc = std::source_location::current())
{
    notEqual(nullptr, actual, loc);
}

template <typename Func> void throws(Func &&action, std::source_location loc = std::source_location::current())
{
    try
    {
        std::invoke(std::forward<Func>(action));
        fail("Expected exception but got none", loc);
    }
    catch (...)
    {
    }
}

template <typename Expected, typename Func>
void throws(Func &&action, std::source_location loc = std::source_location::current())
{
    try
    {
        std::invoke(std::forward<Func>(action));
        fail("Expected exception", loc);
    }
    catch (const Expected &)
    {
    }
    catch (const std::exception &err)
    {
        fail(std::format("Expected exception of type {}, but caught a different type with message",
                         typeid(Expected).name(), err.what()),
             loc);
    }
    catch (...)
    {
        fail(std::format("Expected exception of type {}, but caught a different type", typeid(Expected).name()), loc);
    }
}

} // namespace assert