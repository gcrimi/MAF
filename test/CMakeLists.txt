#message (".......................... Entering ${CMAKE_CURRENT_LIST_FILE} ............................")

include(${MAF_USE_FILE_CONFIG})
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/mafTestConfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/mafTestConfig.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

ADD_SUBDIRECTORY(mafCoreTest)
ADD_SUBDIRECTORY(mafEventBusTest)
ADD_SUBDIRECTORY(mafQATest)
ADD_SUBDIRECTORY(mafModuleTest)

if(${MAF_TIME_SPACE})
  ADD_SUBDIRECTORY(mafTimeSpaceTest)
endif(${MAF_TIME_SPACE})

if(${MAF_SERIALIZATION})
  ADD_SUBDIRECTORY(mafSerializationTest)
endif(${MAF_SERIALIZATION})

if(${MAF_RESOURCES})
  ADD_SUBDIRECTORY(mafPluginTest)
  ADD_SUBDIRECTORY(mafPluginQtTest)
  ADD_SUBDIRECTORY(mafResourcesTest)
endif(${MAF_RESOURCES})

if(${MAF_PLUGINVTK})
  ADD_SUBDIRECTORY(mafPluginVTKTest)
endif(${MAF_PLUGINVTK})

if(${VTK_MAF})
  ADD_SUBDIRECTORY(vtkMAFTest)
endif(${VTK_MAF})

if(${MAF_APPLICATION_LOGIC})
  ADD_SUBDIRECTORY(mafApplicationLogicTest)
endif(${MAF_APPLICATION_LOGIC})

if(${MAF_QTGUI})
  ADD_SUBDIRECTORY(mafGUITest)
endif(${MAF_QTGUI})

if(${BUILD_WRAP} AND ${MAF_SCRIPT_INTERPRETER})
  ADD_SUBDIRECTORY(mafScriptInterpreterTest)
endif(${BUILD_WRAP} AND ${MAF_SCRIPT_INTERPRETER})

if(${MAF_PLUGINCTK})
  #ADD_SUBDIRECTORY(mafPluginCTKTest)
endif(${MAF_PLUGINCTK})

if(${MAF_PLUGIN_OUTOFCORE})
  ADD_SUBDIRECTORY(mafPluginOutOfCoreTest)
endif(${MAF_PLUGIN_OUTOFCORE})

if(${MAF_PLUGIN_ZIP})
  ADD_SUBDIRECTORY(mafPluginZipTest)
endif(${MAF_PLUGIN_ZIP})

#message (".......................... Exiting ${CMAKE_CURRENT_LIST_FILE} ............................")
