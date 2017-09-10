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

#define DAYS_TO_CHECK 16

int main(int argc, char** argv)
{
    Aws::SDKOptions options;
    Aws::InitAPI(options);
    {
        MaildirFormatter mfmt;
        S3Get s3accessor(bucket_name);
        Downloader downl("./mail", DAYS_TO_CHECK, s3accessor, mfmt);

        downl.saveNewObjects();
        downl.printMap();
        std::cout << std::endl << "after purge: " << std::endl;
        downl.purgeMap();
        downl.printMap();

        auto n = std::chrono::system_clock::now();
        std::time_t now = std::chrono::system_clock::to_time_t(n);

        std::cout << "now is " << now << std::endl;

        //s3accessor.saveObjects("./mail", mfmt);
        //s3accessor.printObjects();
    }

    Aws::ShutdownAPI(options);
    return 0;
}

