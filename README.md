# Bring back FPU emulation for i486SX CPU on NetBSD

Essentially, this patchest tries to bring back `options MATH_EMULATE` in the kernel configuration file by reverting
the [dfe83e0](https://github.com/NetBSD/src/commit/dfe83e08ca9688dd195a43113e7bc7c58fcdd14a) commit and adoprting it
to the current stage of NetBSD code.

## Disclaimer

  **Warning! This is a work-in-progress project: nothing runs properly!**

## FPU instructions checklist

| Instruction | Status          | Description                             | Opcode     | Example           |
| ----------- | --------------- | --------------------------------------- | ---------- | ----------------- |
| `fninit`    | ✅ Done        | Initialize FPU                          | `9B DB E3` | `fninit`          |
| `fld`       | ✅ Done        | Load floating-point value               | `D9 /0`    | `fld st(1)`       |
| `fstps`     | ✅ Done        | Store and pop single precision          | `D9 /3`    | `fstps [mem]`     |
| `fldt`      | ✅ Done        | Load 80-bit extended precision          | `DB /5`    | `fldt [mem]`      |
| `fadd`      | ✅ Done        | Add floating-point numbers              | `D8 /0`    | `fadd st(1), st`  |
| `faddl`     | ✅ Done        | Add long double precision               | `DA /0`    | `faddl [mem]`     |
| `fildl`     | 🔄 In Progress |                                         |            |                   |
| `fst`       | ⬜ Unchecked   | Store floating-point value              | `D9 /2`    | `fst st(1)`       |
| `fstp`      | ⬜ Unchecked   | Store and pop floating-point value      | `D9 /3`    | `fstp st(1)`      |
| `fstpt`     | ⬜ Unchecked   | Store 80-bit extended precision and pop | `DB /7`    | `fstpt [mem]`     |
| `faddp`     | ⬜ Unchecked   | Add and pop stack                       | `DE /0`    | `faddp st(1), st` |
| `fsub`      | ⬜ Unchecked   | Subtract floating-point numbers         | `D8 /4`    | `fsub st(1), st`  |
| `fmul`      | ⬜ Unchecked   | Multiply floating-point numbers         | `D8 /1`    | `fmul st(1), st`  |
| `fmulp`     | ⬜ Unchecked   | Multiply and pop stack                  | `DE /1`    | `fmulp st(1), st` |
| `fdiv`      | ⬜ Unchecked   | Divide floating-point numbers           | `D8 /6`    | `fdiv st(1), st`  |
| `fdivp`     | ⬜ Unchecked   | Divide and pop stack                    | `DE /6`    | `fdivp st(1), st` |
| `fcom`      | ⬜ Unchecked   | Compare floating-point values           | `D8 /2`    | `fcom st(1)`      |
| `fcomp`     | ⬜ Unchecked   | Compare and pop stack                   | `D8 /3`    | `fcomp st(1)`     |
| `fcompp`    | ⬜ Unchecked   | Compare and pop twice                   | `DA /3`    | `fcompp`          |
| ...         |                |                                          |            |                   |

## Installation

* Read [Chapter 32. Obtaining the sources](https://www.netbsd.org/docs/guide/en/chap-fetch.html)
* Read [Chapter 34. Compiling the kernel](https://www.netbsd.org/docs/guide/en/chap-kernel.html)

* Add the repository contents under `/src/sys/arch` on NetBSD 10.x machine, then run:

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

## Prebuilt drive image

It will appear eventually under [Releases](https://github.com/mezantrop/i486SX_soft_FPU/releases)

## Contacts

If you have an idea, a question, or have found a problem, do not hesitate to open an issue or mail me
directly: [Mikhail Zakharov](zmey20000@yahoo.com). My changes to the original Linus Torvalds and NetBSD code are
licensed under BSD-2-Clause license
