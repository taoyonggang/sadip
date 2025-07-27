from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps

class SadipRecipe(ConanFile):
    name = "sadip"
    version = "0.1"
    package_type = "application"

    # Metadata
    license = "Proprietary"
    author = "SADIP Team"
    url = "https://github.com/your-org/sadip"
    description = "System Application Data Integration Platform"
    topics = ("integration", "data", "platform")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"

    # Sources
    exports_sources = "CMakeLists.txt", "src/*"

    # Dependencies
    requires = [
        "zlib/1.3.1",
        "openssl/1.1.1v",
        "libmysqlclient/8.1.0",
        "boost/1.83.0",
        "librdkafka/2.6.1",
        "protobuf/3.21.12",
        "aws-sdk-cpp/1.11.352",
        "poco/1.12.4",
        "soci/4.0.3"
    ]

    # Default options
    default_options = {
        # AWS SDK options
        "aws-sdk-cpp/*:min_size": True,
        "aws-sdk-cpp/*:s3": True,
        "aws-sdk-cpp/*:sts": True,
        "aws-sdk-cpp/*:kms": True,
        "aws-sdk-cpp/*:build_tools": False,
        
        # POCO options
        "poco/*:shared": True,
        "poco/*:enable_data_sqlite": False,
        "poco/*:enable_data_mysql": False,
        "poco/*:enable_data_postgresql": False,
        "poco/*:enable_data_odbc": False,
        "poco/*:enable_mongodb": True,
        
        # Other options
        "sqlite3/*:build_executable": False,
        "sqlite3/*:enable_column_metadata": True,
        "openssl/*:shared": True,
        "zlib/*:shared": True,
        
        # SOCI options
        "soci/*:shared": True,
        "soci/*:empty": True,
        "soci/*:with_sqlite3": True,
        "soci/*:with_postgresql": True,
        "soci/*:with_mysql": True,
        "soci/*:with_odbc": True,
        "soci/*:with_boost": True,
    }

    def requirements(self):
        # 系统特定的依赖要求
        if self.settings.os == "Windows":
            # Windows specific requirements
            # Note: MySQL 8.4.3, FastDDS 2.11.3 and manually compiled spdlog are required
            self.output.warning("Windows requires MySQL 8.4.3, FastDDS 2.11.3 and manually compiled spdlog")
        else:
            # Linux specific requirements
            # Note: MySQL client and compiled FastDDS, spdlog are required
            self.output.warning("Linux requires MySQL client and compiled FastDDS, spdlog. Refer to Dockerfile")

    def layout(self):
        self.folders.generators = "generators"
        self.folders.build = "build"
        self.folders.source = "src"

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = [self.name]