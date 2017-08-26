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

#include "ts_processor.h"

int main(int argc, char* argv[])
{
    int result = 1;

    do
    {
        if (argc != 4)
        {
            fprintf(stderr,
                "Usage: %s <input_ts> <output_video> <output_audio>\n",
                argv[0]);
            break;
        }

        TSProcessor proc(argv[1], argv[2], argv[3]);

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