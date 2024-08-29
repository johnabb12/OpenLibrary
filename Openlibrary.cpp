#include "Openlibrary.h"
#include "textures.h"




namespace texture {
    // UI textures
    ID3D11ShaderResourceView* Launch_Background_Data_texture = nullptr;
    ID3D11ShaderResourceView* OpenLibrary_Background_Data_texture = nullptr;
    ID3D11ShaderResourceView* Minimize_Logo_White_Data_texture = nullptr;
    ID3D11ShaderResourceView* Minimize_Logo_Black_Data_texture = nullptr;
    ID3D11ShaderResourceView* Close_Logo_White_Data_texture = nullptr;
    ID3D11ShaderResourceView* Close_Logo_Black_Data_texture = nullptr;

}


// Utility function to convert a string to lowercase
std::string toLower(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
    return lower_str;
}

// Tell if the library has to be launch or no
bool Openlibrary::isOpenLibraryLaunched()
{
    return launch; 
}

void Openlibrary::InitializeTextures()
{
    // we're settings up all the drawing texture that our application is going to need
    HRESULT result_Launch_Background_Data_texture = D3DX11CreateShaderResourceViewFromMemory(p_Device, Launch_Background_Data, sizeof(Launch_Background_Data), nullptr, nullptr, &texture::Launch_Background_Data_texture, nullptr);
    HRESULT result_OpenLibrary_Background_Data_texture = D3DX11CreateShaderResourceViewFromMemory(p_Device, OpenLibrary_Background_Data, sizeof(OpenLibrary_Background_Data), nullptr, nullptr, &texture::OpenLibrary_Background_Data_texture, nullptr);
    HRESULT result_Minimize_Logo_White_Data_texture = D3DX11CreateShaderResourceViewFromMemory(p_Device, Minimize_Logo_White_Data, sizeof(Minimize_Logo_White_Data), nullptr, nullptr, &texture::Minimize_Logo_White_Data_texture, nullptr);
    HRESULT result_Minimize_Logo_Black_Data_texture = D3DX11CreateShaderResourceViewFromMemory(p_Device, Minimize_Logo_Black_Data, sizeof(Minimize_Logo_Black_Data), nullptr, nullptr, &texture::Minimize_Logo_Black_Data_texture, nullptr);
    HRESULT result_Close_Logo_Black_Data_texture = D3DX11CreateShaderResourceViewFromMemory(p_Device, Close_Logo_Black_Data, sizeof(Close_Logo_Black_Data), nullptr, nullptr, &texture::Close_Logo_Black_Data_texture, nullptr);
    HRESULT result_Close_Logo_White_Data_texture = D3DX11CreateShaderResourceViewFromMemory(p_Device, Close_Logo_White_Data, sizeof(Close_Logo_White_Data), nullptr, nullptr, &texture::Close_Logo_White_Data_texture, nullptr);

    // we're checking if textures we're loaded correctly
    IM_ASSERT(SUCCEEDED(result_Launch_Background_Data_texture));
    IM_ASSERT(SUCCEEDED(result_OpenLibrary_Background_Data_texture));
    IM_ASSERT(SUCCEEDED(result_Minimize_Logo_White_Data_texture));
    IM_ASSERT(SUCCEEDED(result_Minimize_Logo_Black_Data_texture));
    IM_ASSERT(SUCCEEDED(result_Close_Logo_Black_Data_texture));
    IM_ASSERT(SUCCEEDED(result_Close_Logo_White_Data_texture));
}
Openlibrary::Openlibrary(ID3D11Device* p_Device, HWND window)
    : p_Device(p_Device), window(window), launch(false), favoritefilter(false)
{
    //Calling texture initialize in our constructor
    InitializeTextures();
    // loading from the config file the previous book we saved in our config file
    books = OpenLibraryAPI::loadConfig();
    // Launch threads to fetch book data and summary
    std::thread fetchDataThread(OpenLibraryAPI::fetchBookDataThread, std::ref(isbn_thread), std::ref(bookDataJson), std::ref(mtx_bookinfo), std::ref(isSSLCalled_bookinfo), std::ref(isTaskFinished_bookinfo));
    std::thread fetchSummaryThread(OpenLibraryAPI::fetchBookSummaryThread, std::ref(isbn_thread), std::ref(bookSummaryJson), std::ref(mtx_summary), std::ref(isSSLCalled_summary), std::ref(isTaskFinished_summary));

    // we detach these thread to make them working 24/24 
    fetchDataThread.detach();
    fetchSummaryThread.detach();
}

