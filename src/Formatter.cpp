/*
 * Formatter.cpp
 *
 *  Created on: Aug 27, 2017
 *      Author: paulc
 *
 *      This simple formatter just saves each file using the object name as the file
 */

#include "Formatter.h"
#include <aws/core/Aws.h>
#include <string>
#include <cstddef>

Formatter::Formatter() : m_local_file(NULL), m_fullpath("")
{
}

Formatter::~Formatter()
{
    close();
}

void Formatter::close()
{
    if (m_local_file.is_open())
        m_local_file.close();
}

std::string Formatter::mkname(const Aws::S3::Model::Object obj) const
{
    return obj.GetKey().c_str();
}

void Formatter::open(const Aws::S3::Model::Object obj, const Aws::String pathname, const std::ios_base::openmode mode)
{
    std::string name = mkname(obj);
    Aws::String filename = name.c_str();

    Aws::String c = "";
    close();

    std::size_t n = pathname.find_last_of('/');
    if (n != std::string::npos && n != pathname.length() - 1)
        c = "/";

    m_fullpath = pathname + c + filename;

    m_name = filename;
    m_local_file.open(m_fullpath.c_str(), mode);
}

Aws::OStream &Formatter::getStream(void *buf, size_t sz)
{
    m_ostream.rdbuf()->pubsetbuf((char *) buf, sz);
    return m_ostream;
}

Aws::String Formatter::getKey(Aws::String filename) const
{
    return filename;
}
