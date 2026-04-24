from conan import ConanFile
from conan.tools.cmake import cmake_layout

class BiomeConanFile(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    def configure(self):
        self.options["qt/*"].shared = True
        self.options["qt/*"].qttools = True
        # Обязательные опции для QML и Quick
        self.options["qt/*"].qtdeclarative = True
        self.options["qt/*"].qtquickcontrols2 = True
        self.options["qt/*"].qtshadertools = True
        # TODO: Если применять не будем, лучше отключить использование флагом в Qt.
        self.options["libpq/*"].shared = True

    def requirements(self):
        self.requires("boost/1.83.0")
        self.requires("nlohmann_json/3.11.3")
        self.requires("sqlite3/3.51.3")
        self.requires("openssl/3.6.2")
        self.requires("cpprestsdk/2.10.19")
        self.requires("zlib/1.3.1")
        self.requires("websocketpp/0.8.2")
        self.requires("jwt-cpp/0.7.2")
        self.requires("qt/6.10.1")
        self.requires("libpq/17.7")

    def layout(self):
        cmake_layout(self)
