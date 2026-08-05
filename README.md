<a href="https://github.com/greergan/SlimTS">
  <img src="https://raw.githubusercontent.com/greergan/SlimTS/master/assets/slimts_logo.png" width="75" alt="SlimTS Logo">
</a>

# SlimTS

**SlimTS is a lightweight, high-performance TypeScript runtime and application platform built in C++.** It brings TypeScript and modern JavaScript together with the performance, control, and native capabilities of C++, providing a foundation for building efficient applications that can take advantage of both environments.

SlimTS embeds the TypeScript compiler through [`libtsgo`](https://codeberg.org/greergan/libtsgo) and executes compiled JavaScript using Google V8. Native functionality is exposed to TypeScript through a modular **synthetic module plugin** architecture, allowing C++ components to integrate naturally with standard ES6 module imports.

The platform is designed around a simple principle: **provide a capable runtime without imposing unnecessary overhead.** Functionality is loaded as it is needed, while performance-sensitive operations can be implemented directly in native C++. This makes it possible to build applications that combine the expressiveness and accessibility of TypeScript with the efficiency and system-level capabilities of modern C++.

SlimTS currently focuses on **ES6 modules**, native asynchronous I/O and extensibility through C++ plugins. Its architecture is intended to provide a clean boundary between TypeScript application code and high-performance native services, while keeping that boundary straightforward for developers to use.

## Issue Management
Hosted at [SlimTS on Codeberg](https://codeberg.org/greergan/SlimTS/issues)

Initial Development status

![Fetch API Tasks Left](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fcodeberg.org%2Fapi%2Fv1%2Frepos%2Fgreergan%2FSlimTS%2Fmilestones%2F138104&query=%24.open_issues&label=Fetch%20API%20Tasks%20Left&color=red)
![Fetch API Tasks Closed](https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fcodeberg.org%2Fapi%2Fv1%2Frepos%2Fgreergan%2FSlimTS%2Fmilestones%2F138104&query=%24.closed_issues&label=Fetch%20API%20Tasks%20Closed&color=green)

## Table of Contents

- [Embedded Typescript-go](#embedded-typescript-go)
- [Synthetic Modules](#synthetic-modules)
- [Console Behavior](#console-behavior)
- [Core Plugins](#core-plugins)
  - [console](#console)
  - [http](#http)
- [Prerequisites](#prerequisites)
  - [Google V8](#google-v8)
  - [BoringSSL](#boringssl)
  - [SlimCommon](#slimcommon)
  - [libtsgo](#libtsgo)
- [Building](#building)
- [Running](#running)

[↑ Top](#table-of-contents)

## Embedded Typescript-go

High-performance TypeScript compilation via [libtsgo](https://codeberg.org/greergan/libtsgo), which wraps the Microsoft TypeScript Go compiler.

Typescript compiler options are found embedded within the libtsgo bridge library.


[↑ Top](#table-of-contents)

## Synthetic Modules

Synthetic modules are C++ plugins that seamlessly expose native V8 functionality to the JavaScript runtime. Designed to keep the environment exceptionally lightweight, these modules implement the V8 module interface directly in C++ and allow native behavior to be consumed as standard ES6 imports.

**Key Architecture Benefits:**
* **Lazy-Loaded:** Plugins are initialized entirely on-demand, ensuring fast startup times and a highly optimized memory footprint.
* **Zero-Overhead by Default:** The core runtime is strictly minimal. Even standard utilities like `console` are implemented as synthetic modules and will not consume memory until explicitly imported.
* **Implementation Choice:** The plugin system is highly modular, giving developers the flexibility to choose, swap, or customize the underlying C++ implementation of a module without altering the JavaScript interface.
* **Native ES6 Integration:** Developers interact with high-performance C++ capabilities using standard, familiar JavaScript module syntax.

[↑ Top](#table-of-contents)

## Console Behavior

To maintain a strict zero-overhead environment, the standard output console is not loaded by default. If your application requires logging or terminal output, you must explicitly import the `console` synthetic module.

```ts
import console from 'console'
```

[↑ Top](#table-of-contents)

## Core Plugins

SlimTS ships with the following synthetic module plugins:

### console

A colorized Javascript console impementation.

```ts
import console from 'console'
```
[↑ Top](#table-of-contents)
### http

Built around the standard Fetch API, the `http` plugin provides top-level access to `fetch`, `Request`, `Response`, and related HTTP objects.

Also provides:
* A high-performance fetch implementation powered by C++ coroutines, io_uring, and kernel TLS.
* A promise-based async tcp server based on [SlimCommonNetworkServerTcp](https://codeberg.org/greergan/SlimCommonNetworkServerTcp).

```ts
import { Request, Response, Headers, fetch, server } from 'http';
```

[↑ Top](#table-of-contents)

# JavaScript Fetch API Support Roadmap: Epic Release Tickets

## Epic 1: Global Fetch & AbortController Infrastructure
- [ ] Implement `fetch()` Global Function
  - [ ] Support `Request` object or URL string as the first argument
  - [ ] Support `Init` options object as the second argument
- [ ] Implement `AbortController` Interface
  - [ ] Implement `AbortController.prototype.signal` property (returning `AbortSignal`)
  - [ ] Implement `AbortController.prototype.abort()` method
- [ ] Implement `AbortSignal` Interface
  - [ ] Implement `AbortSignal.prototype.aborted` boolean property
  - [ ] Implement `AbortSignal.prototype.reason` property
  - [ ] Implement `AbortSignal.prototype.onabort` event handler
  - [ ] Implement `AbortSignal.abort()` static method
  - [ ] Implement `AbortSignal.timeout()` static method

## Epic 2: Async HTTP Server & Core Network Transport
- [x] Implement Basic HTTP server request handling
- [ ] Implement default keep-alive support for the HTTP server

## Epic 3: Request Object Implementation
- [ ] Implement `Request.prototype.method` property
- [ ] Implement `Request.prototype.url` property
- [ ] Implement `Request.prototype.headers` property
- [ ] Implement `Request.prototype.destination` property
- [ ] Implement `Request.prototype.referrer` property
- [ ] Implement `Request.prototype.referrerPolicy` property
- [ ] Implement `Request.prototype.mode` property
- [ ] Implement `Request.prototype.credentials` property
- [ ] Implement `Request.prototype.cache` property
- [ ] Implement `Request.prototype.redirect` property
- [ ] Implement `Request.prototype.integrity` property
- [ ] Implement `Request.prototype.keepalive` property
- [ ] Implement `Request.prototype.signal` property
- [ ] Implement `Request.prototype.priority` property
- [ ] Implement `Request.prototype.clone()` method
- [ ] Implement `Request.prototype.body` property (Body Mixin)
- [ ] Implement `Request.prototype.bodyUsed` property (Body Mixin)
- [ ] Implement `Request.prototype.arrayBuffer()` method (Body Mixin)
- [ ] Implement `Request.prototype.blob()` method (Body Mixin)
- [ ] Implement `Request.prototype.formData()` method (Body Mixin)
- [ ] Implement `Request.prototype.json()` method (Body Mixin)
- [ ] Implement `Request.prototype.text()` method (Body Mixin)

## Epic 4: Response Object Implementation
- [ ] Implement `Response.prototype.type` property
- [ ] Implement `Response.prototype.url` property
- [ ] Implement `Response.prototype.redirected` property
- [ ] Implement `Response.prototype.status` property
- [ ] Implement `Response.prototype.ok` property
- [ ] Implement `Response.prototype.statusText` property
- [ ] Implement `Response.prototype.headers` property
- [ ] Implement `Response.prototype.clone()` method
- [ ] Implement `Response.error()` static method
- [ ] Implement `Response.redirect()` static method
- [ ] Implement `Response.json()` static method
- [ ] Implement `Response.prototype.body` property (Body Mixin)
- [ ] Implement `Response.prototype.bodyUsed` property (Body Mixin)
- [ ] Implement `Response.prototype.arrayBuffer()` method (Body Mixin)
- [ ] Implement `Response.prototype.blob()` method (Body Mixin)
- [ ] Implement `Response.prototype.formData()` method (Body Mixin)
- [ ] Implement `Response.prototype.json()` method (Body Mixin)
- [ ] Implement `Response.prototype.text()` method (Body Mixin)

## Epic 5: Headers Object & Guard Handling
- [ ] Implement `Headers.prototype.append()` method
- [ ] Implement `Headers.prototype.delete()` method
- [ ] Implement `Headers.prototype.get()` method
- [ ] Implement `Headers.prototype.has()` method
- [ ] Implement `Headers.prototype.set()` method
- [ ] Implement `Headers.prototype.forEach()` method
- [ ] Implement `Headers.prototype.entries()` method
- [ ] Implement `Headers.prototype.keys()` method
- [ ] Implement `Headers.prototype.values()` method
- [ ] Implement `none` guard logic for Headers
- [ ] Implement `request` guard logic for Headers
- [ ] Implement `request-no-cors` guard logic for Headers
- [ ] Implement `response` guard logic for Headers
- [ ] Implement `immutable` guard logic for Headers

## Epic 6: Advanced Network Features, CORS, and Caching
- [ ] Implement `follow` mode for Redirect Handling
- [ ] Implement `error` mode for Redirect Handling
- [ ] Implement `manual` mode for Redirect Handling
- [ ] Implement `same-origin` CORS mode
- [ ] Implement `no-cors` CORS mode
- [ ] Implement `cors` CORS mode
- [ ] Implement `omit` credentials management
- [ ] Implement `same-origin` credentials management
- [ ] Implement `include` credentials management
- [ ] Implement `default` cache mode
- [ ] Implement `no-store` cache mode
- [ ] Implement `reload` cache mode
- [ ] Implement `no-cache` cache mode
- [ ] Implement `force-cache` cache mode
- [ ] Implement `only-if-cached` cache mode

[↑ Top](#table-of-contents)


## Prerequisites

### Google V8

Several steps are required to get V8 into a compiled state.

1. Install, configure, and fetch the V8 source using [these instructions](https://v8.dev/docs/source-code#using-git)
2. Build the V8 engine using [the first 4 steps of these instructions](https://v8.dev/docs/embed#run-the-example) — this is an extremely long build
3. Checkout branch `13.1.1` prior to running the build

```sh
git checkout 13.1.1
```

### BoringSSL

The SlimTS SSL plugin is linked against [BoringSSL](https://boringssl.googlesource.com/boringssl) libraries.

```
libssl  libpki  libcrypto
```

The following headers are used:

```cpp
#include <openssl/rsa.h>
#include <openssl/x509.h>
```

The build may work with standard [OpenSSL](https://www.openssl.org/) but has not been attempted. Either way, the plugin compiles but is not in a usable state.

SlimTS uses standard installation paths when searching for required headers and libraries.

[↑ Top](#table-of-contents)

### SlimCommon

SlimCommon must be installed prior to building SlimTS. The source is available at [codeberg.org/greergan/SlimCommon](https://codeberg.org/greergan/SlimCommon).

[↑ Top](#table-of-contents)

### libtsgo

A C and C++ callable static library wrapping the TypeScript compiler. Must be installed prior to building SlimTS. Source available at [codeberg.org/greergan/libtsgo](https://codeberg.org/greergan/libtsgo).

[↑ Top](#table-of-contents)

## Building

SlimTS uses [GNU Autoconf](https://www.gnu.org/software/autoconf/) and [GNU Make](https://www.gnu.org/software/make/). It has only been built using [GCC](https://gcc.gnu.org/).

1. Clone the SlimTS source

```sh
git clone git@github.com:greergan/SlimTS.git
```

2. Run `autoreconf` from the SlimTS root directory

```sh
autoreconf -vfi
```

3. Run the configure script

```sh
./configure --prefix=/path/to/install \
    --with-v8=/path/to/google/v8
```

4. Build

```sh
make
make install
```

The output executable is named `slimts`.  
The plugin directory is expected at path/to/slimts/../lib/slimTS  

```
/usr/local/bin/slim
/usr/local/lib/slimTS
```

[↑ Top](#table-of-contents)

## Running

```sh
slim samples/hello_world.mjs
```

[↑ Top](#table-of-contents)
