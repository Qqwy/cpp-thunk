# C++ Thunks

An exploration into lazy evaluation in C++.

Note: This code is highly experimental, and thus **not** ready for production use.

There are probably ways to improve the template metaprogramming; I am a bit rusty and not used to some of the newer constructs that C++2a allows.

## Idea

This repository has four files, each implementing a slightly different templated class 
(and also including some example code in a `main` function which help to test whether the class definition works as intended.)

Both Lazy and Thunk are written as 'normal' easy to read implementations and 'functionless' harder to read implementations that do not use `std::function`.

The 'functionless' versions result in much smaller optimized compiled code (on current versions of clang++ and g++) and probably are also more performant (but I did not test this myself yet).

### Lazy

Wraps a lambda (or any other callable), and behaves as a 'proxy' for the result of the lambda, executing the lambda only at the point where the result value is finally used.
Internally, this is done by defining a user-defined conversion operation to the result of the wrapped lambda.

### Thunk

Similar to `Lazy`, but ensures that if the value is used multiple times, that the lambda is only executed once.
After that the result is re-used rather than recomputing it by calling the lambda again.

This is done by replacing the internal lambda with a trivial lambda simply returning (a copy of) the result after the first call.

Curiously it is possible to define the user-defined conversion operation (`operator Result()`) as `const` member,
even though by calling its stored lambda it actively mutates itself.

### Lazy (Functionless)

A variant of `Lazy` that uses slightly more template trickery to get around using `std::function<Result()>`.
While this makes the implementation less easy to read,
not depending on `std::function` means that no heap allocation is required to create `Lazy` objects 
(and `std::function` has a reputation of being relatively inefficient for this and possibly other reasons, as it does a lot of type erasure that makes life difficult for the compiler to optimize).

### Thunk (Functionless)

A variant of `Thunk` that uses slightly more template trickery to get around using `std::function<Result()>`.

This does mean that its internal data member needs to change from a `std::function<Result()>` to a `std::variant<Lambda, Result>` because
the implementation of the normal `Thunk` with the 'trivial constant-returning lambda' does not work, as different lambdas have different types.

Evaluating the thunk now becomes slightly more involved, requiring the use of `std::visit`.
Interestingly the compiler now does _not_ consider `operator Result()` a `const` member.
However, it seems morally correct to perform a const-cast there because the change to `Thunk` is not _observable_. (don't agree? So sue me ;-) ).

## Exercises for the reader

The following features were left as exercise for the reader.
(Or, put in other words: PRs welcome ;-) )

### Making `Thunk` (Functionless) type-safe
This is probably possible by adding `std::call_once` at the appropriate place in the `visit()` member function,
but I was unable to get it to work in the couple of minutes I attempted to do it.

### Implementing other operators
As example, the extraction `operator<<` and the binary `operator+` have been implemented.
Similar operators and other common functions can be conditionally implemented for `Lazy` and `Thunk` based on the type they wrap,
allowing the user to use them more freely (rather than having to manually convert them in some places).

### Reducing the overhead of `std::variant`

A tagged union seems the way to go for the functionless version of `Thunk`.
However, `std::variant` has a bit of overhead because it tries to be very conservative w.r.t. exception-safety.

There probably is a way to use the correct string of incantations to `noexcept` to convince the compiler that if the passed lambda cannot throw,
the variant will never be invalid and opening up more optimizations in that way, but I have so far been unable to get it to work.

A few years back `std::variant` was not optimized very much in gcc/clang/libc++ yet, but I don't know how much this work has progressed now.
(It is obvously much more likely that I missed to handle some edge case in the current code that prevents the desired optimizations. I am not a perfect programmer.)
