/*
 * EmailFetcherMain.cpp
 *
 *  Created on: Aug 20, 2017
 *      Author: paulc
 */
#include <unistd.h>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/CreateBucketRequest.h>

#include "S3Get.h"
#include "Downloader.h"
#include "MaildirFormatter.h"

const Aws::String bucket_name = "feed1-testaws-pcifarelli-net";
const Aws::String topic_arn   = "arn:aws:sns:us-east-1:331557686640:feed1-testaws-pcifarelli-net";

#define DAYS_TO_CHECK 60

using namespace S3Downloader;

int main(int argc, char** argv)
{
    Aws::SDKOptions options;
    Aws::InitAPI(options);
    {
        MaildirFormatter mfmt;
        Downloader downl("./mail", DAYS_TO_CHECK, topic_arn, bucket_name, mfmt);

        downl.start();
        sleep(60);
        downl.stop();
    }

    Aws::ShutdownAPI(options);
    return 0;
}

