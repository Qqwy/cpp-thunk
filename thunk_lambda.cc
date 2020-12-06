#include <functional>
#include <iostream>
#include <variant>

template<class> inline constexpr bool always_false_v = false;

// A variant of Thunk
// which does not contain a `std::function`
// and because of that it is probably (untested) faster
// and definitely (tested on clang++ and g++) results in smaller code.
template<
  typename Lambda,
  typename Result = std::invoke_result_t<Lambda>,
  std::enable_if_t<std::is_invocable_r<Result, Lambda>::value, bool> = true
  > class Thunk
{
  // Stores the lambda until we evaluate the thunk
  // and then replaces it by the evaluated result.
  std::variant<Lambda, Result> d_thunk;

public:

  Thunk(const Thunk&) = default;
  Thunk(Thunk &&) = default;
  constexpr Thunk(Lambda const &lambda) noexcept :
    d_thunk{lambda}
  {}

  constexpr operator Result() const noexcept(noexcept(std::declval<Lambda>())) {
    // Morally correct const_cast
    // since the *observable* state of Thunk remains unchanged:
    return (const_cast<Thunk*>(this))->visit();
  }

  // a Thunk is convertible to everything Result is convertible to.
  // (fixing the 'single conversion' problem)
  template<typename T>constexpr operator T() const noexcept(noexcept(static_cast<T>(operator Result())))
  {
    return static_cast<T>(operator Result());
  }

private:

  constexpr Result visit() noexcept(noexcept(std::declval<Lambda>())) {
    return std::visit([this](auto &&arg) {
      using T = std::decay_t<decltype(arg)>;
      if constexpr(std::is_same_v<Result, T>) {
        return arg;
      } else if constexpr(std::is_same_v<Lambda, T>){
        Result res = arg();
        d_thunk.template emplace<Result>(res);
        return res;
      } else {
        static_assert(always_false_v<T>, "non-exhaustive visitor!");
      }
    }, d_thunk);
  }
};

// A fun way to auto-deduce the return type
// so we don't have to write it manually.
// (similar to make_tuple, make_array, etc.)
template<typename F>auto constexpr make_thunk(F lambda) -> Thunk<F> {
  return Thunk<F>{lambda};
}

int main(){
  auto res = make_thunk([&]{ return std::string{"Hello, world!"}; });
  std::string evaluated = res;
  std::cout << evaluated << '\n';
}
