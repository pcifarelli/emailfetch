/*
 * MaildirFormatter.h
 *
 *  Created on: Aug 27, 2017
 *      Author: paulc
 */

#ifndef SRC_MAILDIRFORMATTER_H_
#define SRC_MAILDIRFORMATTER_H_

#include "Formatter.h"

class MaildirFormatter: public Formatter {
public:
	MaildirFormatter(Aws::String filename);
	virtual ~MaildirFormatter();

private:
	Aws::String constructName();
};

#endif /* SRC_MAILDIRFORMATTER_H_ */
