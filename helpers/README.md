# C++ Helpers

This repository is a collection of header only helper classes to be used in C++ applications. Everything in the project happens is defined in the `helpers` namespace. 

## Exceptions and Basics - `helpers.h`

### `STR` Macro

The `STR` is a handy macro to concatenate strings and other values. Internally it pushes all its arguments to a `std::stringstream` and then obtains the string value of it so it works on any type for which there is overriden `<<` operator for `std::ostream`:

    std::string x = STR("Hello " << 67 << " world " << 2.67 << true);

### Exceptions

The `helpers::Exception` is intended to be base class for all exceptions used in the application. It inherits from `std::exception` and provides an accessible storage for the `what()` value. Furthermore, unless `NDEBUG` macro is defined, each exception also remembers the source file and line where it was raised. 

To facilitate this, exceptions which inherit from `helpers::Exception` should not be thrown using `throw` keyword, but the provided `THROW` macro which also patches the exception with the source file and line, if appropriate:

    THROW(helpers::Exception("An error"));

#### `NOT_IMPLEMENTED` and `UNREACHABLE`

These are convenience macros that can be inserted into respective code paths. They simply throw `helpers::Exception` with appropriate textual description. 

> This should better be its own exceptions? 

### Assertions

Instead of `c` style `assert`, the macro `ASSERT` which uses the exceptions mechanism is provided. The `ASSERT` macro takes the condition as an argument and can be followed by help message using the standard `<<` `std::ostream` operator. If it fails, the `helpers::AssertionError` exception is thrown:

    ASSERT(x == y) << "x and y are not the same, ouch - x: " << x << ", y: " << y;

If `NDEBUG` is specified, `ASSERT` translates to dead code and will be removed by the optimizer. 

## Events - `events.h`

Provides a simple and easy to use event system. 








