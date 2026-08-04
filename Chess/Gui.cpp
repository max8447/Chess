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
		// "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
		// "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"
		// "6kr/3q1b2/2n1p3/4N3/Q4P2/3B4/8/R5K1 w - - 0 1"
		// "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 "
		// "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
		// "r3k2r/p1ppqpb1/1n2pnp1/1b1PN3/1p2P3/2N2Q1p/PPPBBPPP/1R2K2R w Kkq - 2 2"
		"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"
		// "rnRq1k1r/pp2bppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R b KQ - 0 8"
		// "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10"
		// "4k3/4r3/8/8/8/8/4B3/4K3 w - - 0 1"
		// "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
		// "rnbqkbnr/pppppppp/8/8/8/2N5/PPPPPPPP/R1BQKBNR b KQkq - 1 1"
		// "rnbqk1nr/pppp1ppp/8/4P3/1b6/8/PPP1PPPP/RNBQKBNR w KQkq - 1 3"
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
