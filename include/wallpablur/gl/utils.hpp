#ifndef GL_UTILS_HPP_INCLUDED
#define GL_UTILS_HPP_INCLUDED

#include <span>

#include <gl/mesh.hpp>



namespace gl {

[[nodiscard]] gl::mesh mesh_from_vertices_indices(std::span<const GLfloat>,
    std::span<const GLushort>);



[[nodiscard]] gl::mesh create_quad();
[[nodiscard]] gl::mesh create_sector(size_t);
[[nodiscard]] gl::mesh create_sector_outside(size_t);

}

#endif // GL_UTILS_HPP_INCLUDED
