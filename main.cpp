#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ctime>
#include <cstdlib>

/**
* The number of particles we want to simulate. Since compute shaders are limited in their
* work group size, we'll also need to know the work group size so we can find out how many
* work groups to dispatch
*/
#define NUM_PARTICLES 100000

// This MUST match with local_size_x inside cursor.glsl
#define WORK_GROUP_SIZE 1000

#define SCREENX 1920
#define SCREENY 1080

glm::vec2 defaultCursor = glm::vec2(0.0f, 0.0f);


GLchar* LoadShader(const std::string &file) {

  std::ifstream shaderFile;
  long shaderFileLength;

  shaderFile.open(file, std::ios::binary);

  if (shaderFile.fail()) {
    throw std::runtime_error("COULD NOT FIND SHADER FILE");
  }

  shaderFile.seekg(0, shaderFile.end);
  shaderFileLength = shaderFile.tellg();
  shaderFile.seekg(0, shaderFile.beg);

  GLchar *shaderCode = new GLchar[shaderFileLength + 1];
  shaderFile.read(shaderCode, shaderFileLength);

  shaderFile.close();

  shaderCode[shaderFileLength] = '\0';

  return shaderCode;
}

int main(int argc, char** argv) {
  srand(time(0));

  const GLfloat delta_time = 0.1f;

  //Window Setup
  glfwInit();

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

  GLFWwindow* window = glfwCreateWindow(SCREENX, SCREENY, "particles", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  gladLoadGL();

  //Setup Initial Positions/Velocities
  glm::vec4 positions[NUM_PARTICLES];
  glm::vec4 velocities[NUM_PARTICLES];
  for(int i = 0; i < NUM_PARTICLES; i++) {
    float r = (float)rand() / RAND_MAX;
    r *= 8;
    float velx = r * sin(i) / 10;
    float vely = r * cos(i) / 10;
    positions[i] = glm::vec4(velx, vely, 0.0f, 0.0f);
    velocities[i] = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
  }

  GLuint vao, pos, timebuffer, vel, cursor;

  /**
  * VAO Setup - Even though we'll be sourcing all of our positions from a shader storage buffer,
  * a VAO must be bound for drawing, so let's just create an empty one. We also don't need a VBO
  * since all our rendering will be points
  */
  glCreateVertexArrays(1, &vao);
  glBindVertexArray(vao);

  /**
  * Buffer setup - We'll need 4 SSBOs, one for the position of each point, one for velocity,
  * one for cursor position, and one for delta_time. As cursor and delta_time are small, these
  * could have been implemented using regular uniforms, but I opted for consistency instead
  */
  glCreateBuffers(1, &pos);
  glCreateBuffers(1, &timebuffer);
  glCreateBuffers(1, &vel);
  glCreateBuffers(1, &cursor);
  glNamedBufferData(vel, sizeof(velocities), velocities, GL_STATIC_DRAW);
  glNamedBufferData(pos, sizeof(positions), positions, GL_STATIC_DRAW);
  glNamedBufferData(timebuffer, sizeof(GLfloat), &delta_time, GL_DYNAMIC_DRAW);
  glNamedBufferData(cursor, 2 * sizeof(GLfloat), glm::value_ptr(defaultCursor), GL_DYNAMIC_DRAW);

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, pos);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, vel);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cursor);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, timebuffer);


  /*
  * Basic Shader Setup - This is standard shader loading and compilation. The only difference is that the compute
  * shader must be linked by itself in a separate program
  */
  const GLchar* vertCode = LoadShader("shader.vert");
  const GLchar* fragCode = LoadShader("shader.frag");
  const GLchar* computeCode = LoadShader("cursor.glsl");

  GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
  GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
  GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);

  glShaderSource(vertShader, 1, &vertCode, nullptr);
  glShaderSource(fragShader, 1, &fragCode, nullptr);
  glShaderSource(computeShader, 1, &computeCode, nullptr);
  GLchar infolog[512];
  glCompileShader(vertShader);
  glGetShaderInfoLog(vertShader, 512, nullptr, infolog);

  std::cout << infolog << std::endl;

  glCompileShader(fragShader);
  glGetShaderInfoLog(fragShader, 512, nullptr, infolog);
  std::cout << infolog << std::endl;

  glCompileShader(computeShader);
  glGetShaderInfoLog(computeShader, 512, nullptr, infolog);
  std::cout << infolog << std::endl;

  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertShader);
  glAttachShader(shaderProgram, fragShader);
  glLinkProgram(shaderProgram);
  glDeleteShader(vertShader);
  glDeleteShader(fragShader);

  glGetProgramInfoLog(shaderProgram, 512, nullptr, infolog);

  std::cout << infolog << std::endl;

  GLuint computeProgram = glCreateProgram();
  glAttachShader(computeProgram, computeShader);
  glLinkProgram(computeProgram);
  glDeleteShader(computeShader);

  glGetProgramInfoLog(computeProgram, 512, nullptr, infolog);
  std::cout << infolog << std::endl;

  /**
  * Setup some basic properties for screen size, background color and point size
  * 2.0 Point Size was used to make it easier to see
  */
  glViewport(0, 0, SCREENX, SCREENY);
  glClearColor(0.05f, 0.05, 0.05f, 1.0f);
  glPointSize(2.0f);

  // Draw Loop
  while(glfwWindowShouldClose(window) == 0) {
    glClear(GL_COLOR_BUFFER_BIT);
    /**
    * Get our cursor coordinates in normalized device coordinates
    */
    glfwPollEvents();
    double cursorx;
    double cursory;
    glfwGetCursorPos(window, &cursorx, &cursory);

    cursorx = cursorx - (SCREENX / 2);
    cursorx /= SCREENX;
    cursorx *= 2;

    cursory = cursory - (SCREENY / 2);
    cursory /= SCREENY;
    cursory *= -2;

    /**
    * Copy over the cursor position into the buffer
    */
    glm::vec2 current_cursor = glm::vec2(cursorx, cursory);

    glNamedBufferSubData(cursor, 0, 2 * sizeof(GLfloat), glm::value_ptr(current_cursor));

    /**
    * Fire off our compute shader run. Since we want to simulate 100000 particles, and our work group size is 1000
    * we need to dispatch 100 work groups
    */
    glUseProgram(computeProgram);
    glDispatchCompute(NUM_PARTICLES / WORK_GROUP_SIZE, 1, 1);

    /**
    * Draw the particles
    */
    glUseProgram(shaderProgram);
    glDrawArraysInstanced(GL_POINTS, 0, 1, NUM_PARTICLES);


    glfwSwapBuffers(window);
  }

  /**
  * We're done now, just need to free up our resources
  */

  //OpenGL Shutdown
  glDeleteProgram(shaderProgram);
  glDeleteProgram(computeProgram);
  delete[] vertCode;
  delete[] fragCode;
  delete[] computeCode;

  //Window Shutdown
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
