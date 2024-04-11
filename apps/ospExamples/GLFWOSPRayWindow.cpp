// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "GLFWOSPRayWindow.h"
// ospray_testing
#include "ospray_testing.h"
#include "rkcommon/utility/random.h"
// imgui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
// std
#include <algorithm>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <vector>

// on Windows often only GL 1.1 headers are present
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812D
#endif
#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif
#ifndef GL_RGBA32F
#define GL_RGBA32F 0x8814
#endif
#ifndef GL_RGB32F
#define GL_RGB32F 0x8815
#endif

static bool g_quitNextFrame = false;

static const std::vector<std::string> g_scenes = {"boxes_lit",
    "boxes",
    "cornell_box",
    "curves",
    "gravity_spheres_volume",
    "gravity_spheres_amr",
    "gravity_spheres_isosurface",
    "perlin_noise_volumes",
    "random_spheres",
    "random_discs",
    "random_oriented_discs",
    "streamlines",
    "subdivision_cube",
    "unstructured_volume",
    "unstructured_volume_isosurface",
    "planes",
    "clip_with_spheres",
    "clip_with_planes",
    "clip_gravity_spheres_volume",
    "clip_perlin_noise_volumes",
    "clip_particle_volume",
    "particle_volume",
    "particle_volume_isosurface",
    "vdb_volume",
    "vdb_volume_packed",
    "instancing",
    "test_pt_alloy_roughness",
    "test_pt_carpaint",
    "test_pt_glass",
    "test_pt_thinglass",
    "test_pt_luminous",
    "test_pt_material_tex",
    "test_pt_metal_roughness",
    "test_pt_metallic_flakes",
    "test_pt_mix_tex",
    "test_pt_obj",
    "test_pt_plastic",
    "test_pt_principled_metal",
    "test_pt_principled_plastic",
    "test_pt_principled_glass",
    "test_pt_principled_tex",
    "test_pt_velvet"};

static const std::vector<std::string> g_curveVariant = {
    "bspline", "bezier", "hermite", "catmull-rom", "linear", "cones"};

static const std::vector<std::string> g_renderers = {
    "scivis", "pathtracer", "ao", "debug"};

static const std::vector<std::string> g_debugRendererTypes = {"eyeLight",
    "primID",
    "geomID",
    "instID",
    "Ng",
    "Ns",
    "backfacing_Ng",
    "backfacing_Ns",
    "dPds",
    "dPdt",
    "volume"};

static const std::vector<std::string> g_pixelFilterTypes = {
    "point", "box", "gaussian", "mitchell", "blackmanHarris"};

static const std::vector<std::string> g_AOVs = {
    "color", "depth", "albedo", "primID", "objID", "instID"};

const char *vec_string_getter(void *vec_, int index)
{
  auto v = (std::vector<std::string> *)vec_;
  return v->at(index).c_str();
}

// GLFWOSPRayWindow definitions ///////////////////////////////////////////////

void error_callback(int error, const char *desc)
{
  std::cerr << "GLFW error " << error << ": " << desc << std::endl;
}

GLFWOSPRayWindow *GLFWOSPRayWindow::activeWindow = nullptr;

GLFWOSPRayWindow::GLFWOSPRayWindow(const vec2i &windowSize, bool denoiser)
    : denoiserAvailable(denoiser)
{
  if (activeWindow != nullptr) {
    throw std::runtime_error("Cannot create more than one GLFWOSPRayWindow!");
  }

  activeWindow = this;

  glfwSetErrorCallback(error_callback);
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW!");
  }

  glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
