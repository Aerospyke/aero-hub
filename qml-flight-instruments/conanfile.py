from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class QmlFlightInstrumentsRecipe(ConanFile):
    name = "QML Flight Instruments"
    version = "0.0.1"
    package_type = "application"
    author = "Conor J. Haines"
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "CMakeLists.txt", "application/*"

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        pass

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
