#version 410 core


// Per-vertex inputs
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
//layout (location = 2) in vec3 colour;

uniform mat4 view_matrix;
uniform mat4 proj_matrix;

// Inputs from vertex shader
out VS_OUT
{
    vec3 N;
    vec3 L;
    vec3 V;
    vec3 vertex_colour;
} vs_out;

// Position of light
vec3 light_pos = vec3(100.0, 100.0, 100.0);

void main(void)
{
	vs_out.vertex_colour = vec3(0.0, 0.5, 1.0);//colour;

    // Calculate view-space coordinate
    vec4 P = view_matrix * vec4(position, 1.0);

    // Calculate normal in view-space
    vs_out.N = mat3(view_matrix) * normal;

    // Calculate light vector
    vs_out.L = light_pos - P.xyz;

    // Calculate view vector
    vs_out.V = -P.xyz;

    // Calculate the clip-space position of each vertex
    gl_Position = proj_matrix * P;
}
