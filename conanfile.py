from conan import ConanFile

class FaradoConanFile(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("zlib/1.2.11")
        self.requires("boost/1.83.0")
        self.requires("libcurl/7.88.1")
        self.requires("openssl/3.1.4")
        self.requires("crowcpp-crow/1.0+5")
        self.requires("sqlite3/3.42.0")
        self.requires("cpr/1.10.5")
        self.requires("nlohmann_json/3.11.3")
        self.requires("icu/74.2")
