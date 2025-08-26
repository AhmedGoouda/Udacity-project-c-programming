/* Include any necessary libraries and header files */
#include<stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "../header_files/utils.h"
#include "../header_files/compress.h"


/**
 * @brief Compress data using Run-Length Encoding (RLE)
 * 
 * @param[in] pc_input_data Input data to be compressed
 * @param[in] u64_input_data_size Size of the input data
 * @param[in out] pc_output_data Buffer to hold the compressed output data
 * @param[in out] pu64_output_data_size Pointer to hold the size of the compressed data
 * @return s32 SUCCESS_STATUS on success, error code otherwise 
 */
static s32 s32_rle_compress(const char *pc_input_data, const u64 u64_input_data_size, char *pc_output_data, u64 *pu64_output_data_size)
{
    s32 s32_ret_val = FAILURE_STATUS;

    if (NULL == pc_input_data || NULL == pc_output_data || NULL == pu64_output_data_size)
    {
        s32_ret_val = ERROR_NULL_POINTER;
    }
    else if (0 == u64_input_data_size)
    {
        s32_ret_val = ERROR_INVALID_LENGTH;
    }
    else
    {
        s32_ret_val = SUCCESS_STATUS;

        u64 u64_write_idx = 0;
        u64 u64_char_count = 1;
        char ac_char_count_str[20] = {0}; // Buffer to hold string representation of count
        
        for (u64 i = 0; i < u64_input_data_size; i++)
        {
            if (pc_input_data[i + 1] == pc_input_data[i])
            {
                u64_char_count++;
            }
            else
            {
                if (*pu64_output_data_size <= (u64_write_idx + 2 + sizeof(ac_char_count_str)))
                {
                    LOG("Reallocating memory for compression buffer.");
                    *pu64_output_data_size += DATA_CHUNK_SIZE_BYTES;
                    pc_output_data = (char *)realloc(pc_output_data, *pu64_output_data_size);

                    if (NULL == pc_output_data)
                    {
                        LOG_ERROR("Error reallocating memory for compression buffer: %s", strerror(errno));
                        s32_ret_val == ERROR_MEMORY_ALLOCATION_FAILED;
                        break;
                    }
                }

                if ('\n' == pc_input_data[i])
                {
                    pc_output_data[u64_write_idx++] = '\\';
                    pc_output_data[u64_write_idx++] = 'n';
                }
                else if (pc_input_data[i] >= '0' && pc_input_data[i] <= '9')
                {
                    pc_output_data[u64_write_idx++] = '\\';
                    pc_output_data[u64_write_idx++] = pc_input_data[i];
                }
                else
                {
                    pc_output_data[u64_write_idx++] = pc_input_data[i];
                }

                memset(ac_char_count_str, 0, sizeof(ac_char_count_str));
                snprintf(ac_char_count_str, sizeof(ac_char_count_str), "%lu", u64_char_count);

                strncpy(&pc_output_data[u64_write_idx], ac_char_count_str, strlen(ac_char_count_str));
                u64_write_idx += strlen(ac_char_count_str);
                pc_output_data[u64_write_idx] = '\0';

                u64_char_count = 1;
            }
        }
        *pu64_output_data_size = u64_write_idx;

        LOG("Reallocating compression buffer to the actual compressed size.");
        pc_output_data = (char *)realloc(pc_output_data, *pu64_output_data_size);
        if (NULL == pc_output_data)
        {
            LOG_ERROR("Error reallocating memory to the actual compressed size: %s", strerror(errno));
            s32_ret_val = ERROR_MEMORY_ALLOCATION_FAILED;
        }

        LOG("RLE Compression successful. Compressed size: %lu bytes", *pu64_output_data_size);
    }

    return s32_ret_val;
}

/**
 * @brief Compress the input file using RLE compression
 * 
 * @param[in] input_file_name Path to the input file to be compressed 
 * @return s32 SUCCESS_STATUS on success, error code otherwise 
 */
