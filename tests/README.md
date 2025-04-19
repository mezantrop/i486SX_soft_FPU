# FPU instruction tests

| Test                | Result | Comment                     |
| ------------------- | ------ | ----------------------------|
| float_plus_ld       | OK     |                             |
| short_div_float     | OK     |                             |
| double_minus_float  | OK*    | some precision discrepancy? |
| ld_mul_int          | OK     |                             |
| ld_plus_neg_float   | OK*    | some precision discrepancy? |
| neg_double_mul_long | KO     | Sign missing!               |
| neg_ld_div_char     | OK     |                             |
