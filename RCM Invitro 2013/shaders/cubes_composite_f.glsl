#version 330

uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D game_tex, game_blur_tex;
uniform sampler2D bloom_tex;
uniform vec4 tint;
uniform float darken;
uniform int frame_num;
uniform float aspect_ratio;
uniform vec2 pong_ball_pos;
uniform float time;

noperspective in vec2 v2f_coord;

out vec4 output_colour;
 

 float fl;
 
vec3 mono(vec3 c)
{
    return vec3(clamp(dot(c, vec3(1.0 / 3.0)), 0.0, 1.0));
}

vec3 screen(vec2 p)
{
    // distort
    vec2 p2 = floor(p * 256.0) / 256.0;
    p2.x += pow(0.5 + 0.5 * cos(p2.y * 10.0 + sin(time * 4.0 + floor(time * 0.2) * 10.0) * cos(floor(time * 1.1)) * 3.0 ), 4096.0) * -0.1 * sin(floor(time)) * step(0.2, fract(time * 0.2));
    p2 = floor(p2 * 256.0) / 256.0;

    p2.x += 0.02 * step(0.4, p.y) * step(p.y, 0.7) * step(fract(time * 0.1), 0.03);
    
    return (texture2D(game_tex, p2).rgb + vec3(fl * step(0.95, texture2D(tex1, p2 + vec2(floor(time * 60.0) * 0.1, floor(time) * 0.2) ).g))) * step(0.0, p.x) * step(0.0, p.y) * step(p.x, 1.0) * step(p.y, 1.0);
}

vec3 screen2(vec2 p)
{
    vec3 c = vec3(0.0);
    float w_sum = 0.0;
    
    for(int i = 0; i < 16; ++i)
    {
        float w = pow(1.0 - float(i) / 16.0, 4.0);
        vec2 s = vec2(0.0, float(i) * 0.0002) + p - 1.0 * pow(0.9 + 0.5 * cos(time * 0.5) * sin(time * 0.7), 2.0) * vec2(float(i) * 0.001, float(i) * 0.0001);
        c += screen(mix(s, p + vec2(0.0, float(i) * 0.0002 * (1.0 + fl * 10.0)), fl)) * w;
        w_sum += w;
    }
    
    return c / w_sum;
}

void main()
{
    fl = step(fract(time * 0.1), 0.1);

    output_colour = vec4(0.0);
  
    output_colour = texture2D(bloom_tex, v2f_coord);

    vec3 col2 = texture2D(tex0, v2f_coord).rgb * tint.rgb;

    col2.rgb += col2.rgb * texelFetch(tex1, ivec2(mod(gl_FragCoord.xy + ivec2(frame_num * ivec2(23, 7)), textureSize(tex1, 0))), 0).rrr * 0.3;

    output_colour.rgb += pow(max(vec3(0.0), col2), vec3(0.8));

    vec2 game_coord = (v2f_coord - vec2(0.5)) * vec2(aspect_ratio, 1.0) * 1.0 * 2.0;
    
    game_coord *= mix(0.0, 1.0, smoothstep(0.0, 9.0, time));
    game_coord += pong_ball_pos * (1.0 - smoothstep(1.5, 7.0, time));
    
    vec2 game_texcoord = (game_coord + vec2(1.0)) * 0.5;
    
    float gridx = 1.0 - smoothstep(0.0, 0.1, abs(0.5 - fract(game_texcoord.x * 256.0 / 8.0)));
    float gridy = 1.0 - smoothstep(0.0, 0.1, abs(0.5 - fract(game_texcoord.y * 256.0 / 8.0)));

    vec3 gamecol = output_colour.rgb;
    
    gamecol *= mix(1.0, 0.5 - max(gridx, gridy) * 0.2, smoothstep(1.0, 9.0, time) * smoothstep(-0.01, 0.0, (1.01) - abs(game_coord.y)) *
                    smoothstep(-0.01, 0.0, 1.09 - abs(game_coord.x)));
    

    gamecol = mix(gamecol.rgb, vec3(0.0), mono(texture2D(game_blur_tex, game_texcoord).rgb * 0.8)) + screen2(game_texcoord) * 0.7;
    
    gamecol += (1.0 - smoothstep(0.0, 0.4, distance(game_coord, pong_ball_pos))) * (1.0 - smoothstep(2.0, 3.5, time));
    
    output_colour.rgb = mix(gamecol, output_colour.rgb, smoothstep(18.0+13.0, 21.0+13.0, time)) + vec3(smoothstep(35.0, 36.0, time));
}

