"""
A comprehensive build-and-test script for a C project that implements
a simple run-length encoding (RLE) compression algorithm.

This script compiles the C code, runs multiple test suites to validate
the functionality, and provides detailed feedback on any failures.

It supports different validation modes for certain tests and allows
users to keep or discard temporary files after the tests.

It also generates a summary report of the test results if requested.
And by default, it generates a separate report for failed tests.

Usage:
    python build_and_test.py [options]
Options:
    -s, --suite <number>       Run a specific test suite (0-4).
    -v, --validation-mode      Set validation mode for UI tests: 'strict' (default) or 'warn'.
    -r, --report               Generate a detailed test report.
    -k, --keep-files           Keep all temporary files after the run.
    -l, --list-tests           List available test suites and exit.
    -h, --help                 Show this help message and exit.
Examples:
    python build_and_test.py
    python build_and_test.py -s 1 -v warn -k
    python build_and_test.py --list-tests
    python build_and_test.py -r
Notes:
    - Ensure that 'gcc' is installed and available in your system's PATH.
    - The C source files should be located in the 'src/' directory.
    - Test files should be located in the 'test_files/' directory.
    - Temporary files are created in the 'temp_test_run/' directory.
    - The C executable is named 'compressor'.
"""

import os
import subprocess
import filecmp
import shutil
import glob
import argparse
import re
from typing import Callable
from datetime import datetime
from collections import defaultdict

# --- Configuration ---
C_EXECUTABLE          = "./compressor"
SOURCE_DIR            = "./src/"
SOURCE_TEST_FILES_DIR = "./test_files/"
TEMP_WORK_DIR         = "./temp_test_run/"
REPORT_FILE           = "test_report.txt"
FAILURE_REPORT_FILE   = "failed_test_report.txt"

# --- Color Constants for Output ---
class Colors:
    SUCCESS = '\033[92m' # Green
    FAILURE = '\033[91m' # Red
    WARNING = '\033[93m' # Yellow
    RESET   = '\033[0m'  # Reset to default color

# --- Exit Codes ---
FAILED = False
PASSED = True
SKIPPED = None

# =====================================================================
#  HELPER AND API FUNCTIONS
# =====================================================================

def compile_project() -> bool:
    """Compiles the C project using GCC."""
    print("--- Compiling C Project ---")
    source_files = glob.glob(os.path.join(SOURCE_DIR, '*.c'))
    if not source_files:
        print(f"  {Colors.FAILURE}FAILURE{Colors.RESET}: No .c source files found in the 'src/' directory.")
        return False
    compile_command = ['gcc'] + source_files + ['-o', C_EXECUTABLE]
    print(f"  -> Running command: {' '.join(compile_command)}")
    try:
        result = subprocess.run(compile_command, check=True, capture_output=True, text=True)
        if result.stderr:
            print(f"  {Colors.WARNING}Compiler Warnings:{Colors.RESET}\n" + result.stderr)
        print(f"  {Colors.SUCCESS}SUCCESS:{Colors.RESET} Project compiled successfully.\n")
        return True
    except FileNotFoundError:
        print(f"  {Colors.FAILURE}FAILURE:{Colors.RESET} 'gcc' command not found. Is GCC installed and in your system's PATH?")
        return False
    except subprocess.CalledProcessError as e:
        print(f"  {Colors.FAILURE}FAILURE:{Colors.RESET} Compilation failed. See error message below.\n" + e.stderr)
        return False

def diagnose_failure(original_path: str, generated_path: str) -> str:
    """Analyzes two non-matching files to determine the likely cause of the failure."""
    try:
        with open(original_path, 'rb') as f1, open(generated_path, 'rb') as f2:
            original_content = f1.read()
            generated_content = f2.read()
            
        original_stripped = original_content.replace(b' ', b'').replace(b'\n', b'').replace(b'\r', b'').replace(b'\t', b'')
        generated_stripped = generated_content.replace(b' ', b'').replace(b'\n', b'').replace(b'\r', b'').replace(b'\t', b'')
        if original_stripped == generated_stripped:
            return "Core content matches, but there are differences in whitespace."
            
        if len(original_content) != len(generated_content):
            return (f"File size mismatch. Expected {len(original_content)} bytes, " f"but got {len(generated_content)} bytes.")
        
        if original_content.count(b'\n') != generated_content.count(b'\n'):
            return "The number of newline characters differs."
        
        return "A byte-for-byte comparison failed due to a core logic error."
    except FileNotFoundError:
        return "One of the files for comparison could not be found."
    except Exception as e:
        return f"An unexpected error occurred during diagnosis: {e}"

