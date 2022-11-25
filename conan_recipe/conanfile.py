from conans import ConanFile, CMake, tools


class AsioStreamCompressorConan(ConanFile):
    name = "asio_stream_compressor"
    version = "0.1.2"
    license = "MIT"
    author = "Artem Vasiliev github@yazasnyal.dev"
    url = "https://github.com/YaZasnyal/asio_stream_compressor"
    description = "ZSTD compression support for ASIO streams"
    topics = ("compression", "asio", "header-only", "networking")
    settings = "compiler"
    options = {
        "flavor": ["boost", "asio"],
    }
    default_options = {
        "flavor": "boost",
    } 
    generators = "cmake"
    exports_sources = "../*"
    no_copy_source = True

    @property
    def _source_subfolder(self):
        return "source_subfolder"
        
    @property
    def _min_cppstd(self):
        return 14

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            tools.check_min_cppstd(self, self._min_cppstd)

    def requirements(self):
        self.requires("zstd/1.5.2")
        if self.options.flavor == "boost":
            self.requires("boost/1.79.0")
        else:
            self.requires("asio/1.22.1")
        
    def package_id(self):
        self.info.header_only()

    def package(self):
        self.copy("LICENSE", dst="licenses", src=self._source_subfolder)
        self.copy("*", dst="include", src="include")

    def package_info(self):
        self.cpp_info.names["cmake_find_package"] = "asio_stream_compressor"
        self.cpp_info.names["cmake_find_package_multi"] = "asio_stream_compressor"
        if self.options.flavor == "asio":
            self.cpp_info.defines.append("ASIO_STEREAM_COMPRESSOR_FLAVOUR_STANDALONE")
