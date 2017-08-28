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

Formatter::Formatter(Aws::String filename, const std::ios_base::openmode mode) :
		m_local_file(NULL) {
	open(filename, mode);
}

Formatter::~Formatter() {
	close();
}

void Formatter::close() {
}

void Formatter::open(Aws::String filename, const std::ios_base::openmode mode) {
	if (m_local_file.is_open())
		m_local_file.close();

	m_fullpath = filename;
	std::string fname(filename.c_str());
	std::string name;

	std::size_t n = fname.find_last_of('/');
	if (n != std::string::npos) {
		name = fname.substr(n + 1);
	} else
		name = fname;

	m_name = name.c_str();
	m_local_file.open(filename.c_str(), mode);
}

void Formatter::open(const std::ios_base::openmode mode) {
	open(m_fullpath, mode);
}
