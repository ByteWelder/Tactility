# HW Acceleration using SIMD assembly instructions

Test app accommodates two types of tests: [`functionality test`](#Functionality-test) and [`benchmark test`](#Benchmark-test). Both tests are provided per each function written in assembly (typically per each assembly file). Both test apps use a hard copy of LVGL blending API, representing an ANSI implementation of the LVGL blending functions. The hard copy is present in [`lv_blend`](main/lv_blend/) folder.

Assembly source files could be found in the [`lvgl_port`](../../src/lvgl9/simd/) component. Header file with the assembly function prototypes is provided into the LVGL using Kconfig option `LV_DRAW_SW_ASM_CUSTOM_INCLUDE` and can be found in the [`lvgl_port/include`](../../include/esp_lvgl_port_lv_blend.h)

## Benchmark results

| Color format | Matrix size | Memory alignment |  ASM version   | ANSI C version |
| :----------- | :---------- | :--------------- | :------------- | :------------- |
| ARGB8888     | 128x128     |     16 byte      |     0.327      |     1.600      |
|              | 127x127     |      1 byte      |     0.488      |     1.597      |
| RGB565       | 128x128     |     16 byte      |     0.196      |     1.146      |
|              | 127x127     |      1 byte      |     0.497      |     1.124      |
* this data was obtained by running [benchmark tests](#benchmark-test) on 128x128 16 byte aligned matrix (ideal case) and 127x127 1 byte aligned matrix (worst case)
* the values represent cycles per sample to perform simple fill of the matrix on esp32s3

## Functionality test
* Tests, whether the HW accelerated assembly version of an LVGL function provides the same results as the ANSI version
* A top-level flow of the functionality test:
    * generate a test matrix with test parameters (matrix width, matrix height, memory alignment.. )
    * run an ANSI version of a DUT function with the generated input parameters
    * run an assembly version of a DUT function with the same input parameters
    * compare the results given by the ANSI and the assembly DUTs
    * the results shall be the same
    * repeat all the steps for a set of different input parameters, checking different matrix heights, widths..

## Benchmark test
* Tests, whether the HW accelerated assembly version of an LVGL function provides a performance increase over the ANSI version
* A top-level flow of the functionality test:
    * generate a test matrix with test parameters (matrix width, matrix height, memory alignment.. )
    * run an ANSI version of a DUT function with the generated input parameters multiple times (1000 times for example), while counting CPU cycles
    * run an assembly version of a DUT function with the generated input parameters multiple times (1000 times for example), while counting CPU cycles
    * compare the results given by the ANSI and the assembly DUTs
    * the assembly version of the DUT function shall be faster than the ANSI version of the DUT function

## Run the test app

The test app is intended to be used only with esp32 and esp32s3

    idf.py build

## Example output

```
I (302) main_task: Started on CPU0
I (322) main_task: Calling app_main()
______  _____ ______   _               _   
|  _  \/  ___|| ___ \ | |             | |  
| | | |\ `--. | |_/ / | |_   ___  ___ | |_ 
| | | | `--. \|  __/  | __| / _ \/ __|| __|
| |/ / /\__/ /| |     | |_ |  __/\__ \| |_ 
|___/  \____/ \_|      \__| \___||___/ \__|


Press ENTER to see the list of tests.



Here's the test menu, pick your combo:
(1)	"Test fill functionality ARGB8888" [fill][functionality][ARGB8888]
(2)	"Test fill functionality RGB565" [fill][functionality][RGB565]
(3)	"LV Fill benchmark ARGB8888" [fill][benchmark][ARGB8888]
(4)	"LV Fill benchmark RGB565" [fill][benchmark][RGB565]

Enter test for running.
```

### Example of a functionality test run

```
Running Test fill functionality ARGB8888...
I (81512) LV Fill Functionality: running test for ARGB8888 color format
I (84732) LV Fill Functionality: test combinations: 31824

MALLOC_CAP_8BIT usage: Free memory delta: 0 Leak threshold: -800 
MALLOC_CAP_32BIT usage: Free memory delta: 0 Leak threshold: -800 
./main/test_lv_fill_functionality.c:102:Test fill functionality ARGB8888:PASS
Test ran in 3242ms
```
The test gives a simple FAIL/PASS result after comparison of the two DUTs results.
Also gives us an information about how many combinations (input parameters) the functionality test run with, `31824` in this case.

### Example of a benchmark test run

```
Running LV Fill benchmark ARGB8888...
I (163492) LV Fill Benchmark: running test for ARGB8888 color format
I (163522) LV Fill Benchmark:  ASM ideal case: 5363.123 cycles for 128x128 matrix, 0.327 cycles per sample
I (163572) LV Fill Benchmark:  ASM corner case: 7868.724 cycles for 127x127 matrix, 0.488 cycles per sample

I (163732) LV Fill Benchmark:  ANSI ideal case: 26219.137 cycles for 128x128 matrix, 1.600 cycles per sample
I (163902) LV Fill Benchmark:  ANSI corner case: 25762.178 cycles for 127x127 matrix, 1.597 cycles per sample

MALLOC_CAP_8BIT usage: Free memory delta: -220 Leak threshold: -800 
MALLOC_CAP_8BIT potential leak: Before 393820 bytes free, After 393600 bytes free (delta 220)
MALLOC_CAP_32BIT usage: Free memory delta: -220 Leak threshold: -800 
MALLOC_CAP_32BIT potential leak: Before 393820 bytes free, After 393600 bytes free (delta 220)
./main/test_lv_fill_benchmark.c:69:LV Fill benchmark ARGB8888:PASS
Test ran in 458ms
```

The test provides couple of information:
* Total number of CPU cycles for the whole DUT function
    * `5363.123` cycles for the assembly DUT function
    * `26219.137` cycles for the ANSI DUT function
* Number of CPU cycles per sample, which is basically the total number of CPU cycles divided by the test matrix area
    * `0.327` cycles per sample for the assembly DUT
    * `1.6` cycles per sample for the ANSI DUT
    * In this case, the assembly implementation has achieved a performance increase in around 4.9-times, comparing to the ANSI implementation.
* Range of the CPU cycles (a best case and a corner case scenarios) into which, the DUT functions are expected to fit into
    * The execution time of those function highly depends on the input parameters, thus a boundary scenarios for input parameters shall be set
    * An example of such a boundaries is in a table below
    * The benchmark boundary would help us to get an performance expectations of the real scenarios

Example of an best and corner case input parameters for benchmark test, for a color format `ARGB8888`
| Test matrix params | Memory alignment | Width          | Height         | Stride         |
| :----------------- | :--------------- | :------------- | :------------- | :------------- |
| Best case          | 16-byte aligned  | Multiple of 8  | Multiple of 8  | Multiple of 8  |
| Corner case        | 1-byte aligned   | Not power of 2 | Not power of 2 | Not power of 2 |
