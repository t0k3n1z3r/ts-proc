/**
********************************************************************************
* @file         ts_processor.cpp
* @brief        MPEG-TS file format processor class implementation
* @author       Maksym Koshel (maks.koshel@gmail.com)
* @date         Aug 26, 2017
********************************************************************************
*/

#include "ts_processor.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <endian.h>
#include <stdlib.h>

/*
********************************************************************************
*
********************************************************************************
*/
TSProcessor::TSProcessor(const char* const input, const char* const video,
        const char* const audio)
    : m_input_filename(input)
    , m_video_filename(video)
    , m_audio_filename(audio)
    , m_input_file(NULL)
    , m_video_file(NULL)
    , m_audio_file(NULL)
    , m_input_filesize(0)
    , m_pmt_pid(0x1fff)
    , m_video_pid(0x1fff)
    , m_audio_pid(0x1fff)
{

}

/*
********************************************************************************
*
********************************************************************************
*/
TSProcessor::~TSProcessor()
{
    if (NULL != m_input_file)
    {
        fclose(m_input_file);
        m_input_file = NULL;
    }

    if (NULL != m_video_file)
    {
        fclose(m_video_file);
        m_video_file = NULL;
    }

    if (NULL != m_audio_file)
    {
        fclose(m_audio_file);
        m_audio_file = NULL;
    }
}

/*
********************************************************************************
*
********************************************************************************
*/
STATUS TSProcessor::init()
{
    STATUS result = STATUS_FAIL;

    do
    {
        m_input_file = fopen(m_input_filename.c_str(), "rb");
        if (NULL == m_input_file)
        {
            fprintf(stderr, "Can't open input file (%s). Error: %s\n",
                m_input_filename.c_str(), strerror(errno));
            break;
        }

        m_video_file = fopen(m_video_filename.c_str(), "wb");
        if (NULL == m_video_file)
        {
            fprintf(stderr,
                "Can't open/create video stream file (%s). Error: %s\n",
                m_video_filename.c_str(), strerror(errno));
            break;
        }

        m_audio_file = fopen(m_audio_filename.c_str(), "wb");
        if (NULL == m_audio_file)
        {
            fprintf(stderr,
                "Can't open/create audio stream file (%s). Error: %s\n",
                m_video_filename.c_str(), strerror(errno));
            break;
        }
        
        int r = fseek(m_input_file, 0, SEEK_END);
        if (0 != r)
        {
            result = STATUS_FAIL;
            fprintf(stderr, "Can't seek to the end of file\n");
            break;
        }

        long size = ftell(m_input_file);
        if (size < 0)
        {
            fprintf(stderr, "Something went wrong during ftell()\n");
            break;
        }
        m_input_filesize = size;
        rewind(m_input_file);
        fprintf(stdout, "TSProcessor initialized:\n"
                        "\tInput file: %s (size: %lu bytes)\n"
                        "\tVideo file: %s\n"
                        "\tAudio file: %s\n",
                        m_input_filename.c_str(), m_input_filesize,
                        m_video_filename.c_str(), m_audio_filename.c_str());
        result = STATUS_OK;

    } while(0);

    return result;
}

/*
********************************************************************************
*
********************************************************************************
*/
STATUS TSProcessor::process_pat()
{
    STATUS result = STATUS_FAIL;

    uint16_t stream_id  = 0;
    uint16_t prog_id    = 0;

    do
    {
        struct ts_packet packet;
        result = read_packet(packet);
        if (0 != result)
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
            result = STATUS_FAIL;
            fprintf(stderr, "Number of programs different than 1 (%d)\n",
                prog_num);
            fprintf(stderr, "MPTS (Multiple Program Transport Stream) is not "
                "supported by this implementation\n");
            break;
        }

        prog_id = htobe16(*(uint16_t*)&packet.payload[pi]);
        pi += 2;

        m_pmt_pid = htobe16(*(uint16_t*)&packet.payload[pi]) & 0x1fff;
        pi += 2;

        result = STATUS_OK;

        fprintf(stdout, "PAT found:\n");
        fprintf(stdout, "\tMPEG-TS Stream ID: %d  (0x%x)\n", stream_id,
            stream_id);
        fprintf(stdout, "\tProgram ID: %d (0x%x)\n", prog_id, prog_id);
        fprintf(stdout, "\tPMT PID: %d (0x%x)\n", m_pmt_pid, m_pmt_pid);
        break;

    } while((size_t)ftell(m_input_file) < m_input_filesize);

    return result;
}

