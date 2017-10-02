#version 450

uniform mat4 model;
uniform mat4 projection;

layout(std430, binding = 0) buffer positions
{
  vec4 data[];
};

layout(location = 0) in vec3 position;

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_PointSize;
  float gl_ClipDistance[];
};

void main() {
	gl_Position = vec4(data[gl_InstanceID].xyz, 1.0);
}
