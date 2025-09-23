#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "../header_files/utils.h"

// Helper function to get the size of the file in bytes if it is opened in binary mode
static s32 get_file_size_binary_mode(FILE *p_file, u64 *pu64_file_size)
{
    s32 s32_ret_val = FAILURE_STATUS;

    if (NULL == p_file || NULL == pu64_file_size)
    {
        LOG_ERROR("NULL pointer provided for file or file size.");
        s32_ret_val = ERROR_NULL_POINTER;
    }
    else
    {
        s64 s64_current_pos = ftell(p_file);

        if (s64_current_pos == -1L)
        {
            LOG_ERROR("Error getting current file position: %s", strerror(errno));
            s32_ret_val = ERROR_FILE_READ_FAILED;
        }
        else if (fseek(p_file, 0, SEEK_END) != 0)
        {
            LOG_ERROR("Error seeking to end of file: %s", strerror(errno));
            s32_ret_val = ERROR_FILE_READ_FAILED;
        }
        else
        {
            s64 s64_file_size = ftell(p_file);
            if (s64_file_size == -1L)
            {
                LOG_ERROR("Error getting file size: %s", strerror(errno));
                s32_ret_val = ERROR_FILE_READ_FAILED;
            }
            else
            {
                *pu64_file_size = (u64)s64_file_size;                

                LOG("File size: %lu bytes", *pu64_file_size);
                s32_ret_val = SUCCESS_STATUS;
            }

            // Restore original file position
            if (fseek(p_file, s64_current_pos, SEEK_SET) != 0)
            {
                LOG_ERROR("Error restoring file position: %s", strerror(errno));
                s32_ret_val = ERROR_FILE_READ_FAILED;
            }
        }
    }

    return s32_ret_val;
}

// Helper function to get the size of the file in bytes if it is opened in text mode
static s32 get_file_size_text_mode(FILE *p_file, u64 *pu64_file_size)
{
    s32 s32_ret_val = FAILURE_STATUS;

    if (NULL == p_file || NULL == pu64_file_size)
    {
        LOG_ERROR("NULL pointer provided for file or file size.");
        s32_ret_val = ERROR_NULL_POINTER;
    }
    else
    {
        // Reset file position to the beginning
        if (fseek(p_file, 0, SEEK_SET) != 0)
        {
            LOG_ERROR("Error seeking to beginning of file: %s", strerror(errno));
            s32_ret_val = ERROR_FILE_READ_FAILED;
        }
        else
        {
            u64 u64_size = 0;
            int ch;

            while ((ch = fgetc(p_file)) != EOF)
            {
                u64_size++;
            }

            if (ferror(p_file))
            {
                LOG_ERROR("Error reading file to determine size: %s", strerror(errno));
                s32_ret_val = ERROR_FILE_READ_FAILED;
            }
            else
            {
                *pu64_file_size = u64_size;
                LOG("File size: %lu bytes", *pu64_file_size);
                s32_ret_val = SUCCESS_STATUS;
            }

            // Restore original file position
            if (fseek(p_file, 0, SEEK_SET) != 0)
            {
                LOG_ERROR("Error restoring file position: %s", strerror(errno));
                s32_ret_val = ERROR_FILE_READ_FAILED;
            }
        }
    }

    return s32_ret_val;
}


/**
 * @brief Log message based on the log level
 * 
 * @param[in] enu_log_level Log level of the message
 * @param[in] pc_formatted_msg Formatted message string
 * @param ... Additional arguments for the formatted message
 * @return void
 */
void log_message(tenu_log_level enu_log_level, const char *pc_formatted_msg, ...)
{
    if (enu_log_level > LOG_LEVEL || LOG_LEVEL == LOG_LEVEL_NONE) return;

    const char *string_log_level = "";
    FILE *out_file = stdout;

    switch (enu_log_level) 
    {
        case LOG_LEVEL_DEBUG: string_log_level = "DEBUG"; break;
        case LOG_LEVEL_INFO:  string_log_level = "INFO "; break;
        case LOG_LEVEL_ERROR: string_log_level = "ERROR"; out_file = stderr; break;
        default: return; // Do not log if NONE or unknown
    }

    fprintf(out_file, "[%s] ", string_log_level);

    va_list args;
    va_start(args, pc_formatted_msg);

    vfprintf(out_file, pc_formatted_msg, args);

    va_end(args);

    fprintf(out_file, "\n");
}

