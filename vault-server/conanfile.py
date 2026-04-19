from conan import ConanFile

class FaradoConanFile(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("boost/1.83.0")
        self.requires("nlohmann_json/3.11.3")
        self.requires("sqlite3/3.51.3")
        self.requires("openssl/3.6.2")
        self.requires("cpprestsdk/2.10.19")
        self.requires("zlib/1.3.1")
        self.requires("websocketpp/0.8.2")
        self.requires("jwt-cpp/0.7.2")
