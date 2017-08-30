/**
********************************************************************************
* @file         ts_processor.h
* @brief        MPEG-TS file format processor class declaration
* @author       Maksym Koshel (maks.koshel@gmail.com)
* @date         Aug 26, 2017
********************************************************************************
*/

#ifndef _TS_PROCESSOR_H_
#define _TS_PROCESSOR_H_

#include <fstream>

// Forward declaration of classes
class VideoESWriter;
class AudioESWriter;

/**
********************************************************************************
* @class        TSProcessor
* @brief        Declaration of MPEG-TS file procesisng
* @warning      In order to follow RAII:
*               (http://en.cppreference.com/w/cpp/language/raii), constructor
*               throws exception in case input file can't be opened or PAT and
*               PMT can't be found:
*               (https://en.wikipedia.org/wiki/Program-specific_information)
********************************************************************************
*/
class TSProcessor
{
public:
    /**
    ****************************************************************************
    * @brief    The only allowed constructor for this class. All other
    *           contructors are blocked.
    * @param    [in] input  MPEG-TS file name
    * @warning  throws runtime_errror exception in case if object
    ****************************************************************************
    */
    TSProcessor(const char* const input);
    ~TSProcessor();

    /**
    ****************************************************************************
    * @brief    Does demultiplexing of the SPTS by putting video packets to
    *           video file and audio packets to audio file
    * @param    [in] video  Filename where video ES shall be stored
    * @param    [in] audio  Filename where audio ES shall be stored
    * @warning  Throws exception in case of error
    * @return   void
    ****************************************************************************
    */
    void demux(const char* const video, const char* const audio);

private:
    /**
    ****************************************************************************
    * @brie     Process MPEG-TS file after video PID and audio PID is known
    * @warning  All packets with PID different than m_video_pid and m_audio_pid
    *           will be ignored by this function
    * @return   true on success, false - otherwise
    ****************************************************************************
    */
    bool process_file(VideoESWriter& v, AudioESWriter& a);

    /**
    ****************************************************************************
    * @brief    Read single TS packet from input file at current position
    * @param    [out] packet    Packet will be returned by the function
    * @return   true on success, false - otherwise
    ****************************************************************************
    */
    bool read_packet(struct ts_packet& packet);

    /**
    ****************************************************************************
    * @brief    Does search for PAT (Program Association Table) in the stream.
    *           Basically it starts reading packet by packet from last position
    *           in the stream and searches for packet with PID 0 and skips all
    *           other packets. This function MUST initialize m_pmt_pid with 
    *           PID found in PAT
    * @return   true on sucees (m_pmt_pid set to value found in PAT),
    *           false - otherwise
    ****************************************************************************
    */
    bool process_pat(void);

    /**
    ****************************************************************************
    * @brief    Does search for PMT (Program Map Table) in the stream. Basically
    *           it starts reading packet by packet (like process pat) from last
    *           position in the stream and searches for packet with PID
    *           m_pmt_pid and skips all other packets.
    * @warning  This function must be called only in case if process_pat was
    *           finished successfully and PMT PID was found
    * @return   true on sucees (m_video_pid and m_audio_pid set to value
    *           found in PAT), false - otherwise
    ****************************************************************************
    */
    bool process_pmt(void);

    /**
    ****************************************************************************
    * @brief    Modifies one of 2 class members (m_video_pid or m_audio_pid)
    *           depends on ES type passed
    * @param    [in] pid    Packet ID
    * @param    [in] type   Elementary stream type
    * @note     It modifies m_video_pid and m_audio_pid class members
    * @return   void
    ****************************************************************************
    */
    void save_pid(uint16_t pid, int type);

private:
    TSProcessor()                               = delete;
    TSProcessor(const TSProcessor& r)           = delete;
    TSProcessor& operator= (const TSProcessor&) = delete;

private:
    std::ifstream   m_input_file;       ///< MPEG-TS file stream
    size_t          m_input_filesize;   ///< MPEG-TS file size

    uint16_t        m_pmt_pid;          ///< PID TS packet which contains PMT
    uint16_t        m_video_pid;        ///< Video PID
    uint16_t        m_audio_pid;        ///< Audio PID
};

#endif  /* !_TS_PROCESSOR_H_ */
