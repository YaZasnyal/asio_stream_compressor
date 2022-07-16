# Header only libraries don't build anything, skip debug build
set(VCPKG_BUILD_TYPE release)

# Please see
# https://vcpkg.readthedocs.io/en/latest/maintainers/vcpkg_from_github/ for
# details on how to fill out the arguments
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO <repo>
    REF <ref>
    SHA512 0
    HEAD_REF master
)

# Set this variable to the name this project installs itself as, i.e. the name
# that you can use in a find_package(<name> REQUIRED) command call
set(name <name>)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
    # vcpkg wants CMake package config files in share, so if the project allows
    # changing the path, then we can do that here
    #
    # This option is based on the one provided by cmake-init, for other
    # projects not following best practices, please refer to their
    # documentation or their CMake scripts, or in a worst-case scenario, you
    # have to patch around the project's deficiencies
    "-D${name}_INSTALL_CMAKEDIR=share/${name}"
)

vcpkg_cmake_install()

# vcpkg requires a license file to be installed as well
configure_file(
    "${SOURCE_PATH}/LICENSE"
    "${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright"
    COPYONLY
)
