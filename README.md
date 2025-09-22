# File Compression & Decompression in C

A C-based implementation of file compression and decompression algorithms, created as a project requirement for Udacity’s C Programming course.

> **Note:** This project was created solely for educational purposes as part of Udacity's C Programming course.  
> Unauthorized copying, reuse, or redistribution of this code is prohibited.

## Features

- Compresses files using Run-Length Encoding (RLE) algorithm.
- Decompresses files to their original format.
- Handles text files efficiently.
- Simple command-line interface for ease of use.

## Requirements

- GCC or any C compiler
- Linux or Windows environment

## Build Instruction

```
gcc ./src/compress.c ./src/decompress.c ./src/utils.c ./src/main.c -o compressor 
```

## Usage

```
./compressor -c <input_file> for compression
./compressor -d <input_file> for decompression
./compressor -h for help
```

### Examples

```
./compressor -c ./test_files/test.txt
./compressor -d ./test_files/test.rle
```

## Automated Testing

Run the provided automation script to build and test the project:

```
python build_and_test.py
```

This script will compile the C code, run a series of tests, and verify the output against expected results.

### Generating Custom Test Files

You can also generate custom test files using:

```
python generate_random_test_file.py <filename> <length> <max_run> [--pnct] [--seed SEED] [--line-width WIDTH | --escape-chars MODE] [--rle]
```

- `<filename>`: Name of the file to be generated (e.g., `test.txt`).
- `<length>`: Approximate length of the file in characters (e.g., `1000`).
- `<max_run>`: Maximum run length of repeated characters (e.g., `5`).
- `--pnct`: Include punctuation characters in the generated file.
- `--seed SEED`: Optional random seed for reproducibility.
- `--line-width WIDTH`: Generate a unified file with fixed line width (no escape sequences).
- `--escape-chars MODE`: Escape characters mode (1: No escapes, 2: All escapes, 3: Newline only, 4: Spaces+tabs only).
- `--rle`: Also generate a `.rle` file with expected compression output.

## License
This project is **not licensed** for reuse or redistribution.  


## Acknowledgments
Udacity for providing the C Programming course and project guidelines.
