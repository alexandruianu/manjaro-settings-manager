include_directories(${CMAKE_CURRENT_BINARY_DIR})

file(GLOB MSM_NOTIFIER_GLOB
     msm_notifier.desktop
     resources/language.png
     resources/tux-manjaro.png)

set(MSM_NOTIFIER_SRCS
    ${MSM_NOTIFIER_GLOB}
    main.cpp
    Notifier.cpp
    NotifierApp.cpp
    global.cpp)

set(MSM_NOTIFIER_RESOURCES
    notifier.qrc)
QT5_ADD_RESOURCES (MSM_NOTIFIER_SRCS ${MSM_NOTIFIER_RESOURCES})

add_executable(msm_notifier ${MSM_NOTIFIER_SRCS})
target_compile_definitions(msm_notifier PRIVATE -DPROJECT_VERSION="${PROJECT_VERSION}")
target_link_libraries(msm_notifier
    msm
    Qt5::Widgets)

install(TARGETS msm_notifier DESTINATION ${CMAKE_INSTALL_BINDIR})
