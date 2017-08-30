/**
********************************************************************************
* @file         ts_processor.cpp
* @brief        MPEG-TS file format processor class implementation
* @author       Maksym Koshel (maks.koshel@gmail.com)
* @date         Aug 26, 2017
********************************************************************************
*/

#include "ts_processor.h"

#include <iostream>
#include <exception>
#include <stdexcept>
#include <iterator>

// includes htobe function (could be replaced with own functions)
#include <endian.h>

#include "log.h"
#include "es_writer.h"

/**
********************************************************************************
* @brief        TS packet synchronization byte mask (for big-endian structure)
********************************************************************************
*/
static const uint32_t SYNC_BYTE_MASK = 0xff000000;

/**
********************************************************************************
* @brief        PUSI mask (for bit-endian structure)
********************************************************************************
*/
static const uint32_t PUSI_MASK      = 0x00400000;

/**
********************************************************************************
* @brief        TS packet ID mask (for big-endian structure)
********************************************************************************
*/
static const uint32_t PID_MASK       = 0x001fff00;

/**
********************************************************************************
* @brief        Adaptation field control mask (for big-endian structure)
********************************************************************************
*/
static const uint32_t AFC_MASK       = 0x00000030;

/**
********************************************************************************
* @brief        Macro which validates TS Packet header (header value must be
*               in big-endian structure)
* @return       true in case TS packet is valid, false otherwise
********************************************************************************
*/
static inline bool IS_PACKET_VALID(uint32_t header)
{
    return ((header & SYNC_BYTE_MASK) >> 24 == 0x47);
}

/**
********************************************************************************
* @brief        Size of TS packet header
********************************************************************************
*/
static const uint32_t TS_PACKET_HEADER = 4;

/**
********************************************************************************
* @brief        Size of TS packet payload (including adaptation field)
********************************************************************************
*/
static const uint32_t TS_PACKET_PAYLOAD = 184;

/**
********************************************************************************
* @brief        Size of TS packet (header + payload)
********************************************************************************
*/
static const uint32_t TS_PACKET_SIZE = TS_PACKET_HEADER + TS_PACKET_PAYLOAD;

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
    * @note     Structure of a header (Big Endian)
    ****************************************************************************
    */
    uint32_t header;

    /**
    ****************************************************************************
    * @brief    Payload of TS packet, including Adaptation field
    ****************************************************************************
    */
    uint8_t  payload[TS_PACKET_PAYLOAD];
};

/*
********************************************************************************
*
********************************************************************************
*/
TSProcessor::TSProcessor(const char* const input)
    : m_input_file(input, std::ifstream::binary)
    , m_input_filesize(0)
    , m_pmt_pid(0x1fff)
    , m_video_pid(0x1fff)
    , m_audio_pid(0x1fff)
{
    if (!m_input_file) throw std::runtime_error("Can't open input file");

    m_input_file.seekg(0, m_input_file.end);
    m_input_filesize = m_input_file.tellg();
    m_input_file.seekg(0, m_input_file.beg);

    /**
    ************************************************************************
    * @note     At this moment all packets will be ignored except PID 0.
    *           After this function is finished execution PMT PID is known
    ************************************************************************
    */
    if (!process_pat()) throw std::runtime_error("Can't find PAT");

    /**
    ************************************************************************
    * @note     At this moment all packets will be ignored expept PID
    *           equals to PMT (found in PAT). This function has to
    *           find Video PID and audio PID and initialize corresponding
    *           class members
    ************************************************************************
    */
    if (!process_pmt()) throw std::runtime_error("Can't find PMT");
}

/*
********************************************************************************
*
********************************************************************************
*/
TSProcessor::~TSProcessor()
{
    m_input_file.close();
}

