/*
 * MailFormatter.cpp
 *
 *  Created on: Aug 27, 2017
 *      Author: paulc
 */

#include "MailFormatter.h"

MailFormatter::MailFormatter(Aws::OFStream &local_file) : m_local_file(local_file) {

}

MailFormatter::~MailFormatter() {
	// TODO Auto-generated destructor stub
}

void MailFormatter::open(const char* filename,  const std::ios_base::openmode mode) {
	m_local_file.open(filename, mode);
}



