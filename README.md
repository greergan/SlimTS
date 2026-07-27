<a href="https://github.com/greergan/SlimTS">
  <img src="https://raw.githubusercontent.com/greergan/SlimTS/master/assets/slimts_logo.png" width="75" alt="SlimTS Logo">
</a>

# SlimTS

A TypeScript platform written in C++ targeting the C++23 standard. Only ES6 modules are supported. CommonJS is not supported.

## Table of Contents

- [Overview](#overview)
- [TypeScript Compiler Options](#typescript-compiler-options)
- [Security](#security)
- [Console Behavior](#console-behavior)
- [Prerequisites](#prerequisites)
  - [Google V8](#google-v8)
  - [BoringSSL](#boringssl)
  - [SlimCommon](#slimcommon)
  - [libtsgo](#libtsgo)
- [Building](#building)
- [Running](#running)

## Overview

SlimTS takes in TypeScript or JavaScript ES6 modules, applies type checking and executes them via an embedded V8 engine. CommonJS `require` is not supported.

[↑ Top](#table-of-contents)

## TypeScript Compiler Options

SlimTS uses the `compilerOptions` from [libtsgo](https://codeberg.org/greergan/libtsgo).

[↑ Top](#table-of-contents)

## Security

The security model is lazy-loaded plugins. Even the console is a plugin. This allows for different behavior from the same object under different circumstances.

[↑ Top](#table-of-contents)

## Console Behavior

The console is a plugin and is not loaded by default. If you are not seeing output from console statements, you have not imported the console.

```ts
import console from 'console'
```

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

The output executable is named `slim`. It expects to be installed so that it can locate its plugin files.

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
