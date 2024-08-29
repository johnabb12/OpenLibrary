#pragma once
#include <string>

#include <d3dx11tex.h>
#pragma comment(lib, "D3DX11.lib")
#include <windows.h>
#include <imgui.h>
#include <vector>
#include "OpenlibraryAPI.h"
#include <mutex>
#include <atomic>
#include <thread>

class Openlibrary
{
private:
	ID3D11Device* p_Device;
	HWND window;
	char 
		search[30] = "", // variable used in the Search function
		isbn[15] = "", // variable used in the add a book function
		noteBuffer[1024] = "\0";
	int 
		selected_index = -1, 
		noteID = -1;
	bool
		launch = false,
		favoritefilter = false,
		open_popup = false,
		addnote = false,
		focus_input = false,
		book_popup = false,
		showErrorPopup = false;
	// bookDataJson  & bookSummaryJson are the variable that the thread will stock the info of the openlibrary api request
	std::string 
		bookDataJson,
		bookSummaryJson,
		isbn_thread;
	//atomic boolean to communicate with our thread
	std::atomic<bool> 
		isSSLCalled_summary{ false },
		isSSLCalled_bookinfo{ false },
		isTaskFinished_summary{ false },
		isTaskFinished_bookinfo{ false };
	//mutex variable for the both thread
	std::mutex 
		mtx_summary,
		mtx_bookinfo;
	// list of the books
	std::vector<Book> books;
public:
	// font that we will use
	ImFont
		*MontSerraBold24_texture = nullptr,
		*MontSerraBold20_texture = nullptr,
		*MontSerraBold14_texture = nullptr,
		*MontSerraBold11_texture = nullptr,
		*MontSerraMedium11_texture = nullptr,
		*InterMedium13_texture = nullptr;

	bool isOpenLibraryLaunched();
	void InitializeTextures();

	Openlibrary(ID3D11Device* p_Device, HWND window);

	void FavoriteBooksButton();
	void AddABookButton();
	void AddANote();
	void ShowBookTable();
	void BookPopUp();
	void WindowMisc();
	void DisplayImage(ID3D11ShaderResourceView* image_texture, ImVec2 imageSize, ImVec2 position);
	bool ImageButton(ID3D11ShaderResourceView* image_texture, ImVec2 size, ImVec2 position);
	void SearchBook();
	void Theme();
	void RenderOpenLibraryLauncher();
	void RenderOpenlibrary();
};

