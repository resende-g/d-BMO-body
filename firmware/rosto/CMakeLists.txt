# As fontes ficam em engine/ (e não em src/) porque esta pasta também é um sketch do
# Arduino IDE (rosto.ino), e o Arduino compila tudo o que estiver em src/.
add_library(dbmo_rosto STATIC
  engine/expressions.cpp
  engine/face_engine.cpp
  engine/face_renderer.cpp
  engine/face_subsystem.cpp)
target_include_directories(dbmo_rosto
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})  # rosto_faces.h, a tabela de desenhos
target_link_libraries(dbmo_rosto PUBLIC dbmo_core dbmo_display PRIVATE dbmo_options)

# Pré-visualização no computador: imprime os quadros e gera uma folha PPM.
add_executable(dbmo_face_preview tools/face_preview.cpp)
target_link_libraries(dbmo_face_preview PRIVATE dbmo_rosto dbmo_options)