#ifdef __APPLE_
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  const char *glslVersion = "#version 150";
#else
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  const char *glslVersion = "#version 130";
#endif

  glfwWindow = glfwCreateWindow(
      windowSize.x, windowSize.y, "OSPRay Examples", nullptr, nullptr);

  if (!glfwWindow) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window!");
  }
  glfwMakeContextCurrent(glfwWindow);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui_ImplGlfw_InitForOpenGL(glfwWindow, false);

  // set GLFW callbacks
  glfwSetFramebufferSizeCallback(
      glfwWindow, [](GLFWwindow *, int newWidth, int newHeight) {
        activeWindow->reshape(vec2i{newWidth, newHeight});
      });

  glfwSetCursorPosCallback(glfwWindow, [](GLFWwindow *, double x, double y) {
    ImGuiIO &io = ImGui::GetIO();
    if (!activeWindow->showUi || !io.WantCaptureMouse) {
      activeWindow->motion(vec2f{float(x), float(y)});
    }
  });

  glfwSetKeyCallback(
      glfwWindow, [](GLFWwindow *, int key, int, int action, int) {
        if (action == GLFW_PRESS) {
          switch (key) {
          case GLFW_KEY_G:
            activeWindow->showUi = !(activeWindow->showUi);
            break;
          case GLFW_KEY_Q:
            g_quitNextFrame = true;
            break;
          }
        }
      });

  glfwSetMouseButtonCallback(
      glfwWindow, [](GLFWwindow *, int button, int action, int /*mods*/) {
        auto &w = *activeWindow;
        if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
          auto mouse = activeWindow->previousMouse;
          auto windowSize = activeWindow->windowSize;
          const vec2f pos(mouse.x / static_cast<float>(windowSize.x),
              1.f - mouse.y / static_cast<float>(windowSize.y));

          auto res =
              w.framebuffer.pick(*w.renderer, w.camera, w.world, pos.x, pos.y);

          if (res.hasHit) {
            std::cout << "Picked geometry [inst: " << res.instance
                      << ", model: " << res.model << ", prim: " << res.primID
                      << "]" << std::endl;
          }
        }
      });

  // will chain our callbacks above
  ImGui_ImplGlfw_InstallCallbacks(glfwWindow);

  ImGui_ImplOpenGL3_Init(glslVersion);

  // set initial OpenGL state
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);

  // create OpenGL frame buffer texture
  glGenTextures(1, &framebufferTexture);
  glBindTexture(GL_TEXTURE_2D, framebufferTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // OSPRay setup //

  // set up backplate texture
  std::vector<vec4f> backplate;
  backplate.push_back(vec4f(0.8f, 0.2f, 0.2f, 1.0f));
  backplate.push_back(vec4f(0.2f, 0.8f, 0.2f, 1.0f));
  backplate.push_back(vec4f(0.2f, 0.2f, 0.8f, 1.0f));
  backplate.push_back(vec4f(0.4f, 0.2f, 0.4f, 1.0f));

  backplateTex.setParam(
      "data", cpp::CopiedData(backplate.data(), vec2ul(2, 2)));
  backplateTex.setParam("format", OSP_TEXTURE_RGBA32F);
  addObjectToCommit(backplateTex.handle());

  refreshScene(true);

  // trigger window reshape events with current window size
  glfwGetFramebufferSize(glfwWindow, &this->windowSize.x, &this->windowSize.y);
  reshape(this->windowSize);

  commitOutstandingHandles();
}

GLFWOSPRayWindow::~GLFWOSPRayWindow()
{
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(glfwWindow);
  glfwTerminate();
}

GLFWOSPRayWindow *GLFWOSPRayWindow::getActiveWindow()
{
  return activeWindow;
}

void GLFWOSPRayWindow::mainLoop()
{
  // continue until the user closes the window
  startNewOSPRayFrame();

  while (!glfwWindowShouldClose(glfwWindow) && !g_quitNextFrame) {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (displayCallback)
      displayCallback(this);

    display();

    if (showUi)
      buildUI();

    glDisable(GL_FRAMEBUFFER_SRGB); // Disable SRGB conversion for UI
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(glfwWindow);
  }

  waitOnOSPRayFrame();
}

