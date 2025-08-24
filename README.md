# File Compression & Decompression in C
A C-based implementation of file compression and decompression algorithms, created as a project requirement for Udacity’s C Programming course.

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
gcc ./src/utils.c ./src/compress.c ./src/decompress.c  ./src/main.c -o compressor 
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

## Acknowledgments
Udacity for providing the C Programming course and project guidelines.
