#include "../header_files/constants.h"
#include "../header_files/utils.h"
#include "../header_files/compress.h"
#include "../header_files/decompress.h"


int main(int argc, char const *argv[])
{
    tstr_input_args str_args = {OP_NONE, NULL};
    parse_input_args(argc, argv, &str_args);

    s32 s32_ret_val = FAILURE_STATUS;

    switch (str_args.enu_operation)
    {
    case OP_HELP:
    {
        print_prog_usage(argv[0]);
        s32_ret_val = SUCCESS_STATUS;
        break;
    }
    case OP_COMPRESS:
    {
        s32_ret_val = compress(str_args.pc_input_file);
        break;
    }
    case OP_DECOMPRESS:
    {
        s32_ret_val = decompress(str_args.pc_input_file);
        break;
    }
    default:
    {
        LOG_ERROR("Invalid operation\n");
        break;
    }
    }

    if (SUCCESS_STATUS != s32_ret_val)
    {
        LOG_ERROR("Operation failed with error code: %d", s32_ret_val);
        return 1;
    }

    // char ac_file_basename[256] = {0};
    // s32 s32_ret = get_file_basename(argv[2],ac_file_basename);
    // LOG("Return value: %d", s32_ret);
    // printf("File basename: %s\n", ac_file_basename);

    return 0;
}