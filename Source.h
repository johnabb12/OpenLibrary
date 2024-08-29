#include "imgui.h"
#include <iostream>
#include <ctime>
#include <d3d11.h>
#include "Openlibrary.h"

HWND hwnd;
RECT rc;

#define WIDTH  1007 // Loader Size X
#define HEIGHT 630 // Loader Size Y

using namespace ImGui;

namespace c_gui
{
    class gui
    {

    public:

        DWORD window_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus;
        ImGuiStyle& style = ImGui::GetStyle();

        void begin(const char* name, int x, int y, float oppacity)
        {

            ImGui::SetNextWindowSize(ImVec2(x, y));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(15.0 / 255.0, 15.0 / 255.0, 15.0 / 255.0, oppacity));
            ImGui::Begin(name, NULL, window_flags);
            ImGui::PopStyleColor();
        }

        void end()
        {
            ImGui::End();
        }

        void setpos(float X, float Y)
        {
            SetCursorPos({ X, Y });
        }

        void cText(const char* text, float posX, float posY, ImVec4 color, float op)
        {
            setpos(posX, posY);
            ImGui::TextColored(color, text);
        }

        void CEtext(std::string text, float posY) {
            auto windowWidth = ImGui::GetWindowSize().x;
            auto textWidth = ImGui::CalcTextSize(text.c_str()).x;

            ImGui::SetCursorPos(ImVec2((windowWidth - textWidth) * 0.5f, posY));
            ImGui::Text(text.c_str());
        }

        void mw() { //makes possible moving the window

            GetWindowRect(hwnd, &rc);

            if (ImGui::GetWindowPos().x != 0 || ImGui::GetWindowPos().y != 0)
            {
                MoveWindow(hwnd, rc.left + ImGui::GetWindowPos().x, rc.top + ImGui::GetWindowPos().y, WIDTH, HEIGHT, TRUE);
                ImGui::SetWindowPos(ImVec2(0.f, 0.f));
            }

        }

    };
}
