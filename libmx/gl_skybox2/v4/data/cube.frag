#version 330 core
out vec4 FragColor;
in vec3 localPos;
uniform samplerCube cubemapTexture;
uniform float time_f;
uniform vec2 iResolution;
uniform vec4 iMouse;

vec4 getCubemapColor(vec3 apos, vec2 displacement) {
    // Apply spiral distortion
    float spiralSpeed = 0.2;
    float r = length(apos.xy + displacement);
    float theta = atan(apos.y + displacement.y, apos.x + displacement.x);
    theta += time_f * spiralSpeed;
    r -= mod(time_f * spiralSpeed, 4.0);
    
    vec2 spiralXY = vec2(cos(theta), sin(theta)) * r;
    vec3 finalDir = normalize(vec3(spiralXY, apos.z));
    return texture(cubemapTexture, finalDir);
}

void main(void) {
    vec3 pos = normalize(localPos);
    vec2 normPos = pos.xy;
    
    // Mouse interaction calculations
    vec2 dragVec = vec2(0.0);
    float dragSpeed = 0.0;
    float verticalFactor = 0.0;
    vec2 dragDir = vec2(0.0);
    
    if(iMouse.z > 0.0) {
        vec2 clickPos = (iMouse.zw / iResolution.xy) * 2.0 - 1.0;
        vec2 currentPos = (iMouse.xy / iResolution.xy) * 2.0 - 1.0;
        dragVec = currentPos - clickPos;
        dragSpeed = length(dragVec);
        dragDir = normalize(dragVec);
        verticalFactor = dragVec.y;
    }

    // Wave distortion parameters
    float dist = length(normPos);
    float speedFactor = 1.0 + verticalFactor * 2.0;
    float waveSize = 0.105 + dragSpeed * 0.2;
    float phase = sin(dist * 6.0 - time_f * speedFactor);
    
    // Calculate displacement
    vec2 displacement = mix(
        normPos * waveSize * phase,
        dragDir * waveSize * phase * (1.0 + dragSpeed * 0.1),
        smoothstep(0.0, 0.5, dragSpeed)
    );

    // Get final color with combined effects
    vec4 texColor = getCubemapColor(pos, displacement);
    
    // Add sparkle effect
    float sparkle = abs(sin(time_f * 10.0 + pos.x * 100.0) * 
                   cos(time_f * 15.0 + pos.y * 100.0));
    vec3 magicalColor = vec3(
        sin(time_f * 2.0) * 0.5 + 0.5,
        cos(time_f * 3.0) * 0.5 + 0.5,
        sin(time_f * 4.0) * 0.5 + 0.5
    );
    
    FragColor = texColor + vec4(magicalColor * sparkle, 1.0);
}
/*
#version 330 core
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
/*
#version 330 core
out vec4 FragColor;
in vec3 localPos;
uniform samplerCube cubemapTexture;
uniform float time_f;
uniform vec2 iResolution;

float pingPong(float x, float length) {
    float modVal = mod(x, length * 2.0);
    return modVal <= length ? modVal : length * 2.0 - modVal;
}

vec4 getCubemapColor(vec3 apos) {
    // Spiral effect parameters
    float spiralSpeed = 0.2;
    float spiralEffect = time_f * spiralSpeed;
    
    // Radial distortion parameters
    float distortionAmount = 0.5;
    
    // Calculate initial polar coordinates from XY
    float r = length(apos.xy);
    float theta = atan(apos.y, apos.x);
    
    // Apply spiral effect
    r -= mod(spiralEffect, 4.0);
    theta += spiralEffect;
    
    // Apply radial distortion
    float normalizedRadius = r / 1.0; // Max radius is 1 since normalized
    float distortedRadius = normalizedRadius + distortionAmount * pow(normalizedRadius, 2.0);
    distortedRadius = clamp(distortedRadius, 0.0, 1.0);
    
    // Generate new distorted XY coordinates
    vec2 distortedXY = vec2(cos(theta), sin(theta)) * distortedRadius;
    
    // Apply time-based rotation
    float rotationTime = pingPong(time_f, 5.0);
    float rotAngle = rotationTime;
    vec2 rotatedXY;
    rotatedXY.x = cos(rotAngle) * distortedXY.x - sin(rotAngle) * distortedXY.y;
    rotatedXY.y = sin(rotAngle) * distortedXY.x + cos(rotAngle) * distortedXY.y;
    
    // Apply ping-pong coordinate warping
    float warpSpeed = 0.1;
    vec2 warpedCoords;
    warpedCoords.x = pingPong(rotatedXY.x + time_f * warpSpeed, 1.0);
    warpedCoords.y = pingPong(rotatedXY.y + time_f * warpSpeed, 1.0);
    
    // Create final direction vector
    vec3 finalDir = normalize(vec3(warpedCoords, apos.z));
    
    return texture(cubemapTexture, finalDir);
}

void main(void) {
    vec3 normalizedPos = normalize(localPos);
    vec4 texColor = getCubemapColor(normalizedPos);
    
    // Sparkle effect
    float sparkleIntensity = abs(sin(time_f * 10.0 + normalizedPos.x * 100.0) * 
                            cos(time_f * 15.0 + normalizedPos.y * 100.0));
    vec3 magicalColor = vec3(
        sin(time_f * 2.0) * 0.5 + 0.5,
        cos(time_f * 3.0) * 0.5 + 0.5,
        sin(time_f * 4.0) * 0.5 + 0.5
    );
    
    FragColor = texColor + vec4(magicalColor * sparkleIntensity, 1.0);
}*/

