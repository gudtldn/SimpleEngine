module SimpleEngine.App;
import SimpleEngine.Utils;


double Application::CurrentTime = 0.0;
double Application::LastTime = 0.0;
double Application::DeltaTime = 1.0 / 60.0;
double Application::FixedDeltaTime = 1.0 / 60.0;


void Application::Startup(const char* cmd_line)
{
    Startup(se::string_utils::ToU8String(cmd_line));
}

void Application::Startup(const wchar_t* cmd_line)
{
    Startup(se::string_utils::ToU8String(cmd_line));
}

void Application::Startup(const std::u8string& cmd_line)
{
    const char* cmd = reinterpret_cast<const char*>(cmd_line.c_str());
    std::println("startup, cmd: {}", cmd);
}

void Application::Shutdown()
{
    std::println("shutdown");
}
