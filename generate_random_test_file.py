"""
Generate a random text file for testing RLE compression, and optionally generate
the expected RLE compressed output file.

Usage:
  python generate_random_test_file.py <filename> <length> <max_run> [--seed <seed>] [--rle]
    - <filename>: Output text file name
    - <length>: Approximate file length in characters
    - <max_run>: Maximum run length of repeated characters
    - --pnct: Include punctuation characters in the generated file
    - --seed <seed>: Optional random seed for reproducibility
    - --rle: Also generate .rle file with expected compression output
    - --line-width <width>: Generate a unified file with fixed line width (no escape chars)
    - --escape-chars <mode>: Escape characters mode
        1 = No escapes
        2 = All escapes (default)
        3 = Newline only
        4 = Spaces+tabs only

Example:
    python generate_random_test_file.py test.txt 1000 10 --seed 42 --rle
    -> This will create 'test.txt' with ~1000 characters and 'test.rle' with the expected RLE output.
       Seed 42 ensures reproducibility with the same input.
       
    python generate_random_test_file.py test2.txt 5000 5 --rle
    -> This will create 'test2.txt' with ~5000 characters and 'test2.rle' with the expected RLE output.
    
    python generate_random_test_file.py test3.txt 2000 20
    -> This will create 'test3.txt' with ~2000 characters without generating an RLE file.
    
    python generate_random_test_file.py test4.txt 1500 15 --escape-chars 3 --rle
    -> This will create 'test4.txt' with ~1500 characters including only newline as escape character and 'test4.rle' with the expected RLE output.
    
    python generate_random_test_file.py test5.txt 2500 8 --escape-chars 1 --rle
    -> This will create 'test5.txt' with ~2500 characters without any escape characters and 'test5.rle' with the expected RLE output.
    
    python generate_random_test_file.py test6.txt 3000 10 --line-width 80 --rle
    -> This will create 'test6.txt' with ~3000 characters formatted to 80 characters per line (no escape characters) and 'test6.rle' with the expected RLE output.
    
    python generate_random_test_file.py test7.txt 4000 12 --pnct --rle
    -> This will create 'test7.txt' with ~4000 characters including punctuation and 'test7.rle' with the expected RLE output.
    
Note:
- The generated text file will contain a mix of letters, digits, punctuation, spaces, newlines, and tabs based on punctuation and escape mode.
- The RLE compression rules are custom and always include the count, even if it's 1.
- The --line-width and --escape-chars options are mutually exclusive; only one can be used at a time.
"""

import random
import string
import argparse

ESCAPE_NONE = 1
ESCAPE_ALL = 2
ESCAPE_NEWLINE = 3
ESCAPE_WHITESPACE = 4

def generate_txt_test_file(filename: str, length: int, max_run: int, punctuation: bool=False, escape_chars: int = ESCAPE_ALL, seed: int = None) -> None:
    """Generate a text file with random characters and repeated runs for RLE testing."""
    if seed is not None:
        random.seed(seed)
        
    # Base character set (no escape chars)
    if punctuation:
        chars = string.ascii_letters + string.digits + string.punctuation
    else:
        chars = string.ascii_letters + string.digits

    # Add escape characters based on mode
    if escape_chars == ESCAPE_ALL:           # all escape chars
        chars += " \n\t"
    elif escape_chars == ESCAPE_NEWLINE:     # newline only
        chars += "\n"
    elif escape_chars == ESCAPE_WHITESPACE:  # whitespace + tab only
        chars += " \t"
    # escape_chars == ESCAPE_NONE means no extra chars

    with open(filename, "w") as f:
        generated = 0
        while generated < length:
            ch = random.choice(chars)
            run_length = random.randint(1, max_run)
            run_length = min(run_length, length - generated)
            f.write(ch * run_length)
            generated += run_length

    print(f"Generated {filename} with ~{length} characters (seed={seed}).")

    