void GLFWOSPRayWindow::reshape(const vec2i &newWindowSize)
{
  windowSize = newWindowSize;

  // create new frame buffer
  auto buffers = OSP_FB_COLOR | OSP_FB_DEPTH | OSP_FB_ACCUM | OSP_FB_VARIANCE
      | OSP_FB_ALBEDO | OSP_FB_NORMAL | OSP_FB_ID_PRIMITIVE | OSP_FB_ID_OBJECT
      | OSP_FB_ID_INSTANCE;
  framebuffer =
      cpp::FrameBuffer(windowSize.x, windowSize.y, OSP_FB_RGBA32F, buffers);

  refreshFrameOperations();

  // reset OpenGL viewport and orthographic projection
  glViewport(0, 0, windowSize.x, windowSize.y);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, windowSize.x, 0.0, windowSize.y, -1.0, 1.0);

  // update camera
  arcballCamera->updateWindowSize(windowSize);

  camera.setParam("aspect", windowSize.x / float(windowSize.y));
  camera.commit();
}

void GLFWOSPRayWindow::updateCamera()
{
  camera.setParam("aspect", windowSize.x / float(windowSize.y));
  camera.setParam("stereoMode", cameraStereoMode);
  const auto xfm = arcballCamera->transform();
  if (rendererType == OSPRayRendererType::PATHTRACER && cameraMotionBlur) {
    camera.removeParam("transform");
    std::vector<affine3f> xfms;
    xfms.push_back(lastXfm);
    xfms.push_back(xfm);
    camera.setParam("motion.transform", cpp::CopiedData(xfms));
    camera.setParam("shutter", range1f(1.0f - cameraMotionBlur, 1.0f));
    camera.setParam("shutterType", cameraShutterType);
    camera.setParam("rollingShutterDuration", cameraRollingShutter);
    renderCameraMotionBlur = true;
  } else {
    camera.removeParam("motion.transform");
    camera.setParam("transform", xfm);
    camera.setParam("shutter", range1f(0.5f));
  }
}

void GLFWOSPRayWindow::motion(const vec2f &position)
{
  const vec2f mouse(position.x, position.y);
  if (previousMouse != vec2f(-1)) {
    const bool leftDown =
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool rightDown =
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool middleDown =
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    const vec2f prev = previousMouse;

    bool cameraChanged = leftDown || rightDown || middleDown;

    if (leftDown) {
      const vec2f mouseFrom(clamp(prev.x * 2.f / windowSize.x - 1.f, -1.f, 1.f),
          clamp(prev.y * 2.f / windowSize.y - 1.f, -1.f, 1.f));
      const vec2f mouseTo(clamp(mouse.x * 2.f / windowSize.x - 1.f, -1.f, 1.f),
          clamp(mouse.y * 2.f / windowSize.y - 1.f, -1.f, 1.f));
      arcballCamera->rotate(mouseFrom, mouseTo);
    } else if (rightDown) {
      arcballCamera->zoom(mouse.y - prev.y);
    } else if (middleDown) {
      arcballCamera->pan(vec2f(mouse.x - prev.x, prev.y - mouse.y));
    }

    if (cameraChanged) {
      if (cancelFrameOnInteraction) {
        currentFrame.cancel();
        waitOnOSPRayFrame();
      }
      updateCamera();
      addObjectToCommit(camera.handle());
    }
  }

  previousMouse = mouse;
}