/**
 * @brief Open a file with the specified mode
 * 
 * @param[in] pc_ile_name file name to open
 * @param[in] mode  mode to open the file in (e.g., "r", "w", "rb", "wb")
 * @param[in out] ppf_input_file pointer to the opened file pointer
 * @return s32 SUCCESS_STATUS on success, error code otherwise
 */
s32 open_file(const char *pc_ile_name, const char *mode, FILE **ppf_input_file)
{
    s32 s32_ret_val = FAILURE_STATUS;

    if (NULL == pc_ile_name || NULL == mode || NULL == ppf_input_file)
    {
        LOG_ERROR("NULL pointer provided for file name, mode, or input file pointer.");
        return ERROR_NULL_POINTER;
    }
    else
    {
        *ppf_input_file = fopen(pc_ile_name, mode);

        if (NULL == *ppf_input_file)
        {
            LOG_ERROR("Error opening file '%s': %s", pc_ile_name, strerror(errno));
            s32_ret_val = ERROR_FILE_NOT_OPENED;
        }
        else
        {
            LOG("File '%s' opened successfully.", pc_ile_name);
            s32_ret_val = SUCCESS_STATUS;
        }
    }

    return s32_ret_val;
}

/**
 * @brief Close the given file and assign NULL to its pointer
 * 
 * @param[in] pp_file Pointer to the file pointer to close
 * @return s32 SUCCESS_STATUS on success, error code otherwise 
 */
s32 close_file(FILE **pp_file)
{
    s32 s32_ret_val = FAILURE_STATUS;

    if (NULL == *pp_file)
    {
        LOG_ERROR("Attempted to close a NULL file pointer.");
        s32_ret_val = ERROR_NULL_POINTER;
    }
    else if (fclose(*pp_file) != 0)
    {
        LOG_ERROR("Error closing file: %s", strerror(errno));
        s32_ret_val = ERROR_FILE_NOT_CLOSED;
    }
    else
    {
        LOG("File closed successfully.");
        *pp_file = NULL;
        s32_ret_val = SUCCESS_STATUS;
    }

    return s32_ret_val;
}

/**
 * @brief 
 * 
 * @param[in] p_file Pointer to the file to read from
 * @param[in out] ppc_read_data_buff Pointer to the buffer that will hold the read data
 * @param[in out] pu64_read_data_size Pointer to the variable that will hold the size of the read data
 * @return s32 SUCCESS_STATUS on success, error code otherwise  
 */
s32 read_file(FILE *p_file, char **ppc_read_data_buff, u64 *pu64_read_data_size)
{
    s32 s32_ret_val = FAILURE_STATUS;

    if (NULL == p_file || NULL == ppc_read_data_buff || NULL == pu64_read_data_size)
    {
        LOG_ERROR("NULL pointer provided for file or read buffer or read size.");
        s32_ret_val = ERROR_NULL_POINTER;
    }
    else
    {
        do
        {
            s32_ret_val = get_file_size_text_mode(p_file, pu64_read_data_size);
            ERROR_BREAK(s32_ret_val);

            if (0 == *pu64_read_data_size)
            {
                LOG_ERROR("File is empty.");
                s32_ret_val = ERROR_EMPTY_FILE;
                break;
            }

            *ppc_read_data_buff = (char *)malloc(*pu64_read_data_size);

            if (NULL == *ppc_read_data_buff)
            {
                LOG_ERROR("Error allocating memory for read data buffer: %s", strerror(errno));
                s32_ret_val = ERROR_MEMORY_ALLOCATION_FAILED;
                break;
            }

            memset(*ppc_read_data_buff, 0, *pu64_read_data_size);
            
            u64 u64_read_bytes = fread(*ppc_read_data_buff, sizeof(char), *pu64_read_data_size, p_file);

            if (ferror(p_file))
            {
                LOG_ERROR("Error reading file: %s", strerror(errno));
                s32_ret_val = ERROR_FILE_READ_FAILED;

                if (NULL != *ppc_read_data_buff)
                {
                    free(*ppc_read_data_buff);
                    *ppc_read_data_buff = NULL;
                }

                break;
            }
            else if (u64_read_bytes != *pu64_read_data_size)
            {
                LOG_ERROR("Read bytes (%lu) do not match expected file size (%lu).", u64_read_bytes, *pu64_read_data_size);
                s32_ret_val = ERROR_FILE_READ_FAILED;

                if (NULL != *ppc_read_data_buff)
                {
                    free(*ppc_read_data_buff);
                    *ppc_read_data_buff = NULL;
                }

                break;
            }
            else
            {
                LOG("Read %lu bytes successfully.", *pu64_read_data_size);
                s32_ret_val = SUCCESS_STATUS;
            }
            
        } while (0);
    }

    return s32_ret_val;
}

