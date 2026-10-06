# The tech demo VPK (VAOE00002), built from the top-level tree's targets.
add_executable(aoe_vita_demo ${CMAKE_SOURCE_DIR}/src/platform/vita_main.c)
target_link_libraries(aoe_vita_demo PRIVATE aoe_demo_app
  SceDisplay_stub SceCtrl_stub ScePower_stub)
target_compile_options(aoe_vita_demo PRIVATE -Wall -Wextra)
target_link_options(aoe_vita_demo PRIVATE
  "-Wl,-T,${CMAKE_SOURCE_DIR}/cmake/vita_sce_gap.ld")
vita_create_self(demo_eboot.bin aoe_vita_demo)
vita_create_vpk(VitaAoEDemo.vpk VAOE00002 demo_eboot.bin
  VERSION "00.10"
  NAME "AoE RoR Tech Demo"
  FILE ${CMAKE_SOURCE_DIR}/assets/vita/sce_sys/icon0.png sce_sys/icon0.png
  FILE ${CMAKE_SOURCE_DIR}/assets/vita/sce_sys/livearea/contents/bg.png
       sce_sys/livearea/contents/bg.png
  FILE ${CMAKE_SOURCE_DIR}/assets/vita/sce_sys/livearea/contents/startup.png
       sce_sys/livearea/contents/startup.png
  FILE ${CMAKE_SOURCE_DIR}/assets/vita/sce_sys/livearea/contents/template.xml
       sce_sys/livearea/contents/template.xml)
