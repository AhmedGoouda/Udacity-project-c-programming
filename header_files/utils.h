#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdbool.h>

#include "constants.h"

// typedefs for various data types
typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned int    u32;
typedef unsigned long   u64;
typedef signed   char   s8;
typedef signed   short  s16;
typedef signed   int    s32;
typedef signed   long   s64;
typedef float           f32;
typedef double          f64;


// Enum for operation type
typedef enum {
    OP_NONE,
    OP_COMPRESS,
    OP_DECOMPRESS,
    OP_HELP
} tenu_operation;

// Struct to hold parsed arguments
typedef struct {
    tenu_operation enu_operation;
    const char *pc_input_file;
} tstr_input_args;

// Log level enum, including NONE
typedef enum {
    LOG_LEVEL_NONE = 0,   // No logs at all
    LOG_LEVEL_ERROR,      // Only errors
    LOG_LEVEL_INFO,       // Info and errors
    LOG_LEVEL_DEBUG       // Debug, info, and errors
} tenu_log_level;

// Logging macros
#define LOG(fm, ...)       log_message(LOG_LEVEL_DEBUG, fm, ##__VA_ARGS__)
#define LOG_INFO(fm, ...)  log_message(LOG_LEVEL_INFO,  fm, ##__VA_ARGS__)
#define LOG_ERROR(fm, ...) log_message(LOG_LEVEL_ERROR, fm, ##__VA_ARGS__)

// Error break macro
#define ERROR_BREAK(value)                                                                \
    if (SUCCESS_STATUS != value)                                                          \
    {                                                                                     \
        fprintf(stderr, "Error: %d, at File: %s, Line: %d\n", value, __FILE__, __LINE__); \
        break;                                                                            \
    }
\

// Set the global log level here
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

// Function prototypes

/**
 * @brief Log message based on the log level
 * 
 * @param[in] enu_log_level Log level of the message
 * @param[in] pc_formatted_msg Formatted message string
 * @param ... Additional arguments for the formatted message
 * @return void
 */
void log_message(tenu_log_level enu_log_level, const char *pc_formatted_msg, ...);

/**
 * @brief Open a file with the specified mode
 * 
 * @param[in] pc_ile_name file name to open
 * @param[in] mode  mode to open the file in (e.g., "r", "w", "rb", "wb")
 * @param[in out] ppf_input_file pointer to the opened file pointer
 * @return s32 SUCCESS_STATUS on success, error code otherwise
 */
s32 open_file(const char *pc_ile_name, const char *mode, FILE **ppf_input_file);

/**
 * @brief Close the given file
 * 
 * @param[in] p_file Pointer to the file to be closed
 * @return s32 SUCCESS_STATUS on success, error code otherwise 
 */
s32 close_file(FILE *p_file);

/**
 * @brief 
 * 
 * @param[in] p_file Pointer to the file to read from
 * @param[in out] ppc_read_data_buff Pointer to the buffer that will hold the read data
 * @param[in out] pu64_read_data_size Pointer to the variable that will hold the size of the read data
 * @return s32 SUCCESS_STATUS on success, error code otherwise  
 */
s32 read_file(FILE *p_file, char **ppc_read_data_buff, u64 *pu64_read_data_size);

/**
 * @brief 
 * 
 * @param[in] p_file Pointer to the file to write to
 * @param[in] pc_write_buffer Pointer to the buffer containing data to write
 * @param[in] u64_write_size Size of the data to write
 * @return s32 SUCCESS_STATUS on success, error code otherwise  
 */
s32 write_file(FILE *p_file, const char *pc_write_buffer, const u64 u64_write_size);

/**
 * @brief Check the existence of a file
 * 
 * @param[in] pc_file_name Path to the file to check its existence
 * @return true if file exists, false otherwise
 */
bool check_file_exists(const char *pc_file_name);

/**
 * @brief Get the file basename object
 * 
 * @param[in] pc_file_path Path to the file
 * @param[in out] pc_file_basename Buffer to hold the file basename (without extension)
 * @return s32 SUCCESS_STATUS on success, error code otherwise 
 */
s32 get_file_basename(const char *pc_file_path, char *pc_file_basename);

/**
 * @brief Get the file extension
 * 
 * @param[in] pc_file_path Path to the file
 * @param[in out] pc_file_extension Buffer to hold the file extension (without dot)
 * @return s32 SUCCESS_STATUS on success, error code otherwise  
 */
s32 get_file_extension(const char *pc_file_path, char *pc_file_extension);


/**
 * @brief Add file extension to a file basename
 * 
 * @param[in] pc_file_basename File basename without extension 
 * @param[in] pc_extension Extension to add (without dot)
 * @return s32 SUCCESS_STATUS on success, error code otherwise 
 */
s32 add_file_extension(char *pc_file_basename, const char *pc_extension);

/**
 * @brief Create a output file path based on the input file path and desired extension
 * 
 * @param[in] pc_input_file_path Path to the input file
 * @param[in] pc_output_file_extention Desired output file extension (without dot)
 * @param[in out] ppc_out_file_path Pointer to hold the generated output file path
 * @return s32 SUCCESS_STATUS on success, error code otherwise 
 */
s32 create_output_file(const char *pc_input_file_path, char *pc_output_file_extention, char **ppc_out_file_path);

/**
 * @brief Free allocated memory and set its pointer to NULL
 * 
 * @param[in] pv_data Pointer to the allocated memory to be freed
 * @return void
 */
void free_allocated_memory(void *pv_data);

/**
 * @brief Print the program usage instructions
 * 
 * @param[in] pc_prog_name Name of the program (usually argv[0])
 * @return void
 */
void print_prog_usage(const char *pc_prog_name);

/**
 * @brief Parse command line input arguments
 * 
 * @param[in] argc Number of command line arguments
 * @param[in] argv Array of command line argument strings
 * @param[in out] pstr_args Pointer to the structure to hold parsed arguments
 * @return void
 */
void parse_input_args(int argc, const char *argv[], tstr_input_args *pstr_args) ;

#endif // UTILS_H