/**
 * @brief 
 * 
 * @param[in] p_file Pointer to the file to write to
 * @param[in] pc_write_buffer Pointer to the buffer containing data to write
 * @param[in] u64_write_size Size of the data to write
 * @return s32 SUCCESS_STATUS on success, error code otherwise  
 */
s32 write_file(FILE *p_file, const char *pc_write_buffer, const u64 u64_write_size)
{
    s32 s32_ret_val = FAILURE_STATUS;

    if (NULL == p_file || NULL == pc_write_buffer)
    {
        LOG_ERROR("NULL pointer provided for file or write buffer.");
        s32_ret_val = ERROR_NULL_POINTER;
    }
    else if (0 == u64_write_size)
    {
        LOG_ERROR("Size of the data to write is zero.");
        s32_ret_val = ERROR_INVALID_LENGTH;
    }
    else
    {
        size_t written_size = fwrite(pc_write_buffer, sizeof(char), u64_write_size, p_file);

        if (written_size != u64_write_size)
        {
            LOG_ERROR("Error writing to file: %s", strerror(errno));
            s32_ret_val = ERROR_FILE_WRITE_FAILED;
        }
        else
        {
            LOG("Successfully wrote %zu bytes to file.", written_size);
            s32_ret_val = SUCCESS_STATUS;
        }
    }

    return s32_ret_val;
}

/**
 * @brief Check the existence of a file
 * 
 * @param[in] pc_file_name Path to the file to check its existence
 * @return true if file exists, false otherwise
 */
bool check_file_exists(const char *pc_file_name)
{
    bool b_file_exist = false;

    if (pc_file_name == NULL)
    {
        LOG_ERROR("NULL pointer provided for file name.");
    }
    else
    {
        FILE *p_file = fopen(pc_file_name, "r");

        if (NULL == p_file)
        {
            LOG("File %s does not exist", pc_file_name);
        }
        else
        {
            LOG("File %s exists", pc_file_name);
            b_file_exist = true;
            fclose(p_file);
        }
    }

    return b_file_exist;
}

/**
 * @brief  Delete the specified file
 * 
 * @param[in] pc_file_name Path to the file to delete
 * @return true if file exists, false otherwise
 */
s32 delete_file(const char *pc_file_name)
{
    s32 s32_ret_val = FAILURE_STATUS;

    if (NULL == pc_file_name)
    {
        s32_ret_val = ERROR_NULL_POINTER;
    }
    else
    {
        if (0 == remove(pc_file_name))
        {
            s32_ret_val == SUCCESS_STATUS;
            LOG_INFO("File %s deleted successfully.", pc_file_name);
        }
        else
        {
            s32_ret_val = ERROR_DELETE_FILE;
            LOG_ERROR("Error deleteing file: %s", strerror(errno));
        }
    }

    return s32_ret_val;
}

/**
 * @brief Get the file basename object
 * 
 * @param[in] pc_file_path Path to the file
 * @param[in out] pc_file_basename Buffer to hold the file basename (without extension)
 * @return s32 SUCCESS_STATUS on success, error code otherwise 
 */