/*
********************************************************************************
*
********************************************************************************
*/
bool TSProcessor::process_pat()
{
    bool result = false;

    uint16_t stream_id  = 0;
    uint16_t prog_id    = 0;

    do
    {
        struct ts_packet packet;
        result = read_packet(packet);
        if (!result)
        {
            break;
        }

        int pid  = (packet.header & PID_MASK) >> 8;
        int pusi = (packet.header & PUSI_MASK);

        // Skip packet if it's not 0 (which has PAT)
        if (0 != pid)
        {
            continue;
        }

        size_t pi = (pusi) ? 1 : 0;

        // Skip table ID
        pi += 1;

        /**
        ************************************************************************
        * @note     Section size is actually 12 bites you can recognize it as a
        *           mask, but because in this implementation syntax_indicator
        *           and reserved bits will be ignored I decided to read 2 bytes
        *           and mask it to get proper value
        * @note     Next 16 bits is TS stream ID
        ************************************************************************
        */
        uint16_t sec_len = htobe16(*(uint16_t*)&packet.payload[pi]) & 0xfff;
        pi += 2;

        stream_id = htobe16(*(uint16_t*)&packet.payload[pi]);
        pi += 2;

        /**
        ************************************************************************
        * @note     In bits: 2 - reserved, 5 - version, 1 - next indicator,
        *           8 - section number, 8 - last section number 
        * @note     This bits currently ignored, so position was advanced
        ************************************************************************
        */
        pi += 3;

        /**
        ************************************************************************
        * @note     Calculating number of programs we need to read:
        *           first 4 is Packet CRC bits
        *           5 is table ID, sec_len (including unused bits) and stream_id
        *           last 4 is Program size
        *           prog_num = ((sec_len) - pat_header - CRC) / prog_size
        ************************************************************************
        */
        int prog_num = (sec_len - 5 - 4) / 4;

        /**
        ************************************************************************
        * @warning  The STRONG assumption of this implementation is that file
        *           on input is SPTS (Single Program Transport Stream)
        ************************************************************************
        */
        if (prog_num != 1)
        {
            result = false;
            log::error() << "Number of programs different than 1: " << prog_num;
            log::error() << "MPTS is not supported by this implementaiton";
            break;
        }

        prog_id = htobe16(*(uint16_t*)&packet.payload[pi]);
        pi += 2;

        m_pmt_pid = htobe16(*(uint16_t*)&packet.payload[pi]) & 0x1fff;
        pi += 2;

        log::info() << "MPEG-TS Stream ID: " << stream_id;
        log::info() << "Program ID:" << prog_id;
        log::info() << "PMT PID: " << m_pmt_pid;
        result = true;
        break;

    } while((size_t)m_input_file.tellg() < m_input_filesize);

    return result;
}

/*
********************************************************************************
*
********************************************************************************
*/
bool TSProcessor::process_pmt()
{
    bool result = false;

    do
    {
        struct ts_packet packet;
        result = read_packet(packet);
        if (!result)
        {
            break;
        }

        int pid  = (packet.header & PID_MASK) >> 8;
        int pusi = (packet.header & PUSI_MASK);

        // Skip packet if it's not PMT
        if (pid != m_pmt_pid)
        {
            continue;
        }

        size_t pi = (pusi) ? 1 : 0;

        pi += 1; // Skipped table_id

        /**
        ************************************************************************
        * @note     Section size is actually 12 bites you can recognize it as a
        *           mask, but because in this implementation syntax_indicator
        *           and reserved bits will be ignored I decided to read 2 bytes
        *           and mask it to get proper value
        * @note     Next 16 bits is TS stream ID
        ************************************************************************
        */
        uint16_t sec_len = htobe16(*(uint16_t*)&packet.payload[pi]) & 0xfff;
        pi += 2;

        int prog_num = htobe16(*(uint16_t*)&packet.payload[pi]);
        pi += 2;

        /**
        ************************************************************************
        * @note     Skip 3 bytes which corresponds to fields in bits:
        *           2 - reserved, 5 version_number, 1 - next_inducator,
        *           8 - section_number, 8 - last section number.
        ************************************************************************
        */
        pi += 3;

        uint16_t pcr_pid = htobe16(*(uint16_t*)&packet.payload[pi]) & 0x1fff;
        pi += 2;

        uint16_t pinfo_size = htobe16(*(uint16_t*)&packet.payload[pi]) & 0xfff;
        pi += 2;

        // All bytes read + crc
        int left = sec_len - 9 - pinfo_size - 4;

        while(left > 0)
        {
            /**
            ********************************************************************
            * @note  Stream type (Audir, Video, etc.)
            ********************************************************************
            */
            int st = packet.payload[pi];
            pi += 1;
            left -= 1;
            
            /**
            ********************************************************************
            * @note  Elementary stream PID of type st
            ********************************************************************
            */
            uint16_t el = htobe16(*(uint16_t*)&packet.payload[pi]) & 0x1fff;
            pi += 2;
            left -= 2;

            uint16_t es_ilen = htobe16(*(uint16_t*)&packet.payload[pi]) & 0xfff;
            pi += 2;
            left -= 2;

            pi += es_ilen;
            left -= es_ilen;
            save_pid(el, st);
        }

        result = true;

        log::info() << "Program number: " << prog_num;
        log::info() << "Video PID: " << m_video_pid;
        log::info() << "Audio PID: " << m_audio_pid;
        log::info() << "PCR PID: " << pcr_pid;
        break;

    } while((size_t)m_input_file.tellg() < m_input_filesize);

    return result;
}

