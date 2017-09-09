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
#include "MaildirFormatter.h"

const Aws::String bucket_name = "feed1-testaws-pcifarelli-net";

int main(int argc, char** argv)
{
    Aws::SDKOptions options;
    Aws::InitAPI(options);
    {
        MaildirFormatter mfmt;
        S3Get s3accessor(bucket_name);
        s3accessor.saveObjects("./mail", mfmt);
        s3accessor.printObjects();
    }

    Aws::ShutdownAPI(options);
    return 0;
}