s32 get_file_basename(const char *pc_file_path, char *pc_file_basename)
{
    s32 s32_ret_val = FAILURE_STATUS;

    if (NULL == pc_file_path || NULL == pc_file_basename)
    {
        s32_ret_val = ERROR_NULL_POINTER;
    }
    else
    {
        const char *pc_last_dot = strrchr(pc_file_path, '.');
        const char *pc_last_slash = strrchr(pc_file_path, '/');

        if ((NULL == pc_last_dot) || (NULL != pc_last_slash && pc_last_dot < pc_last_slash))
        {
            s32_ret_val = ERROR_FILE_EXTENSION;
        }
        else
        {
            u8 u8_file_extension_len = strlen(pc_last_dot); // include the dot itself
            strncpy(pc_file_basename, pc_file_path, strlen(pc_file_path) - u8_file_extension_len);
            pc_file_basename[strlen(pc_file_path) - u8_file_extension_len] = '\0'; // Null-terminate the string
            LOG("File basename: %s", pc_file_basename);
            s32_ret_val = SUCCESS_STATUS;
        }
    }

    return s32_ret_val;
}

/**
 * @brief Get the file extension
 * 
 * @param[in] pc_file_path Path to the file
 * @param[in out] pc_file_extension Buffer to hold the file extension (without dot)
 * @return s32 SUCCESS_STATUS on success, error code otherwise  
 */
s32 get_file_extension(const char *pc_file_path, char *pc_file_extension)
{
    s32 s32_ret_val = FAILURE_STATUS;

    if (NULL == pc_file_path || NULL == pc_file_extension)
    {
        s32_ret_val = ERROR_NULL_POINTER;
    }
    else
    {
        const char *pc_last_dot = strrchr(pc_file_path, '.');
        const char *pc_last_slash = strrchr(pc_file_path, '/');

        if ((NULL == pc_last_dot) || (NULL != pc_last_slash && pc_last_dot < pc_last_slash))
        {
            s32_ret_val = ERROR_FILE_EXTENSION;
        }
        else
        {
            strncpy(pc_file_extension, pc_last_dot + 1, strlen(pc_last_dot + 1));
            pc_file_extension[strlen(pc_last_dot + 1)] = '\0'; // Null-terminate the string
            LOG("File extension: %s", pc_file_extension);
            s32_ret_val = SUCCESS_STATUS;
        }
    }

    return s32_ret_val;
}

/**
 * @brief Add file extension to a file basename
 * 
 * @param[in] pc_file_basename File basename without extension 
 * @param[in] pc_extension Extension to add (without dot)
 * @return s32 SUCCESS_STATUS on success, error code otherwise 
 */
s32 add_file_extension(char *pc_file_basename, const char *pc_extension)
{
    s32 s32_ret_val = FAILURE_STATUS;

    if (NULL == pc_file_basename || NULL == pc_extension)
    {
        s32_ret_val = ERROR_NULL_POINTER;
    }
    else
    {   
        snprintf(&pc_file_basename[strlen(pc_file_basename)], strlen(pc_extension) + 2, ".%s", pc_extension);
        LOG("File path after adding extension: %s", pc_file_basename);
        s32_ret_val = SUCCESS_STATUS;
    }

    return s32_ret_val;
}

/**
 * @brief Create a output file path based on the input file path and desired extension
 * 
 * @param[in] pc_input_file_path Path to the input file
 * @param[in] pc_output_file_extention Desired output file extension (without dot)
 * @param[in out] ppc_out_file_path Pointer to hold the generated output file path
 * @return s32 SUCCESS_STATUS on success, error code otherwise 
 */
