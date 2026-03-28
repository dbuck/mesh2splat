///////////////////////////////////////////////////////////////////////////////
//         Mesh2Splat: fast mesh to 3D gaussian splat conversion             //
//        Copyright (c) 2025 Electronic Arts Inc. All rights reserved.       //
///////////////////////////////////////////////////////////////////////////////

#include "utils/normalizedUvUnwrapping.hpp"
#include "renderer/renderer.hpp"
#include "glewGlfwHandlers/glewGlfwHandler.hpp"
#include "renderer/guiRendererConcreteMediator.hpp"
#include "utils/argparser.hpp"
#include "renderer/RenderPasses.hpp"
#include <filesystem>
#include <thread>
#include <chrono>

static int runCli(const std::string& inputPath, const std::string& outputPath, int resolution) {
    // Determine parent folder
    std::filesystem::path inPath(inputPath);
    std::string parentFolder = inPath.parent_path().string();

    GlewGlfwHandler glewGlfwHandler(glm::ivec2(resolution, resolution), "Mesh2Splat-CLI", false);

    Camera camera(
        glm::vec3(0.0f, 0.0f, 5.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        -90.0f,
        0.0f
    );

    IoHandler ioHandler(glewGlfwHandler.getWindow(), camera);
    if (glewGlfwHandler.init() == -1) {
        fprintf(stderr, "Failed to initialize GLEW/GLFW\n");
        return -1;
    }

    Renderer renderer(glewGlfwHandler.getWindow(), camera);
    renderer.initialize();

    // Set gaussian std dev (default from GUI is 0.65)
    renderer.setStdDevFromImGui(0.65f);

    printf("Loading model: %s\n", inputPath.c_str());
    renderer.resetModelMatrices();
    renderer.getSceneManager().loadModel(inputPath, parentFolder);
    renderer.gaussianBufferFromSize(resolution * resolution);
    renderer.setFormatType(0);
    renderer.setViewportResolutionForConversion(resolution);
    renderer.enableRenderPass(conversionPassName);

    // Render a frame to run the conversion pass
    renderer.clearingPrePass(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    renderer.renderFrame();
    glfwSwapBuffers(glewGlfwHandler.getWindow());

    // Render a second frame to ensure GPU work completes
    renderer.clearingPrePass(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    renderer.renderFrame();
    glfwSwapBuffers(glewGlfwHandler.getWindow());

    glFinish();

    printf("Exporting PLY: %s\n", outputPath.c_str());
    renderer.getSceneManager().exportPlySync(outputPath, 0);

    printf("Done.\n");
    glfwTerminate();
    return 0;
}

int main(int argc, char** argv) {
    InputParser input(argc, argv);

    // CLI mode: mesh2splat --input file.glb --output file.ply [--resolution 2048]
    if (input.cmdOptionExists("--input")) {
        const std::string& inputFile = input.getCmdOption("--input");
        std::string outputFile;
        if (input.cmdOptionExists("--output")) {
            outputFile = input.getCmdOption("--output");
        } else {
            // Default: same name with .ply extension
            std::filesystem::path p(inputFile);
            outputFile = (p.parent_path() / p.stem()).string() + ".ply";
        }
        int resolution = 2048;
        if (input.cmdOptionExists("--resolution")) {
            resolution = std::stoi(input.getCmdOption("--resolution"));
        }
        printf("Mesh2Splat CLI mode\n");
        printf("  Input:      %s\n", inputFile.c_str());
        printf("  Output:     %s\n", outputFile.c_str());
        printf("  Resolution: %d\n", resolution);
        return runCli(inputFile, outputFile, resolution);
    }

    // GUI mode (original)
    GlewGlfwHandler glewGlfwHandler(glm::ivec2(1080, 720), "Mesh2Splat");

    Camera camera(
        glm::vec3(0.0f, 0.0f, 5.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        -90.0f,
        0.0f
    );

    IoHandler ioHandler(glewGlfwHandler.getWindow(), camera);
    if(glewGlfwHandler.init() == -1) return -1;

    ioHandler.setupCallbacks();

    ImGuiUI ImGuiUI(0.65f, 0.5f); //TODO: give a meaning to these params
    ImGuiUI.initialize(glewGlfwHandler.getWindow());

    Renderer renderer(glewGlfwHandler.getWindow(), camera);
    renderer.initialize();
    GuiRendererConcreteMediator guiRendererMediator(renderer, ImGuiUI);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(glewGlfwHandler.getWindow())) {

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        ioHandler.processInput(deltaTime);

        renderer.clearingPrePass(ImGuiUI.getSceneBackgroundColor());

        ImGuiUI.preframe();
        ImGuiUI.renderUI();

        guiRendererMediator.update();

        renderer.renderFrame();

        ImGuiUI.displayGaussianCounts(renderer.getTotalGaussianCount(), renderer.getVisibleGaussianCount());
        ImGuiUI.postframe();

        glfwSwapBuffers(glewGlfwHandler.getWindow());
    }

    glfwTerminate();

    return 0;
}

