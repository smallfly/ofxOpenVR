#version 150

uniform sampler2D tex0;
in vec4	modelNormal;
out vec4 fragColor;

const float ONE_OVER_PI = 1.0 / 3.14159265;

void main() {
    vec3 normal = normalize(modelNormal.xyz);
	// spherical projection based on the surface normal
	vec2 coord = vec2( 0.5 + 0.5 * atan( normal.x, -normal.z ) * ONE_OVER_PI, acos( normal.y ) * ONE_OVER_PI );
	fragColor = texture( tex0, coord );
}