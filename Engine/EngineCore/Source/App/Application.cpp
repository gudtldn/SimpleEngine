module SimpleEngine.App;
import std;

double Application::CurrentTime = 0.0;
double Application::LastTime = 0.0;
double Application::DeltaTime = 1.0 / 60.0;
double Application::FixedDeltaTime = 1.0 / 60.0;


void Application::Startup(const wchar_t* cmd_line)
{
    std::println("startup");
}

void Application::Shutdown()
{
    std::println("shutdown");
}
