# need ${CTK_DIR} that is set by ExternalProject_Add for ctk
set(CTK_SUPERBUILD_DIR "${MAF_EXTERNAL_BUILD_DIR}/Build/CTK")
set(CTK_DIR "${CTK_SUPERBUILD_DIR}")
FIND_PACKAGE(CTK REQUIRED)

set(CTK_USE_FILE "${CTK_SUPERBUILD_DIR}/UseCTK.cmake")
include("${CTK_SUPERBUILD_DIR}/CTKConfig.cmake")

include(${CTK_USE_FILE})

