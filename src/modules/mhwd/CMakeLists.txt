include_directories(${CMAKE_CURRENT_BINARY_DIR})

file(GLOB MSM_MHWD_GLOB
     msm_mhwd.desktop)

set(MSM_MHWD_SRCS
    ${MSM_MHWD_GLOB}
    MhwdModule.cpp
    libmhwd/mhwd.cpp
    libmhwd/vita/string.cpp)

set(MSM_MHWD_UI
    ui/PageMhwd.ui)
qt5_wrap_ui(MSM_MHWD_SRCS ${MSM_MHWD_UI})

set(MSM_MHWD_RESOURCES
    mhwd.qrc)
QT5_ADD_RESOURCES (MSM_MHWD_SRCS ${MSM_MHWD_RESOURCES})

add_library(msm_mhwd MODULE ${MSM_MHWD_SRCS})
target_compile_definitions(msm_mhwd PRIVATE -DPROJECT_VERSION="${PROJECT_VERSION}")
target_link_libraries(msm_mhwd
    hd
    msm
    Qt5::Widgets
    KF5::ConfigWidgets
    KF5::CoreAddons)

install(TARGETS msm_mhwd DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES msm_mhwd.desktop DESTINATION ${SERVICES_INSTALL_DIR})

kauth_install_actions(org.manjaro.msm.mhwd msm_mhwd.actions)

add_executable(msm_mhwd_authhelper MhwdAuthHelper.cpp)
target_link_libraries(msm_mhwd_authhelper KF5::Auth KF5::ConfigCore)
install(TARGETS msm_mhwd_authhelper DESTINATION ${KAUTH_HELPER_INSTALL_DIR})
kauth_install_helper_files(msm_mhwd_authhelper org.manjaro.msm.mhwd root)
