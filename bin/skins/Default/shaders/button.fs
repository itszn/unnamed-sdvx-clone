#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(location=1) in vec2 fsTex;
layout(location=0) out vec4 target;

uniform sampler2D mainTex;
uniform bool hasSample;

varying vec4 position;

uniform float trackPos;
uniform float trackScale;
uniform float hiddenCutoff;
uniform float hiddenFadeWindow;
uniform float suddenCutoff;
uniform float suddenFadeWindow;

void main()
{	
	vec4 mainColor = texture(mainTex, fsTex.xy);
    if(hasSample)
    {
        float addition = abs(0.5 - fsTex.x) * - 1.;
        addition += 0.2;
        addition = max(addition,0.);
        addition *= 2.8;
        mainColor.xyzw += addition;
    }

    target = mainColor;
    
    float off = trackPos + position.y * trackScale;
    
    float hiddenCutoffFade = hiddenCutoff - hiddenFadeWindow;
    if (off < hiddenCutoff) {
        target = target * max(0.0f, (off - hiddenCutoffFade) / hiddenFadeWindow);
    }

    float suddenCutoffFade = suddenCutoff + suddenFadeWindow;
    if (off > suddenCutoff) {
        target = target * max(0.0f, (suddenCutoffFade - off) / suddenFadeWindow);
    }
}