/*
********************************************************************************
*
********************************************************************************
*/
void TSProcessor::demux(const char* const video, const char* const audio)
{
    /**
    ****************************************************************************
    * @note     Video and Audio stream might be replaced with whatsever
    *           implementation of payload handler
    ****************************************************************************
    */
    VideoESWriter v(video);
    AudioESWriter a(audio);


    if (!video) throw std::runtime_error("Can't open output file for video ES");
    if (!audio) throw std::runtime_error("Can't open output file for audio ES");

   /**
    ************************************************************************
    * @note     At this moment all packets will be ignored except video and
    *           audio PID
    ************************************************************************
    */
    if (!process_file(v, a))
        throw std::runtime_error("Can't process file");
}

/*
********************************************************************************
*
********************************************************************************
*/
bool TSProcessor::read_packet(struct ts_packet& packet)
{
    bool result = false;

    do
    {
        m_input_file.read((char*)&packet.header, TS_PACKET_HEADER);
        if (!m_input_file)
        {
            log::error() << "Can't read TS packet header input file";
            break;
        }

        packet.header = htobe32(packet.header);
        if (!IS_PACKET_VALID(packet.header))
        {
            log::error() << "Sync byte of TS packet has wrong value";
            break;
        }

        m_input_file.read((char*)packet.payload, TS_PACKET_PAYLOAD);
        if (!m_input_file)
        {
            log::error() << "Can't read TS packet payload input file";
            break;
        }

        result = true;
    } while(0);

    return result;
}

/*
********************************************************************************
*
********************************************************************************
*/
bool TSProcessor::process_file(VideoESWriter& v, AudioESWriter& a)
{
    bool result = true;

    do
    {
        struct ts_packet packet;
        result = read_packet(packet);
        if (!result)
        {
            break;
        }

        int pid  = (packet.header & PID_MASK) >> 8;
        int pusi = (packet.header & PUSI_MASK);
        int afc  = (packet.header & AFC_MASK) >> 4;
        if (pid != m_video_pid && pid != m_audio_pid)
        {
            continue;
        }

        size_t pi = 0;
        if (2 == afc || 3 == afc) // 10 or 11 in bin
        {
            pi = packet.payload[pi];
            pi += 1;
        }

        if (pusi)
        {
            /**
            ********************************************************************
            * @warning  Start code compliance is not checked since there is an
            *           assumption that valid input stream goes on input
            * @note     PES packet lenght and start code is skipped start code
            *           is 4 bytes including stream ID and Size of PES is
            *           bytes.
            ********************************************************************
            */
            pi += 4 + 2;

            uint32_t opes = (htobe32(*(uint32_t*)&packet.payload[pi])
                & 0x0000ff00) >> 8;
            pi += 3 + opes;
        }

        if (pid == m_video_pid)
        {
            v.write(&packet.payload[pi], TS_PACKET_PAYLOAD - pi);
        }
        else
        {
            a.write(&packet.payload[pi], TS_PACKET_PAYLOAD - pi);
        }

    } while((size_t)m_input_file.tellg() < m_input_filesize);

    return result;
}

/*
********************************************************************************
*
********************************************************************************
*/
void TSProcessor::save_pid(uint16_t pid, int type)
{
    // List may be not full (H.264 and AAC here)
    switch(type)
    {
        // Video types
        case 0x01:
        case 0x02:
        case 0x10:
        case 0x1B:
        case 0x24:
            m_video_pid = pid;
            break;

        // Audio types
        case 0x03:
        case 0x0F:
            m_audio_pid = pid;
            break;
    }
}
