#pragma once

#include "Includes.h"

#include "ChessEngine.h"

#define DEFAULT_TITLE "Chess"
#define DEFAULT_WIDTH 1280
#define DEFAULT_HEIGHT 720

class Gui
{
private:

	// Window properties
	const char*			Title;
	int					Width;
	int					Height;


	// GLFW properties
	GLFWwindow*			GLFWWindow;
	bool				bUseVSync;


	// ImGui properties
	ImGuiContext*		ImGuiContext;
	ImGuiViewport*		ImGuiViewport;


	// ChessEngine properties
	std::unique_ptr<ChessEngine> ChessEngine;

public:

	Gui(const char* InTitle, int InWidth, int InHeight, bool InbUseVSync = true);
	Gui();
	~Gui();

	void StartGui();

private:

	void InitializeGui();
	void ShutdownGui();
	void RenderGui();

private:

	static void ErrorCallback(int error_code, const char* description);
};
