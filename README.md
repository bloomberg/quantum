# Quantum Library : A scalable C++ coroutine framework
[![Build status](https://travis-ci.com/bloomberg/quantum.svg?branch=master)](https://travis-ci.org/bloomberg/quantum) 

**Quantum** is a full-featured and powerful C++ framework build on top of the [Boost coroutine](https://www.boost.org/doc/libs/1_65_0/libs/coroutine2/doc/html/index.html) library. The framework allows users to dispatch units of work (a.k.a. _tasks_) as coroutines and execute them concurrently using the 'reactor' pattern.

### Features

* **NEW** Added support for simpler V2 coroutine API which returns computed values [directly](https://github.com/bloomberg/quantum/wiki/4.-Quick-reference-guide).
* Header-only library and interface-based design.
* Full integration with Boost asymmetric coroutine library.
* Highly parallelized coroutine framework for CPU-bound workloads.
* Support for long-running or blocking IO tasks.
* Allows explicit and implicit cooperative yielding between coroutines.
* Task continuations and coroutine chaining for serializing work execution.
* Synchronous and asynchronous dispatching using futures and promises similar to STL.
* Support for _streaming futures_ which allows faster processing of large data sets.
* Support for _future references_.
* Cascading execution output during task continuations (a.k.a. _past_ futures).
* Task prioritization.
* Internal error handling and exception forwarding.
* Ability to write lock-free code by synchronizing coroutines on dedicated queues.
* Coroutine-friendly mutexes and condition variables for locking critical code paths or synchronizing access to external objects.
* Fast pre-allocated memory pools for internal objects and coroutines.
* Parallel `forEach` and `mapReduce` functions.
* Various stats API.
* `Sequencer` class allowing strict FIFO ordering of tasks based on sequence ids.

### Sample code
**Quantum** is very simple and easy to use:
```c++
using namespace Bloomberg::quantum;

// Define a coroutine
int getDummyValue(CoroContextPtr<int> ctx)
{
    int value;
    ...           //do some work
    ctx->yield(); //be nice and let other coroutines run (optional cooperation)
    ...           //do more work and calculate 'value'
    return ctx->set(value);
}

// Create a dispatcher
Dispatcher dispatcher;

// Dispatch a work item to do some work and return a value
int result = dispatcher.post(getDummyValue)->get();
```

Chaining tasks can also be straightforward. In this example we produce various types in a sequence.
```c++
using namespace Bloomberg::quantum;

// Create a dispatcher
Dispatcher dispatcher;

auto ctx = dispatcher.postFirst([](CoroContextPtr<int> ctx)->int {
    return ctx->set(55); //Set the 1st value
})->then([](CoroContextPtr<double> ctx)->int {
    // Get the first value and add something to it
    return ctx->set(ctx->getPrev<int>() + 22.33); //Set the 2nd value
})->then([](CoroContextPtr<std::string> ctx)->int {
    return ctx->set("Hello world!"); //Set the 3rd value
})->finally([](CoroContextPtr<std::list<int>> ctx)->int {
    return ctx->set(std::list<int>{1,2,3}); //Set 4th value
})->end();

int i = ctx->getAt<int>(0); //This will throw 'FutureAlreadyRetrievedException'
                            //since future was already read in the 2nd coroutine
double d = ctx->getAt<double>(1); //returns 77.33
std::string s = ctx->getAt<std::string>(2); //returns "Hello world!";
std::list<int>& listRef = ctx->getRefAt<std::list<int>>(3); //get list reference
std::list<int>& listRef2 = ctx->getRef(); //get another list reference.
                                          //The 'At' overload is optional for last chain future
std::list<int> listValue = ctx->get(); //get list value
```
Chaining with the **new** V2 api:
```c++
using namespace Bloomberg::quantum;

// Create a dispatcher
Dispatcher dispatcher;

auto ctx = dispatcher.postFirst([](VoidContextPtr ctx)->int {
    return 55; //Set the 1st value
})->then([](VoidContextPtr ctx)->double {
    // Get the first value and add something to it
    return ctx->getPrev<int>() + 22.33; //Set the 2nd value
})->then([](VoidContextPtr ctx)->std::string {
    return "Hello world!"; //Set the 3rd value
})->finally([](VoidContextPtr ctx)->std::list<int> {
    return {1,2,3}; //Set 4th value
})->end();
```

### Building and installing
**Quantum** is a header-only library and as such no targets need to be built. To install simply run:
```shell
> cmake -Bbuild <options> .
> cd build
> make install
```

### CMake options
Various **CMake** options can be used to configure the output:
* `QUANTUM_BUILD_DOC`        : Build Doxygen documentation. Default `OFF`.
* `QUANTUM_ENABLE_DOT`       : Enable generation of DOT viewer files. Default `OFF`.
* `QUANTUM_VERBOSE_MAKEFILE` : Enable verbose cmake output. Default `ON`.
* `QUANTUM_ENABLE_TESTS`     : Builds the `tests` target. Default `OFF`.
* `QUANTUM_BOOST_STATIC_LIBS`: Link with Boost static libraries. Default `ON`.
* `QUANTUM_BOOST_USE_MULTITHREADED` : Use Boost multi-threaded libraries. Default `ON`.
* `QUANTUM_USE_DEFAULT_ALLOCATOR` : Use default system supplied allocator instead of Quantum's. Default `OFF`.
* `QUANTUM_ALLOCATE_POOL_FROM_HEAP` : Pre-allocates object pools from heap instead of the application stack. Default `OFF`.
* `QUANTUM_BOOST_USE_SEGMENTED_STACKS` : Use Boost segmented stacks for coroutines. Default `OFF`.
* `QUANTUM_BOOST_USE_PROTECTED_STACKS` : Use Boost protected stacks for coroutines (slow!). Default `OFF`.
* `QUANTUM_BOOST_USE_FIXEDSIZE_STACKS` : Use Boost fixed size stacks for coroutines. Default `OFF`.
* `QUANTUM_INSTALL_ROOT`     : Specify custom install path.
                               Default is `/usr/local/include` for Linux or `c:/Program Files` for Windows.
* `QUANTUM_PKGCONFIG_DIR`    : Specify custom install path for the `quantum.pc` file. Default is `${QUANTUM_INSTALL_ROOT}/share/pkgconfig`.
                               To specify a relative path from `QUANTUM_INSTALL_ROOT`, omit leading `/`. 
* `QUANTUM_EXPORT_PKGCONFIG` : Generate `quantum.pc` file. Default `ON`.
* `QUANTUM_CMAKE_CONFIG_DIR` : Specify a different install directory for the project's config, target and version files. Default is `${QUANTUM_INSTALL_ROOT}/share/cmake`.
* `QUANTUM_EXPORT_CMAKE_CONFIG` : Generate CMake config, target and version files. Default `ON`.
* `BOOST_ROOT`               : Specify a different Boost install directory.
* `GTEST_ROOT`               : Specify a different GTest install directory.

Note: options must be preceded with `-D` when passed as arguments to CMake.

### Running tests
Run the following from the top directory:
```shell
> cmake -Bbuild -DQUANTUM_ENABLE_TESTS=ON <options> .
> cd build
> make quantum_test && ctest
```

### Using
To use the library simply include `<quantum/quantum.h>` in your application. Also, the following libraries must be included in the link:
* `boost_context`
* `pthread`

**Quantum** library is fully is compatible with `C++11`, `C++14` and `C++17` language features. See compiler options below for more details.

### Compiler options
The following compiler options can be set when building your application:
* `__QUANTUM_PRINT_DEBUG` : Prints debug and error information to `stdout` and `stderr` respectively.
* `__QUANTUM_USE_DEFAULT_ALLOCATOR` : Disable pool allocation for internal objects (other than coroutine stacks) and
use default system allocators instead.
* `__QUANTUM_ALLOCATE_POOL_FROM_HEAP` : Pre-allocates object pools from heap instead of the application stack (default).
This affects internal object allocations other than coroutines. Coroutine pools are always heap-allocated due to their size.
* `__QUANTUM_BOOST_USE_SEGMENTED_STACKS` : Uses boost segmented stack for on-demand coroutine stack growth. Note that
**Boost.Context** library must be built with property `segmented-stacks=on` and applying `BOOST_USE_UCONTEXT` and
`BOOST_USE_SEGMENTED_STACKS` at b2/bjam command line.
* `__QUANTUM_BOOST_USE_PROTECTED_STACKS` : Uses boost protected stack for runtime bound-checking. When using this option,
coroutine creation (but not runtime efficiency) becomes more expensive.
* `__QUANTUM_BOOST_USE_FIXEDSIZE_STACKS` : Uses boost fixed size stack. This defaults to system default allocator.
                                        
### Application-wide settings
Various application-wide settings can be configured via `ThreadTraits`, `AllocatorTraits` and `StackTraits`.

### Documentation
Please see the [wiki](https://github.com/bloomberg/quantum/wiki) page for a detailed overview of this library, use-case scenarios and examples.

For class description visit the [API reference](https://bloomberg.github.io/quantum) page.
