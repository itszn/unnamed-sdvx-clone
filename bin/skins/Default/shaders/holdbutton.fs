#ifdef EMBEDDED
varying vec2 fsTex;
varying vec4 position;
#else
//#extension GL_ARB_separate_shader_objects : enable
layout(location=1) in vec2 fsTex;
layout(location=0) out vec4 target;
in vec4 position;
#endif

uniform sampler2D mainTex;
uniform float objectGlow;
uniform float trackPos;
uniform float trackScale;
uniform float hiddenCutoff;
uniform float hiddenFadeWindow;
uniform float suddenCutoff;
uniform float suddenFadeWindow;

// 20Hz flickering. 0 = Miss, 1 = Inactive, 2 & 3 = Active alternating.
uniform int hitState;

#ifdef EMBEDDED
void main()
{    
    vec4 mainColor = texture(mainTex, fsTex.xy);

    target = mainColor;
	target.xyz = target.xyz * (1.0 + objectGlow * 0.3);
    target.a = min(1.0, target.a + target.a * objectGlow * 0.9);
}
#else
void main()
{    
    vec4 mainColor = texture(mainTex, fsTex.xy);

    target = mainColor;

    float off = trackPos + position.y * trackScale;
    
    if(hiddenCutoff < suddenCutoff)
    {
        float hiddenCutoffFade = hiddenCutoff - hiddenFadeWindow;
        if (off < hiddenCutoff) {
            target = target * max(0.0, (off - hiddenCutoffFade) / hiddenFadeWindow);
        }

        float suddenCutoffFade = suddenCutoff + suddenFadeWindow;
        if (off > suddenCutoff) {
            target = target * max(0.0, (suddenCutoffFade - off) / suddenFadeWindow);
        }
    }
    else
    {
        float hiddenCutoffFade = hiddenCutoff + hiddenFadeWindow;
        if (off > hiddenCutoff) {
            target = target * clamp((off - hiddenCutoffFade) / hiddenFadeWindow, 0.0, 1.0);
        }

        float suddenCutoffFade = suddenCutoff - suddenFadeWindow;
        if (off < suddenCutoff) {
            target = target * clamp((suddenCutoffFade - off) / suddenFadeWindow, 0.0, 1.0);
        }

        if (off > suddenCutoff && off < hiddenCutoff) {
            target = target * 0.0;
        }
    }


    target.xyz = target.xyz * (1.0 + objectGlow * 0.3);
    target.a = min(1.0, target.a + target.a * objectGlow * 0.9);
}
#endif