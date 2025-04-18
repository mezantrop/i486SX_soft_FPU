# Bring back FPU emulation for i486SX CPU on NetBSD

Essentially, this patchest tries to bring back `options MATH_EMULATE` to the kernel configuration file by reverting
the [dfe83e0](https://github.com/NetBSD/src/commit/dfe83e08ca9688dd195a43113e7bc7c58fcdd14a) commit and adopting it
to the current stage of NetBSD code.

## Disclaimer

  **Warning! This is a work-in-progress project: nothing runs properly!**

## FPU instructions checklist

| Instruction | Status   | Description                                     | Opcode     | Example              |
| ----------- | -------- | ----------------------------------------------- | ---------- | -------------------- |
| `fninit`    | ✅ OK   | Initialize FPU                                  | `9B DB E3` | `fninit`             |
| `fld`       | ✅ OK   | Load floating-point value                       | `D9 /0`    | `fld st(1)`          |
| `fstps`     | ✅ OK   | Store and pop single precision                  | `D9 /3`    | `fstps [mem]`        |
| `fstpt`     | ✅ OK   | Store 80-bit extended precision and pop         | `DB /7`    | `fstpt [mem]`        |
| `fldt`      | ✅ OK   | Load 80-bit extended precision                  | `DB /5`    | `fldt [mem]`         |
| `fadd`      | ✅ OK   | Add floating-point numbers                      | `D8 /0`    | `fadd st(1), st`     |
| `faddl`     | ✅ OK   | Add long double precision                       | `DA /0`    | `faddl [mem]`        |
| `filds`     | ✅ OK   | Load integer (short)                            | `DB /0`    | `filds [mem]`        |
| `fildl`     | ✅ OK   | load long integer                               | `DB /A`    | `fildl [mem]`        |
| `fdiv`      | ✅ OK   | Divide floating-point numbers                   | `D8 /6`    | `fdiv st(1), st`     |
| `fmul`      | ✅ OK   | Multiply floating-point numbers                 | `D8 /1`    | `fmul st(1), st`     |
| `fsub`      | ✅ OK   | Subtract floating-point numbers                 | `D8 /4`    | `fsub st(1), st`     |
| `fsubp`     | ✅ OK   | Subtract with pop                               | `DE /5`    | `fsubp st(1), st(0)` |
| `fcom`      | ✅ OK   | Compare floating-point values                   | `D8 /2`    | `fcom st(1)`         |
| `ftst`      | ✅ OK   | Compare ST(0) with 0.0                          | `D9 E4`    | `ftst`               |
| `fdivp`     | ✅ OK   | Divide with pop                                 | `DE /7`    | `fdivp st(1), st(0)` |
| `fucom`     | ✅ OK   | Unordered compare ST(0), ST(i)                  | `DD E0+i`  | `fucom st(1)`        |
| `fucomp`    | ✅ OK   | Unordered compare and pop                       | `DD E8+i`  | `fucomp st(1)`       |
| `fxch`      | ✅ OK   | Exchange ST(0) with ST(i)                       | `D9 C8+i`  | `fxch st(1)`         |
| `fchs`      | ✅ OK   | Change sign of ST(0)                            | `D9 E0`    | `fchs`               |
| `fabs`      | ✅ OK   | Absolute value of ST(0)                         | `D9 E1`    | `fabs`               |
| `frndint`   | ✅ OK   | Round ST(0) to integer, respecting control word | `D9 FC`    | `frndint`            |
| `fscale`    | 🕓 Todo | Scale ST(0) by ST(1)                            | `D9 FD`    | `fscale`             |
| `fsqrt`     | 🕓 Todo | Square root of ST(0)                            | `D9 FA`    | `fsqrt`              |
| ...         | ...      | ...                                             | ...        | ...                  |

## Installation

* Read [Chapter 32. Obtaining the sources](https://www.netbsd.org/docs/guide/en/chap-fetch.html)
* Read [Chapter 34. Compiling the kernel](https://www.netbsd.org/docs/guide/en/chap-kernel.html)

* Add the repository contents under `/src/sys/arch` on NetBSD 10.x machine, then run:

``` sh
$ cd /usr/src/sys/arch/i386/conf/
$ vi GENERIC_TINY_486SX# or GENERIC_PS2TINY_486SX or create your own kernel configuration with "options MATH_EMULATE"
$ config GENERIC_TINY_486SX
$ cd ../compile/GENERIC_TINY_486SX
$ make depend
$ make

# If everythig good, install the new kernel under root:
# mv /netbsd /netbsd.old
# mv netbsd /
```

## Prebuilt drive image

It will appear eventually under [Releases](https://github.com/mezantrop/i486SX_soft_FPU/releases)

## Contacts

If you have an idea, a question, or have found a problem, do not hesitate to open an issue or mail me
directly: [Mikhail Zakharov](zmey20000@yahoo.com). My changes to the original Linus Torvalds and NetBSD code are
licensed under BSD-2-Clause license