def generate_txt_unified_file(filename: str, length: int, max_run: int, punctuation: bool=False, line_width: int = 80, seed: int = None) -> None:
    """Generate a file with fixed-width lines, each line separated by exactly one newline.
        No escape sequences (only letters, digits, punctuation)."""
    if seed is not None:
        random.seed(seed)

    if punctuation:
        chars = string.ascii_letters + string.digits + string.punctuation
    else:
        chars = string.ascii_letters + string.digits

    generated = 0
    line_len = 0

    with open(filename, "w") as f:
        while generated < length:
            ch = random.choice(chars)
            run_length = random.randint(1, max_run)
            run_length = min(run_length, length - generated)
            for _ in range(run_length):
                f.write(ch)
                generated += 1
                line_len += 1
                if line_len >= line_width:
                    f.write("\n")
                    line_len = 0

        # ensure file ends with newline
        if line_len > 0:
            f.write("\n")
            
    print (f"Generated unified file {filename} with ~{length} characters (seed={seed}, line_width={line_width}).")


def rle_compress(input_filename: str, output_filename: str) -> None:
    """
    Compress file using the rules (always include count, even if 1):
      'A'      -> 'A1'
      'AAA'    -> 'A3'
      '1'      -> '\\11'
      '111'    -> '\\13'
      '\\'     -> '\\\\1'
      '\\\\\\' -> '\\\\3'
      ' '      -> ' 1'
      '   '    -> ' 3'
      '\\n'    -> '\\n1'
      '\\n\\n' -> '\\n2'
      '\\t'    -> '\\t1'
      '\\t\\t' -> '\\t2'
    """
    with open(input_filename, "r") as f:
        data = f.read()

    if not data:
        open(output_filename, "w").close()
        return

    compressed = []
    count = 1
    prev = data[0]

    for ch in data[1:]:
        if ch == prev:
            count += 1
        else:
            compressed.append(encode_run(prev, count))
            prev = ch
            count = 1
    compressed.append(encode_run(prev, count))  # last run

    with open(output_filename, "w") as f:
        f.write("".join(compressed))

    print(f"Compressed {input_filename} -> {output_filename}")


def encode_run(ch: str, count: int) -> str:
    """Encode one run of a character based on the custom rules (always with count)."""
    if ch == "\n":
        return f"\\n{count}"
    elif ch == "\t":
        return f"\\t{count}"
    elif ch == " ":
        return f" {count}"
    elif ch == "\\":
        return f"\\\\{count}"
    elif ch.isdigit():
        return f"\\{ch}{count}"
    else:
        return f"{ch}{count}"


def main():
    parser = argparse.ArgumentParser(description="Generate random test file and optional RLE compressed file.")
    parser.add_argument("filename", help="Output text file name")
    parser.add_argument("length", type=int, help="Approximate file length in characters")
    parser.add_argument("max_run", type=int, help="Maximum run length of repeated characters")
    parser.add_argument("--pnct", action="store_true", help="Include punctuation characters in the generated file")
    parser.add_argument("--seed", type=int, help="Optional random seed for reproducibility", default=None, metavar="")
    parser.add_argument("--rle", action="store_true", help="Also generate .rle file with expected compression output")
    # Mutually exclusive group for line-width vs escape-chars
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--line-width", type=int, help="Line width for unified file generation (no escapes)", default=None, metavar="")
    group.add_argument("--escape-chars", type=int, choices=[1,2,3,4], default=2,
                        help="Escape characters mode {1, 2, 3, 4}: "
                            "1 = No escapes, "
                            "2 = All escapes, "
                            "3 = Newline only, "
                            "4 = Spaces+tabs only", metavar="")
    
    args = parser.parse_args()

    # Step 1: Generate text file
    if args.line_width is not None:
        generate_txt_unified_file(args.filename, args.length, args.max_run, args.pnct, args.line_width, args.seed)
    else:
        generate_txt_test_file(args.filename, args.length, args.max_run, args.pnct, args.escape_chars, args.seed)

    # Step 2: Optionally generate expected .rle file
    if args.rle:
        text_file_basename = args.filename.rsplit('.', 1)[0]
        rle_filename = text_file_basename + ".rle"
        rle_compress(args.filename, rle_filename)

if __name__ == "__main__":
    main()
