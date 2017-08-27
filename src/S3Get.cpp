/*
 * S3Get.cpp
 *
 *  Created on: Aug 26, 2017
 *      Author: paulc
 */

#include "S3Get.h"

S3Get::S3Get(Aws::String b_name) {
	bucket_name = b_name;

}

S3Get::~S3Get() {
}

void S3Get::list(void) const {
    Aws::S3::S3Client s3_client;
    auto outcome = s3_client.ListBuckets();

    if (outcome.IsSuccess()) {
        std::cout << "Your Amazon S3 buckets:" << std::endl;

        Aws::Vector<Aws::S3::Model::Bucket> bucket_list =
            outcome.GetResult().GetBuckets();

        for (auto const &bucket: bucket_list) {
            std::cout << "  * " << bucket.GetName() << std::endl;
        }
    } else {
        std::cout << "ListBuckets error: "
                  << outcome.GetError().GetExceptionName() << " - "
                  << outcome.GetError().GetMessage() << std::endl;
    }
}

