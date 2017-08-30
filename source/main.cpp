/**
********************************************************************************
* @file         main.cpp
* @brief        Entry point to application
* @author       Maksym Koshel (maks.koshel@gmail.com)
* @date         Aug 26, 2017
********************************************************************************
*/

#include <memory>

#include "ts_processor.h"
#include "log.h"

int main(int argc, char* argv[])
{
    int result = 0;
    std::unique_ptr<TSProcessor> proc;

    log::set(log::LEVEL::debug);
    if (argc != 4)
    {

        log::error() << "Usage: " << argv[0]
            << "<input_ts> <output_video> <output_audio>";
        return 1;
    }

    try
    {
        proc = std::unique_ptr<TSProcessor>(new TSProcessor(argv[1]));
        proc->demux(argv[2], argv[3]);
        result = 0;
    }
    catch(std::runtime_error& r)
    {
        log::error() << r.what();
        result = 1;
    }

    log::info() << argv[0] << " finished with " << result << " result";

    return result;
}