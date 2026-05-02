#include "Gui.h"

#define DEFAULT_DPI 96.f

Gui::Gui(const char* InTitle, int InWidth, int InHeight, bool InbUseVSync)
	: Title(InTitle)
	, Width(InWidth)
	, Height(InHeight)
	, bUseVSync(InbUseVSync)
{
	InitializeGui();
}

Gui::Gui()
	: Title(DEFAULT_TITLE)
	, Width(DEFAULT_WIDTH)
	, Height(DEFAULT_HEIGHT)
	, bUseVSync(TRUE)
{
	InitializeGui();
}

Gui::~Gui()
{
	ShutdownGui();
}

void Gui::StartGui()
{
	ImVec4 Clear_Color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	while (!glfwWindowShouldClose(GLFWWindow))
	{
		glfwPollEvents();

		if (glfwGetWindowAttrib(GLFWWindow, GLFW_ICONIFIED) != 0)
		{
			ImGui_ImplGlfw_Sleep(10);
			continue;
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		RenderGui();

		ImGui::Render();

		int DisplayWidth, DisplayHeight;
		glfwGetFramebufferSize(GLFWWindow, &DisplayWidth, &DisplayHeight);
		glViewport(0, 0, DisplayWidth, DisplayHeight);
		glClearColor(Clear_Color.x * Clear_Color.w, Clear_Color.y * Clear_Color.w, Clear_Color.z * Clear_Color.w, Clear_Color.w);
		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(GLFWWindow);
	}
}

void Gui::InitializeGui()
{
	int Error;
	if ((Error = glfwInit()) != GLFW_TRUE)
	{
		printf("glfwInit failed with code %d\n", Error);
		return;
	}

	glfwSetErrorCallback(Gui::ErrorCallback);

	GLFWWindow = glfwCreateWindow(Width, Height, Title, NULL, NULL);

	if (!GLFWWindow)
	{
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(GLFWWindow);
	glfwSwapInterval((int)bUseVSync);

	IMGUI_CHECKVERSION();
	ImGuiContext = ImGui::CreateContext();
	ImGuiViewport = ImGui::GetMainViewport();

	HWND hWnd = glfwGetWin32Window(GLFWWindow);
	float DPIScale = (float)GetDpiForWindow(hWnd) / DEFAULT_DPI;

	ImGuiStyle& Style = ImGui::GetStyle();
	Style.ScaleAllSizes(DPIScale);

	ImGui::StyleColorsDark();

	ImGuiIO& IO = ImGui::GetIO();
	IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	IO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	IO.FontGlobalScale = DPIScale;

	ImGui_ImplGlfw_InitForOpenGL(GLFWWindow, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
	ImGui_ImplOpenGL3_Init();

	// https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation

	const char* FENPosition =
		"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
		// "rnbqkbnr/8/8/8/8/8/PPPPPPPP/RNBQK2R w KQkq - 0 1"
		// "8/8/8/4k3/8/8/8/RNBQ1BP1 w - - 0 1"
		// "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"
		// "1B6/2n5/p1N1P2R/P1K3N1/4Pk2/1Q2p2p/6nP/1B4R1 w - - 0 1"
		// "K6k/8/8/8/8/8/p7/8 b - - 0 1"
		// "8/P7/8/8/8/8/8/k6K w - - 0 1"
		// "8/8/8/8/8/8/5k2/6K1 w - - 149 75"
		// "k7/8/8/8/8/8/6Q1/7K b - - 12 63"
		;

	ChessEngine = std::make_unique<::ChessEngine>(FENPosition);
}

void Gui::ShutdownGui()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(GLFWWindow);
	glfwTerminate();
}

void Gui::RenderGui()
{
	ImGui::SetNextWindowPos(ImGuiViewport->Pos);
	ImGui::SetNextWindowSize(ImGuiViewport->Size);

	ImGuiWindowFlags WindowFlags =
		ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoBringToFrontOnFocus; // so we aren't able to bring the board window to front while promoting

	if (ImGui::Begin(Title, NULL, WindowFlags))
	{
		ChessEngine->Update();
		ChessEngine->Draw();
	}
	ImGui::End();

	// ImGui::ShowDemoWindow();
}

void Gui::ErrorCallback(int error_code, const char* description)
{
	fprintf(stderr, "GLFW error: %d (%s)\n", error_code, description);
}
