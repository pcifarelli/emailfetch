/*
 * S3Get.h
 *
 *  Created on: Aug 26, 2017
 *      Author: paulc
 */

#ifndef SRC_S3GET_H_
#define SRC_S3GET_H_

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/CreateBucketRequest.h>


class S3Get {
public:
	S3Get() {};
	S3Get(Aws::String b_name);
	virtual ~S3Get();

	void list( void ) const;

private:
	Aws::String bucket_name;
};

#endif /* SRC_S3GET_H_ */
