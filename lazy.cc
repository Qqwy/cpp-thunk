#include <iostream>
#include <functional>

template<typename Result>class Lazy
{
    std::function<Result()> d_lambda;
public:
    Lazy(const Lazy &) = default;
    Lazy(Lazy &&) = default;

    constexpr Lazy(std::function<Result()> lambda) :
        d_lambda(lambda)
    {};

    // Lazy is convertible to Result.
    constexpr operator Result() const
    {
        return d_lambda();
    }

    // Lazy is convertible to everything Result is convertible to.
    // (fixing the 'single conversion' problem)
    template<typename T>constexpr operator T() const
    {
        return static_cast<T>(d_lambda());
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
template<typename Result> std::ostream &operator<<(std::ostream &out, Lazy<Result> const &val) {
    return out << (val.operator Result());
};

// Another nice example:
// Overload common operations in a way that build up lazily
// and only are evaluated all the way at the end once we decide to convert.
template<typename Lhs, typename Rhs> auto operator+(Lazy<Lhs> const &lhs, Lazy<Rhs> const &rhs) -> Lazy<decltype(std::declval<Lhs>() + std::declval<Rhs>())> {
    return make_lazy([=]{
        return (lhs.operator Lhs()) + (rhs.operator Rhs());
        }
    );
};


// A fun way to auto-deduce the return type
// so we don't have to write it manually.
// (similar to make_tuple, make_array, etc.)
template<typename F>auto constexpr make_lazy(F lambda) -> Lazy<std::invoke_result_t<F>> {
    return Lazy<std::invoke_result_t<F>>{lambda};
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
template<typename F>auto constexpr operator+(LazyProxy const &, F lambda) -> Lazy<std::invoke_result_t<F>> {
    return make_lazy(lambda);
};

#define lazy LazyProxy{} + [&]

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

    return sum;
}
