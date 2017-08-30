/**
********************************************************************************
* @file         es_writer.h
* @brief        Elementary Writer classes definition
* @author       Maksym Koshel (maks.koshel@gmail.com)
* @date         Aug 30, 2017
********************************************************************************
*/

#ifndef _ES_WRITER_H_
#define _ES_WRITER_H_

#include <fstream>

/**
********************************************************************************
* @class 		ESWriter
* @brief 		Abstract class with interface method write to process Elementary
*				streams
* @note 		Base class was created in order to possible extend
*				implementation per ES. For video might be usefull to split
*				VCL and non-VCL data (or index it). But at a moment
*				implementation is identical.
********************************************************************************
*/
class ESWriter
{
public:
	virtual ~ESWriter();

	/**
	****************************************************************************
	* @brief 	Interface method which has to be implemented to write ES into
	*			separate file
	* @param 	[in] buffer 	ES buffer will be written
	* @param 	[in] size 		Size of the ES buffer
	* @return 	void
	****************************************************************************
	*/
	virtual void write(const uint8_t* const buffer, size_t size) = 0;

protected:
	/**
	****************************************************************************
	* @brief 	Contructor of abstract class is protected in order to prevent
	*			object creation directly
	****************************************************************************
	*/
	ESWriter(const char* const filename);

private:
	ESWriter()                            = delete;
    ESWriter(const ESWriter& r)           = delete;
    ESWriter& operator= (const ESWriter&) = delete;

protected:
	std::ofstream   m_file;       ///< Output file stream handler
};

/**
********************************************************************************
* @class 		VideoESWriter
* @brief 		Video Elementary Stream writer, implements write method
*				VideoESWriter
********************************************************************************
*/
class VideoESWriter : public ESWriter
{
public:
	VideoESWriter(const char* const filename);
	~VideoESWriter();

	/**
	****************************************************************************
	* @brief 	Implementation interface from base class which handles Video ES
	* @param 	[in] buffer 	ES buffer will be written
	* @param 	[in] size 		Size of the ES buffer
	* @return 	void
	****************************************************************************
	*/
	void write(const uint8_t* const buffer, size_t size);

private:
	VideoESWriter()                               	= delete;
    VideoESWriter(const VideoESWriter& r)           = delete;
    VideoESWriter& operator= (const VideoESWriter&) = delete;
};

/**
********************************************************************************
* @class 		AudioESWriter
* @brief 		Audio Elementary Stream writer, implements write method
*				AudioESWriter
********************************************************************************
*/
class AudioESWriter : public ESWriter
{
public:
	AudioESWriter(const char* const filename);
	~AudioESWriter();

	/**
	****************************************************************************
	* @brief 	Implementation interface from base class which handles Audio ES
	* @param 	[in] buffer 	ES buffer will be written
	* @param 	[in] size 		Size of the ES buffer
	* @return 	void
	****************************************************************************
	*/
	void write(const uint8_t* const buffer, size_t size);
private:
	AudioESWriter()                               	= delete;
    AudioESWriter(const AudioESWriter& r)           = delete;
    AudioESWriter& operator= (const AudioESWriter&) = delete;
};

#endif /* !_ES_WRITER_H_ */
