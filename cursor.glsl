#version 450 core

layout(std430, binding = 0) buffer pos
{
  vec4 positions[];
};

layout(std430, binding = 1) buffer vel
{
  vec4 velocities[];
};

layout(std430, binding = 2) buffer c
{
  vec2 cursor;
};

layout(std430, binding = 3) buffer time
{
  float dt;
};

layout(local_size_x = 1000, local_size_y = 1, local_size_z = 1) in;

const float G = 0.0001;

void main() {
  // Get easier references to positions/velocities
  vec4 position = positions[gl_GlobalInvocationID.x];
  vec4 velocity = velocities[gl_GlobalInvocationID.x];

  //Build our 2D acceleration using universal gravitation
  vec2 r;
  vec2 accel2D;
  float magnitude;

  r.x = cursor.x - position.x;
  r.y = cursor.y - position.y;
  magnitude = length(r);
  r = normalize(r);
  accel2D = r * (G) / (magnitude * magnitude);

  //Update our velocity and position
  velocity.x = velocity.x + (accel2D.x * dt);
  velocity.y = velocity.y + (accel2D.y * dt);

  position = position + (velocity * dt);

  //Save our position and velocities for displaying
  positions[gl_GlobalInvocationID.x] = position;
  velocities[gl_GlobalInvocationID.x] = velocity;
}
