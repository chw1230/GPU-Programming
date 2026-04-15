/*
 * ============================================================
 * [5주차 실습] Lighting Maps + Emission + 주기적 색상 변화 + 회전
 * ============================================================
 *
 * 과제 구현 요약:
 *
 *  [과제 1] Emission Map (matrix.jpg) 추가
 *    - 물체(컨테이너 큐브)에 matrix.jpg를 emissionMap으로 매핑하여
 *      마치 스스로 발광하는 것처럼 보이게 한다.
 *    - 프래그먼트 셰이더에서 조명 계산과 무관하게 텍스처 색상을 그대로 더한다.
 *
 *  [과제 2] 램프 큐브의 색상 & 빛의 색상 주기적 변화
 *    - R, G, B 각 채널을 서로 다른 주기의 sin 함수로 변화시켜
 *      시간에 따라 색이 부드럽게 바뀌도록 한다.
 *    - 빛의 색(lightColor)과 램프 큐브의 표시 색(LightColor uniform)을
 *      동일한 값으로 설정하여 "램프 큐브 = 빛 자체"를 시각적으로 표현한다.
 *
 *  [과제 3] 램프 큐브 회전
 *    - sin 함수로 램프 큐브의 위치(lightPos)를 시간에 따라 이동시킨다.
 *    - glm::rotate로 Y축 기준 자체 회전을 적용한다.
 *
 * 사용 셰이더:
 *   - shader/4.2.lighting_maps.vs / .fs : 컨테이너 큐브 (Phong 조명 + emission)
 *   - shader/4.2.light_cube.vs / .fs    : 램프 큐브 (단색 출력)
 *
 * 사용 텍스처:
 *   - resources/textures/container2.png          : diffuse map (기본 색상)
 *   - resources/textures/container2_specular.png : specular map (반사 강도)
 *   - resources/textures/matrix.jpg              : emission map (자체 발광) [과제 1]
 * ============================================================
 */

#include <glad/glad.h>   
#include <GLFW/glfw3.h>  

#include <shader_m.h>   
#include <camera.h>      
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>                 
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>        
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);

// ---- 창 해상도 설정 ----
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;

// ---- 카메라 전역 변수 ----
// 초기 위치: z=3 (물체로부터 3칸 뒤)
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;   // 마우스 X 이전 프레임 위치
float lastY = SCR_HEIGHT / 2.0f;  // 마우스 Y 이전 프레임 위치
bool firstMouse = true;            // 첫 마우스 입력 시 점프 방지 플래그

// ---- 프레임 타이밍 ----
// deltaTime: 이전 프레임과 현재 프레임 사이의 시간 간격
// -> 카메라 이동 속도를 프레임레이트에 독립적으로 만들기 위해 사용
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ---- 조명 위치 (초기값) ----
// 렌더 루프 안에서 매 프레임 sin 함수로 업데이트됨 [과제 3]
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

