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

#include <string>
#include <stdio.h>
#include <stdint.h>

/**
********************************************************************************
* @enum         STATUS
* @brief        Error codes might be returned
********************************************************************************
*/
typedef enum
{
    STATUS_OK   = 0,
    STATUS_FAIL = 1
} STATUS;

/**
********************************************************************************
* @def          SYNC_BYTE_MASK
* @brief        TS packet synchronization byte mask (for big-endian structure)
********************************************************************************
*/
#define SYNC_BYTE_MASK  0xff000000

/**
********************************************************************************
* @def          IS_PACKET_VALID
* @brief        Macro which validates TS Packet header (header value must be
*               in big-endian structure)
********************************************************************************
*/
#define IS_PACKET_VALID(header) (((header & SYNC_BYTE_MASK) >> 24) == 0x47)

/**
********************************************************************************
* @def          PUSI_MASK
* @brief        PUSI mask (for bit-endian structure)
********************************************************************************
*/
#define PUSI_MASK       0x00400000

/**
********************************************************************************
* @def          PID_MASK
* @brief        TS packet ID mask (for big-endian structure)
********************************************************************************
*/
#define PID_MASK        0x001fff00

/**
********************************************************************************
* @def          AFC_MASK
* @brief        Adaptation field control mask (for big-endian structure)
********************************************************************************
*/
#define AFC_MASK        0x00000030

/**
********************************************************************************
* @def          TS_PACKET_HEADER
* @brief        Size of TS packet header
********************************************************************************
*/
#define TS_PACKET_HEADER    4

/**
********************************************************************************
* @def          TS_PACKET_PAYLOAD
* @brief        Size of TS packet payload (including adaptation field)
********************************************************************************
*/
#define TS_PACKET_PAYLOAD   184

/**
********************************************************************************
* @def          TS_PACKET_SIZE
* @brief        Size of TS packet (header + payload)
********************************************************************************
*/
#define TS_PACKET_SIZE      (TS_PACKET_HEADER + TS_PACKET_PAYLOAD)

/**
********************************************************************************
* @struct       ts_packet
* @brief        Definition of MPEG-TS packet structure
********************************************************************************
*/
struct ts_packet
{
    /**
    ****************************************************************************
    * @brief    TS packet header
    * @note     Structure of a header (Big Endian):
    ****************************************************************************
    */
    uint32_t header;

    /**
    ****************************************************************************
    * @brief    Payload of TS packet, including Adaptation field
    ****************************************************************************
    */
    uint8_t  payload[184];

};

/**
********************************************************************************
* @class        TSProcessor
* @brief        Declaration of MPEG-TS file procesisng
* @note         This class has initialization method in order to
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
    * @param    [in] video  Video elementary stream file name
    * @param    [in] audio  Audio elementary stream file name
    ****************************************************************************
    */
    TSProcessor(const char* const input, const char* const video,
        const char* const audio);
    
    ~TSProcessor();

    /**
    ****************************************************************************
    * @brief    Does initialization of the object by opening all files given
    *           in constructor
    * @return   STATUS_OK on success, STATUS_FAIL - otherwise
    ****************************************************************************
    */
    STATUS init(void);

    /**
    ****************************************************************************
    * @brief    Performs demultiplex of MPEG-TS file. This function has 3 main
    *           stages: 1 - find PAT in the stream, 2 - find PMT in the stream,
    *           3 - Process file ignoring all PIDs different than video
    *           and audio found in PMT. All packets except video and audio will
    *           be ignored.
    * @return   STATUS_OK on success, STATUS_FAIL - otherwise
    ****************************************************************************
    */
    STATUS demux(void);

private:
    /**
    ****************************************************************************
    * @brie     Process MPEG-TS file after video PID and audio PID is known
    * @warning  All packets with PID different than m_video_pid and m_audio_pid
    *           will be ignored by this function
    * @return   STATUS_OK on success, STATUS_FAIL - otherwise
    ****************************************************************************
    */
    STATUS process_file(void);

    /**
    ****************************************************************************
    * @brief    Read single TS packet from input file at current position
    * @param    [out] packet    Packet will be returned by the function
    * @return   STATUS_OK on success, STATUS_FAIL - otherwise
    ****************************************************************************
    */
    STATUS read_packet(struct ts_packet& packet);

    /**
    ****************************************************************************
    * @brief    Does search for PAT (Program Association Table) in the stream.
    *           Basically it starts reading packet by packet from last position
    *           in the stream and searches for packet with PID 0 and skips all
    *           other packets. This function MUST initialize m_pmt_pid with 
    *           PID found in PAT
    * @return   STATUS_OK on sucees (m_pmt_pid set to value found in PAT),
    *           STATUS_FAIL - otherwise
    ****************************************************************************
    */
    STATUS process_pat(void);

    /**
    ****************************************************************************
    * @brief    Does search for PMT (Program Map Table) in the stream. Basically
    *           it starts reading packet by packet (like process pat) from last
    *           position in the stream and searches for packet with PID
    *           m_pmt_pid and skips all other packets.
    * @warning  This function must be called only in case if process_pat was
    *           finished successfully and PMT PID was found
    * @return   STATUS_OK on sucees (m_video_pid and m_audio_pid set to value
    *           found in PAT), STATUS_FAIL - otherwise
    ****************************************************************************
    */
    STATUS process_pmt(void);

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

private:    // Blocked implementations
    TSProcessor();
    TSProcessor(const TSProcessor& r);
    TSProcessor& operator= (const TSProcessor&);

private:
    std::string     m_input_filename;   ///< Input MPEG-TS file name
    std::string     m_video_filename;   ///< Output video ES file name
    std::string     m_audio_filename;   ///< Output audio ES file name

    FILE*           m_input_file;       ///< MPEG-TS file descriptor
    FILE*           m_video_file;       ///< Video ES file descriptor
    FILE*           m_audio_file;       ///< Audio ES file descriptor

    size_t          m_input_filesize;   ///< MPEG-TS file size

    uint16_t        m_pmt_pid;          ///< PID TS packet which contains PMT

    uint16_t        m_video_pid;        ///< Video PID
    uint16_t        m_audio_pid;        ///< Audio PID
};

#endif  /* !_TS_PROCESSOR_H_ */
