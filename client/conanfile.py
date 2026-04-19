from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMakeToolchain, CMakeDeps

class MyQmlProject(ConanFile):
    name = "kaban"
    version = "0.1.0"

    settings = "os", "compiler", "build_type", "arch"

    generators = "CMakeToolchain", "CMakeDeps"

    def requirements(self):
        self.requires("qt/6.10.1")

    def configure(self):
        self.options["qt/*"].shared = True
        self.options["qt/*"].qttools = True
        # Обязательные опции для QML и Quick
        self.options["qt/*"].qtdeclarative = True
        self.options["qt/*"].qtquickcontrols2 = True
        self.options["qt/*"].qtshadertools = True

    def layout(self):
        cmake_layout(self)