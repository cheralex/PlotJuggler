
include_directories(  ../ )

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

QT5_WRAP_UI ( UI_SRC  dataload_csv.ui  )

SET( SRC
    dataload_apbin.cpp
    )

add_library(DataLoadAPBin SHARED ${SRC} ${UI_SRC}  )
target_link_libraries(DataLoadAPBin
     ${Qt5Core_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Xml_LIBRARIES}
    ${Qt5Svg_LIBRARIES} 
    plotjuggler_base)



install(TARGETS DataLoadAPBin DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )

