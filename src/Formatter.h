/*
 * Formatter.h
 *
 *  Created on: Aug 27, 2017
 *      Author: paulc
 *
 *      This simple formatter just saves each file using the object name as the file
 */

#ifndef SRC_FORMATTER_H_
#define SRC_FORMATTER_H_

#include <stdlib.h>
#include <aws/core/Aws.h>
#include <fstream>
#include <ios>

class Formatter
{
public:
    Formatter(Aws::String filename, const std::ios_base::openmode mode = std::ios::out | std::ios::binary);
    virtual ~Formatter();

    virtual void open(Aws::String filename, const std::ios_base::openmode mode = std::ios::out | std::ios::binary);
    virtual void open(const std::ios_base::openmode mode = std::ios::out | std::ios::binary);
    virtual void close();
    virtual Aws::OFStream &getStream()
    {
        return m_local_file;
    }
    Aws::String getName(void)
    {
        return m_name;
    }

private:
    Aws::OFStream m_local_file;
    Aws::String m_name;		// this is just the name of the file
    Aws::String m_fullpath;	// this is the fullpath, including the filename
};

#endif /* SRC_MAILFORMATTER_H_ */
