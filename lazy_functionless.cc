#include <iostream>
#include <functional>

template<
  typename Lambda,
  typename Result = std::invoke_result_t<Lambda>,
  std::enable_if_t<std::is_invocable_r<Result, Lambda>::value, bool> = true
  > class Lazy
{
  Lambda d_lambda;
public:
  Lazy(const Lazy &) = default;
  Lazy(Lazy &&) = default;
  constexpr Lazy(Lambda lambda) :
    d_lambda(lambda)
  {}

      // Lazy is convertible to Result.
      constexpr operator Result() const
      {
          return d_lambda();
      }

      // Lazy is convertible to everything Result is convertible to.
      // (fixing the 'single conversion' problem)
      template<typename T>constexpr operator T() const
      {
        return static_cast<T>(static_cast<Result>(d_lambda()));
      }
};


// It might be possible to add SFINAE `noexcept` specifiers
// to both the constructor and the cast operator, based on the type of the lambda that was passed.


// Overload all operations that `Result` supports for `Lazy<Result>`
// as example we overload `operator<<` but many more could be added
// in a similar vein.
//
// These do not alter any runtime, but help the compiler with type-checking.
// (essentially allowing the user to write manual casts in less places)
template<typename T,
         typename TRes = std::invoke_result_t<T>
         > std::ostream &operator<<(std::ostream &out, Lazy<T> const &val)
{
    return out << (val.operator TRes());
};

// Another nice example:
// Overload common operations in a way that build up lazily
// and only are evaluated all the way at the end once we decide to convert.
template<
  typename Lhs,
  typename LhsRes = std::invoke_result_t<Lhs>,
  typename Rhs,
  typename RhsRes = std::invoke_result_t<Rhs>
         > auto operator+(Lazy<Lhs> const &lhs, Lazy<Rhs> const &rhs) ->
  decltype(auto)
{
    return make_lazy([=]{
        return (lhs.operator LhsRes()) + (rhs.operator RhsRes());
        }
    );
};


// A fun way to auto-deduce the return type
// so we don't have to write it manually.
// (similar to make_tuple, make_array, etc.)
template<typename F>auto constexpr make_lazy(F lambda) -> Lazy<F> {
  return Lazy<F>{lambda};
}

// An even more concise way,
// using the preprocessor to hide the final bits of boilerplate
// For some people this might be too much magic.
#define LAZY(arg) make_lazy([]{ return (arg);})


// Or, if you like new control structures:
// ;-)
struct LazyProxy{
    LazyProxy() = default;
};
template<typename F>auto constexpr operator+(LazyProxy const &, F lambda) -> Lazy<F> {
    return make_lazy(lambda);
};

#define lazy LazyProxy{} + [&]




// A Thunk.
// Similar to `Lazy`, but will only compute the result once.
// Idea: Calling `d_fun` the first time will evaluate `lambda` but then replace `d_fun` with a trivial lambda
// which does not do any more work but simply returns the previous result.
// (Requires Result to be copyable for obvious reasons).
template<typename Result>class Thunk
{
  std::function<Result()> d_lambda;

public:
  Thunk(std::function<Result()> lambda):
    d_lambda([=, this]{
      Result res = lambda();
      d_lambda = [=]{ return res; };
      return res;
    })
  {}

  Thunk(const Thunk &) = default;
  Thunk(Thunk &&) = default;

  constexpr operator Result() const {
    return d_lambda();
  }

  // a Thunk is convertible to everything Result is convertible to.
  // (fixing the 'single conversion' problem)
  // 'conceptually' const, but secretly the initial call to `d_lambda` modifies d_lambda itself.
  template<typename T>constexpr operator T() const
  {
    return static_cast<T>(d_lambda());
  }
};

// Helpers for `Thunk` can be made in a similar vein as was done for `Lazy`.
template<typename Result> std::ostream &operator<<(std::ostream &out, Thunk<Result> const &val) {
  return out << (val.operator Result());
};


int main()
{
    auto res = make_lazy([]{ return std::string("Hello world!\n");});
    auto res2 = LAZY("Mellogoth");
    std::string res3 = lazy { return "Mellogoth2"; };

    // res, res2 and res3 are only created on the following line:
    // (disregarding compiler optimizations)
    std::cout << res << res2 << res3;


    auto fourtytwo = lazy { return 42; };
    auto sixtynine = lazy { return 69; };
    int sum = (fourtytwo + sixtynine);

    std::cout << "The result is: " << sum << '\n';

    auto difficult_computation = Thunk<std::string>{[&]{
      std::cout << "Performing work!\n";
      return "Result Computed!\n";
    }};

    std::cout << difficult_computation << difficult_computation << difficult_computation;

    return sum;
}
