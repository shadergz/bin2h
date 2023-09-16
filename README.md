# bin2h
- A simple 'binary to C-header' program that can work as platform-independent (just needs a C compiler to do the work)

## Usage example
- A simple usage example that dumps eleven bytes from the self bin2h executable file into the default output filename (resolved at runtime)

```bash
./bin2h bin2h --symbol-name=binary_file_dump --count=11 --skip=10 --column-size=12
```

- The result from the above command (bin2h.h):

```c
/*  Auto generated header file by 'bin2h' (VERSION: 0.0.1)
 *  at 21:03:04 - 12/28/21
*/

#pragma once

#if defined (__cplusplus)
extern "C" {
#endif

unsigned long long binary_file_dump_size = 11;

unsigned char binary_file_dump[11] = {
    0x7f, 0x45, 0x4c, 0x46, 0x02, 0x01, 0x01, 0x00, 
    0x00, 0x00, 0x00
};

#if defined (__cplusplus)
}
#endif

```

## How to compile
- Just type:

```bash
mkdir build
cd build
build> cmake ..
build> make
```

## How to install
- Simply type the following command in the build directory as root:

```bash
build> make install
```
