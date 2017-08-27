/*
 * S3Get.cpp
 *
 *  Created on: Aug 26, 2017
 *      Author: paulc
 */

#include "S3Get.h"

S3Get::S3Get(Aws::String b_name) {
	m_bucket_name = b_name;

}

S3Get::~S3Get() {
}

void S3Get::listBuckets(void) const {
    auto outcome = m_s3_client.ListBuckets();

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

void S3Get::listObjects() const {
	s3object_list list = getObjectList();

	if ( !list.empty() ) {
		for (auto const &s3_object: list)
			std::cout << "* " << s3_object.GetKey() << std::endl;
	}
	else
		std::cout << "Error getting objects from bucket " << m_bucket_name << std::endl;
}

s3object_list S3Get::getObjectList( void ) const {
	s3object_list list;
	Aws::S3::Model::ListObjectsRequest objects_request;
	objects_request.WithBucket(m_bucket_name);

	auto result = m_s3_client.ListObjects(objects_request);
	if (result.IsSuccess()) {
	        list = result.GetResult().GetContents();
	} else {
	    std::cout << "ListObjects error: " <<
	        result.GetError().GetExceptionName() << " " <<
	        result.GetError().GetMessage() << std::endl;
	}
	return list;
}

bool S3Get::objSaveAs(Aws::String key, Aws::String path) const {
	Aws::S3::Model::GetObjectRequest object_request;
	object_request.WithBucket(m_bucket_name).WithKey(key);
    Aws::OFStream local_file;

	auto result = m_s3_client.GetObject(object_request);

	if (result.IsSuccess()) {
		local_file.open(path.c_str(), std::ios::out | std::ios::binary);
		local_file << result.GetResult().GetBody().rdbuf();
		return true;
	}
	return false;
}

void S3Get::saveObjects(Aws::String dir) const {
	s3object_list list = getObjectList();

	if ( !list.empty() ) {
		for (auto const &s3_object: list) {
			Aws::String path = dir + "/" + s3_object.GetKey();
			objSaveAs(s3_object.GetKey(), path);
		}
	}
	else
		std::cout << "Error getting objects from bucket " << m_bucket_name << std::endl;
}


