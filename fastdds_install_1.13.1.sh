#!/bin/bash

# add-apt-repository -y ppa:ecal/ecal-5.9

apt update &&  apt install -y  git cmake libasio-dev libtinyxml2-dev libssl-dev libp11-dev libengine-pkcs11-openssl softhsm2 libengine-pkcs11-openssl libpython3-dev swig libcurl4-openssl-dev libiodbc2 libiodbc2-dev libmysqlclient-dev libpq-dev  libcurl4-openssl-dev p11-kit libyaml-cpp-dev liblz4-dev libzstd-dev uuid-dev software-properties-common librdkafka-dev openjdk-11-jdk 

apt install -y libboost-filesystem-dev libboost-iostreams-dev libboost-system-dev libboost-thread-dev libboost-regex-dev libboost-chrono-dev libboost-atomic-dev

openssl engine pkcs11 -t

mkdir -p ~/git

cd ~/git && git clone --branch release-1.11.0 https://github.com/google/googletest googletest-distribution
cd ~/git/googletest-distribution/ && mkdir build && cd build/ && cmake .. &&  make -j$(npro) &&  make install
cd ~/git && git clone https://github.com/eProsima/foonathan_memory_vendor.git
cd ~/git && mkdir -p foonathan_memory_vendor/build && cd foonathan_memory_vendor/build &&git checkout v1.3.1 \
&& cmake .. -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC"  -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_SHARED_LIBS=ON&&cmake --build . --parallel $(nproc)  --target install
cd ~/git && git clone https://github.com/eProsima/Fast-CDR.git
cd ~/git &&mkdir Fast-CDR/build && cd Fast-CDR/build && git checkout v2.1.3 && cmake .. -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC"  -DCMAKE_INSTALL_PREFIX=/usr/local &&   cmake --build . --parallel $(nproc)  --target install
cd ~/git &&git clone https://github.com/eProsima/Fast-DDS.git
cd ~/git &&mkdir Fast-DDS/build && cd Fast-DDS/build && git checkout v2.13.1&& cmake .. -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DFASTDDS_STATISTICS=OFF -DCMAKE_CXX_FLAGS="-fPIC"  -DCMAKE_INSTALL_PREFIX=/usr/local &&  cmake --build . --parallel $(nproc)  --target install
cd ~/git &&  git clone https://github.com/eProsima/Fast-DDS-python.git && cd Fast-DDS-python &&git checkout v1.4.0 \
 && mkdir -p fastdds_python/build &&   cd fastdds_python/build && cmake ..    &&  cmake --build . --parallel $(nproc)  --target install

cd ~/git && git clone https://github.com/protocolbuffers/protobuf.git
cd protobuf && git checkout  v3.8.0 && git submodule update --init --recursive && mkdir -p cmake/build/ && cd cmake/build/ \
    && cmake .. -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC"  &&  cmake --build . --parallel $(nproc)  --target install

cd ~/git && git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c && git checkout v1.3.8 \
    && cmake -Bbuild -H. -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC" -DPAHO_ENABLE_TESTING=OFF -DPAHO_BUILD_STATIC=ON  -DPAHO_WITH_SSL=ON -DPAHO_HIGH_PERFORMANCE=ON \
    &&  cmake --build build --target install \
    &&ldconfig

cd ~/git && git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp
cd aws-sdk-cpp && git checkout 1.11.327 && mkdir build && cd build && cmake .. -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC"  \
    -DCMAKE_PREFIX_PATH=/usr/local \
    -DBUILD_ONLY="s3" &&  cmake --build . --parallel $(nproc)  --config=Release &&  cmake --install . --config=Release

cd ~/git && git clone https://github.com/CppMicroServices/CppMicroServices.git 
cd CppMicroServices && git checkout v3.7.6 && git submodule init && git submodule update \
&& mkdir -p build && cd build && cmake .. -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC"  -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_SHARED_LIBS=ON \
&&  cmake --build . --parallel $(nproc)  --target install 


cd ~/git &&  git clone https://github.com/SOCI/soci.git
cd soci && git checkout v4.0.3 && mkdir build && cd build \
    && cmake .. -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC"  -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_SHARED_LIBS=ON -DSOCI_CXX11=ON && cmake --build . --parallel $(nproc)  --target install

cd ~/git && git clone -b poco-1.12.4-release https://github.com/pocoproject/poco.git
cd poco && mkdir cmake-build && cd cmake-build \
    && cmake .. -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC" -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_SHARED_LIBS=ON -DENABLE_CPPPARSER=ON -DENABLE_XML=ON -DENABLE_JSON=ON &&  cmake --build . --parallel $(nproc)  --target install

#RUN git config --global http.sslverify false
#COPY spdlog ~/git/spdlog 
cd ~/git && git clone https://github.com/gabime/spdlog
cd spdlog && git checkout v1.12.0 && mkdir build && cd build && cmake .. -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC"  -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_SHARED_LIBS=ON && cmake --build . --parallel $(nproc)  --target install


#dev_util

cd ~/git && git clone https://github.com/eProsima/dev-utils.git && cd dev-utils && git checkout v0.5.0 && mkdir -p build/cmake_utils && cd build/cmake_utils \
    && cmake ../../../dev-utils/cmake_utils -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC" \
    &&  cmake --build . --parallel $(nproc)  --target install \
    && cd ../../../dev-utils/ && mkdir -p build/cpp_utils && cd build/cpp_utils \
    && cmake ../../../dev-utils/cpp_utils -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC" \
    &&  cmake --build . --parallel $(nproc)  --target install

#dds_pipe