def sanitize_for_test_case_path(text: str) -> str:
    """Converts a description string into a valid directory name."""
    text = text.replace(' ', '_').replace('/', '_').replace('(', '').replace(')', '')
    return re.sub(r'[^a-zA-Z0-9_-]', '', text)

def run_test_case(description: str, test_logic_func: Callable[..., bool], keep_all_files: bool=False) -> dict:
    """
    A generic test case runner that creates a unique subdirectory for each test
    and cleans up only on success.
    """
    print(f"  -> {description}...", end="")
    
    case_dir_name = sanitize_for_test_case_path(description)
    case_work_dir = os.path.join(TEMP_WORK_DIR, case_dir_name)
    
    if os.path.exists(case_work_dir):
        shutil.rmtree(case_work_dir)
    os.makedirs(case_work_dir)
    
    test_passed = FAILED
    result_details = ""
    
    try:
        result_tuple = test_logic_func(case_work_dir)
        test_passed = result_tuple[0]
        
        if test_passed == PASSED:
            print(f" {Colors.SUCCESS}SUCCESS{Colors.RESET}")
        else:
            failure_message = result_tuple[1]
            # Check for optional file paths for diagnosis
            if len(result_tuple) == 4:
                golden_path, generated_path = result_tuple[2], result_tuple[3]
                diagnosis = diagnose_failure(golden_path, generated_path)
                result_details = f"{failure_message}\n      -> Diagnosis: {diagnosis}"
            else:
                result_details = failure_message
            print(f" {Colors.FAILURE}FAILURE{Colors.RESET}: {failure_message}")
        
    except subprocess.TimeoutExpired as e:
        print(f" {Colors.FAILURE}FAILURE{Colors.RESET}: The C program timed out after {e.timeout} seconds (likely an infinite loop).")
    except subprocess.CalledProcessError as e:
        print(f" {Colors.FAILURE}FAILURE{Colors.RESET}: The C program crashed or returned an unexpected error (Exit Code {e.returncode}).")
        if e.stderr:
            print(f"      -> Stderr: {e.stderr.strip()}")
    except Exception as e:
        print(f" {Colors.FAILURE}FAILURE{Colors.RESET}: An unexpected error occurred in the Python test script: {e}")
    finally:
        if test_passed and not keep_all_files:
            shutil.rmtree(case_work_dir)
    
    return {
        "description": description,
        "status": "PASS" if test_passed else "FAIL",
        "details": result_details
    }

def cleanup_temp_dirs() -> None:
    """Removes the top-level temporary directory if it's empty."""
    try:
        if os.path.exists(TEMP_WORK_DIR) and not os.listdir(TEMP_WORK_DIR):
            os.rmdir(TEMP_WORK_DIR)
    except OSError:
        pass

