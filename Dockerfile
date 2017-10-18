FROM amazonlinux

# set the working directory
WORKDIR /emailfetch

# install needed packages
RUN yum update -y && yum install -y libcurl-devel openssl-devel libuuid-devel zlib-devel shadow-utils binutils busybox

RUN mkdir -p /vmail
RUN mkdir -p /emailfetch/bin
COPY emailfetch/emailfetch /emailfetch/bin
ADD emailfetch/lib /emailfetch/lib
ADD emailfetch/cfg /emailfetch/cfg
#RUN groupadd -g 5000 vmail
#RUN useradd -u 5000 -g 5000 vmail

VOLUME ["/vmail"]

# set the LD_LIBRARY_PATH
ENV PATH ${PATH}:/emailfetch/bin
ENV LD_LIBRARY_PATH /emailfetch/lib
# these if you are not on an EC2 instance
#ENV AWS_ACCESS_KEY_ID <access key id>
#ENV AWS_SECRET_ACCESS_KEY <secret key>

# install these for debug purposes
#RUN yum install -y net-tools
#RUN yum install -y iputils
#RUN yum install -y python27-pip
#RUN pip install --upgrade pip
#RUN pip install --upgrade setuptools
#RUN pip install awscli --upgrade --user

# run emailfetch
ENTRYPOINT ["/emailfetch/bin/emailfetch","-m","/vmail/cfg/emailfetch.json"]