s32 compress(const char *input_file_name)
{
    s32 s32_ret_val = FAILURE_STATUS;

    if (NULL == input_file_name)
    {
        s32_ret_val = ERROR_NULL_POINTER;
    }
    else
    {
        LOG_INFO("Compressing file: %s", input_file_name);

        FILE *pf_in_file = NULL;
        FILE *pf_out_file = NULL;
        char *pc_out_file_path = NULL;

        char *pc_raw_data_buff = NULL;
        u64 u64_raw_data_size = 0;

        char *pc_compressed_buff = NULL;
        u64 u64_max_compressed_size = 0;
        u64 u64_compressed_size = 0;

        char ac_input_file_extention[5] = {0};

        do
        {
            s32_ret_val = get_file_extension(input_file_name, ac_input_file_extention);
            ERROR_BREAK(s32_ret_val);
            
            if (0 != strcmp("txt", ac_input_file_extention))
            {
                LOG_ERROR("Only .txt files are supported for compression.");
                s32_ret_val = ERROR_FILE_EXTENSION;
                break;
            }

            s32_ret_val = open_file(input_file_name, "r", &pf_in_file);
            ERROR_BREAK(s32_ret_val);

            s32_ret_val = read_file(pf_in_file, &pc_raw_data_buff, &u64_raw_data_size);
            ERROR_BREAK(s32_ret_val);

            s32_ret_val = close_file(&pf_in_file);
            ERROR_BREAK(s32_ret_val);


            if ((u64_raw_data_size / 2) < UINT32_MAX)
            {
                u64_max_compressed_size = 2 * u64_raw_data_size; // Worst case scenario
            }
            else
            {
                LOG_ERROR("Input file length is too large to compress.");
                s32_ret_val = ERROR_INVALID_LENGTH;
                break;
            }

            pc_compressed_buff = (char *)malloc(u64_max_compressed_size);

            if (NULL == pc_compressed_buff)
            {
                LOG_ERROR("Error allocating memory for compression buffer: %s", strerror(errno));
                s32_ret_val = ERROR_MEMORY_ALLOCATION_FAILED;
                break;
            }

            s32_ret_val = s32_rle_compress(pc_raw_data_buff, u64_raw_data_size, pc_compressed_buff, &u64_compressed_size);
            ERROR_BREAK(s32_ret_val);


            s32_ret_val = create_output_file(input_file_name, "rle", &pc_out_file_path);
            ERROR_BREAK(s32_ret_val);

            s32_ret_val = open_file(pc_out_file_path, "w", &pf_out_file);
            ERROR_BREAK(s32_ret_val);

            s32_ret_val = write_file(pf_out_file, pc_compressed_buff, u64_compressed_size);
            ERROR_BREAK(s32_ret_val);

            s32_ret_val = close_file(&pf_out_file);
            ERROR_BREAK(s32_ret_val);

            LOG_INFO("File compressed successfully to: %s", pc_out_file_path);

        } while (0);

        // Clean-up
        if (SUCCESS_STATUS != s32_ret_val)
        {
            LOG_ERROR("Exit compression loop with error code: %d", s32_ret_val);

            if (NULL != pf_in_file)
            {
                s32_ret_val = close_file(&pf_in_file);
                LOG_INFO("Close pf_in_file: %d", s32_ret_val);
            }

            if (NULL != pf_out_file)
            {
                s32_ret_val = close_file(&pf_out_file);
                LOG_INFO("Close pf_out_file: %d", s32_ret_val);
            }

            if (true == check_file_exists(pc_out_file_path))
            {
                s32_ret_val = delete_file(pc_out_file_path);
            }
        }

        // Free allocated memory
        free_allocated_memory(pc_raw_data_buff);
        free_allocated_memory(pc_out_file_path);
        free_allocated_memory(pc_compressed_buff);
    }

    return s32_ret_val;
}