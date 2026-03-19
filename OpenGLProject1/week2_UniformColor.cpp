#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

#define ALLOW_REPEAT_KEYSTROKE 1

void framebuffer_size_callback(GLFWwindow * window, int width, int height);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// color
float rgb[3] = { 0.2f, 0.2f, 0.2f };

// 값의 최대와 최소를 적용하여 범위 안에서 값이 유지될 수 있도록 하는 함수
void clamp(float& value, float min, float max) {
    if (value < min)
        value = min;
    if (value > max)
        value = max;
}

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;\n"
"out vec3 ourColor;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos, 1.0);\n"
"   ourColor = aColor;\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec4 ourColor;\n"
"void main()\n"
"{\n"
"   FragColor = ourColor;"
"}\n\0";

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // build and compile our shader program
    // ------------------------------------
    // vertex shader 객체 생성
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // 컴파일 성공, 실패 여부 확인
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // 컴파일 성공, 실패 여부 확인
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    // 두 shaders를 연결
    // shader program 객체 생성
    unsigned int shaderProgram = glCreateProgram();
    // 서로 연결
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // linking errors 확인
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    // shaders 필요없어지면 삭제 -> 메모리 절약 가능
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions         // colors
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  // bottom left
         0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f   // top 

    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 메모리 상에 올라온 데이터의 구조가 어떻게 해석되어야 하는지 OepnGL에 전달
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    // glBindVertexArray(0);

    // as we only have a single shader, we could also just activate our shader once beforehand if we want to 
    glUseProgram(shaderProgram);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // input
        // -----
        processInput(window);

        // render
        // ------
        float timeValue = glfwGetTime();
        // green 변동 (RGB에서 적용되도록 값 0.0 ~ 1.0 사이로 변환)
        float greenValue = (sin(timeValue) / 2.0f) + 0.5f;  // 0.0 ~ 1.0
        // uniform 위치 찾기
        int ourColorLoc = glGetUniformLocation(shaderProgram, "ourColor");
        glUseProgram(shaderProgram);
        
        // 삼각형 색상 설정 (R, G, B, A)
        glUniform4f(ourColorLoc,
            0.0f,         // 빨강 고정
            greenValue,   // 초록이 시간에 따라 변함
            0.0f,         // 파랑 고정
            1.0f);

        // 배경 부분
        glClearColor(rgb[0], rgb[1], rgb[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // draw our first triangle
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        glDrawArrays(GL_TRIANGLES, 0, 3);
        // glBindVertexArray(0); // no need to unbind it every time

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // 변화값
    float delta = 0.0005f;

#if ALLOW_REPEAT_KEYSTROKE
    // F1, F2: Red 증감
    if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS) {
        rgb[0] += delta;
        clamp(rgb[0], 0.0f, 1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS) {
        rgb[0] -= delta;
        clamp(rgb[0], 0.0f, 1.0f);
    }

    // F3, F4: Green 증감
    if (glfwGetKey(window, GLFW_KEY_F3) == GLFW_PRESS) {
        rgb[1] += delta;
        clamp(rgb[1], 0.0f, 1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS) {
        rgb[1] -= delta;
        clamp(rgb[1], 0.0f, 1.0f);
    }
    // F5, F6: Blue 증감
    if (glfwGetKey(window, GLFW_KEY_F5) == GLFW_PRESS) {
        rgb[2] += delta;
        clamp(rgb[2], 0.0f, 1.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_F6) == GLFW_PRESS) {
        rgb[2] -= delta;
        clamp(rgb[2], 0.0f, 1.0f);
    }
#else   
    static bool pressed_f1_f6[6] = { false, };
    for (int i = 0; i < 6; i++)
    {
        if (glfwGetKey(window, GLFW_KEY_F1 + i) == GLFW_PRESS)
            pressed_f1_f6[i] = true;

        if (glfwGetKey(window, GLFW_KEY_F1 + i) == GLFW_RELEASE && pressed_f1_f6[i] == true)
        {
            if (i % 2 == 0) rgb[i / 2] += 0.1f;
            else rgb[i / 2] -= 0.1f;
            clamp(rgb[i / 2], 0.0f, 1.0f);
            pressed_f1_f6[i] = false;

            printf("backgroundColor : %.2f %.2f %.2f\n", rgb[0], rgb[1], rgb[2]);
        }
    }
#endif
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}
