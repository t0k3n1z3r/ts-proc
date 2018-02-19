/**
********************************************************************************
* @file         main.cpp
* @brief        Entry point to application
* @author       Maksym Koshel (maks.koshel@gmail.com)
* @date         Aug 26, 2017
********************************************************************************
*/

#include <stdio.h>
#include <stdint.h>
#include <endian.h>
#include <limits.h>
#include <memory.h>
#include <argp.h>

#include "ts_processor.h"

/**
********************************************************************************
* @struct       CmdParams
* @brief        Command line arguments definition
********************************************************************************
*/
struct CmdParams
{
    char i_file[PATH_MAX];  ///< Input TS file
    char v_file[PATH_MAX];  ///< Output Video file
    char a_file[PATH_MAX];  ///< Output Audio file

    CmdParams()
    {
        memset(i_file, 0, PATH_MAX * sizeof(char));
        memset(v_file, 0, PATH_MAX * sizeof(char));
        memset(a_file, 0, PATH_MAX * sizeof(char));
    }
};

const char* argp_program_version = VERSION; ///< ARGP version place holder

/**
********************************************************************************
* @brief        Mandatory application arguments
********************************************************************************
*/
static char s_args_str[] = "<input_ts> <output_video> <output_audio>";

/**
********************************************************************************
* @brief        ARGP documentation place holder
********************************************************************************
*/
static char s_info_str[] = "Primitive MPEG-TS demuxer\v"
                        "In order to run tool execute:\n\t"
                        "ts-proc in.ts video.file audio.file";

/**
********************************************************************************
* @brief        E-mail to write bugs placeholder
********************************************************************************
*/
const char* argp_program_bug_address = "Maksym Koshel (maks.koshel@gmail.com)";

/**
********************************************************************************
* @brief        Optional arguments
********************************************************************************
*/
static struct argp_option s_cmd_options[] =
{
    { 0, 0, 0, 0, 0, 0 }
};

/**
********************************************************************************
* @brief        Parse single argument at a time
* @param        [in] key    Argument key
* @param        [in] arg    Argument value
* @param        [in,out]    Parsing state
* @return       0 on success, error code - otherwise
********************************************************************************
*/
static error_t cmd_parse_handler(int key, char* const arg,
                                  struct argp_state* const state)
{
    error_t result = 0;
    static int c = 0;
    CmdParams* cmd = static_cast<CmdParams*>(state->input);

    switch(key)
    {
        case ARGP_KEY_END:
        {
            if (c < 3)
            {
                argp_usage(state); ///< @note This function calls exit inside
            }
            break;
        }

        default:
            if (0 == key && c == 0)
            {
                strncpy(cmd->i_file, arg, PATH_MAX - 1);
                c = 1;
                break;
            }

            if (0 == key && c == 1)
            {
                strncpy(cmd->v_file, arg, PATH_MAX - 1);
                c = 2;
                break;
            }

            if (0 == key && c == 2)
            {
                strncpy(cmd->a_file, arg, PATH_MAX - 1);
                c = 3;
                break;
            }
    }

    return result;
}

/**
********************************************************************************
* @brief        ARGP control structure
********************************************************************************
*/
static struct argp argp_controls =
{
    .options        = s_cmd_options,
    .parser         = cmd_parse_handler,
    .args_doc       = s_args_str,
    .doc            = s_info_str,
    .children       = NULL,
    .help_filter    = NULL,
    .argp_domain    = NULL
};

int main(int argc, char* argv[])
{
    CmdParams cmd;
    int result = 1;

    do
    {
        result = argp_parse(&argp_controls, argc, argv, 0, 0, &cmd);
        if (0 != result)
        {
            fprintf(stderr, "Can't parse command line arguments. Error: %d\n",
                result);
            break;
        }

        TSProcessor proc(cmd.i_file, cmd.v_file, cmd.a_file);
        result = proc.init();
        if (STATUS_OK != result)
        {
            break;
        }

        result = proc.demux();
        if (STATUS_OK != result)
        {
            break;
        }

    } while(0);
    
    fprintf(stdout, "Processing of MPEG-TS file (%s) done with result: %s\n",
        argv[1], (STATUS_OK == result) ? "success" : "fail");

    return result;
}