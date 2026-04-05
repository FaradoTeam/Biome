from conan import ConanFile

class FaradoConanFile(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("boost/1.83.0")
        self.requires("nlohmann_json/3.11.3")
        self.requires("openssl/3.1.4")
