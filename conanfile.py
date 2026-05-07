from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class AeroHubRecipe(ConanFile):
    name = "Aero Hub"
    version = "0.0.1"
    package_type = "application"
    author = "Conor J. Haines"
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "CMakeLists.txt", "AeroHubApplication/*", "CyberTheme/*"

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
