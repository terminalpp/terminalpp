# TODO

- log should define default stream, which starts as cerr, but can be also a file, or so, for which all stream logs would output. Separate logs can then define specific streams
- log documentation

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

## Hash - `hash.h`

Provides an implementation of memory efficient version of variable sized hashes. These can be used whenever string with hexadecimal representation will be used and they occupy only half the space required. The hash can be printed the same way as a string would be and impementation for `std::hash` is provided so that the hash can be used as key in various `stl` containers. 

## String Operations - `strings.h`

Contains a few simple functions for string manipulation that I feel are sorely missing from the standard library. 

## BaseObject - `object.h`

Implements the `Object` class which has some functionality useful for objects and their hierarchies, such as a virtual destructor which people often forget and interoperability with events using the `HANDLER` macro (see the section discussing events below).

## Events - `events.h`

Provides a simple and easy to use event system. Each event can be associated with multiple *event handlers* - functions or object methods that will be called when the event is triggered. Event handlers can be added using `+=` operator and removed using the `-=` operator.

The event handler function takes an instantiation of `EventPayload` class which is parametrized by the type of the *sender*, i.e. the object which triggered the event and a *payload*, which is any type, or `void` which holds the information associated with the event. 

The event payload acts as a smart pointer to the payload itself (unless the payload is `void`) and contains own property `sender` and method `stopDispatch`, which terminates the dispatching of the event in case that multiple handlers are associated with the event and further dispatching is not needed.

An `HANDLER` macro is provided which takes as an argument either a function, or a method and creates the appropriate `EventHandler` from it. It works by calling the function `CreateEventHandler_` which can be redefined in different contexts to create instantiate the handler properly. Notably for use outside objects, creates event handler from a function and when used inside `helpers::Object` or its children, will either create handler from a function, or from method of `this` object.

The `helpers::Object` also provides the `trigger` helper method which triggers any event whose sender is `helpers::Object` from the appropriate payload:

The following is an example of how events can be used:

    struct Mouse {
	    size_t x;
		size_t y;
		unsigned buttons;
	};

	typedef helpers::EventPayload<void, helpers::Object> VoidEvent;
	typedef helpers::EventPayload<Mouse, helpers::Object> MouseEvent;

	void functionHandler(VoidEvent & e) {
		std::cout << "void event triggered from " << e.sender << std::endl;
	}

    class EventTester : public helpers::Object {
	public:
	    helpers::Event<VoidEvent> onChange;
		helpers::Event<MouseEvent> onMouseChange;

		void methodHandler(MouseEvent & e) {
			std::cout << "Mouse coordinates " << e->x << ":" << e->y << std::endl;
		}

		void test() {
			onChange += HANDLER(functionHandler);
			onMouseChange += HANDLER(EventTester::methodHandler);

			trigger(onChange);
			trigger(onMouseChange, Mouse{10,15,0});
		}
	}; 






