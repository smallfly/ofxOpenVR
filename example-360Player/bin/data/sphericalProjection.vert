#version 150

uniform mat4 modelViewProjectionMatrix;
uniform mat4 normalMatrix;

in vec4 position;
in vec4 normal;

out vec4 modelNormal;

void main() {
	modelNormal = normal;
	gl_Position = modelViewProjectionMatrix * position;

}