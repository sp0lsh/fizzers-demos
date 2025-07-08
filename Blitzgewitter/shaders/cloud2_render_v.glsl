#version 330

uniform mat4 modelview;
uniform vec4 frustum;
uniform float znear;

in vec4 vertex;

noperspective out vec2 v2f_coord;
noperspective out vec3 v2f_rd;

void main()
{
   v2f_coord = vertex.xy * 0.5 + vec2(0.5);

   vec2 t = vec2(mix(frustum.x, frustum.y, v2f_coord.x), mix(frustum.z, frustum.w, v2f_coord.y));
   mat3 m = transpose(mat3(modelview));
   
    v2f_rd = m * vec3(t, -znear);
    gl_Position = vertex;
}
