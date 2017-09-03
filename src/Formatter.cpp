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

Formatter::Formatter() : m_local_file(NULL)
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

void Formatter::open(Aws::String pathname, Aws::String name, const std::ios_base::openmode mode)
{
    close();

    std::size_t n = pathname.find_last_of('/');
    if (n != std::string::npos && n != pathname.length() - 1)
        pathname += '/';

    m_fullpath = pathname + name;

    m_name = name;
    m_local_file.open(m_fullpath.c_str(), mode);
}