/*
********************************************************************************
*
********************************************************************************
*/
STATUS TSProcessor::process_pmt()
{
    STATUS result = STATUS_FAIL;

    do
    {
        struct ts_packet packet;
        result = read_packet(packet);
        if (0 != result)
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

        result = STATUS_OK;

        fprintf(stdout, "PMT found:\n");
        fprintf(stdout, "\tProgram number: %d (0x%x)\n", prog_num, prog_num);
        fprintf(stdout, "\tVideo PID: %d (0x%x)\n", m_video_pid, m_video_pid);
        fprintf(stdout, "\tAudio PID: %d (0x%x)\n", m_audio_pid, m_audio_pid);
        fprintf(stdout, "\tPCR PID:   %d (0x%x)\n", pcr_pid, pcr_pid);

        break;

    } while((size_t)ftell(m_input_file) < m_input_filesize);

    return result;
}

/*
********************************************************************************
*
********************************************************************************
*/
STATUS TSProcessor::demux()
{
    STATUS result = STATUS_FAIL;

    do
    {
        /**
        ************************************************************************
        * @note     At this moment all packets will be ignored except PID 0.
        *           After this function is finished execution PMT PID is known
        ************************************************************************
        */
        result = process_pat();
        if (STATUS_OK != result)
        {
            break;
        }

        /**
        ************************************************************************
        * @note     At this moment all packets will be ignored expept PID
        *           equals to PMT (found in PAT). This function has to
        *           find Video PID and audio PID and initialize corresponding
        *           class members
        ************************************************************************
        */
        result = process_pmt();
        if (STATUS_OK != result)
        {
            break;
        }

        /**
        ************************************************************************
        * @note     At this moment all packets will be ignored except video and
        *           audio PID
        ************************************************************************
        */
        result = process_file();
        if (STATUS_OK != result)
        {
            break;
        }

    } while(0);

    return result;
}

/*
********************************************************************************
*
********************************************************************************
*/
STATUS TSProcessor::read_packet(struct ts_packet& packet)
{
    STATUS result = STATUS_FAIL;

    do
    {
        size_t read_bytes = fread(&packet.header, 1, TS_PACKET_HEADER,
            m_input_file);
        if (TS_PACKET_HEADER != read_bytes)
        {
            fprintf(stderr, "Can't read TS packet header from %s file!\n",
                m_input_filename.c_str());
            break;
        }

        packet.header = htobe32(packet.header);
        if (!IS_PACKET_VALID(packet.header))
        {
            fprintf(stderr, "Sync byte of TS packet has wrong value\n");
            break;
        }

        read_bytes = fread(&packet.payload, 1, TS_PACKET_PAYLOAD, m_input_file);
        if (TS_PACKET_PAYLOAD != read_bytes)
        {
            fprintf(stderr, "Can't read TS packet payload from %s file!",
                m_input_filename.c_str());
            break;
        }

        result = STATUS_OK;
    } while(0);

    return result;
}

/*
********************************************************************************
*
********************************************************************************
*/
STATUS TSProcessor::process_file()
{
    STATUS result = STATUS_OK;
    
    do
    {
        struct ts_packet packet;
        result = read_packet(packet);
        if (0 != result)
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

        FILE* f = (pid == m_video_pid) ? m_video_file : m_audio_file;
        size_t w_bytes = fwrite(&packet.payload[pi], 1, TS_PACKET_PAYLOAD - pi,
            f);
        if (TS_PACKET_PAYLOAD - pi != w_bytes)
        {
            result = STATUS_FAIL;
            fprintf(stderr, "Can't write to (%s) file (%lu) bytes!\n",
                "test", TS_PACKET_PAYLOAD - pi);
            break;
        }

    } while((size_t)ftell(m_input_file) < m_input_filesize);

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