int main()
{
    // =========================================================
    // GLFW 초기화 및 창 생성
    // =========================================================

    glfwInit();
    // OpenGL 3.3 Core Profile 사용
    // Core Profile: 구식 API(deprecated) 제거, 현대적인 셰이더 기반 방식 강제
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // macOS 호환성
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);  

    // 창 크기 변경 시 뷰포트 자동 갱신
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // 마우스 이동 시 카메라 방향 갱신
    glfwSetCursorPosCallback(window, mouse_callback);
    // 마우스 스크롤 시 줌(FOV) 조절
    glfwSetScrollCallback(window, scroll_callback);

    // 마우스 커서를 창 안에 숨기고 고정 (FPS 스타일 카메라 제어)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // =========================================================
    // GLAD 초기화: OS별 OpenGL 함수 포인터 로드
    // =========================================================
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // 깊이 테스트 활성화: 앞에 있는 물체가 뒤를 가리도록 (z-buffer 사용)
    glEnable(GL_DEPTH_TEST);

    // =========================================================
    // 셰이더 프로그램 컴파일 및 링크
    // =========================================================
    // lightingShader: 컨테이너 큐브용 (Phong 조명 + diffuse/specular/emission 맵)
    Shader lightingShader("shader/4.2.lighting_maps.vs", "shader/4.2.lighting_maps.fs");
    // lightCubeShader: 램프 큐브용 (단순 단색 출력)
    Shader lightCubeShader("shader/4.2.light_cube.vs", "shader/4.2.light_cube.fs");

    // =========================================================
    // 정점 데이터 정의
    // =========================================================
    // 큐브 6면 × 2삼각형 × 3정점 = 36개 정점
    // 각 정점: 위치(3) + 법선(3) + 텍스처 좌표(2) = float 8개
    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    // =========================================================
    // [컨테이너 큐브] VAO / VBO 설정
    // =========================================================
    // VAO (Vertex Array Object): 정점 속성 설정을 기억하는 객체
    // VBO (Vertex Buffer Object): 정점 데이터를 GPU 메모리에 저장하는 버퍼
    unsigned int VBO, cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);

    // VBO에 정점 데이터 업로드 - 규브랑 상자는 둘다 정육면체 이니까
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);

    // attribute 0: 위치 (x, y, z) 부분
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // attribute 1: 법선 벡터 (nx, ny, nz) 부분
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // attribute 2: 텍스처 UV 좌표 (u, v) 부분
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // =========================================================
    // [램프 큐브] VAO 설정 (VBO는 동일한 것 재사용)
    // =========================================================
    // 램프 큐브는 위치(aPos)만 필요 → attribute 0만 설정
    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // stride는 여전히 8 * sizeof(float) (같은 VBO 사용)
    // 법선, 텍스처 좌표는 읽지만 셰이더에서 사용하지 않으므로 attribute 설정 생략
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // =========================================================
    // 텍스처 로드 [과제 1 포함]
    // =========================================================
    // diffuse map — 물체의 기본 색상/패턴 텍스처
    unsigned int diffuseMap = loadTexture("resources/textures/container2.png");
    // specular map — 각 픽셀의 반사 강도 텍스처  -> 나무 상자의 철로 된 가셍이? 부분 
    //   흰색 영역 = 하이라이트 강함, 검정 영역 = 하이라이트 없음
    unsigned int specularMap = loadTexture("resources/textures/container2_specular.png");
    // emission map — 자체 발광 텍스처 -> 나무 상자위에 씌우기
    unsigned int emissionMap = loadTexture("resources/textures/matrix.jpg");

    // =========================================================
    // 셰이더 텍스처 슬롯 연결
    // =========================================================
    lightingShader.use();
    lightingShader.setInt("material.diffuse", 0);  // GL_TEXTURE0 → diffuseMap
    lightingShader.setInt("material.specular", 1);  // GL_TEXTURE1 → specularMap
    lightingShader.setInt("material.emission", 2);  // GL_TEXTURE2 → emissionMap 

    // =========================================================
    // 렌더 루프
    // =========================================================
    while (!glfwWindowShouldClose(window))
    {
 
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // ---- 키보드 입력 처리 ----
        processInput(window);

        // ---- 화면 초기화 ----
        // 어두운 회색 배경 (0.1, 0.1, 0.1)
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        // 컬러 버퍼 + 깊이 버퍼 동시 초기화
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ---- 컨테이너 큐브 셰이더 활성화 ----
        lightingShader.use();
        lightingShader.setVec3("light.position", lightPos);   // 광원 위치 전달
        lightingShader.setVec3("viewPos", camera.Position);   // 카메라 위치 전달 (specular 계산용)

        // =========================================================
        // [과제 2] 빛 색상 주기적 변화
        // =========================================================
        // R, G, B 채널을 서로 다른 주기의 sin 함수로 계산
        //   - sin 값의 범위: -1 ~ +1
        //   - 음수 → 검정(0)으로 클램핑되므로 실제로는 0 ~ 1로 변화
        //   - 각 채널의 주기가 달라서 색이 단조롭지 않고 다양하게 변함
        glm::vec3 lightColor;
        lightColor.x = static_cast<float>(sin(glfwGetTime() * 2.0));  // R: 빠른 주기
        lightColor.y = static_cast<float>(sin(glfwGetTime() * 0.7));  // G: 느린 주기
        lightColor.z = static_cast<float>(sin(glfwGetTime() * 1.3));  // B: 중간 주기

        // diffuse = lightColor * 0.5 : 주 조명은 빛 색의 절반 강도
        glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f);
        // ambient = diffuseColor * 0.2 : 환경광은 diffuse의 20% (그림자 부분용)
        glm::vec3 ambientColor = diffuseColor * glm::vec3(0.2f);

        lightingShader.setVec3("light.ambient", ambientColor);
        lightingShader.setVec3("light.diffuse", diffuseColor);
        // specular는 (1,1,1) 흰색 고정: 하이라이트는 빛 색에 무관하게 항상 하얗게 보임
        lightingShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);

        // 광택도 설정: 64 = 꽤 광택 있는 표면 (값이 클수록 하이라이트가 작고 날카로움)
        lightingShader.setFloat("material.shininess", 64.0f);

        // ---- 카메라 변환 행렬 계산 ----
        // projection: 원근 투영 (FOV는 마우스 스크롤로 변경 가능)
        // view: 카메라의 위치와 방향에 따른 변환
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);

        // 컨테이너 큐브는 원점에 고정 (model = 단위행렬)
        glm::mat4 model = glm::mat4(1.0f);
        lightingShader.setMat4("model", model);

        // =========================================================
        // [과제 1] 텍스처 슬롯 바인딩 (emission 포함)
        // =========================================================
        // 슬롯 0 활성화 후 diffuse 텍스처 바인딩 → 셰이더의 material.diffuse(슬롯 0)로 접근
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);

        // 슬롯 1 활성화 후 specular 텍스처 바인딩 → 셰이더의 material.specular(슬롯 1)로 접근
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        // 슬롯 2 활성화 후 emission 텍스처 바인딩 → 셰이더의 material.emission(슬롯 2)로 접근
        // [과제 1] matrix.jpg를 emission map으로 사용하여 자체 발광 효과 구현
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, emissionMap);

        // 컨테이너 큐브 그리기 (삼각형 36개 = 큐브 6면)
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // =========================================================
        // [과제 2, 3] 램프 큐브 렌더링
        // =========================================================
        lightCubeShader.use();

        // [과제 2] 램프 큐브 색상 = 빛의 색상과 동일하게 설정
        // diffuseColor + ambientColor를 합산하여 밝기를 더 잘 보이게 함
        // -> 시각적으로 "이 큐브가 조명을 발산하고 있다"는 것을 표현
        lightCubeShader.setVec3("LightColor", diffuseColor + ambientColor);
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);

        model = glm::mat4(1.0f);

        // [과제 3] 램프 큐브 위치 이동: sin 함수로 매 프레임 위치 갱신
        // X축: sin(t) * 2 → 주기적으로 좌우로 왕복 이동 (진폭 2)
        // Y축: sin(t/2) * 1 → X축의 절반 속도로 위아래 흔들림 (진폭 1)
        lightPos.x = 1.0f + sin(glfwGetTime()) * 2.0f;
        lightPos.y = sin(glfwGetTime() / 2.0f) * 1.0f;

        // translate: 계산된 lightPos로 이동
        model = glm::translate(model, lightPos);

        // [과제 3] rotate: Y축 기준 자체 회전
        // glfwGetTime() * 2.0f → 시간이 지날수록 각도 증가 (2.0이 각속도, 클수록 빠름)
        // 회전축: (0, 1, 0) = Y축
        model = glm::rotate(model, (float)glfwGetTime() * 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));

        // scale: 0.2배로 작게 축소 (조명 위치 표시용 작은 큐브)
        model = glm::scale(model, glm::vec3(0.2f));

        lightCubeShader.setMat4("model", model);

        // 램프 큐브 그리기
        glBindVertexArray(lightCubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // ---- 더블 버퍼링: 백버퍼(렌더된 화면) → 프론트버퍼(화면에 표시) 스왑 ----
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // =========================================================
    // 자원 해제 (GPU 메모리 반환)
    // =========================================================
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteBuffers(1, &VBO);

    // GLFW 종료: 창 파괴 및 시스템 자원 반환
    glfwTerminate();
    return 0;
}

