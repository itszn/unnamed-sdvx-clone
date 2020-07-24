#extension GL_ARB_separate_shader_objects : enable
layout(location=0) in vec2 inPos;
layout(location=1) in vec2 inTex;


uniform ivec2 viewport;
uniform float songtime;

out gl_PerVertex
{
    vec4 gl_Position;
};
layout(location=1) out vec2 fsTex;

#define pi 3.1415926535897932384626433832795

uniform mat4 proj;
uniform mat4 world;

void main()
{
    float aspectRatio = float(viewport.x) / viewport.y;

    float fieldOfViewInRadians = pi * 0.5f;
    float near = 1.0f;
    float far = 200.0f;

    float f = 1.0f / tan(fieldOfViewInRadians / 2);
    float rangeInv = 1.0f / (near - far);
 
    mat4 prespective = mat4(
        f / aspectRatio,    0,  0,                          0,
        0,                  f,  0,                          0,
        0,                  0,  (near + far) * rangeInv,    -1,
        0,                  0,  near * far * rangeInv * 2,  0
    );

    //vec3 pos = vec3(0,-9 + 10*(1-cos(inPos.x*pi/2)),-10);
    vec3 pos = vec3(0,-9 + 15*pow(inPos.x,2),-10);
    mat4 translate = mat4(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        pos.x, pos.y, pos.z, 1
    );

    //vec3 scl = vec3(5,100,1);
    vec3 scl = vec3(15,100,1);
    mat4 scale = mat4(
        scl.x, 0, 0, 0,
        0, scl.y, 0, 0,
        0, 0, scl.z, 0,
        0, 0, 0, 1
    );

    float rot = pi * 0.28;
    mat4 rotx = mat4(
        1, 0, 0, 0,
        0, cos(rot), -sin(rot), 0,
        0,  sin(rot),   cos(rot),     0,
        0,       0,        0,     1
    );


    fsTex = inTex;

    /*
      return [
       1,       0,        0,     0,
       0,  cos(a),  -sin(a),     0,
       0,  sin(a),   cos(a),     0,
       0,       0,        0,     1
    ];*/

    vec2 out_pos = inPos.xy;
    mat4 model = translate * rotx * scale;
    gl_Position = prespective * model * vec4(out_pos.xy, 0, 1);
}