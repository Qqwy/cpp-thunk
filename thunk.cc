#include <functional>
#include <iostream>

// A Thunk.
// Computes the result lazily (on demand), and will only compute the result once.
// Idea: Calling `d_fun` the first time will evaluate `lambda` but then replace `d_fun` with a trivial lambda
// which does not do any more work but simply returns the previous result.
// (This requires Result to be copyable for obvious reasons).
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

// A fun way to auto-deduce the return type
// so we don't have to write it manually.
// (similar to make_tuple, make_array, etc.)
template<typename F>auto constexpr make_thunk(F lambda) -> Thunk<std::invoke_result_t<F>> {
  return Thunk<std::invoke_result_t<F>>{lambda};
}


// Overload all operations that `Result` supports for `Thunk<Result>`
// as example we overload `operator<<` but many more could be added
// in a similar vein.
//
// These do not alter any runtime details, but help the compiler with type-checking.
// (requiring less manual template annotations from the user)
template<typename Result> std::ostream &operator<<(std::ostream &out, Thunk<Result> const &val) {
  return out << (val.operator Result());
};

// Another nice example:
// Overload common operations in a way that build up lazily
// and only are evaluated all the way at the end once we decide to convert.
template<typename Lhs, typename Rhs> auto operator+(Thunk<Lhs> const &lhs, Thunk<Rhs> const &rhs) -> Thunk<decltype(std::declval<Lhs>() + std::declval<Rhs>())> {
  return make_thunk([=]{
    Lhs tmp_lhs = lhs.operator Lhs();
    Rhs tmp_rhs = rhs.operator Rhs();
    return tmp_lhs + tmp_rhs;
  });
};


// An even more concise way to specify thunks
// using the preprocessor to hide the final bits of boilerplate
// For some people (me included) this might be too much magic.
#define THUNK(arg) make_thunk([]{ return (arg);})


// Or, if you like new control structures (which I do very much):
// ;-)
struct ThunkProxy{
  ThunkProxy() = default;
};
template<typename F>auto constexpr operator+(ThunkProxy const &, F lambda) -> Thunk<std::invoke_result_t<F>> {
  return make_thunk(lambda);
};

#define thunk ThunkProxy{} + [&]


// An example
// Note that thunks are indeed only evaluated once and then the result values are re-used.
int main()
{
  auto res = make_thunk([]{ return std::string("Hello world!\n");});
  auto res2 = THUNK("Mellogoth");
  std::string res3 = thunk { return "Mordor"; };

  // res, res2 and res3 are only created on the following line:
  // (disregarding compiler optimizations)
  std::cout << res << res2 << res3;


  auto fourtytwo = thunk { std::cout << "Evaluated 42! "; return 42; };
  auto sixtynine = thunk { return 69; };
  auto sum = (fourtytwo + sixtynine);

  std::cout << "The result is: " << (fourtytwo + sixtynine) << ' ' << (fourtytwo + sixtynine) << ' ' << sum << '\n';

  auto difficult_computation = Thunk<std::string>{[&]{
    std::cout << "Performing difficult work!\n";
    return "Result Computed!\n";
  }};

  std::cout << difficult_computation << difficult_computation << difficult_computation;

  return sum;
}