// =========================================================
// 키보드 입력 처리
// =========================================================
// 매 프레임 호출되어 눌린 키에 따라 카메라를 이동
// deltaTime을 곱하는 이유: FPS와 무관하게 일정한 이동 속도 유지
void processInput(GLFWwindow* window)
{
    // ESC: 창 닫기
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // WASD: 카메라 앞/뒤/왼/오른 이동
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// =========================================================
// 창 크기 변경 콜백
// =========================================================
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // 뷰포트를 새 창 크기에 맞게 조정 (Retina 디스플레이는 실제 픽셀이 더 클 수 있음)
    glViewport(0, 0, width, height);
}

// =========================================================
// 마우스 이동 콜백 — 카메라 시점 회전
// =========================================================
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    // 첫 번째 마우스 입력 시: 이전 위치를 현재 위치로 초기화 (시점 점프 방지)
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    // Y축 부호 반전: GLFW는 화면 위쪽이 y=0이지만
    // OpenGL 카메라는 위쪽이 양의 Y 방향이므로 반전 필요
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// =========================================================
// 마우스 스크롤 콜백 — 줌(FOV) 조절
// =========================================================
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // yoffset: 스크롤 방향 (위=양수, 아래=음수)
    // Camera 클래스 내부에서 Zoom(FOV) 값 증감 처리
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// =========================================================
// 텍스처 로드 유틸리티 함수
// =========================================================
// 이미지 파일을 읽어 GPU에 업로드하고 텍스처 ID를 반환
unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);  // GPU 텍스처 객체 생성

    int width, height, nrComponents;
    // stbi_load: 이미지를 CPU 메모리로 로드
    // nrComponents: 채널 수 (1=그레이, 3=RGB, 4=RGBA)
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        // 채널 수에 따라 OpenGL 포맷 결정
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;   // 그레이스케일
        else if (nrComponents == 3)
            format = GL_RGB;   // JPG 등 불투명 이미지
        else if (nrComponents == 4)
            format = GL_RGBA;  // PNG 등 알파 채널 포함 이미지

        glBindTexture(GL_TEXTURE_2D, textureID);
        // 이미지 데이터를 GPU 메모리에 업로드
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        // Mipmap 자동 생성: 원거리 오브젝트를 작은 크기 텍스처로 렌더링할 때 사용
        //   → 성능 향상 + 계단 현상(aliasing) 감소
        glGenerateMipmap(GL_TEXTURE_2D);

        // 텍스처 래핑 설정: UV가 0~1 범위를 벗어날 때 반복(GL_REPEAT)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // U축
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  // V축
        // 필터링 설정:
        //   MIN (축소): Mipmap 선형 보간 — 가장 자연스러운 축소 품질
        //   MAG (확대): 선형 보간 — 확대 시 부드럽게 표현
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);  // CPU 메모리 해제 (GPU로 업로드 완료)
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