void Openlibrary::FavoriteBooksButton()
{
    ImGui::SetCursorPos(ImVec2(150, 71)); // ButtonPosition
    ImGui::PushFont(MontSerraBold20_texture);
    if (ImGui::Button((favoritefilter) ? "All Books" : "Favorite Books", ImVec2(200, 50)))
    {
        favoritefilter = !favoritefilter; //  everytime Favorite Books Button will be clicked it will define favoritefilter boolean to his opposite
    }
    ImGui::PopFont();

}

void Openlibrary::AddABookButton()
{
    
    ImGui::SetCursorPos(ImVec2(150, 156)); // ButtonPosition
    ImGui::PushFont(MontSerraBold20_texture);
    if (ImGui::Button("Add a Books", ImVec2(200, 50)))
    {
        focus_input = true;
        ImGui::OpenPopup("Enter a valid ISBN");
    }
    // if the Add a Books button is clicked we will open a Popup with an input text to let the user write his ISBN

    if (ImGui::BeginPopupModal("Enter a valid ISBN", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        // Force keyboard focus when we open the popup
        if (focus_input)
        {
            ImGui::SetKeyboardFocusHere();
            focus_input = false;
        }

        ImGui::InputText("ISBN", isbn, IM_ARRAYSIZE(isbn));

        // Boutons pour valider ou annuler
        if (ImGui::Button("OK", ImVec2(150, 0)))
        {
           
            // Checking if this ISBN is not already in our books data
            if (OpenLibraryAPI::isBookInCollection(books, isbn)) {
                ImGui::Text("Book already exist !");
            } 
            else
            {
                isbn_thread = isbn; // settings the isbn_thread variable (variable that fetchDataThread & fetchSummaryThread are reading to make the HTTP request to openlibrary website)
                isSSLCalled_bookinfo.store(true);
                isSSLCalled_summary.store(true);
                
                ImGui::CloseCurrentPopup();
            }


            
            
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(150, 0)))
        {
            ImGui::CloseCurrentPopup();
            open_popup = false;
        }

        ImGui::EndPopup();
    }
    ImGui::PopFont();
}

