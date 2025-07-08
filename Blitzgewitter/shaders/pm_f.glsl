// This shader simulates particle physics and collision detection.

#version 330

uniform sampler2D depth_tex;
uniform sampler2D particle_pos_tex, particle_vel_tex;

uniform mat4 view_mv, view_p;
uniform mat4 inv_view_mv, inv_view_p;

uniform float gravity_scale;

in vec2 tc;

out vec4 output0, output1;

// Returns the viewspace point for the pixel at the given screen coordinate.
vec3 vsPointForCoord(vec2 tc)
{
	float z_over_w = texture2D(depth_tex, tc).r;
	vec4 w_pos = vec4(tc.x * 2.0 - 1.0, tc.y * 2.0 - 1.0, z_over_w, 1.0);
	vec4 v_pos = inv_view_p * w_pos;
	return (v_pos / v_pos.w).xyz;
}

void main()
{
	// Read in the particle's state.
	vec3 pos = texture2D(particle_pos_tex, tc).xyz;
    vec3 v = texture2D(particle_vel_tex, tc).xyz;

    v += vec3(0.0, -1e-4 * gravity_scale, 0.0) * 3.0;

	pos += v / 8.0 * 3.0;

    vec4 mv_pos = view_mv * vec4(pos, 1.0);
    vec4 clip_pos = view_p * mv_pos;

    vec2 proj_tc0 = clip_pos.xy / clip_pos.w * 0.5 + vec2(0.5);

	vec3 g_mv_pos = vsPointForCoord(proj_tc0);
    
    if(proj_tc0.x > 0.0 && proj_tc0.x < 1.0 && proj_tc0.y > 0.0 && proj_tc0.y < 1.0 &&
			g_mv_pos.z < -1e-2 && g_mv_pos.z > mv_pos.z && g_mv_pos.z < (mv_pos.z + 4.0))
    {
		// Retrieve a surface normal and reflect the particle's velocity vector by this normal.
		vec2 eps = vec2(1.0) / textureSize(depth_tex, 0).xy;

		vec2 proj_tc1 = proj_tc0 + vec2(eps.x, 0.0);
		vec2 proj_tc2 = proj_tc0 + vec2(0.0, eps.y);

		vec3 p0 = vsPointForCoord(proj_tc0);
		vec3 p1 = vsPointForCoord(proj_tc1);
		vec3 p2 = vsPointForCoord(proj_tc2);
		
		vec3 n = mat3(inv_view_mv) * normalize(cross(p1.xyz - p0.xyz, p2.xyz - p0.xyz));
		vec3 r = reflect(v, n);
		
		pos += n * 2e-2;
		v = r * 0.1 + cos(pos * 1e5) * 1e-3;
    }

	// Write the updated state for this particle.
	output0.xyz = pos;
	output1.xyz = v;
}
