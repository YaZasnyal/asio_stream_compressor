from conan import ConanFile


class Recipe(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps", "VirtualRunEnv"

    def layout(self):
        self.folders.generators = "conan"

    def requirements(self):
        self.requires("zstd/1.5.2")
        self.requires("boost/1.83.0")
        self.requires("asio/1.22.1")

        # Testing only dependencies below
        # self.requires("catch2/3.0.1")