void Openlibrary::AddANote()
{
    // If addnote is true, display the popup
    if (addnote) {
        ImGui::OpenPopup("Add Note");

        if (ImGui::BeginPopupModal("Add Note", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {

            // Multiline text input for entering the note
            ImGui::InputTextMultiline("##note", noteBuffer, sizeof(noteBuffer), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 10));

            // Button to submit the note
            if (ImGui::Button("Submit", ImVec2(120, 0))) {
                OpenLibraryAPI::addPersonalNote(books, books[selected_index].title, noteBuffer);
                OpenLibraryAPI::saveConfig(books);
                addnote = false;  // Close the popup
                noteID = -1;
                memset(noteBuffer, 0, sizeof(noteBuffer));
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();

            // Button to cancel the note entry
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                addnote = false;  // Close the popup
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
}


void Openlibrary::ShowBookTable() {
    ImGuiStyle& style = ImGui::GetStyle();
    const std::string actualsearch = search;
    ImVec2 oldItemSpacing = style.ItemSpacing;
    ImVec2 oldFramePadding = style.FramePadding;

    // Convert the search string to lowercase
    std::string search_lower = toLower(actualsearch);

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(121, 121, 121, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(121, 121, 121, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(121, 121, 121, 0));
    ImGui::SetCursorPosX(23);
    ImGui::SetCursorPosY(-15);

    if (ImGui::BeginTable("Books", 5, ImGuiTableFlags_BordersInnerH)) {
        // Set column headers with specific sizes
        ImGui::TableSetupColumn("##ISBN", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("##Book Name", ImGuiTableColumnFlags_WidthFixed, 243.0f);
        ImGui::TableSetupColumn("##Author", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("##Subject", ImGuiTableColumnFlags_WidthFixed, 220.0f);
        ImGui::TableSetupColumn("##Action", ImGuiTableColumnFlags_WidthFixed, 230.0f);

        ImGui::TableHeadersRow();
        ImGui::PopStyleColor(3);

        if (favoritefilter) {
            for (int i = 0; i < books.size(); ++i) {
                const auto& book = books[i];
                // Convert book title, author, and subjects to lowercase for comparison
                std::string title_lower = toLower(book.title);
                std::string author_lower = toLower(book.author);
                std::string subjects_lower = toLower(book.subjects);

                if (book.is_favorite && (title_lower.find(search_lower) != std::string::npos ||
                    author_lower.find(search_lower) != std::string::npos ||
                    subjects_lower.find(search_lower) != std::string::npos)) {
                    ImGui::TableNextRow();

                    // Select the entire row
                    ImGui::TableNextColumn();
                    ImGui::PushID(i);  // Use PushID to avoid ID conflicts with other widgets
                    ImGui::Spacing();

                    if (ImGui::Selectable(book.isbn.c_str(), selected_index == i, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick, ImVec2(815, 20))) {
                        selected_index = i;
                        book_popup = true;
                    }

                    ImGui::TableNextColumn();
                    ImGui::Spacing();
                    ImGui::Text("%s", book.title.c_str());

                    ImGui::TableNextColumn();
                    ImGui::Spacing();
                    ImGui::Text("%s", book.author.c_str());

                    ImGui::TableNextColumn();
                    ImGui::Spacing();
                    ImGui::Text("%s", book.subjects.c_str());

                    // Action buttons in the last column
                    ImGui::TableNextColumn();
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 87.0f / 255.0f, 200.0f / 255.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 77.0f / 255.0f, 240.0f / 255.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 20.0f / 255.0f, 190.0f / 255.0f, 1.0f));

                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5)); // Adjust padding here
                    if (ImGui::Button("Note", ImVec2(70, 30))) {
                        addnote = true;
                        strncpy_s(noteBuffer, sizeof(noteBuffer), book.personal_note.c_str(), _TRUNCATE); // Set the note buffer to the actual personal note
                        noteID = i;
                    }
                    ImGui::PopStyleVar();
                    ImGui::SameLine();
                    ImGui::PopStyleColor(3);

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(230.0f / 255.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(190.0f / 255.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5)); // Adjust padding here
                    if (ImGui::Button((books[i].is_favorite ? "UnFavorite" : "Favorite"), ImVec2(70, 30))) {
                        OpenLibraryAPI::markAsFavorite(books, books[i].title);
                        OpenLibraryAPI::saveConfig(books);
                    }
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor(3);

                    ImGui::PopID();
                }
            }
        }
        else {
            for (int i = 0; i < books.size(); ++i) {
                const auto& book = books[i];
                // Convert book title, author, and subjects to lowercase for comparison
                std::string title_lower = toLower(book.title);
                std::string author_lower = toLower(book.author);
                std::string subjects_lower = toLower(book.subjects);

                if (title_lower.find(search_lower) != std::string::npos ||
                    author_lower.find(search_lower) != std::string::npos ||
                    subjects_lower.find(search_lower) != std::string::npos) {

                    ImGui::TableNextRow();

                    // Select the entire row
                    ImGui::TableNextColumn();
                    ImGui::PushID(i);  // Use PushID to avoid ID conflicts with other widgets
                    ImGui::Spacing();

                    if (ImGui::Selectable(book.isbn.c_str(), selected_index == i, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick, ImVec2(815, 20))) {
                        selected_index = i;
                        book_popup = true;
                    }

                    ImGui::TableNextColumn();
                    ImGui::Spacing();
                    ImGui::Text("%s", book.title.c_str());

                    ImGui::TableNextColumn();
                    ImGui::Spacing();
                    ImGui::Text("%s", book.author.c_str());

                    ImGui::TableNextColumn();
                    ImGui::Spacing();
                    ImGui::Text("%s", book.subjects.c_str());

                    // Action buttons in the last column
                    ImGui::TableNextColumn();
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 87.0f / 255.0f, 200.0f / 255.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 77.0f / 255.0f, 240.0f / 255.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 20.0f / 255.0f, 190.0f / 255.0f, 1.0f));

                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5)); // Adjust padding here
                    if (ImGui::Button("Note", ImVec2(70, 30))) {
                        addnote = true;
                        strncpy_s(noteBuffer, sizeof(noteBuffer), book.personal_note.c_str(), _TRUNCATE); // Set the note buffer to the actual personal note
                        noteID = i;
                    }
                    ImGui::PopStyleVar();
                    ImGui::SameLine();
                    ImGui::PopStyleColor(3);

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(230.0f / 255.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(190.0f / 255.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 5)); // Adjust padding here
                    if (ImGui::Button((books[i].is_favorite ? "UnFavorite" : "Favorite"), ImVec2(70, 30))) {
                        OpenLibraryAPI::markAsFavorite(books, books[i].title);
                        OpenLibraryAPI::saveConfig(books);
                    }
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor(3);

                    ImGui::PopID();
                }
            }
        }

        ImGui::EndTable();
    }
}





