/* Include any necessary libraries and header files */
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "../header_files/utils.h"
#include "../header_files/decompress.h"


/**
 * @brief Decompress data using Run-Length Encoding (RLE)
 * 
 * @param[in] pc_input_data Input data to be decompressed
 * @param[in] u64_input_data_size Size of the input data
 * @param[in] pc_output_data Buffer to hold the decompressed output data
 * @param[in out] pu64_output_data_size Pointer to hold the size of the decompressed data
 * @return s32 SUCCESS_STATUS on success, error code otherwise 
 */
static s32 s32_rle_decompress(const char *pc_input_data, const u64 u64_input_data_size, char *pc_output_data, u64 *pu64_output_data_size)
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

        char non_digit_char;
        char ac_char_cnt_string[20] = {0};
        u8 u8_char_cnt_str_idx = 0;
        u64 u64_char_cnt = 0;
        u64 u64_write_idx = 0;

        for (u64 i = 0; i < u64_input_data_size; i++)
        {
            if (*pu64_output_data_size == u64_write_idx)
            {
                LOG("Reallocating memory for decompression buffer.");
                *pu64_output_data_size += DATA_CHUNK_SIZE_BYTES;
                pc_output_data = (char *)realloc(pc_output_data, *pu64_output_data_size);

                if (NULL == pc_output_data)
                {
                    LOG_ERROR("Error reallocating memory for decompression buffer: %s", strerror(errno));
                    s32_ret_val == ERROR_MEMORY_ALLOCATION_FAILED;
                    break;
                }
            }

            if ('\\' == pc_input_data[i] && (i + 1) < u64_input_data_size)
            {
                if ('n' == pc_input_data[i + 1])
                {
                    non_digit_char = '\n';
                }
                else if ('t' == pc_input_data[i + 1])
                {
                    non_digit_char = '\t';
                }
                else if (pc_input_data[i + 1] >= '0' && pc_input_data[i + 1] <= '9')
                {
                    non_digit_char = pc_input_data[i + 1];
                }
                else
                {
                    non_digit_char = '\\';
                }
                i++; // Skip the next character as it's part of the escape sequence
            }
            else
            {
                non_digit_char = pc_input_data[i];
            }

            pc_output_data[u64_write_idx++] = non_digit_char;

            i++;
            while ((pc_input_data[i] >= '0') && (pc_input_data[i] <= '9'))
            {
                ac_char_cnt_string[u8_char_cnt_str_idx++] = pc_input_data[i++];
            }
            i--;

            u64_char_cnt = atoi(ac_char_cnt_string);
            u8_char_cnt_str_idx = 0;
            memset(ac_char_cnt_string, 0, sizeof(ac_char_cnt_string));

            for (u64 j = 1; j < u64_char_cnt; j++)
            {
                if (*pu64_output_data_size == u64_write_idx)
                {
                    LOG("Reallocating memory for decompression buffer.");
                    *pu64_output_data_size += DATA_CHUNK_SIZE_BYTES;
                    pc_output_data = (char *)realloc(pc_output_data, *pu64_output_data_size);

                    if (NULL == pc_output_data)
                    {
                        LOG_ERROR("Error reallocating memory for decompression buffer: %s", strerror(errno));
                        s32_ret_val == ERROR_MEMORY_ALLOCATION_FAILED;
                        break;
                    }
                }

                pc_output_data[u64_write_idx++] = non_digit_char;

            }
            ERROR_BREAK(s32_ret_val);
        }
        *pu64_output_data_size = u64_write_idx;

        LOG("Reallocating decompression buffer to the actual decompressed size.");
        pc_output_data = (char *)realloc(pc_output_data, *pu64_output_data_size);
        if (NULL == pc_output_data)
        {
            LOG_ERROR("Error reallocating memory to the actual decompressed size: %s", strerror(errno));
            s32_ret_val = ERROR_MEMORY_ALLOCATION_FAILED;
        }

        LOG("RLE Decompression successful. Decompressed size: %lu bytes", *pu64_output_data_size);
    }

    return s32_ret_val;
}

/**
 * @brief Decompress the input file using RLE compression
 * 
 * @param[in] input_file_name Path to the input file to be decompressed
 * @return s32 SUCCESS_STATUS on success, error code otherwise 
 */
s32 decompress(const char *input_file_name)
{
    s32 s32_ret_val = FAILURE_STATUS;

    if (NULL == input_file_name)
    {
        s32_ret_val = ERROR_NULL_POINTER;
    }
    else
    {
        LOG_INFO("Decompressing file: %s", input_file_name);

        FILE *pf_in_file = NULL;
        FILE *pf_out_file = NULL;
        char *pc_out_file_path = NULL;
        char *pc_raw_data_buff = NULL;
        u64 u64_raw_data_size = 0;

        char *pc_decompressed_buff = NULL;
        u64 u64_decompressed_size = 0;

        char ac_input_file_extention[5] = {0};

        do
        {
            s32_ret_val = get_file_extension(input_file_name, ac_input_file_extention);
            ERROR_BREAK(s32_ret_val);

            if (0 != strcmp("rle", ac_input_file_extention))
            {
                LOG_ERROR("Invalid file extension for decompression. Expected .rle");
                s32_ret_val = ERROR_FILE_EXTENSION;
                break;
            }

            s32_ret_val = open_file(input_file_name, "r", &pf_in_file);
            ERROR_BREAK(s32_ret_val);

            s32_ret_val = read_file(pf_in_file, &pc_raw_data_buff, &u64_raw_data_size);
            ERROR_BREAK(s32_ret_val);

            s32_ret_val = close_file(&pf_in_file);
            ERROR_BREAK(s32_ret_val);

            u64_decompressed_size = u64_raw_data_size / 2;  // best scenario of size
            pc_decompressed_buff = (char *)malloc(u64_decompressed_size);

            if (NULL == pc_decompressed_buff)
            {
                LOG_ERROR("Error allocating memory for decompression buffer: %s", strerror(errno));
                s32_ret_val = ERROR_MEMORY_ALLOCATION_FAILED;
                break;
            }

            s32_ret_val = s32_rle_decompress(pc_raw_data_buff, u64_raw_data_size, pc_decompressed_buff, &u64_decompressed_size);
            ERROR_BREAK(s32_ret_val);

            s32_ret_val = create_output_file(input_file_name, "txt", &pc_out_file_path);
            ERROR_BREAK(s32_ret_val);

            s32_ret_val = open_file(pc_out_file_path, "w", &pf_out_file);
            ERROR_BREAK(s32_ret_val);

            s32_ret_val = write_file(pf_out_file, pc_decompressed_buff, u64_decompressed_size);
            ERROR_BREAK(s32_ret_val);

            s32_ret_val = close_file(&pf_out_file);
            ERROR_BREAK(s32_ret_val);

            LOG_INFO("File decompressed successfully to: %s", pc_out_file_path);

        } while (0);

        // Clean-up
        if (SUCCESS_STATUS != s32_ret_val)
        {
            LOG_ERROR("Exit decompression loop with error code: %d", s32_ret_val);

            if (NULL != pf_in_file)
            {
                s32_ret_val = close_file(&pf_in_file);
            }

            if (NULL != pf_out_file)
            {
                s32_ret_val = close_file(&pf_out_file);
            }

            if (true == check_file_exists(pc_out_file_path))
            {
                s32_ret_val = delete_file(pc_out_file_path);
            }
        }

        // Free allocated memory
        free_allocated_memory(pc_raw_data_buff);
        free_allocated_memory(pc_out_file_path);
        free_allocated_memory(pc_decompressed_buff);
    }

    return s32_ret_val;
}