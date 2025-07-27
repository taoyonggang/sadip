#!/bin/bash

apt install -y  openjdk-11-jdk 

update-alternatives --set java /usr/lib/jvm/java-11-openjdk-amd64/bin/java

# 安装目录
INSTALL_DIR="/opt/gradle"
# Gradle 版本
GRADLE_VERSION="7.5.1"
# Gradle 下载链接
GRADLE_URL=" https://mirrors.cloud.tencent.com/gradle/gradle-${GRADLE_VERSION}-bin.zip"
# 下载文件名
GRADLE_ZIP="gradle-${GRADLE_VERSION}-bin.zip"

# 创建安装目录
sudo mkdir -p $INSTALL_DIR
cd $INSTALL_DIR

# 下载 Gradle
echo "下载 Gradle..."
wget $GRADLE_URL

# 解压 Gradle
echo "解压 Gradle..."
sudo unzip -q $GRADLE_ZIP
sudo rm $GRADLE_ZIP

# 设置环境变量
echo "设置环境变量..."
GRADLE_HOME=$INSTALL_DIR/gradle-${GRADLE_VERSION}
echo "export GRADLE_HOME=$GRADLE_HOME" >> /etc/profile
echo 'export PATH=$PATH:$GRADLE_HOME/bin' >> /etc/profile
source /etc/profile

# 验证安装
gradle -v

# 配置阿里云仓库到全局 gradle.properties
#echo "配置阿里云仓库..."
#GRADLE_PROPERTIES=$HOME/.gradle/gradle.properties
#echo "systemProp.http.proxyHost=mirrors.aliyun.com" >> $GRADLE_PROPERTIES
#echo "systemProp.http.proxyPort=80" >> $GRADLE_PROPERTIES
#echo "systemProp.https.proxyHost=mirrors.aliyun.com" >> $GRADLE_PROPERTIES
#echo "systemProp.https.proxyPort=80" >> $GRADLE_PROPERTIES
#echo "systemProp.http.nonProxyHosts=local|*.local|169.254/16|*.169.254/16" >> $GRADLE_PROPERTIES

#echo "Gradle 安装和配置完成。"

echo "开始编译Fast-DDS-Gen"
rm -rf ~/git/Fast-DDS-Gen/
cd ~/git &&git clone --recursive https://github.com/eProsima/Fast-DDS-Gen.git && cd Fast-DDS-Gen && git checkout v2.3.0 && gradle assemble

cp -r ~/git/Fast-DDS-Gen/ /opt
echo 'PATH=$PATH:/opt/Fast-DDS-Gen/scripts'  >> /etc/profile

source /etc/profile

#fastddsgen --version

echo "编译Fast-DDS-Gen成功"
