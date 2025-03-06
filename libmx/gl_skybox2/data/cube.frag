#version 300 es
precision highp float;
out vec4 FragColor;
in vec3 localPos; 
uniform samplerCube cubemapTexture;
uniform float time_f;

void main(void) {
    vec3 originalDir = normalize(localPos);
    
    // Ripple effect parameters
    float speed = 5.0;
    float amplitude = 0.03;
    float wavelength = 10.0;
    
    // Calculate ripple displacement
    float ripple = sin(originalDir.x * wavelength + time_f * speed) * amplitude;
    ripple += sin(originalDir.y * wavelength + time_f * speed) * amplitude;
    vec3 rippleDir = originalDir + vec3(ripple, ripple, 0.0);
    rippleDir = normalize(rippleDir);
    
    // Apply spiral distortion to the rippled direction
    float r = length(rippleDir.xy);
    float theta = atan(rippleDir.y, rippleDir.x);
    float spiralEffect = time_f * 0.2;
    r -= mod(spiralEffect, 4.0);
    theta += spiralEffect;
    vec2 distortedXY = vec2(cos(theta), sin(theta)) * r;
    vec3 spiralDir = normalize(vec3(distortedXY, rippleDir.z));
    
    // Sample original and distorted cubemap colors
    vec4 originalColor = texture(cubemapTexture, originalDir);
    vec4 distortedColor = texture(cubemapTexture, spiralDir);
    
    // Mix original and distorted colors
    vec4 mixedColor = distortedColor; //mix(originalColor, distortedColor, 0.5);
    
    // Calculate sparkle effect using original direction
    float sparkle = abs(sin(time_f * 10.0 + originalDir.x * 100.0) * cos(time_f * 15.0 + originalDir.y * 100.0));
    vec3 magicalColor = vec3(
        sin(time_f * 2.0) * 0.5 + 0.5,
        cos(time_f * 3.0) * 0.5 + 0.5,
        sin(time_f * 4.0) * 0.5 + 0.5
    );
    mixedColor.rgb += magicalColor * sparkle;
    
    FragColor = mixedColor;
}

/*#version 300 es
precision highp float;
out vec4 FragColor;
in vec3 localPos; 
uniform samplerCube cubemapTexture;
uniform float time_f;
uniform vec2 iResolution;

vec4 getCubemapColor(vec3 apos, samplerCube cube) {
    float r = length(apos.xy);
    float theta = atan(apos.y, apos.x);
    float spiralEffect = time_f * 0.2;
    r -= mod(spiralEffect, 4.0);
    theta += spiralEffect;
    vec2 distortedXY = vec2(cos(theta), sin(theta)) * r;
    vec3 newApos = normalize(vec3(distortedXY, apos.z));
    vec4 texColor = texture(cube, newApos);
    return vec4(texColor.rgb, 1.0);
}

void main(void) {
    vec3 tc = normalize(localPos);
    vec4 texColor = getCubemapColor(localPos, cubemapTexture);
    float sparkle = abs(sin(time_f * 10.0 + tc.x * 100.0) * cos(time_f * 15.0 + tc.y * 100.0));
    vec3 magicalColor = vec3(sin(time_f * 2.0) * 0.5 + 0.5, cos(time_f * 3.0) * 0.5 + 0.5, sin(time_f * 4.0) * 0.5 + 0.5);
    FragColor = texColor + vec4(magicalColor * sparkle, 1.0);
}
*/