#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <type_traits>

class NonCopyable
{
  protected:
    constexpr NonCopyable() = default;
    ~NonCopyable() = default;

    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;

    NonCopyable(NonCopyable &&) = delete;
    NonCopyable &operator=(NonCopyable &&) = delete;
};

template <typename TFactory> class FactoryBase : NonCopyable
{
  protected:
    struct Factory
    {
      private:
        friend TFactory;
        explicit Factory() = default;
    };
};

template <typename T> class Managed : protected FactoryBase<Managed<T>>
{
  protected:
    using typename FactoryBase<Managed<T>>::Factory;

  public:
    using Ptr = std::shared_ptr<T>;

    static Ptr Create(auto &&...args)
        requires std::is_constructible_v<T, Factory, decltype(args) &&...>
    {
        return std::make_shared<T>(Factory{}, std::forward<decltype(args)>(args)...);
    }
};

template <typename T> class Singleton : protected FactoryBase<Singleton<T>>
{
  protected:
    using typename FactoryBase<Singleton<T>>::Factory;

  public:
    static T &Instance()
        requires std::is_constructible_v<T, Factory>
    {
        static T instance(Factory{});
        return instance;
    }
};

template <typename T> class SharedInstance : protected FactoryBase<SharedInstance<T>>
{
  protected:
    using typename FactoryBase<SharedInstance<T>>::Factory;

  public:
    using Ptr = std::shared_ptr<T>;
    static Ptr Acquire()
        requires std::is_constructible_v<T, Factory>
    {
        static std::mutex mutex;
        static std::weak_ptr<T> instance;

        if (auto shared = instance.lock())
        {
            return shared;
        }

        std::lock_guard<std::mutex> lock(mutex);
        auto shared = instance.lock();
        if (!shared)
        {
            shared = std::make_shared<T>(Factory{});
            instance = shared;
        }
        return shared;
    }
};

// template <typename T> class ObjectPool
// {
//   public:
//     auto Get()
//     {

//         std::lock_guard lock(_mutex);

//         if (_pool.empty())
//             return std::unique_ptr(new T(), [this](T *object) {
//                 if (_pool.size() < _limit)
//                     _pool.emplace_back(object);
//                 else
//                     delete object;
//             });

//         auto object = std::move(_pool.back());
//         _pool.pop_back();
//         return std::unique_ptr(object.release(), [this](T *object) {
//             if (_pool.size() < _limit)
//                 _pool.emplace_back(object);
//             else
//                 delete object;
//         });
//     }

//   private:
//     std::mutex _mutex;
//     std::vector<std::unique_ptr<T>> _pool;
//     std::size_t _limit = 0;
// };