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

    virtual void open(const Aws::S3::Model::Object obj, const Aws::String pathname, const std::ios_base::openmode mode = std::ios::out | std::ios::binary);
    virtual std::string mkname(const Aws::S3::Model::Object obj) const;
    virtual void close();
    virtual Aws::OStream &getStream() { return m_local_file; }
    virtual Aws::OStream &getStream(void *buf, size_t sz);
    Aws::String &getName(void) { return m_name; }

    // getKey is used to create a map of what we've already downloaded.  The key is the thing you want to use for comparisons.
    // you are passed the object and the filename to construct the key from
    // currently, it is not templated, because the only existing use cases are string keys
    virtual Aws::String getKey(Aws::String filename) const;

private:
    Aws::OFStream m_local_file;
    std::stringstream m_ostream;

protected:
    Aws::String m_name;		// this is just the name of the file
    Aws::String m_fullpath;	// this is the fullpath, including the filename
};

#endif /* SRC_MAILFORMATTER_H_ */
