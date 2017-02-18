in vec3 viewPosition;
in vec3 viewNormal;

out vec4 c; // Output color

// Properties of directional light
uniform vec4 lightAmbient;
uniform vec4 lightDiffuse;
uniform vec4 lightSpecular;
uniform vec3 lightDirection;

// Properties of mesh material
uniform vec4 matKa;
uniform vec4 matKd;
uniform vec4 matKs;
uniform vec4 matKsExp;

void main() {
    // camera is at (0,0,0) after the modelview transformation
    vec3 eyedir = normalize(vec3(0, 0, 0) - viewPosition);

    // reflected light direction
    vec3 reflectDir = -reflect(viewLightDirection, viewNormal);

    // Phong lighting
    float kd = max(dot(viewLightDirection, viewNormal), 0.0f);
    float ks = max(dot(reflectDir, eyedir), 0.0f);

    // compute the final color
    c = matKa * lightAmbient + matKd * kd * lightDiffuse +
    matKs * pow(ks, matKsExp) * lightSpecular;
}
