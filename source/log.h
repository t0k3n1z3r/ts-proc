/**
********************************************************************************
* @file         log.h
* @brief        Trivial log system interface definition
* @author       Maksym Koshel (maks.koshel@gmail.com)
* @date         Aug 30, 2017
********************************************************************************
*/

#ifndef _LOG_H_
#define _LOG_H_

#include <sstream>

namespace log
{
    /**
    ****************************************************************************
    * @enum     LEVEL
    * @brief    Logging level definition  
    ****************************************************************************
    */
    enum class LEVEL:uint16_t
    {
        error   = 0,
        warning = 1,
        info    = 2,
        debug   = 3
    };
    
    /**
    ****************************************************************************
    * @brief    Set global log level which will be used to filter messages
    * @note     By defaul warning level will be set. No need to call this
    *           function to enable logs
    * @return   void
    ****************************************************************************
    */
    void set(LEVEL level);
    
    // debug classes (output in destructor)

    class debug
    {
    public:
        mutable std::ostringstream sink;
        ~debug();
    };
    
    class info
    {
    public:
        mutable std::ostringstream sink;
        ~info();
    };
    
    class warning
    {
    public:
        mutable std::ostringstream sink;
        ~warning();
    };

    class error
    {
    public:
        mutable std::ostringstream sink;
        ~error();
    };

    // Implemented operators

    template<class V>
    const debug &operator<<(const debug &o, const V& v)
    {
        o.sink << v;
        return o;
    }

    template<class V>
    const info &operator<<(const info &o, const V& v)
    {
        o.sink << v;
        return o;
    }

    template<class V>
    const warning &operator<<(const warning &o, const V& v)
    {
        o.sink << v;
        return o;
    }

    template<class V>
    const error &operator<<(const error &o, const V& v)
    {
        o.sink << v;
        return o;
    }
}

#endif /* !_LOG_H_ */