void Openlibrary::BookPopUp()
{
    if (book_popup)
    {
        ImGui::SetNextWindowFocus();

        ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("BOOK INFORMATION", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse  | ImGuiWindowFlags_AlwaysAutoResize)) {


            ImGui::BeginChild("ScrollingRegion", ImVec2(400, 400), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

            ImGui::SetKeyboardFocusHere();

            ImGui::PushFont(MontSerraBold24_texture);
            ImGui::Text(books[selected_index].title.c_str());
            ImGui::PopFont();

            ImGui::PushFont(MontSerraBold11_texture);
            ImGui::Text("Author : "); ImGui::SameLine();
            ImGui::PushFont(MontSerraMedium11_texture);
            ImGui::Text(books[selected_index].author.c_str());
            ImGui::PopFont(); ImGui::PopFont();

            ImGui::PushFont(MontSerraBold11_texture);
            ImGui::Text("Publish Date : "); ImGui::SameLine();
            ImGui::PushFont(MontSerraMedium11_texture);
            ImGui::Text(books[selected_index].publish_date.c_str());
            ImGui::PopFont(); ImGui::PopFont();

            ImGui::PushFont(MontSerraBold11_texture);
            ImGui::Text("Number of Pages : "); ImGui::SameLine();
            ImGui::PushFont(MontSerraMedium11_texture);
            ImGui::Text(std::to_string(books[selected_index].number_of_pages).c_str());
            ImGui::PopFont(); ImGui::PopFont();

            ImGui::PushFont(MontSerraBold11_texture);
            ImGui::Text("Description : "); ImGui::SameLine();
            ImGui::PushFont(MontSerraMedium11_texture);
            ImGui::Text(books[selected_index].description.c_str());
            ImGui::PopFont(); ImGui::PopFont();

            ImGui::PushFont(MontSerraBold11_texture);
            ImGui::Text("Subjects : "); ImGui::SameLine();
            ImGui::PushFont(MontSerraMedium11_texture);
            ImGui::Text(books[selected_index].subjects.c_str());
            ImGui::PopFont(); ImGui::PopFont();

            ImGui::PushFont(MontSerraBold11_texture);
            ImGui::Text("Summary : "); ImGui::SameLine();
            ImGui::PushFont(MontSerraMedium11_texture);
            ImGui::TextWrapped(books[selected_index].summary.c_str());
            ImGui::PopFont(); ImGui::PopFont();

            ImGui::PushFont(MontSerraBold11_texture);
            ImGui::Text("Personal Note : "); ImGui::SameLine();
            ImGui::PushFont(MontSerraMedium11_texture);
            ImGui::TextWrapped(books[selected_index].personal_note.c_str());
            ImGui::PopFont(); ImGui::PopFont();

            ImGui::EndChild();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(255 / 255.0f, 0 / 255.0f, 0 / 255.0f, 255 / 255.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(230 / 255.0f, 0 / 255.0f, 0 / 255.0f, 255 / 255.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(180 / 255.0f, 0 / 255.0f, 0 / 255.0f, 255 / 255.0f));
            if (ImGui::Button("Remove Books", ImVec2(195, 0))) {
                books.erase(books.begin() + selected_index);
                OpenLibraryAPI::saveConfig(books);

                book_popup = false;
            }
            ImGui::SameLine();
            ImGui::PopStyleColor(3);
            if (ImGui::Button("Cancel", ImVec2(195, 0))) {

                book_popup = false;
            }

            ImGui::End();
        }
    }
}

void Openlibrary::WindowMisc()
{
    // we push color style
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
    if (!launch)
    {
        // set the misc button of the tab in white if we're still in the launcher
        if (ImageButton(texture::Close_Logo_White_Data_texture, ImVec2(15, 15), ImVec2(973, 2)))
            exit(0);
            
        
        if (ImageButton(texture::Minimize_Logo_White_Data_texture, ImVec2(15, 3), ImVec2(948, 8)))
            ShowWindow(window, SW_MINIMIZE);// function of windows API with the flags SW_MINIMIZE to minimize the window when we call it
    }
    else
    {
        // set the misc button of the tab in black if we're in the application
        if (ImageButton(texture::Close_Logo_Black_Data_texture, ImVec2(15, 15), ImVec2(973, 2)))
            exit(0);

        if (ImageButton(texture::Minimize_Logo_Black_Data_texture, ImVec2(15, 3), ImVec2(948, 8)))
            ShowWindow(window, SW_MINIMIZE); // function of windows API with the flags SW_MINIMIZE to minimize the window when we call it
    }
    // we pop color style
    ImGui::PopStyleColor(3);
}

// function to display image by texture size and position
void Openlibrary::DisplayImage(ID3D11ShaderResourceView* image_texture, ImVec2 imageSize, ImVec2 position) {

    ImGui::SetCursorPos(position);
    ImGui::Image((void*)image_texture, imageSize);
}

// function to use ImageButton
bool Openlibrary::ImageButton(ID3D11ShaderResourceView* image_texture, ImVec2 size, ImVec2 position) {
    ImGui::SetCursorPos(position);
    if (ImGui::ImageButton((void*)image_texture, size))
        return true;
    return false;
}

// Search book function we're retrieving the word of the user in the variable "search"
void Openlibrary::SearchBook()
{
    ImGui::SetCursorPos(ImVec2(650, 122));
    ImGui::SetNextItemWidth(260); // set width typing size of the inputText
    ImGui::InputTextWithHint("##SearchBook", "Search Book By Title\\Author\\Subjects", search, sizeof(search));
}

void Openlibrary::RenderOpenLibraryLauncher()
{
    // displaying the image background of the launcher
    DisplayImage(texture::Launch_Background_Data_texture, ImVec2(ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y), ImVec2(0, 0));
    // calling the Window misc
    WindowMisc();

    ImGui::SetCursorPos(ImVec2(328,418));
    ImGui::PushFont(MontSerraBold20_texture);

    // if LAUNCH button is clicked we're settings "launch" boolean to true and the program will call RenderOpenlibrary()
    if (ImGui::Button("LAUNCH",ImVec2(350,60)))
    {
        launch = true;
    }
    ImGui::PopFont();
}

void Openlibrary::RenderOpenlibrary() {
    // displaying the image background of the application
    DisplayImage(texture::OpenLibrary_Background_Data_texture, ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight()), ImVec2(0, 0));
    // calling the Window misc
    WindowMisc();
    // calling the Favorite Books Button
    FavoriteBooksButton();
    // calling the Add A Book Button
    AddABookButton();
    ImGui::SetCursorPos(ImVec2(5, 218));
    ImGui::PushFont(MontSerraBold24_texture);

    // rendering the texte above the BookList to know if we're actually displaying the list of non favorite books or if the list is displaying only favorite books
    // if favoritefilter == true -> we render the Text "LIST OF FAVORITE BOOKS" if favoritefilter == false  we rendering the text "LIST OF BOOKS"
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), (favoritefilter ? "LIST OF FAVORITE BOOKS" : "LIST OF BOOKS")); 
    ImGui::PopFont();

    ImGui::PushFont(InterMedium13_texture);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));

    SearchBook();

    ImGui::PopStyleColor();
    ImGui::PopFont();
    // calling the book pop up when we want to see more detail of the book in the list
    BookPopUp();
    
    ImGui::PushFont(InterMedium13_texture);

    // settings up the size and pos in the window of the parent invisible window
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetWindowWidth(), 387), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(5, 265));

    // creating the parent invisible window
    ImGui::Begin("Books Displaying Parent", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
    {
        //settings the position of the child window to x 0 y 0
        ImGui::SetCursorPos(ImVec2(0, 0));

        // creating child window
        ImGui::BeginChild("Books displaying Child", ImVec2(ImGui::GetWindowWidth() - 15, 365), ImGuiWindowFlags_AlwaysVerticalScrollbar, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
        {
            // calling ShowBookTable to diplay all the book list inside of it
            ShowBookTable();
        }
        ImGui::EndChild();
    }
    ImGui::End();

    // calling Add A note button
    AddANote();
    ImGui::PopFont();
    // if the atomic boolean of the thread is telling us that he finished to make the HTTP request theisTaskFinished_bookinfo && isTaskFinished_summary are going to be True
    if (isTaskFinished_bookinfo.load() && isTaskFinished_summary.load())
    {
        if (!bookDataJson.empty() && !bookSummaryJson.empty()) {
            try {
                //parsing book data in our variable and saving into the config file
                Book book = OpenLibraryAPI::parseBookData(isbn, bookDataJson);
                if (book.author.empty() || book.title.empty())
                {
                    showErrorPopup = true;
                }
                else
                {
                    OpenLibraryAPI::addBookSummary(book, bookSummaryJson);
                    books.push_back(book);
                    OpenLibraryAPI::saveConfig(books);
                }

            }
            catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
        else {
            std::cerr << "Failed to fetch book data or summary." << std::endl;
        }
        // set up every atomic variable to false to avoid the thread to make new useless http request
        isTaskFinished_bookinfo.store(false);
        isTaskFinished_summary.store(false);
        isSSLCalled_bookinfo.store(false);
        isSSLCalled_summary.store(false);
        memset(isbn, 0, sizeof(isbn));
    }
    if (showErrorPopup)
    {
        ImGui::OpenPopup("Error");

        if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("This ISBN doesn't exist");

            if (ImGui::Button("OK"))
            {
                ImGui::CloseCurrentPopup();
                showErrorPopup = false;
            }

            ImGui::EndPopup();
        }
    }
}
// color them
void Openlibrary::Theme()
{
    ImGuiStyle* style = &ImGui::GetStyle();
    ImGui::GetStyle().WindowRounding = 5.0f;// <- Set this on init or use ImGui::PushStyleVar()
    ImGui::GetStyle().ChildRounding = 5.0f;// <- Set this on init or use ImGui::PushStyleVar()
    ImGui::GetStyle().FrameRounding = 5.0f;
    ImGui::GetStyle().FramePadding.y = 11.0f; // frame padding for SearchBook InputText
    style->Colors[ImGuiCol_Text] = ImColor(0, 0, 0, 255);

    style->Colors[ImGuiCol_Button] = ImColor(255, 255, 255, 255);
    style->Colors[ImGuiCol_ButtonActive] = ImColor(240, 240, 240, 255);
    style->Colors[ImGuiCol_ButtonHovered] = ImColor(190, 190, 190, 255);
    
    style->Colors[ImGuiCol_WindowBg] = ImColor(0, 0, 0, 255);
    style->Colors[ImGuiCol_ChildBg] = ImColor(255, 255, 255, 255);

    style->Colors[ImGuiCol_TableHeaderBg] = ImColor(255, 255, 255, 0);

    style->Colors[ImGuiCol_FrameBg] = ImColor(255, 255, 255, 255);
    style->Colors[ImGuiCol_FrameBgActive] = ImColor(71, 71, 71, 0);
    style->Colors[ImGuiCol_FrameBgHovered] = ImColor(41, 40, 41, 0);



    style->Colors[ImGuiCol_Header] = ImColor(121, 121, 121, 255);
    style->Colors[ImGuiCol_HeaderActive] = ImColor(111, 111, 111, 255);
    style->Colors[ImGuiCol_HeaderHovered] = ImColor(100, 100, 100, 255);

    style->Colors[ImGuiCol_PopupBg] = ImColor(255,255, 255,255);
    style->Colors[ImGuiCol_TitleBgActive] = ImColor(255, 255, 255, 255);
}


