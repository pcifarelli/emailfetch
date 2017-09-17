/*
 * EmailFetcherMain.cpp
 *
 *  Created on: Aug 20, 2017
 *      Author: paulc
 */
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/CreateBucketRequest.h>

#include "S3Get.h"
#include "Downloader.h"
#include "MaildirFormatter.h"

const Aws::String bucket_name = "feed1-testaws-pcifarelli-net";
const Aws::String topic_arn   = "arn:aws:sns:us-east-1:331557686640:feed1-testaws-pcifarelli-net";

#define DAYS_TO_CHECK 60

int main(int argc, char** argv)
{
    Aws::SDKOptions options;
    Aws::InitAPI(options);
    {
        MaildirFormatter mfmt;
        S3Get s3accessor(bucket_name);
        Downloader downl("./mail", DAYS_TO_CHECK, topic_arn, s3accessor, mfmt);

        downl.saveNewObjects();

        //s3accessor.saveObjects("./mail", mfmt);
        //s3accessor.printObjects();
    }

    Aws::ShutdownAPI(options);
    return 0;
}