void GLFWOSPRayWindow::display()
{
  static float totalRenderTime = 0.f;
  static auto displayStart = std::chrono::steady_clock::now();

  static bool firstFrame = true;
  if (firstFrame || currentFrame.isReady()) {
    // display frame rate in window title
    const auto displayEnd = std::chrono::steady_clock::now();
    const auto displayMilliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            displayEnd - displayStart);
    const auto renderTime = currentFrame.duration();
    totalRenderTime += renderTime;

    // update fps every 10 frames or every second
    framesCounter++;
    if (framesCounter > 9 || totalRenderTime > 1.f
        || displayMilliseconds > std::chrono::seconds(1)) {
      displayFPS = 1000.f * float(framesCounter) / displayMilliseconds.count();
      latestFPS = float(framesCounter) / totalRenderTime;

      displayStart = displayEnd;
      totalRenderTime = 0.f;
      framesCounter = 0;

      updateTitleBar();
    }

    waitOnOSPRayFrame();

    auto fbChannel = OSP_FB_COLOR;
    if (showDepth)
      fbChannel = OSP_FB_DEPTH;
    if (showAlbedo)
      fbChannel = OSP_FB_ALBEDO;
    if (showPrimID)
      fbChannel = OSP_FB_ID_PRIMITIVE;
    if (showGeomID)
      fbChannel = OSP_FB_ID_OBJECT;
    if (showInstID)
      fbChannel = OSP_FB_ID_INSTANCE;
    auto *fb = framebuffer.map(fbChannel);

    // Copy and normalize depth layer if showDepth is on
    float *depthFb = static_cast<float *>(fb);
    std::vector<float> depthCopy;
    if (showDepth) {
      depthCopy.assign(depthFb, depthFb + (windowSize.x * windowSize.y));
      depthFb = depthCopy.data();

      float minDepth = rkcommon::math::inf;
      float maxDepth = rkcommon::math::neg_inf;

      for (int i = 0; i < windowSize.x * windowSize.y; i++) {
        if (std::isinf(depthFb[i]))
          continue;
        minDepth = std::min(minDepth, depthFb[i]);
        maxDepth = std::max(maxDepth, depthFb[i]);
      }
      const float rcpDepthRange = 1.f / (maxDepth - minDepth);

      for (int i = 0; i < windowSize.x * windowSize.y; i++) {
        depthFb[i] = (depthFb[i] - minDepth) * rcpDepthRange;
      }
    }
    unsigned int *ids = static_cast<unsigned int *>(fb);
    std::vector<vec3f> idFbArray(windowSize.x * windowSize.y);
    float *idFb = reinterpret_cast<float *>(idFbArray.data());
    if (showPrimID || showGeomID || showInstID) {
      for (int i = 0; i < windowSize.x * windowSize.y; i++) {
        const unsigned int id = ids[i];
        idFbArray[i] =
            id == -1u ? vec3f(0.f) : rkcommon::utility::makeRandomColor(id);
      }
    }

    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D,
        0,
        showDepth ? GL_DEPTH_COMPONENT
                  : ((showAlbedo || showPrimID || showGeomID || showInstID)
                          ? GL_RGB32F
                          : GL_RGBA32F),
        windowSize.x,
        windowSize.y,
        0,
        showDepth ? GL_DEPTH_COMPONENT
                  : ((showAlbedo || showPrimID || showGeomID || showInstID)
                          ? GL_RGB
                          : GL_RGBA),
        GL_FLOAT,
        showDepth ? depthFb
                  : ((showPrimID || showGeomID || showInstID) ? idFb : fb));

    framebuffer.unmap(fb);

    commitOutstandingHandles();

    startNewOSPRayFrame();
    firstFrame = false;
  }

  glEnable(GL_FRAMEBUFFER_SRGB); // Turn on sRGB conversion for OSPRay frame
  glClear(GL_COLOR_BUFFER_BIT);

  // render textured quad with OSPRay frame buffer contents
  glBegin(GL_QUADS);

  glTexCoord2f(0.f, 0.f);
  glVertex2f(0.f, 0.f);

  glTexCoord2f(0.f, 1.f);
  glVertex2f(0.f, windowSize.y);

  glTexCoord2f(1.f, 1.f);
  glVertex2f(windowSize.x, windowSize.y);

  glTexCoord2f(1.f, 0.f);
  glVertex2f(windowSize.x, 0.f);

  glEnd();
}

void GLFWOSPRayWindow::startNewOSPRayFrame()
{
  if (updateFrameOpsNextFrame) {
    refreshFrameOperations();
    updateFrameOpsNextFrame = false;
  }
  lastXfm = arcballCamera->transform();
  currentFrame = framebuffer.renderFrame(*renderer, camera, world);
  if (renderCameraMotionBlur) {
    camera.removeParam("motion.transform");
    camera.setParam("transform", lastXfm);
    addObjectToCommit(camera.handle());
    renderCameraMotionBlur = false;
  }
}

void GLFWOSPRayWindow::waitOnOSPRayFrame()
{
  currentFrame.wait();
}

void GLFWOSPRayWindow::addObjectToCommit(OSPObject obj)
{
  objectsToCommit.push_back(obj);
}

