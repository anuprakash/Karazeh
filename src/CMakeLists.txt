INCLUDE(cmake/macros/ConfigureRSync)
INCLUDE(GenerateExportHeader)

INCLUDE_DIRECTORIES(../include)
INCLUDE_DIRECTORIES(../deps)
INCLUDE_DIRECTORIES(../deps/md5)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/exports)

IF(WIN32)
  ADD_DEFINITIONS("/wd4251")
ENDIF()

IF(APPLE)
  SET(CMAKE_CXX_FLAGS "-std=c++11 -Wc++11-extensions")

  FIND_LIBRARY(COREFOUNDATION_LIBRARY NAMES CoreFoundation)
  MARK_AS_ADVANCED (COREFOUNDATION_LIBRARY)
ENDIF()

SET(Karazeh_SRCS
  ../include/karazeh/hashers/md5_hasher.hpp
  ../include/karazeh/operations/create.hpp
  ../include/karazeh/operations/update.hpp
  ../include/karazeh/operations/delete.hpp
  ../include/karazeh/config.hpp
  ../include/karazeh/delta_encoder.hpp
  ../include/karazeh/downloader.hpp
  ../include/karazeh/exception.hpp
  ../include/karazeh/file_manager.hpp
  ../include/karazeh/hasher.hpp
  ../include/karazeh/karazeh.hpp
  ../include/karazeh/logger.hpp
  ../include/karazeh/operation.hpp
  ../include/karazeh/patcher.hpp
  ../include/karazeh/path_resolver.hpp
  ../include/karazeh/release_manifest.hpp
  ../include/karazeh/version_manifest.hpp

  ../deps/json11/json11.hpp
  ../deps/json11/json11.cpp
  ../deps/md5/md5.hpp
  ../deps/md5/md5.cpp
  ../deps/binreloc/binreloc.h
  ../deps/binreloc/binreloc.c

  hashers/md5_hasher.cpp
  operations/create.cpp
  operations/delete.cpp
  operations/update.cpp

  delta_encoder.cpp
  downloader.cpp
  file_manager.cpp
  logger.cpp
  operation.cpp
  patcher.cpp
  path_resolver.cpp
  version_manifest.cpp
)

# generate library
ADD_LIBRARY(kzh SHARED ${Karazeh_SRCS})

GENERATE_EXPORT_HEADER(kzh
  BASE_NAME kzh
  EXPORT_MACRO_NAME KARAZEH_EXPORT
  EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/exports/karazeh_export.h
  STATIC_DEFINE KARAZEH_BUILT_AS_STATIC
)

TARGET_LINK_LIBRARIES(kzh ${Karazeh_LIBRARIES} rsync)

IF(APPLE)
  TARGET_LINK_LIBRARIES(kzh ${COREFOUNDATION_LIBRARY})
ENDIF()

