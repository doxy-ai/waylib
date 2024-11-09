add_library(stylizer_pbr pbr.cpp)
target_link_libraries(stylizer_pbr PUBLIC stylizer::core)
add_library(stylizer::pbr ALIAS stylizer_pbr)

stylizer_embed(stylizer_pbr shaders/pbr_gbuffer_data.wgsl)
stylizer_embed(stylizer_pbr shaders/pbr_gbuffer.wgsl)
stylizer_embed(stylizer_pbr shaders/pbr_material_data.wgsl)
stylizer_embed(stylizer_pbr shaders/pbr_material.wgsl)
