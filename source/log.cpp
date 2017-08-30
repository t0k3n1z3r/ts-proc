/**
********************************************************************************
* @file         log.cpp
* @brief        Trivial log system implementation
* @author       Maksym Koshel (maks.koshel@gmail.com)
* @date         Aug 30, 2017
********************************************************************************
*/

#include "log.h"

#include <iostream>

/**
********************************************************************************
* @brief        Global log level which used to filter messages
********************************************************************************
*/
static log::LEVEL s_log_level{ log::LEVEL::warning };

/**
********************************************************************************
* @brief        Generic log message print function
* @param        [in, out] out   Stream will be used to output message
* @param        [in] level      Current level
* @param        [in] prefix     Prefix string
* @param        [in] str        String will be printed
* @return       void
********************************************************************************
*/  
static void print_log_message(std::ostream& out, log::LEVEL lvl,
    const std::string& prefix, const std::string& str)
{
    if(lvl <= s_log_level)
    {
        out << prefix << " " << str << std::endl;
    }
}

/*
********************************************************************************
*
********************************************************************************
*/
void log::set(log::LEVEL level)
{
    s_log_level = level;
}

/*
********************************************************************************
*
********************************************************************************
*/
log::debug::~debug()
{
    print_log_message(std::cout, log::LEVEL::debug, "[D]", sink.str());
}

/*
********************************************************************************
*
********************************************************************************
*/    
log::info::~info()
{
    print_log_message(std::cout,log::LEVEL::info, "[I]", sink.str());
}

/*
********************************************************************************
*
********************************************************************************
*/  
log::warning::~warning()
{
    print_log_message(std::cerr,log::LEVEL::warning, "[W]", sink.str());
}

/*
********************************************************************************
*
********************************************************************************
*/
log::error::~error()
{
    print_log_message(std::cerr,log::LEVEL::error, "[E]", sink.str());
}
