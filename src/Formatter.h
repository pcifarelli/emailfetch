/*
 * Formatter.h
 *
 *  Created on: Aug 27, 2017
 *      Author: paulc
 */

#ifndef SRC_FORMATTER_H_
#define SRC_FORMATTER_H_

#include <stdlib.h>
#include <aws/core/Aws.h>
#include <fstream>
#include <ios>

class Formatter {
public:
	Formatter(Aws::OFStream &local_file);
	virtual ~Formatter();

	virtual void open(const char* filename,  const std::ios_base::openmode mode);
	virtual Aws::OFStream &getStream() { return m_local_file; }
	virtual Aws::String getName( void ) { return m_name; }

private:
	Aws::OFStream &m_local_file;
	Aws::String    m_name;
};

#endif /* SRC_MAILFORMATTER_H_ */
