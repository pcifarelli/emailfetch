/*
 * Formatter.cpp
 *
 *  Created on: Aug 27, 2017
 *      Author: paulc
 */

#include "Formatter.h"
#include <aws/core/Aws.h>
#include <string>
#include <cstddef>

Formatter::Formatter(Aws::OFStream &local_file) : m_local_file(local_file) {

}

Formatter::~Formatter() {
	// TODO Auto-generated destructor stub
}

void Formatter::open(const char* filename,  const std::ios_base::openmode mode) {

	std::string fname(filename);
	std::string name;

	std::size_t n = fname.find_last_of('/');
	if (n != std::string::npos) {
		name = fname.substr(n+1);
	}
	else
		name = fname;

	m_name = name.c_str();
	std::cout << m_name << std::endl;
	m_local_file.open(filename, mode);
}