s32 create_output_file(const char *pc_input_file_path, char *pc_output_file_extention, char **ppc_out_file_path)
{
    s32 s32_ret_val = FAILURE_STATUS;

    if (NULL == pc_input_file_path || NULL == pc_output_file_extention || NULL == ppc_out_file_path)
    {
        s32_ret_val = ERROR_NULL_POINTER;
    }
    else
    {
        size_t max_out_path_len = strlen(pc_input_file_path) + strlen(pc_output_file_extention) + 4; // +4 for safety as we may have a suffix _xyz
        *ppc_out_file_path = (char *)malloc(max_out_path_len);
        char *pc_out_file_path_shadow = (char *)malloc(max_out_path_len);

        if (NULL == *ppc_out_file_path || NULL == pc_out_file_path_shadow)
        {
            LOG_ERROR("Error allocating memory for out file path or its shadow: %s", strerror(errno));
            s32_ret_val = ERROR_MEMORY_ALLOCATION_FAILED;
        }
        else
        {
            memset(*ppc_out_file_path, 0, max_out_path_len);
            memset(pc_out_file_path_shadow, 0, max_out_path_len);

            do
            {
                s32_ret_val = get_file_basename(pc_input_file_path, *ppc_out_file_path);
                ERROR_BREAK(s32_ret_val);

                u16 u16_out_file_basename_len = strlen(*ppc_out_file_path);

                s32_ret_val = add_file_extension(*ppc_out_file_path, pc_output_file_extention);
                ERROR_BREAK(s32_ret_val);

                strncpy(pc_out_file_path_shadow, *ppc_out_file_path, strlen(*ppc_out_file_path));
                pc_out_file_path_shadow[strlen(*ppc_out_file_path)] = '\0'; // Null-terminate the string

                u8 u8_file_suffix_cnt = 1;

                LOG("Checking file existence: %s", pc_out_file_path_shadow);
                while (true == check_file_exists(pc_out_file_path_shadow))
                {
                    snprintf(&pc_out_file_path_shadow[u16_out_file_basename_len], 8, "_%d.%s", u8_file_suffix_cnt, pc_output_file_extention);
                    LOG("Checking file existence: %s", pc_out_file_path_shadow);
                    u8_file_suffix_cnt++;
                }
                strncpy(*ppc_out_file_path, pc_out_file_path_shadow, strlen(pc_out_file_path_shadow));
                free_allocated_memory(pc_out_file_path_shadow);
                LOG("Generated output file path: %s", *ppc_out_file_path);

            } while (0);
        }
    }

    return s32_ret_val;
}

/**
 * @brief Free allocated memory and set its pointer to NULL
 * 
 * @param[in] pv_data Pointer to the allocated memory to be freed
 * @return void
 */
void free_allocated_memory(void *pv_data)
{
    if (NULL != pv_data)
    {
        free(pv_data);
        pv_data = NULL;
    }
}

/**
 * @brief Print the program usage instructions
 * 
 * @param[in] pc_prog_name Name of the program (usually argv[0])
 * @return void
 */
void print_prog_usage(const char *pc_prog_name)
{
    printf("Usage:\n");
    printf("%s -c <input_file> for compression\n", pc_prog_name);
    printf("%s -d <input_file> for decompression\n", pc_prog_name);
    printf("%s -h to see this menu\n", pc_prog_name);
}

/**
 * @brief Parse command line input arguments
 * 
 * @param[in] argc Number of command line arguments
 * @param[in] argv Array of command line argument strings
 * @param[in out] pstr_args Pointer to the structure to hold parsed arguments
 * @return void
 */
void parse_input_args(int argc, const char *argv[], tstr_input_args *pstr_args) 
{
    if (NULL == pstr_args)
    {
        LOG_ERROR("NULL pointer provided for args structure.");
        return;
    }
    else
    {
        pstr_args->enu_operation = OP_HELP;

        if (argc < 2)
        {
            LOG_ERROR("No arguments given");
        }
        else if (0 == strcmp(argv[1], "-h"))
        {
            LOG("Help argument detected");
        }
        else if ((0 == strcmp(argv[1], "-c") || 0 == strcmp(argv[1], "-d")) && argc == 3)
        {
            pstr_args->enu_operation = (argv[1][1] == 'c') ? OP_COMPRESS : OP_DECOMPRESS;
            pstr_args->pc_input_file = argv[2];
        }
        else
        {
            LOG_ERROR("Invalid arguments");
        }
    }
}
