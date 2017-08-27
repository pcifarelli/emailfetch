/*
 * MailFormatter.h
 *
 *  Created on: Aug 27, 2017
 *      Author: paulc
 */

#ifndef SRC_MAILFORMATTER_H_
#define SRC_MAILFORMATTER_H_

#include <stdlib.h>
#include <aws/core/Aws.h>
#include <fstream>
#include <ios>

class MailFormatter {
public:
	MailFormatter(Aws::OFStream &local_file);
	virtual ~MailFormatter();

	virtual void open(const char* filename,  const std::ios_base::openmode mode);
	virtual Aws::OFStream &getStream() { return m_local_file; }

private:
	Aws::OFStream &m_local_file;
};

#endif /* SRC_MAILFORMATTER_H_ */
