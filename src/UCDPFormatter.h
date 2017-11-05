/*
 * UCDPFormatter.h
 *
 *  Created on: Oct 29, 2017
 *      Author: paulc
 */

#ifndef SRC_UCDPFORMATTER_H_
#define SRC_UCDPFORMATTER_H_

#include "Formatter.h"

class UCDPFormatter: public S3Downloader::Formatter
{
public:
    UCDPFormatter(Aws::String workdir);
    virtual ~UCDPFormatter();
};

#endif /* SRC_UCDPFORMATTER_H_ */
