# - Find vorbis
# Find the native vorbis includes and libraries
#
#  Vorbis_INCLUDE_DIR - where to find vorbis.h, etc.
#  OGG_INCLUDE_DIR    - where to find ogg/ogg.h, etc.
#  Vorbis_LIBRARIES   - List of libraries when using vorbis(file).
#  Vorbis_FOUND       - True if vorbis found.

if(Vorbis_INCLUDE_DIR)
    # Already in cache, be silent
    set(Vorbis_FIND_QUIETLY TRUE)
endif(Vorbis_INCLUDE_DIR)
find_path(OGG_INCLUDE_DIR ogg/ogg.h)
find_path(Vorbis_INCLUDE_DIR vorbis/vorbisfile.h)
# MSVC built ogg/vorbis may be named ogg_static and vorbis_static
find_library(OGG_LIBRARY NAMES ogg ogg_static)
find_library(Vorbis_LIBRARY NAMES vorbis vorbis_static)
find_library(VorbisFILE_LIBRARY NAMES vorbisfile vorbisfile_static)
# Handle the QUIETLY and REQUIRED arguments and set Vorbis_FOUND
# to TRUE if all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vorbis DEFAULT_MSG
    OGG_INCLUDE_DIR Vorbis_INCLUDE_DIR
    OGG_LIBRARY Vorbis_LIBRARY VorbisFILE_LIBRARY)

if(Vorbis_FOUND)
  set(Vorbis_LIBRARIES ${VorbisFILE_LIBRARY} ${Vorbis_LIBRARY}
      ${OGG_LIBRARY})
else(Vorbis_FOUND)
  set(Vorbis_LIBRARIES)
endif(Vorbis_FOUND)

mark_as_advanced(OGG_INCLUDE_DIR Vorbis_INCLUDE_DIR)
mark_as_advanced(OGG_LIBRARY Vorbis_LIBRARY VorbisFILE_LIBRARY)

