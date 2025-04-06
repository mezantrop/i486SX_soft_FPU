# FPU instruction tests

| Test                | Result | Comment                                                              |
| ------------------- | ------ | -------------------------------------------------------------------- |
| float_plus_ld       | OK     |                                                                      |
| short_div_float     | KO     |                                                                      |
| double_minus_float  | OK*    | possible some precision discrepancy                                  |
| ld_mul_int          | KO     |                                                                      |
| ld_plus_neg_float   | KO     | 4.000002L + -1.000001L returned 5.000000. Incorrect sign processing? |
| neg_double_mul_long | KO     |                                                                      |
| neg_ld_div_char     | KO     |                                                                      |