void GLFWOSPRayWindow::updateTitleBar()
{
  std::stringstream windowTitle;
  windowTitle << "OSPRay: " << std::setprecision(4) << " render: " << latestFPS
              << " fps, app: " << displayFPS << " fps";
  if (latestFPS > 0.f && latestFPS < 2.f) {
    float progress = currentFrame.progress();
    windowTitle << " | ";
    int barWidth = 20;
    std::string progBar;
    progBar.resize(barWidth + 2);
    auto start = progBar.begin() + 1;
    auto end = start + progress * barWidth;
    std::fill(start, end, '=');
    std::fill(end, progBar.end(), '_');
    *end = '>';
    progBar.front() = '[';
    progBar.back() = ']';
    windowTitle << progBar;
  }

  glfwSetWindowTitle(glfwWindow, windowTitle.str().c_str());
}

void GLFWOSPRayWindow::buildUI()
{
  ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
  ImGui::Begin("press 'g' to hide/show UI", nullptr, flags);

  static int whichScene = 0;
  if (ImGui::Combo("scene##whichScene",
          &whichScene,
          vec_string_getter,
          (void *)&g_scenes,
          g_scenes.size())) {
    scene = g_scenes[whichScene];
    refreshScene(true);
  }

  if (scene == "curves") {
    static int whichCurveVariant = 0;
    if (ImGui::Combo("curveVariant##whichCurveVariant",
            &whichCurveVariant,
            vec_string_getter,
            (void *)&g_curveVariant,
            g_curveVariant.size())) {
      curveVariant = g_curveVariant[whichCurveVariant];
      refreshScene(true);
    }
  }

  if (scene == "unstructured_volume") {
    if (ImGui::Checkbox("show cells", &showUnstructuredCells))
      refreshScene(true);
  }

  static int whichRenderer = 0;
  static int whichDebuggerType = 0;
  if (ImGui::Combo("renderer##whichRenderer",
          &whichRenderer,
          vec_string_getter,
          (void *)&g_renderers,
          g_renderers.size())) {
    rendererTypeStr = g_renderers[whichRenderer];

    if (rendererType == OSPRayRendererType::DEBUGGER)
      whichDebuggerType = 0; // reset UI if switching away from debug renderer

    if (rendererTypeStr == "scivis")
      rendererType = OSPRayRendererType::SCIVIS;
    else if (rendererTypeStr == "pathtracer")
      rendererType = OSPRayRendererType::PATHTRACER;
    else if (rendererTypeStr == "ao")
      rendererType = OSPRayRendererType::AO;
    else if (rendererTypeStr == "debug")
      rendererType = OSPRayRendererType::DEBUGGER;
    else
      rendererType = OSPRayRendererType::OTHER;

    refreshScene();
  }

  if (rendererType == OSPRayRendererType::DEBUGGER) {
    if (ImGui::Combo("debug type##whichDebugType",
            &whichDebuggerType,
            vec_string_getter,
            (void *)&g_debugRendererTypes,
            g_debugRendererTypes.size())) {
      renderer->setParam("method", g_debugRendererTypes[whichDebuggerType]);
      addObjectToCommit(renderer->handle());
    }
  }

  ImGui::Checkbox("cancel frame on interaction", &cancelFrameOnInteraction);

  static int whichAOV = 0;
  if (ImGui::Combo("AOV Display##whichAOV",
          &whichAOV,
          vec_string_getter,
          (void *)&g_AOVs,
          g_AOVs.size())) {
    const auto &aovStr = g_AOVs[whichAOV];
    showDepth = showAlbedo = showPrimID = showGeomID = showInstID = false;
    if (aovStr == "depth")
      showDepth = true;
    if (aovStr == "albedo")
      showAlbedo = true;
    if (aovStr == "primID")
      showPrimID = true;
    if (aovStr == "objID")
      showGeomID = true;
    if (aovStr == "instID")
      showInstID = true;
  }

  ImGui::Separator();
  if (denoiserAvailable) {
    if (ImGui::Checkbox("denoiser", &denoiserEnabled))
      updateFrameOpsNextFrame = true;
  }

  ImGui::Separator();

  // the gaussian pixel filter is the default,
  // which is at position 2 in the list
  static int whichPixelFilter = 2;
  if (ImGui::Combo("pixelfilter##whichPixelFilter",
          &whichPixelFilter,
          vec_string_getter,
          (void *)&g_pixelFilterTypes,
          g_pixelFilterTypes.size())) {
    pixelFilterTypeStr = g_pixelFilterTypes[whichPixelFilter];

    OSPPixelFilterType pixelFilterType =
        OSPPixelFilterType::OSP_PIXELFILTER_GAUSS;
    if (pixelFilterTypeStr == "point")
      pixelFilterType = OSPPixelFilterType::OSP_PIXELFILTER_POINT;
    else if (pixelFilterTypeStr == "box")
      pixelFilterType = OSPPixelFilterType::OSP_PIXELFILTER_BOX;
    else if (pixelFilterTypeStr == "gaussian")
      pixelFilterType = OSPPixelFilterType::OSP_PIXELFILTER_GAUSS;
    else if (pixelFilterTypeStr == "mitchell")
      pixelFilterType = OSPPixelFilterType::OSP_PIXELFILTER_MITCHELL;
    else if (pixelFilterTypeStr == "blackmanHarris")
      pixelFilterType = OSPPixelFilterType::OSP_PIXELFILTER_BLACKMAN_HARRIS;

    rendererPT.setParam("pixelFilter", pixelFilterType);
    rendererSV.setParam("pixelFilter", pixelFilterType);
    rendererAO.setParam("pixelFilter", pixelFilterType);
    rendererDBG.setParam("pixelFilter", pixelFilterType);
    addObjectToCommit(renderer->handle());
  }

  ImGui::Separator();

  if (ImGui::SliderInt("targetFrames", &targetFrames, 0, 64)) {
    framebuffer.setParam("targetFrames", targetFrames);
    addObjectToCommit(framebuffer.handle());
  }

  static int spp = 1;
  if (ImGui::SliderInt("pixelSamples", &spp, 1, 64)) {
    rendererPT.setParam("pixelSamples", spp);
    rendererSV.setParam("pixelSamples", spp);
    rendererAO.setParam("pixelSamples", spp);
    rendererDBG.setParam("pixelSamples", spp);
    addObjectToCommit(renderer->handle());
  }

  static float varianceThreshold = 0.0f;
  if (ImGui::SliderFloat("varianceThreshold", &varianceThreshold, 0.f, .5f)) {
    renderer->setParam("varianceThreshold", varianceThreshold);
    addObjectToCommit(renderer->handle());
  }

  static vec3f bgColorSRGB{0.0f}; // imGUI's widget implicitly uses sRGB
  if (ImGui::ColorEdit3("backgroundColor", bgColorSRGB)) {
    bgColor = vec3f(std::pow(bgColorSRGB.x, 2.2f),
        std::pow(bgColorSRGB.y, 2.2f),
        std::pow(bgColorSRGB.z, 2.2f)); // approximate
    rendererPT.setParam("backgroundColor", bgColor);
    rendererSV.setParam("backgroundColor", bgColor);
    rendererAO.setParam("backgroundColor", bgColor);
    rendererDBG.setParam("backgroundColor", bgColor);
    addObjectToCommit(renderer->handle());
  }

  static bool useTestTex = false;
  if (ImGui::Checkbox("backplate texture", &useTestTex)) {
    if (useTestTex) {
      rendererPT.setParam("map_backplate", backplateTex);
      rendererSV.setParam("map_backplate", backplateTex);
      rendererAO.setParam("map_backplate", backplateTex);
      rendererDBG.setParam("map_backplate", backplateTex);
    } else {
      rendererPT.removeParam("map_backplate");
      rendererSV.removeParam("map_backplate");
      rendererAO.removeParam("map_backplate");
      rendererDBG.removeParam("map_backplate");
    }
    addObjectToCommit(renderer->handle());
  }

  if (ImGui::Checkbox("renderSunSky", &renderSunSky)) {
    if (renderSunSky) {
      sunSky.setParam("direction", sunDirection);
      world.setParam("light", cpp::CopiedData(sunSky));
      addObjectToCommit(sunSky.handle());
    } else {
      cpp::Light light("ambient");
      light.setParam("visible", false);
      light.commit();
      world.setParam("light", cpp::CopiedData(light));
    }
    addObjectToCommit(world.handle());
  }
  if (renderSunSky) {
    if (ImGui::DragFloat3("sunDirection", sunDirection, 0.01f, -1.f, 1.f)) {
      sunSky.setParam("direction", sunDirection);
      addObjectToCommit(sunSky.handle());
      addObjectToCommit(world.handle());
    }
    if (ImGui::DragFloat("turbidity", &turbidity, 0.1f, 1.f, 10.f)) {
      sunSky.setParam("turbidity", turbidity);
      addObjectToCommit(sunSky.handle());
      addObjectToCommit(world.handle());
    }
    if (ImGui::DragFloat(
            "horizonExtension", &horizonExtension, 0.01f, 0.f, 1.f)) {
      sunSky.setParam("horizonExtension", horizonExtension);
      addObjectToCommit(sunSky.handle());
      addObjectToCommit(world.handle());
    }
  }

  if (rendererType == OSPRayRendererType::PATHTRACER) {
    static int lightSamples = -1;
    if (ImGui::SliderInt("lightSamples", &lightSamples, -1, 32)) {
      renderer->setParam("lightSamples", lightSamples);
      addObjectToCommit(renderer->handle());
    }

    static int maxDepth = 20;
    if (ImGui::SliderInt("maxPathLength", &maxDepth, 1, 64)) {
      renderer->setParam("maxPathLength", maxDepth);
      addObjectToCommit(renderer->handle());
    }

    static int rouletteDepth = 1;
    if (ImGui::SliderInt("roulettePathLength", &rouletteDepth, 1, 64)) {
      renderer->setParam("roulettePathLength", rouletteDepth);
      addObjectToCommit(renderer->handle());
    }

    static float minContribution = 0.001f;
    if (ImGui::SliderFloat("minContribution", &minContribution, 0.f, 1.f)) {
      renderer->setParam("minContribution", minContribution);
      addObjectToCommit(renderer->handle());
    }

    static float maxContribution = 10000.f;
    if (ImGui::SliderFloat("maxContribution", &maxContribution, 0.f, 10000.f)) {
      renderer->setParam("maxContribution", maxContribution);
      addObjectToCommit(renderer->handle());
    }

    if (ImGui::SliderFloat("camera motion blur", &cameraMotionBlur, 0.f, 1.f)) {
      updateCamera();
      addObjectToCommit(camera.handle());
    }

    if (cameraMotionBlur > 0.0f) {
      static const char *cameraShutterTypes[] = {"global",
          "rolling right",
          "rolling left",
          "rolling down",
          "rolling up"};
      if (ImGui::BeginCombo("shutterType##cameraShutterType",
              cameraShutterTypes[cameraShutterType])) {
        for (uint32_t n = 0; n < 5; n++) {
          bool is_selected = (cameraShutterType == n);
          if (ImGui::Selectable(cameraShutterTypes[n], is_selected))
            cameraShutterType = (OSPShutterType)n;
          if (is_selected)
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
      }

      updateCamera();
      addObjectToCommit(camera.handle());
    }

    if (cameraShutterType != OSP_SHUTTER_GLOBAL
        && ImGui::SliderFloat(
            "rollingShutterDuration", &cameraRollingShutter, 0.f, 1.0f)) {
      updateCamera();
      addObjectToCommit(camera.handle());
    }
  } else if (rendererType == OSPRayRendererType::SCIVIS) {
    static bool shadowsEnabled = false;
    if (ImGui::Checkbox("shadows", &shadowsEnabled)) {
      renderer->setParam("shadows", shadowsEnabled);
      addObjectToCommit(renderer->handle());
    }

    static bool visibleLights = false;
    if (ImGui::Checkbox("visibleLights", &visibleLights)) {
      renderer->setParam("visibleLights", visibleLights);
      addObjectToCommit(renderer->handle());
    }

    static int aoSamples = 0;
    if (ImGui::SliderInt("aoSamples", &aoSamples, 0, 64)) {
      renderer->setParam("aoSamples", aoSamples);
      addObjectToCommit(renderer->handle());
    }

    static float samplingRate = 1.f;
    if (ImGui::SliderFloat("volumeSamplingRate", &samplingRate, 0.001f, 2.f)) {
      renderer->setParam("volumeSamplingRate", samplingRate);
      addObjectToCommit(renderer->handle());
    }
  } else if (rendererType == OSPRayRendererType::AO) {
    static int aoSamples = 1;
    if (ImGui::SliderInt("aoSamples", &aoSamples, 0, 64)) {
      renderer->setParam("aoSamples", aoSamples);
      addObjectToCommit(renderer->handle());
    }

    static float aoIntensity = 1.f;
    if (ImGui::SliderFloat("aoIntensity", &aoIntensity, 0.f, 1.f)) {
      renderer->setParam("aoIntensity", aoIntensity);
      addObjectToCommit(renderer->handle());
    }

    static float samplingRate = 1.f;
    if (ImGui::SliderFloat("volumeSamplingRate", &samplingRate, 0.001f, 2.f)) {
      renderer->setParam("volumeSamplingRate", samplingRate);
      addObjectToCommit(renderer->handle());
    }
  }

  static const char *cameraStereoModes[] = {
      "none", "left", "right", "side-by-side", "top/bottom"};
  if (ImGui::BeginCombo("camera stereoMode##cameraStereoMode",
          cameraStereoModes[cameraStereoMode])) {
    for (uint32_t n = 0; n < 5; n++) {
      bool is_selected = (cameraStereoMode == n);
      if (ImGui::Selectable(cameraStereoModes[n], is_selected))
        cameraStereoMode = (OSPStereoMode)n;
      if (is_selected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();

    updateCamera();
    addObjectToCommit(camera.handle());
  }

  if (uiCallback) {
    ImGui::Separator();
    uiCallback();
  }

  ImGui::End();
}

void GLFWOSPRayWindow::commitOutstandingHandles()
{
  auto handles = objectsToCommit.consume();
  if (!handles.empty()) {
    for (auto &h : handles)
      ospCommit(h);
    framebuffer.resetAccumulation();
  }
}

void GLFWOSPRayWindow::refreshScene(bool resetCamera)
{
  auto builder = testing::newBuilder(scene);
  testing::setParam(builder, "rendererType", rendererTypeStr);
  if (scene == "curves") {
    testing::setParam(builder, "curveVariant", curveVariant);
  } else if (scene == "unstructured_volume") {
    testing::setParam(builder, "showCells", showUnstructuredCells);
  }
  testing::commit(builder);

  world = testing::buildWorld(builder);
  testing::release(builder);

  switch (rendererType) {
  case OSPRayRendererType::PATHTRACER: {
    renderer = &rendererPT;
    if (renderSunSky)
      world.setParam("light", cpp::CopiedData(sunSky));
    break;
  }
  case OSPRayRendererType::SCIVIS:
    renderer = &rendererSV;
    break;
  case OSPRayRendererType::AO:
    renderer = &rendererAO;
    break;
  case OSPRayRendererType::DEBUGGER:
    renderer = &rendererDBG;
    break;
  default:
    throw std::runtime_error("invalid renderer chosen!");
  }
  // retains a set background color on renderer change
  renderer->setParam("backgroundColor", bgColor);
  addObjectToCommit(renderer->handle());

  world.commit();

  if (resetCamera) {
    // create the arcball camera model
    arcballCamera.reset(
        new ArcballCamera(world.getBounds<box3f>(), windowSize));
    lastXfm = arcballCamera->transform();

    // init camera
    camera.setParam("position", vec3f(0.0f, 0.0f, 1.0f));
    updateCamera();
    addObjectToCommit(camera.handle());
  }
}

void GLFWOSPRayWindow::refreshFrameOperations()
{
  framebuffer.setParam("targetFrames", targetFrames);
  if (denoiserEnabled) {
    cpp::ImageOperation d("denoiser");
    framebuffer.setParam("imageOperation", cpp::CopiedData(d));
  } else {
    framebuffer.removeParam("imageOperation");
  }

  framebuffer.commit();
}
