#include "assert.h"

namespace assert
{

#if defined(__cpp_lib_stacktrace)

AssertionFailure::AssertionFailure(std::string_view what, std::source_location loc, std::stacktrace stacktrace)
    : std::runtime_error(
          std::format("{}:{} in {}\n{}\n{}", loc.file_name(), loc.line(), loc.function_name(), what, stacktrace)),
      _loc(loc)
{
}

AssertionFailure::AssertionFailure(std::source_location loc, std::stacktrace stacktrace)
    : std::runtime_error(std::format("{}:{} in {}\n{}", loc.file_name(), loc.line(), loc.function_name(), stacktrace)),
      _loc(loc)
{
}

#else

AssertionFailure::AssertionFailure(std::string_view what, std::source_location loc)
    : std::runtime_error(std::format("{}:{} in {}\n{}", loc.file_name(), loc.line(), loc.function_name(), what)),
      _loc(loc)
{
}

AssertionFailure::AssertionFailure(std::source_location loc)
    : std::runtime_error(std::format("{}:{} in {}", loc.file_name(), loc.line(), loc.function_name())), _loc(loc)
{
}

#endif

[[noreturn]] void fail(std::string_view msg, const std::source_location &loc)
{
    throw AssertionFailure(msg);
}
} // namespace assert