module SimpleEngine.Editor.App;

import SimpleEngine.Config;
import SimpleEngine.Logging;
import SimpleEngine.Platform.Types;
import SimpleEngine.Subsystems.PlatformSubsystem;
import SimpleEngine.Subsystems.RenderSubsystem;
import std;
import <SDL3/SDL_gpu.h>;


EditorApplication::EditorApplication()
    : Application(EApplicationMode::Editor)
{
}

void EditorApplication::RegisterSubsystems()
{
    Application::RegisterSubsystems();

    // Window 초기화
    if (PlatformSubsystem* platform_subsystem = engine_instance->GetSubsystem<PlatformSubsystem>())
    {
        using namespace se::config;

        const std::filesystem::path solution_path = std::filesystem::current_path().parent_path().parent_path();
        const std::filesystem::path config_path = solution_path / u8"Config/EngineConfig.toml";

        ParseResult result = Config::ReadConfig(config_path);
        if (!result.has_value())
        {
            ConsoleLog(ELogLevel::Error, u8"Failed to read config file: {}", result.error().description());
            return;
        }

        Config& config = result.value();
        platform_subsystem->PrepareWindow(
            config.GetValueOrStore<std::u8string>(u8"window.title", u8"SimpleEngine"),
            config.GetValueOrStore<int32>(u8"window.width", 1280),
            config.GetValueOrStore<int32>(u8"window.height", 720),
            SDL_WINDOW_RESIZABLE
        );

        if (!config.WriteConfig(config_path))
        {
            ConsoleLog(ELogLevel::Error, u8"Failed to write config file: {}", config_path.generic_u8string());
            return;
        }
    }

    // GPU Device 초기화
    if (RenderSubsystem* render_subsystem = engine_instance->RegisterSubsystem<RenderSubsystem>())
    {
        render_subsystem->ConfigureSwapchain(
            SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
            SDL_GPU_PRESENTMODE_MAILBOX
        );
    }
}

void EditorApplication::Render() const
{
    const RenderSubsystem* render_subsystem = engine_instance->GetSubsystem<RenderSubsystem>();
    render_subsystem->RenderFrame();
}

// bool EditorApplication::PostInitialize()
// {
//     Application::PostInitialize();
//
//     SdlSubsystem* sdl_subsystem = engine_instance->GetSubSystem<SdlSubsystem>();
//     cached_window = sdl_subsystem->GetWindow();
//     cached_gpu_device = sdl_subsystem->GetGpuDevice();
//
//     IMGUI_CHECKVERSION();
//     ImGui::CreateContext();
//
//     ImGuiIO& IO = ImGui::GetIO();
//     IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
//     IO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
//     IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
//     IO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows
//
//     ImGui::StyleColorsDark();
//
//     ImGui_ImplSDL3_InitForSDLGPU(cached_window);
//     ImGui_ImplSDLGPU3_InitInfo init_info;
//     init_info.Device = cached_gpu_device;
//     init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(cached_gpu_device, cached_window);
//     init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
//     ImGui_ImplSDLGPU3_Init(&init_info);
//
//     return true;
// }
//
// void EditorApplication::Update(float delta_time)
// {
//     Application::Update(delta_time);
//
//     // Poll and handle events (inputs, window resize, etc.)
//     // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
//     // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
//     // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
//     // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
//     // [If using SDL_MAIN_USE_CALLBACKS: call ImGui_ImplSDL3_ProcessEvent() from your SDL_AppEvent() function]
//     // SDL_Event event;
//     // while (SDL_PollEvent(&event))
//     // {
//     //     ImGui_ImplSDL3_ProcessEvent(&event);
//     //     if (event.type == SDL_EVENT_QUIT)
//     //         done = true;
//     //     if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
//     //         done = true;
//     // }
// }
//
// void EditorApplication::PreRender()
// {
//     Application::PreRender();
//
//     ImGui_ImplSDLGPU3_NewFrame();
//     ImGui_ImplSDL3_NewFrame();
//     ImGui::NewFrame();
// }
//
// void EditorApplication::Render()
// {
//     Application::Render();
//
//     ImGui::Begin("Test");
//     {
//         ImGui::Text("Hello, world!");
//         ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
//         ImGui::Text("Engine average %.3f ms/frame (%.1f FPS)", GetDeltaTime() * 1000.0, 1.0 / GetDeltaTime());
//     }
//     ImGui::End();
// }
//
// void EditorApplication::PostRender()
// {
//     Application::PostRender();
//
//     ImGui::Render();
//
//     ImDrawData* draw_data = ImGui::GetDrawData();
//     const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
//     SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(cached_gpu_device); // Acquire a GPU command buffer
//
//     // Acquire a swapchain texture
//     SDL_GPUTexture* swapchain_texture;
//     SDL_AcquireGPUSwapchainTexture(command_buffer, cached_window, &swapchain_texture, nullptr, nullptr);
//
//     if (swapchain_texture != nullptr && !is_minimized)
//     {
//         // This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
//         Imgui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);
//
//         constexpr SDL_FColor clear_color = {0.25f, 0.25f, 0.25f, 1.0f};
//
//         // Setup and start a render pass
//         SDL_GPUColorTargetInfo target_info = {};
//         target_info.texture = swapchain_texture;
//         target_info.clear_color = SDL_FColor{clear_color.r, clear_color.g, clear_color.b, clear_color.a};
//         target_info.load_op = SDL_GPU_LOADOP_CLEAR;
//         target_info.store_op = SDL_GPU_STOREOP_STORE;
//         target_info.mip_level = 0;
//         target_info.layer_or_depth_plane = 0;
//         target_info.cycle = false;
//         SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);
//
//         // Render ImGui
//         ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
//
//         SDL_EndGPURenderPass(render_pass);
//     }
//
//     // Submit the command buffer
//     SDL_SubmitGPUCommandBuffer(command_buffer);
// }
