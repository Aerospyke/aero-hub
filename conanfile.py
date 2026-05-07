from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class AerodynamicsFollower(ConanFile):
    name = "maintelliq-ui-mockups"
    version = "0.0.1"
    package_type = "application"
    author = "Linova"
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "CMakeLists.txt", "src/*", "include/*"

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        pass
        # self.requires("osa-cbm-codec/1.2.1")
        # self.requires("arrow/22.0.0")

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
