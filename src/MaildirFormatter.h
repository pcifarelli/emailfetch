/*
 * MaildirFormatter.h
 *
 *  Created on: Aug 27, 2017
 *      Author: paulc
 */

#ifndef SRC_MAILDIRFORMATTER_H_
#define SRC_MAILDIRFORMATTER_H_

#include "MailFormatter.h"

class MaildirFormatter: public MailFormatter {
public:
	MaildirFormatter(Aws::OFStream &strm);
	virtual ~MaildirFormatter();

private:
};

#endif /* SRC_MAILDIRFORMATTER_H_ */
