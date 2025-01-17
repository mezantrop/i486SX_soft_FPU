# Bring back FPU emulation for i486SX CPU on NetBSD

Essentially, this patchest tries to bring back `options MATH_EMULATE` in the kernel configuration file by reverting
[Commit dfe83e0](https://github.com/NetBSD/src/commit/dfe83e08ca9688dd195a43113e7bc7c58fcdd14a)

## Disclaimer

**Warning! This is untested code, which potentially doesn't work**

## Installation

* Read [Chapter 32. Obtaining the sources](https://www.netbsd.org/docs/guide/en/chap-fetch.html)
* Read [Chapter 34. Compiling the kernel](https://www.netbsd.org/docs/guide/en/chap-kernel.html)

* Add the repository contents under /src/sys/arch on NetBSD 10.0 machine, then run:

``` sh
$ cd /usr/src/sys/arch/i386/conf/
$ vi GENERIC_TINY_486SX       # or GENERIC_PS2TINY_486SX or create your own kernel configuration with "options MATH_EMULATE"
$ config GENERIC_TINY_486SX
$ cd ../compile/GENERIC_TINY_486SX
$ make depend
$ make

# If everythig good, install the new kernel under root:
# mv /netbsd /netbsd.old
# mv netbsd /
```

## Contacts

If you have an idea, a question, or have found a problem, do not hesitate to open an issue or mail me directly: [Mikhail Zakharov](zmey20000@yahoo.com)
The changes I made to the original Linus Torvalds and NetBSD code are licensed under BSD-2-Clause license
