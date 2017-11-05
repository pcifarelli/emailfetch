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
#include "Formatter.h"

namespace S3Downloader
{

typedef Aws::Vector<Aws::S3::Model::Object> s3object_list;
typedef Aws::S3::Model::Object s3object;

/**
 *  The purpose of this class is to encapsulate downloading objects from AWS S3
 */
class S3Get
{
public:
    S3Get();
    S3Get(Aws::String b_name, unsigned int timeout = 30000);
    S3Get(Aws::String b_name, Aws::String access_id, Aws::String secret_key, unsigned int timeout = 30000);
    virtual ~S3Get();

    Aws::String bucketName() const { return m_bucket_name; }
    void listBuckets(void) const;
    void listObjects(void) const;
    void saveObjects(Formatter &fmtr);

    void printObjects();
    s3object_list &getObjectList(void);
    s3object_list refreshObjectList(void);

    // save to a file using the Formatter to open and save the file
    bool objSaveAs(const Aws::S3::Model::Object obj, Formatter &fmtr) const;
    // save it to a buffer
    bool objGet(const Aws::S3::Model::Object obj, void *buf, size_t sz) const;

private:
    Aws::S3::S3Client *m_s3_client;
    Aws::String m_bucket_name;
    s3object_list m_object_list;
};

}
#endif /* SRC_S3GET_H_ */
