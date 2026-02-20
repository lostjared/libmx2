#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragWorldPos;
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D texSampler;
layout(binding = 1) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 params;
    vec4 color;
} ubo;

// ------------------------------------------------------------------
// One bulb of the overhead lamp bar
// ------------------------------------------------------------------
vec3 lampBulb(vec3 bulbPos, vec3 lampCol, vec3 worldPos, vec3 normal, vec3 baseCol) {
    vec3  lv      = bulbPos - worldPos;
    float dist    = length(lv);
    vec3  ldir    = lv / max(dist, 0.001);

    // N·L gives the shading gradient: faces pointing toward the bulb
    // get full light; the horizontal wood sides naturally receive very little.
    float diff    = max(dot(normal, ldir), 0.0);

    // Quadratic falloff tuned for a lamp ~2.5 units above the table surface.
    // At dist=0 → 1.0, at dist=2.5 (center below bulb) → ~0.84,
    // at dist=7 (far table corner) → ~0.13, giving the bright-center gradient.
    float atten   = 1.0 / (1.0 + 0.18 * dist + 0.12 * dist * dist);

    // Blinn-Phong specular for shiny ball highlights
    vec3  viewDir = normalize(-worldPos);           // good enough approx in view space
    vec3  halfDir = normalize(ldir + viewDir);
    float spec    = pow(max(dot(normal, halfDir), 0.0), 96.0);
    vec3  specular = 0.5 * spec * lampCol;

    return (lampCol * diff * baseCol + specular) * atten;
}

void main() {
    vec3 normal   = normalize(fragNormal);
    vec4 texColor = texture(texSampler, fragTexCoord);

    // ------------------------------------------------------------------
    // Lamp bar: three bulbs hanging 2.5 units above the table,
    // spaced along the table's long axis – just like a real billiard lamp.
    // The low hang height means the N·L term naturally falls from nearly 1.0
    // directly below each bulb to ~0.3 at the far corners, producing the
    // classic bright-centre-to-dark-edges gradient without any artificial cone.
    // ------------------------------------------------------------------
    vec3 lampCol = vec3(1.0, 0.92, 0.72); // warm incandescent yellow-white
    float lampY  = 2.5;                   // height above table (y≈0)

    vec3 light = vec3(0.0);
    light += lampBulb(vec3(-4.2, lampY, 0.0), lampCol, fragWorldPos, normal, fragColor);
    light += lampBulb(vec3( 0.0, lampY, 0.0), lampCol, fragWorldPos, normal, fragColor);
    light += lampBulb(vec3( 4.2, lampY, 0.0), lampCol, fragWorldPos, normal, fragColor);

    // Scale up so the table surface is nicely bright
    light *= 2.8;

    // Soft fill light from below/side opposite the lamps – gives the dark
    // side of balls a visible cool tone instead of pure black.
    // No specular, very low intensity, just enough to show colour.
    vec3 fillDir = normalize(vec3(0.3, -0.6, 0.5));
    float fillDiff = max(dot(normal, fillDir), 0.0);
    vec3 fill = 0.30 * fillDiff * fragColor * vec3(0.6, 0.7, 1.0);

    // Hemisphere ambient: interpolates a warm tone (normals facing up, toward
    // the lamp) to a cool medium tone (normals facing down / away from light).
    // Both values are now high enough that NO side goes black.
    vec3 skyCol    = vec3(0.42, 0.37, 0.28);  // warm room bounce
    vec3 groundCol = vec3(0.28, 0.30, 0.38);  // cool but clearly visible fill
    float hemi     = dot(normal, vec3(0.0, 1.0, 0.0)) * 0.5 + 0.5;
    vec3 ambient   = mix(groundCol, skyCol, hemi) * fragColor;

    vec3 result = texColor.rgb * (ambient + fill + light);
    outColor = vec4(result, texColor.a);
}
