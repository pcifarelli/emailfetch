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
#include <aws/s3/model/Object.h>
#include <fstream>
#include <ios>

class Formatter
{
public:
    Formatter();
    virtual ~Formatter();

    virtual void open(const Aws::S3::Model::Object obj, const Aws::String pathname, const Aws::String filename, const std::ios_base::openmode mode = std::ios::out | std::ios::binary);
    virtual void close();
    virtual Aws::OStream &getStream()
    {
        return m_local_file;
    }
    virtual Aws::OStream &getStream(void *buf, size_t sz);
    Aws::String &getName(void)
    {
        return m_name;
    }

private:
    Aws::OFStream m_local_file;
    std::stringstream m_ostream;

protected:
    Aws::String m_name;		// this is just the name of the file
    Aws::String m_fullpath;	// this is the fullpath, including the filename
};

#endif /* SRC_MAILFORMATTER_H_ */