def generate_report(all_results: list[dict], overall_summary: dict, report_file: str=REPORT_FILE) -> None:
    """Generates a text file report from the collected test results."""
    print(f"\nGenerating test report to '{report_file}'...")
    with open(report_file, "w") as f:
        f.write("======================================\n")
        f.write("         Test Automation Report         \n")
        f.write("======================================\n\n")
        f.write(f"Run Date: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
        
        f.write("--- Overall Summary ---\n")
        f.write(f"Total Suites Run: {overall_summary['passed'] + overall_summary['failed']  + overall_summary['skipped']}\n")
        f.write(f"Suites Passed: {overall_summary['passed']}\n")
        f.write(f"Suites Failed: {overall_summary['failed']}\n")
        f.write(f"Suites Skipped: {overall_summary['skipped']}\n")
        f.write("-----------------------\n\n")

        # Group results by suite
        grouped_results = defaultdict(list)
        for res in all_results:
            grouped_results[res['suite']].append(res)

        for suite_name, cases in grouped_results.items():
            f.write(f"--- {suite_name} ---\n")
            for case in cases:
                f.write(f"  [{case['status']}] {case['description']}\n")
                if case['status'] == 'FAIL':
                    # Indent details for readability
                    indented_details = "\n".join([f"    -> {line.strip()}" for line in case['details'].split('\n')])
                    f.write(f"{indented_details}\n")
            f.write("\n")
    print("Report generation complete.")

# =====================================================================
#  TEST SUITE IMPLEMENTATIONS
# =====================================================================
def run_suite(suite_name: str, test_cases: list[Callable[..., bool]], keep_files_flag: bool) -> tuple[bool, list[dict]]:
    """A generic function to run a list of test cases for a suite."""
    suite_results = []
    for desc, logic in test_cases:
        result = run_test_case(desc, logic, keep_files_flag)
        result['suite'] = suite_name
        suite_results.append(result)
    
    suite_passed = all(res['status'] == 'PASS' for res in suite_results)
    return suite_passed, suite_results

# =====================================================================
#  SUITE 0: SHOW HELP MESSAGE
# =====================================================================
def run_show_help_suite(golden_help_message: str) -> tuple[bool, list[dict]]:
    """SUITE 0: Prints the captured help message for visual inspection."""
    print("  -> Displaying the captured output of './compressor -h':\n")
    print("---------- Program Help Output ----------")
    print(golden_help_message)
    print("---------------------------------------")
    return PASSED, [{"suite": "Suite 0: Show Help Message", "description": "Show Help Message", "status": "PASS", "details": ""}]

# =====================================================================
#  SUITE 1: USER INTERFACE & INPUT HANDLING TESTS
# =====================================================================
def run_ui_and_input_tests(golden_help_message: str, validation_mode: str, keep_files_flag: bool) -> tuple[bool, list[dict]]:
    """SUITE 1: Runs tests for invalid arguments, non-existent files, and other user input errors."""
    
    def test_bad_args_prints_usage(args_to_test, work_dir): # work_dir is unused but required by the interface of run_test_case
        result = subprocess.run([C_EXECUTABLE] + args_to_test, capture_output=True, text=True, timeout=10)
        full_output = (result.stderr + result.stdout).strip()
        if full_output.endswith(golden_help_message):
            return (PASSED,)
        else:
            return (FAILED, f"Output did not end with the expected usage message.\n      -> Expected (to end with): '{golden_help_message}'\n      -> Got: '{full_output}'")

    def test_non_existent_file(work_dir):
        try:
            non_existent_file = os.path.join(work_dir, "this_file_does_not_exist.txt")
            subprocess.run([C_EXECUTABLE, "-c", non_existent_file], check=True, capture_output=True, text=True, timeout=10)
            return (FAILED, "Program did not error on a non-existent file.")
        except subprocess.CalledProcessError as e:
            error_message = e.stderr.lower()
            # Check for common keywords indicating a file-not-found error
            expected_keywords = ["error", "open", "opening", "no such file", "does not", "doesn't", "exist", "cannot", "can't", "found"]
            if any(k in error_message for k in expected_keywords):
                return (PASSED,)
            else:
                return (FAILED, f"Incorrect error message for non-existent file. Got: '{e.stderr.strip()}'")

    def test_empty_file_compression(work_dir, mode):
        with open(os.path.join(work_dir, "empty.txt"), "w") as f: pass
        try:
            subprocess.run([C_EXECUTABLE, "-c", os.path.join(work_dir, "empty.txt")], check=True, capture_output=True, text=True, timeout=10)
            if mode == 'strict':
                return(FAILED, "Program did not error out on an empty input file as expected.")
            else:
                return(PASSED, "Program did not error out on an empty input file.")
        except subprocess.CalledProcessError as e:
            error_message = e.stderr.lower()
            # Check for keywords indicating an empty file error
            expected_keywords = ["empty", "zero", "length"]
            if not any(keyword in error_message for keyword in expected_keywords):
                if mode == 'strict':
                    return(FAILED, f"Program errored, but stderr message was not as expected. Got: '{e.stderr.strip()}'")
                else:
                    return(PASSED, f"Program errored, but stderr message was not as expected. Got: '{e.stderr.strip()}'")
            
            if os.path.exists(os.path.join(work_dir, "empty.rle")):
                if mode == 'strict':
                    return(FAILED, f"Program created an output file despite the empty input.")
                else:
                    return(PASSED, f"Program created an output file despite the empty input.")
                
            return (PASSED,)

    def test_empty_file_decompression(work_dir, mode):
        with open(os.path.join(work_dir, "empty.rle"), "w") as f: pass
        try:
            subprocess.run([C_EXECUTABLE, "-d", os.path.join(work_dir, "empty.rle")], check=True, capture_output=True, text=True, timeout=10)
            if mode == 'strict':
                return(FAILED, f"Program did not error out on an empty input file as expected.")
            else:
                return(PASSED, f"Program did not error out on an empty input file.")
        except subprocess.CalledProcessError as e:
            error_message = e.stderr.lower()
            # Check for keywords indicating an empty file error
            expected_keywords = ["empty", "zero", "length"]
            if not any(keyword in error_message for keyword in expected_keywords):
                if mode == 'strict':
                    return(FAILED, f"Program errored, but stderr message was not as expected. Got: '{e.stderr.strip()}'")
                else:
                    return(PASSED, f"Program errored, but stderr message was not as expected. Got: '{e.stderr.strip()}'")

            if os.path.exists(os.path.join(work_dir, "empty.txt")):
                if mode == 'strict':
                    return(FAILED, f"Program created an output file despite the empty input.")
                else:
                    return(PASSED, f"Program created an output file despite the empty input.")
                    
            return (PASSED,)

    def test_wrong_extension_for_compression(work_dir, mode):
        with open(os.path.join(work_dir, "file.rle"), "w") as f: f.write("dummy content")
        try:
            subprocess.run([C_EXECUTABLE, "-c", os.path.join(work_dir, "file.rle")], check=True, capture_output=True, text=True, timeout=10)
            if mode == 'strict':
                return(FAILED, f"Program did not error when compressing a .rle file.")
            else:
                return(PASSED, f"Program did not error when compressing a .rle file.")
        except subprocess.CalledProcessError as e:
            error_message = e.stderr.lower()
            # Check for common keywords indicating a wrong extension error
            expected_keywords = ["invalid", "extension", "file", "type"]
            if any(k in error_message for k in expected_keywords):
                return (PASSED,)
            else:
                if mode == 'strict':
                    return(FAILED, f"Incorrect error message for wrong extension. Got: '{e.stderr.strip()}'")
                else:
                    return(PASSED, f"Incorrect error message for wrong extension. Got: '{e.stderr.strip()}'")

    def test_wrong_extension_for_decompression(work_dir, mode):
        with open(os.path.join(work_dir, "file.txt"), "w") as f: f.write("dummy content")
        try:
            subprocess.run([C_EXECUTABLE, "-d", os.path.join(work_dir, "file.txt")], check=True, capture_output=True, text=True, timeout=10)
            if mode == 'strict':
                return(FAILED, f"Program did not error when decompressing a .txt file.")
            else:
                return(PASSED, f"Program did not error when decompressing a .txt file.")
        except subprocess.CalledProcessError as e:
            error_message = e.stderr.lower()
            # Check for common keywords indicating a wrong extension error
            expected_keywords = ["invalid", "extension", "file", "type"]
            if any(k in error_message for k in expected_keywords):
                return (PASSED,)
            else:
                if mode == 'strict':
                    return(FAILED, f"Incorrect error message for wrong extension. Got: '{e.stderr.strip()}'")
                else:
                    return(PASSED, f"Incorrect error message for wrong extension. Got: '{e.stderr.strip()}'")

    # Define all UI tests
    ui_tests = [
        ("Testing with no arguments", lambda work_dir: test_bad_args_prints_usage([], work_dir)),
        ("Testing with too few arguments", lambda work_dir: test_bad_args_prints_usage(["-c"], work_dir)),
        ("Testing with too many arguments", lambda work_dir: test_bad_args_prints_usage(["-c", "f1.txt", "f2.txt"], work_dir)),
        ("Testing with only a flag", lambda work_dir: test_bad_args_prints_usage(["-c"], work_dir)),
        ("Testing with only an invalid flag", lambda work_dir: test_bad_args_prints_usage(["-x"], work_dir)),
        ("Testing with an invalid flag and a filename", lambda work_dir: test_bad_args_prints_usage(["-x", "file.txt"], work_dir)),
        ("Testing with both -c and -d flags and a filename", lambda work_dir: test_bad_args_prints_usage(["-c", "-d", "file.txt"], work_dir)),
        ("Testing with -h flag", lambda work_dir: test_bad_args_prints_usage(["-h"], work_dir)),
        ("Testing with only a filename", lambda work_dir: test_bad_args_prints_usage(["file.txt"], work_dir)),
        ("Testing with two filenames", lambda work_dir: test_bad_args_prints_usage(["file1.txt", "file2.txt"], work_dir)),
        ("Testing with a flag and two filenames", lambda work_dir: test_bad_args_prints_usage(["-c", "file1.txt", "file2.txt"], work_dir)),
        ("Testing non-existent input file", test_non_existent_file),
        (f"Testing graceful failure on empty file compression (mode: {validation_mode})", lambda work_dir: test_empty_file_compression(work_dir, validation_mode)),
        (f"Testing graceful failure on empty file decompression (mode: {validation_mode})", lambda work_dir: test_empty_file_decompression(work_dir, validation_mode)),
        (f"Testing wrong file extension for compression (mode: {validation_mode})", lambda work_dir: test_wrong_extension_for_compression(work_dir, validation_mode)),
        (f"Testing wrong file extension for decompression (mode: {validation_mode})", lambda work_dir: test_wrong_extension_for_decompression(work_dir, validation_mode)),
    ]
    
    return run_suite("Suite 1: User Interface & Input Handling", ui_tests, keep_files_flag)

def run_output_file_naming_tests(keep_files_flag: bool) -> tuple[bool, list[dict]]:
    """SUITE 2: Runs all feature-specific tests related to file naming conflicts."""
    
    def test_single_comp_conflict(work_dir):
        base_name = "conflict"
        with open(os.path.join(work_dir, f"{base_name}.txt"), "w") as f: f.write("content")
        with open(os.path.join(work_dir, f"{base_name}.rle"), "w") as f: f.write("dummy")
        subprocess.run([C_EXECUTABLE, "-c", os.path.join(work_dir, f"{base_name}.txt")], check=True, capture_output=True, text=True, timeout=10)
        if not os.path.exists(os.path.join(work_dir, f"{base_name}_1.rle")):
            return(FAILED, f"'_1.rle' file not created.")
        return (PASSED,)

    def test_single_decomp_conflict(work_dir):
        base_name = "conflict"
        with open(os.path.join(work_dir, f"{base_name}.txt"), "w") as f: f.write("dummy")
        with open(os.path.join(work_dir, f"{base_name}.rle"), "w") as f: f.write("3A")
        subprocess.run([C_EXECUTABLE, "-d", os.path.join(work_dir, f"{base_name}.rle")], check=True, capture_output=True, text=True, timeout=10)
        if not os.path.exists(os.path.join(work_dir, f"{base_name}_1.txt")):
            return(FAILED, f"'_1.txt' file not created.")
        return (PASSED,)

    def test_multi_comp_conflict(work_dir):
        base_name = "multi"
        with open(os.path.join(work_dir, f"{base_name}.txt"), "w") as f: f.write("content")
        with open(os.path.join(work_dir, f"{base_name}.rle"), "w") as f: f.write("dummy")
        for i in range(1, 10):
            with open(os.path.join(work_dir, f"{base_name}_{i}.rle"), "w") as f: f.write("dummy")
        subprocess.run([C_EXECUTABLE, "-c", os.path.join(work_dir, f"{base_name}.txt")], check=True, capture_output=True, text=True, timeout=10)
        if not os.path.exists(os.path.join(work_dir, f"{base_name}_10.rle")):
            return(FAILED, f"'_10.rle' file not created.")
        return (PASSED,)

    def test_multi_decomp_conflict(work_dir):
        base_name = "multi"
        with open(os.path.join(work_dir, f"{base_name}.rle"), "w") as f: f.write("3A")
        with open(os.path.join(work_dir, f"{base_name}.txt"), "w") as f: f.write("dummy")
        for i in range(1, 10):
            with open(os.path.join(work_dir, f"{base_name}_{i}.txt"), "w") as f: f.write("dummy")
        subprocess.run([C_EXECUTABLE, "-d", os.path.join(work_dir, f"{base_name}.rle")], check=True, capture_output=True, text=True, timeout=10)
        if not os.path.exists(os.path.join(work_dir, f"{base_name}_10.txt")):
            return(FAILED, f"'_10.txt' file not created.")
        return (PASSED,)

    file_naming_tests = [
        ("Testing single compression conflict (_1.rle)", test_single_comp_conflict),
        ("Testing single decompression conflict (_1.txt)", test_single_decomp_conflict),
        ("Testing multiple compression conflicts (_10.rle)", test_multi_comp_conflict),
        ("Testing multiple decompression conflicts (_10.txt)", test_multi_decomp_conflict),
    ]
    
    return run_suite("Suite 2: Output File Naming", file_naming_tests, keep_files_flag)

def run_decoupled_correctness_tests(keep_files_flag: bool) -> tuple[bool|None, list[dict]]:
    """SUITE 3: Runs decoupled tests for compression and decompression against known-good files."""
    
    tests_found = 0
    test_cases = []

    # Find all .txt files and check for corresponding .rle files for compression test
    txt_files = [f for f in os.listdir(SOURCE_TEST_FILES_DIR) if f.endswith('.txt')]
    for txt_filename in txt_files:
        base_name, _ = os.path.splitext(txt_filename)
        rle_filename = base_name + ".rle"
        # Ensure the corresponding .rle file exists
        if not os.path.exists(os.path.join(SOURCE_TEST_FILES_DIR, rle_filename)): continue
        
        tests_found += 1
        def create_comp_test_logic(txt_filename, rle_filename, base_name):
            def comp_test_logic(work_dir):
                shutil.copy(os.path.join(SOURCE_TEST_FILES_DIR, txt_filename), work_dir)
                shutil.copy(os.path.join(SOURCE_TEST_FILES_DIR, rle_filename), work_dir)
                subprocess.run([C_EXECUTABLE, "-c", os.path.join(work_dir, txt_filename)], check=True, capture_output=True, text=True, timeout=100)
                program_output = os.path.join(work_dir, base_name + "_1.rle")
                golden_file = os.path.join(work_dir, rle_filename)
                
                if not os.path.exists(program_output):
                    return(FAILED, f"Program did not create an output file.")
                
                if not filecmp.cmp(program_output, golden_file, shallow=False):
                    return (FAILED, "Output does not match known-good .rle file.", golden_file, program_output)
                
                return (PASSED,)
            return comp_test_logic
        
        test_cases.append((f"Verifying compression of '{txt_filename}'", create_comp_test_logic(txt_filename, rle_filename, base_name)))

    # Find all .rle files and check for corresponding .txt files for decompression test
    rle_files = [f for f in os.listdir(SOURCE_TEST_FILES_DIR) if f.endswith('.rle')]
    for rle_filename in rle_files:
        base_name, _ = os.path.splitext(rle_filename)
        txt_filename = base_name + ".txt"
        # Ensure the corresponding .txt file exists
        if not os.path.exists(os.path.join(SOURCE_TEST_FILES_DIR, txt_filename)): continue
        
        tests_found += 1
        def create_decomp_test_logic(rle_filename, txt_filename, base_name):
            def decomp_test_logic(work_dir):
                shutil.copy(os.path.join(SOURCE_TEST_FILES_DIR, rle_filename), work_dir)
                shutil.copy(os.path.join(SOURCE_TEST_FILES_DIR, txt_filename), work_dir)
                subprocess.run([C_EXECUTABLE, "-d", os.path.join(work_dir, rle_filename)], check=True, capture_output=True, text=True, timeout=100)
                program_output = os.path.join(work_dir, base_name + "_1.txt")
                golden_file = os.path.join(work_dir, txt_filename)
                
                if not os.path.exists(program_output):
                    return(FAILED, f"Program did not create an output file.")
                
                if not filecmp.cmp(program_output, golden_file, shallow=False):
                    return (FAILED, "Output does not match known-good .txt file.", golden_file, program_output)
                
                return (PASSED,)
            return decomp_test_logic

        test_cases.append((f"Verifying decompression of '{rle_filename}'", create_decomp_test_logic(rle_filename, txt_filename, base_name)))

    if tests_found == 0:
        print(f"  {Colors.WARNING}SKIPPED:{Colors.RESET} No matching '.txt' and '.rle' file pairs found in '{SOURCE_TEST_FILES_DIR}'.")
        return (SKIPPED, [{"suite": "Suite 3: Decoupled Compression/Decompression Correctness", 
                           "description": f"No matching '.txt' and '.rle' file pairs found in '{SOURCE_TEST_FILES_DIR}'.",
                           "status": "SKIP", "details": ""}])

    return run_suite("Suite 3: Decoupled Compression/Decompression Correctness", test_cases, keep_files_flag)

def run_round_trip_integrity_tests(keep_files_flag: bool) -> tuple[bool|None, list[dict]]:
    """SUITE 4: Performs a full compress -> decompress cycle and verifies integrity."""
    
    txt_files = [f for f in os.listdir(SOURCE_TEST_FILES_DIR) if f.endswith('.txt')]
    if not txt_files:
        print(f"  {Colors.WARNING}SKIPPED:{Colors.RESET} No .txt files found to test.")
        return (SKIPPED, [{"suite": "Suite 4: Round-Trip Integrity", 
                           "description": "No .txt files found to test.",
                           "status": "SKIP", "details": ""}])
    
    test_cases = []
    for txt_filename in txt_files:
        
        def create_round_trip_logic(txt_filename):
            def test_logic(work_dir):
                original_source_path = os.path.join(SOURCE_TEST_FILES_DIR, txt_filename)
                working_txt_path = os.path.join(work_dir, txt_filename)
                shutil.copy(original_source_path, working_txt_path)
                
                base_name, _ = os.path.splitext(txt_filename)
                compressed_path = os.path.join(work_dir, base_name + ".rle")
                decompressed_path = os.path.join(work_dir, base_name + "_1.txt")
                
                subprocess.run([C_EXECUTABLE, "-c", working_txt_path], check=True, capture_output=True, text=True, timeout=100)
                subprocess.run([C_EXECUTABLE, "-d", compressed_path], check=True, capture_output=True, text=True, timeout=100)
                
                if not os.path.exists(decompressed_path):
                    return(FAILED, f"Decompression did not create an output file.")
                
                if not filecmp.cmp(original_source_path, decompressed_path, shallow=False):
                    return (FAILED, "Decompressed file does not match original.", original_source_path, decompressed_path)
                
                return (PASSED,)
            return test_logic

        test_cases.append((f"Testing round-trip for '{txt_filename}'", create_round_trip_logic(txt_filename)))
            
    return run_suite("Suite 4: Round-Trip Integrity", test_cases, keep_files_flag)

# =====================================================================
#  MAIN FUNCTION
# =====================================================================
def main():
    """Parses command-line arguments and runs the selected test suites."""

    suite_definitions = {
        0: ("Suite 0: Show Help Message", None),
        1: ("Suite 1: User Interface & Input Handling", None),
        2: ("Suite 2: Output File Naming", None),
        3: ("Suite 3: Decoupled Compression/Decompression Correctness", None),
        4: ("Suite 4: Round-Trip Integrity", None)
    }

    parser = argparse.ArgumentParser(
        description="A comprehensive build-and-test script for the C compressor project.",
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument(
        '-s', '--suite',
        type=int,
        choices=suite_definitions.keys(),
        help="Run a specific test suite:\n" +
             "\n".join([f"{num}: {desc}" for num, (desc, _) in sorted(suite_definitions.items())]) +
             "\n(Default: Run all suites)"
    )
    parser.add_argument(
        '-v', '--validation-mode',
        choices=['strict', 'warn'],
        default='strict',
        help="Set the mode for certain UI validation tests:\n"
             "'strict': Fail the test case on an invalid error message (default).\n"
             "'warn':   Issue a warning but pass the test case."
    )
    parser.add_argument(
        '-r', '--report',
        action='store_true',
        help="Generate a 'test_report.txt' file summarizing the run."
    )
    parser.add_argument(
        '-k', '--keep-files',
        action='store_true',
        help="Keep all temporary files after the run, even for successful tests."
    )
    parser.add_argument(
        '-l', '--list-tests',
        action='store_true',
        help="List available test suites and exit."
    )
    args = parser.parse_args()

    if args.list_tests:
        print("Available test suites:")
        for num, (desc, _) in sorted(suite_definitions.items()):
            print(f"  {num}: {desc}")
        exit(0)

    if not compile_project():
        exit(1)

    try:
        help_result = subprocess.run([C_EXECUTABLE, "-h"], check=True, capture_output=True, text=True, timeout=5)
        golden_help_message = (help_result.stderr + help_result.stdout).strip()
        if not golden_help_message:
            print(f"{Colors.FAILURE}FATAL:{Colors.RESET} Could not retrieve a valid help/usage message from './compressor -h'. Exiting.")
            exit(1)
    except Exception as e:
        print(f"{Colors.FAILURE}FATAL:{Colors.RESET} Failed to get help message from './compressor -h': {e}. Exiting.")
        exit(1)
    
    # Setup top-level temp directory
    if os.path.exists(TEMP_WORK_DIR):
        shutil.rmtree(TEMP_WORK_DIR)
    os.makedirs(TEMP_WORK_DIR)

    # Update suites that need dynamic arguments
    suite_definitions[0] = (suite_definitions[0][0], lambda: run_show_help_suite(golden_help_message))
    suite_definitions[1] = (suite_definitions[1][0], lambda: run_ui_and_input_tests(golden_help_message, args.validation_mode, args.keep_files))
    suite_definitions[2] = (suite_definitions[2][0], lambda: run_output_file_naming_tests(args.keep_files))
    suite_definitions[3] = (suite_definitions[3][0], lambda: run_decoupled_correctness_tests(args.keep_files))
    suite_definitions[4] = (suite_definitions[4][0], lambda: run_round_trip_integrity_tests(args.keep_files))

    results = {"passed": 0, "failed": 0, "skipped": 0}
    all_test_results = []
    fail_test_results = []
    suites_to_run = sorted(suite_definitions.keys()) if args.suite is None else [args.suite]

    for suite_num in suites_to_run:
        desc, suite_func = suite_definitions[suite_num]
        print(f"\n======================================")
        print(f"  {desc}")
        print(f"======================================\n")
        
        suit_result, suite_results = suite_func()
        all_test_results.extend(suite_results)
        
        if suit_result == PASSED:
            results["passed"] += 1
        elif suit_result == FAILED:
            results["failed"] += 1
            fail_test_results.extend(suite_results)
        else:
            results["skipped"] += 1
    
    print("\n--- Overall Test Summary ---")
    total_tests = results['passed'] + results['failed'] + results['skipped']
    print(f"Total Test Suites Run: {total_tests}")
    print(f"Passed : {Colors.SUCCESS}{results['passed']}{Colors.RESET}")
    print(f"Failed : {Colors.FAILURE if results['failed'] > 0 else ''}{results['failed']}{Colors.RESET}")
    print(f"Skipped: {Colors.WARNING if results['skipped'] > 0 else ''}{results['skipped']}{Colors.RESET}")
    print("----------------------------")
    
    if args.report:
        generate_report(all_test_results, results)

    # Final cleanup logic
    if results["failed"] > 0:
        # Create a separate failure-only report by default regardless of args.report
        generate_report(fail_test_results, results, report_file=FAILURE_REPORT_FILE)
        
        print(f"\n{Colors.WARNING}Run complete. Artifacts for failed tests are preserved in '{TEMP_WORK_DIR}'.{Colors.RESET}")
    elif args.keep_files:
        print(f"\n{Colors.WARNING}Run complete. All test artifacts kept in '{TEMP_WORK_DIR}' as requested.{Colors.RESET}")
    else:
        print("\nAll conducted tests passed. Cleaning up temporary files...")
        cleanup_temp_dirs()

    # Exit with code 1 if any tests failed
    if results["failed"] > 0:
        exit(1)

if __name__ == "__main__":
    main()
