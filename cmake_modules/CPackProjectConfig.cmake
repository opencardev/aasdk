# CPack project configuration to harmonize packaging for AASDK
# If CPACK_DEBIAN_PACKAGE_RELEASE was not explicitly set, pick it up from environment

if(NOT DEFINED CPACK_DEBIAN_PACKAGE_RELEASE)
  if(DEFINED ENV{DISTRO_DEB_RELEASE} AND NOT "$ENV{DISTRO_DEB_RELEASE}" STREQUAL "")
    set(CPACK_DEBIAN_PACKAGE_RELEASE "$ENV{DISTRO_DEB_RELEASE}")
  endif()
endif()
