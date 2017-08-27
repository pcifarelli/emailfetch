/*
 * S3Get.h
 *
 *  Created on: Aug 26, 2017
 *      Author: paulc
 */

#ifndef SRC_S3GET_H_
#define SRC_S3GET_H_

#include <stdlib.h>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/Object.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <fstream>
#include <ios>

typedef Aws::Vector<Aws::S3::Model::Object> s3object_list;
typedef Aws::S3::Model::Object s3object;

class S3Get {
public:
	S3Get() {};
	S3Get(Aws::String b_name);
	virtual ~S3Get();

	void listBuckets( void ) const;
	void listObjects( void ) const;
	void saveObjects( Aws::String dir ) const;
	s3object_list getObjectList( void ) const;
	bool objSaveAs(Aws::String key, Aws::String path) const;

private:
	Aws::S3::S3Client 	m_s3_client;
	Aws::String 		m_bucket_name;
};

#endif /* SRC_S3GET_H_ */
