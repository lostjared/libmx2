#version 330 core
in vec2 TexCoord;
out vec4 color;
uniform sampler2D textTexture;
uniform float time_f;
uniform float alpha;

void main(void) {
    vec2 tc = TexCoord;
    float rippleSpeed = 5.0;
    float rippleAmplitude = 0.03;
    float rippleWavelength = 10.0;
    float twistStrength = 1.0;
    float radius = length(tc - vec2(0.5, 0.5));
    float ripple = sin(tc.x * rippleWavelength + time_f * rippleSpeed) * rippleAmplitude;
    ripple += sin(tc.y * rippleWavelength + time_f * rippleSpeed) * rippleAmplitude;
    vec2 rippleTC = tc + vec2(ripple, ripple);
    float angle = twistStrength * (radius - 1.0) + time_f;
    float cosA = cos(angle);
    float sinA = sin(angle);
    mat2 rotationMatrix = mat2(cosA, -sinA, sinA, cosA);
    vec2 twistedTC = (rotationMatrix * (tc - vec2(0.5, 0.5))) + vec2(0.5, 0.5);
    vec4 originalColor = texture(textTexture, tc);
    vec4 twistedRippleColor = texture(textTexture, mix(rippleTC, twistedTC, 0.5));
    color = twistedRippleColor * alpha;
}