cd ~/git &&  git clone https://github.com/eProsima/DDS-Pipe.git && cd DDS-Pipe && git checkout v0.3.0 && mkdir -p build/ddspipe_core && cd build/ddspipe_core \
    && cmake ../../../DDS-Pipe/ddspipe_core -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC" \
    &&  cmake --build . --parallel $(nproc)  --target install 
cd ~/git &&  cd ~/git/DDS-Pipe && mkdir -p build/ddspipe_participants && cd build/ddspipe_participants \
    && cmake ~/git/DDS-Pipe/ddspipe_participants/ -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC" \
    &&  cmake --build . --parallel $(nproc)  --target install 
cd ~/git &&  cd ~/git/DDS-Pipe && mkdir -p build/ddspipe_yaml && cd build/ddspipe_yaml \
    && cmake ~/git/DDS-Pipe/ddspipe_yaml/ -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC" \
    &&  cmake --build . --parallel $(nproc)  --target install

# apt install libasio-dev libtinyxml2-dev libyaml-cpp-dev -y

# ddsrecorder_participants
cd ~/git && git clone --recurse-submodules https://github.com/eProsima/DDS-Record-Replay.git  && cd DDS-Record-Replay && git checkout v0.3.0  && mkdir -p build/ddsrecorder_participants \
    && cd build/ddsrecorder_participants && cmake ../../../DDS-Record-Replay/ddsrecorder_participants \
    -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC" -DCMAKE_INSTALL_PREFIX=/usr/local && cmake --build . --parallel $(nproc)  --target install

# ddsrecorder_yaml
cd ~/git && cd DDS-Record-Replay && mkdir -p build/ddsrecorder_yaml && cd build/ddsrecorder_yaml \
    && cmake ~/git/DDS-Record-Replay/ddsrecorder_yaml -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC" \
    -DCMAKE_INSTALL_PREFIX=/usr/local &&  cmake --build . --parallel $(nproc)  --target install

# ddsrecorder
cd ~/git && cd DDS-Record-Replay && mkdir -p build/ddsrecorder_tool && cd build/ddsrecorder_tool \
    && cmake ~/git/DDS-Record-Replay/ddsrecorder -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC"  -DCMAKE_INSTALL_PREFIX=/usr/local \
    &&   cmake --build . --parallel $(nproc)  --target install

# ddsreplayer
cd ~/git && cd DDS-Record-Replay && mkdir -p build/ddsreplayer_tool && cd build/ddsreplayer_tool \
    && cmake ~/git/DDS-Record-Replay/ddsreplayer -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC"  -DCMAKE_INSTALL_PREFIX=/usr/local \
    &&   cmake --build . --parallel $(nproc)  --target install 

# ddsrouter
# RUN pip install -U jsonschema
#RUN apt install libasio-dev libtinyxml2-dev libyaml-cpp-dev -y

cd ~/git && git clone https://github.com/eProsima/DDS-Router.git && cd DDS-Router && git checkout v2.1.0 && mkdir -p mkdir build/ddsrouter_core \
   && cd build/ddsrouter_core && cmake ~/git/DDS-Router/ddsrouter_core  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC"  -DCMAKE_INSTALL_PREFIX=/usr/local \
   &&   cmake --build . --parallel $(nproc)  --target install

# ddsrouter_yaml
cd ~/git && cd ~/git/DDS-Router && mkdir -p build/ddsrouter_yaml && cd build/ddsrouter_yaml \
    && cmake ~/git/DDS-Router/ddsrouter_yaml -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC"  -DCMAKE_INSTALL_PREFIX=/usr/local \
    &&   cmake --build . --parallel $(nproc)  --target install

# ddsrouter_tool
cd ~/git && cd ~/git/DDS-Router && mkdir -p build/ddsrouter_tool &&cd build/ddsrouter_tool \
    && cmake ~/git/DDS-Router/tools/ddsrouter_tool -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC"  -DCMAKE_INSTALL_PREFIX=/usr/local \
    &&   cmake --build . --parallel $(nproc)  --target install

#RUN cd ~/git/databus2/node/src/ && fastddsgen node.idl -replace && fastddsgen file.idl -replace 


#RUN apt-get install -y liblz4-dev -y


cd ~/git && git clone https://github.com/lz4/lz4.git
cd ~/git && cd lz4/build/cmake && mkdir build && cd build && git checkout v1.9.4 \
	&& cmake .. -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fPIC"  -DCMAKE_INSTALL_PREFIX=/usr/local  &&  cmake --build . --parallel $(nproc)  --target install
 ln -s /usr/local/lib/cmake/lz4/lz4Config.cmake /usr/local/lib/cmake/lz4/LZ4Config.cmake &&  ln -s /usr/local/lib/cmake/LZ4 /usr/local/lib/cmake/lz4


cd ~/git && git clone https://github.com/confluentinc/librdkafka.git 
#COPY librdkafka ~/git/librdkafka
cd ~/git && cd librdkafka && git checkout v2.3.0  && mkdir build && cd build \
    && cmake .. -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release  -DCMAKE_CXX_FLAGS="-fPIC"  -DCMAKE_INSTALL_PREFIX=/usr/local  && cmake --build . --parallel $(nproc)  --target install


apt install -y tzdata \
    && ln -fs /usr/share/zoneinfo/${TZ} /etc/localtime \
    && echo ${TZ} > /etc/timezone \
    && dpkg-reconfigure --frontend noninteractive tzdata \
    && rm -rf /var/lib/apt/lists/*

#apt-get autoclean -y && apt-get clean -y && apt-get autoremove -y
