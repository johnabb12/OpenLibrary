#include "OpenlibraryAPI.h"
// we set up this flags before including http lib so we're able to use OPENSSL
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

void OpenLibraryAPI::fetchBookDataThread(std::string& isbn, std::string& result, std::mutex& mtx, std::atomic<bool>& isSSLCalled, std::atomic<bool>& isTaskFinished) {
    while (true) 
    {
        // Sleep for 300 milliseconds to limit CPU usage since the thread is running continuously.
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        // Check if an SSL call should be made.
        if (isSSLCalled.load()) {
            // Create an SSL client to communicate with openlibrary.org.
            httplib::SSLClient cli("openlibrary.org");

            // Construct the URL for the API request using the provided ISBN.
            std::string url = "/api/books?bibkeys=ISBN:" + isbn + "&format=json&jscmd=data";
            // Perform the GET request.
            auto res = cli.Get(url.c_str());

            // If the response is successful (status code 200), update the result.
            if (res && res->status == 200) {
                // Lock the mutex to safely update the result string.
                std::lock_guard<std::mutex> lock(mtx);
                result = res->body;
            }
            else {
                std::cerr << "HTTP request failed: " << (res ? res->status : -1) << std::endl;
            }
                isTaskFinished.store(true); // Tell that our task is finished
                isSSLCalled.store(false); // set back isSSLCalled to false to avoid any duplicated call in the thread

        }
    }
    
    
    
}

void OpenLibraryAPI::fetchBookSummaryThread(std::string& isbn, std::string& result, std::mutex& mtx, std::atomic<bool>& isSSLCalled, std::atomic<bool>& isTaskFinished) {
    while (true) 
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(300)); // Sleep to reduce CPU usage.

        if (isSSLCalled.load()) { // Check if an SSL call is required.
            httplib::SSLClient cli("openlibrary.org"); // Create an SSL client to connect to OpenLibrary.

            std::string url = "/api/books?bibkeys=ISBN:" + isbn + "&jscmd=details&format=json"; // Construct the URL.
            auto res = cli.Get(url.c_str()); // Perform the GET request.

            if (res && res->status == 200) { // Check if the request was successful.
                std::lock_guard<std::mutex> lock(mtx); // Lock the mutex to safely update the result.
                result = res->body; // Update the result with the response body.
            }
            else { // Log an error message if the request failed.
                std::cerr << "HTTP request failed: " << (res ? res->status : -1) << std::endl;
            }

            isTaskFinished.store(true); // Tell that our task is finished
            isSSLCalled.store(false); // set back isSSLCalled to false to avoid any duplicated call in the thread
        }
    }
}

Book OpenLibraryAPI::parseBookData(const std::string& isbn, const std::string& jsonData) {
    Book book;
    book.isbn = isbn; // Add the ISBN to the book
    auto jsonDataRoot = json::parse(jsonData, nullptr, false);

    // Check if JSON parsing failed
    if (jsonDataRoot.is_discarded()) {
        throw std::runtime_error("Failed to parse JSON.");
    }

    // Iterate over the JSON items
    for (const auto& key : jsonDataRoot.items()) {
        const auto& bookData = key.value();

        // Check and assign the title if available
        if (bookData.contains("title") && bookData["title"].is_string()) {
            book.title = bookData["title"];
        }

        // Check and assign the publish date if available
        if (bookData.contains("publish_date") && bookData["publish_date"].is_string()) {
            book.publish_date = bookData["publish_date"];
        }

        // Check and assign the number of pages if available
        if (bookData.contains("number_of_pages") && bookData["number_of_pages"].is_number_integer()) {
            book.number_of_pages = bookData["number_of_pages"];
        }

        // Check and assign the description if available, otherwise set a default message
        if (bookData.contains("notes") && bookData["notes"].is_string()) {
            book.description = bookData["notes"];
        }
        else {
            book.description = "No description available.";
        }

        // Check and assign the author if available, otherwise set a default message
        if (bookData.contains("authors") && bookData["authors"].is_array() && !bookData["authors"].empty()) {
            book.author = bookData["authors"][0].contains("name") && bookData["authors"][0]["name"].is_string() ? bookData["authors"][0]["name"] : "Unknown author";
        }
    }

    return book; // Return the populated book object
}

void OpenLibraryAPI::addBookSummary(Book& book, const std::string& jsonData) {
    auto jsonDataRoot = json::parse(jsonData, nullptr, false);

    // Check if JSON parsing failed
    if (jsonDataRoot.is_discarded()) {
        throw std::runtime_error("Failed to parse JSON.");
    }

    // Iterate over the JSON items
    for (const auto& key : jsonDataRoot.items()) {
        const auto& bookDetails = key.value();

        // Check and assign the summary if available
        if (bookDetails.contains("details") && bookDetails["details"].contains("description")) {
            if (bookDetails["details"]["description"].is_string()) {
                book.summary = bookDetails["details"]["description"].get<std::string>();
                if (book.summary.empty())
                    book.summary = "No Summary available.";
            }
            else if (bookDetails["details"]["description"].contains("value")) {
                book.summary = bookDetails["details"]["description"]["value"].get<std::string>();
                if (book.summary.empty())
                    book.summary = "No Summary available.";
            }
        }

        // Check and assign the theme if available
        if (bookDetails.contains("details") && bookDetails["details"].contains("subjects")) {
            if (bookDetails["details"]["subjects"][0].is_string()) {
                    book.subjects = bookDetails["details"]["subjects"][0].get<std::string>();
            }
        }
    }
}



void OpenLibraryAPI::saveConfig(const std::vector<Book>& books) {
    json j;

    // Iterate over each book in the vector and add its details to the JSON object
    for (const auto& book : books) {
        j.push_back({
            {"isbn", book.isbn},
            {"title", book.title},
            {"author", book.author},
            {"publish_date", book.publish_date},
            {"number_of_pages", book.number_of_pages},
            {"description", book.description},
            {"theme", book.subjects},
            {"personal_note", book.personal_note},
            {"summary", book.summary}, // Add summary to the config file
            {"is_favorite", book.is_favorite} // Save the favorite status
            });
    }

    // Open a file for writing
    std::ofstream file("books.json");
    if (file.is_open()) {
        file << j.dump(4); // Write JSON object to file with pretty printing
        file.close(); // Close the file
    }
    else {
        std::cerr << "Unable to open file for writing." << std::endl; // Print error message if file cannot be opened
    }
}

std::vector<Book> OpenLibraryAPI::loadConfig() {
    std::vector<Book> books;
    std::ifstream file("books.json");

    // Open the file for reading
    if (file.is_open()) {
        json j;
        file >> j; // Read the JSON data from the file

        // Iterate over each item in the JSON array and populate the Book objects
        for (const auto& item : j) {
            Book book;
            book.isbn = item["isbn"].get<std::string>();
            book.title = item["title"].get<std::string>();
            book.author = item["author"].get<std::string>();
            book.publish_date = item["publish_date"].get<std::string>();
            book.number_of_pages = item["number_of_pages"].get<int>();
            book.description = item["description"].get<std::string>();
            book.subjects = item["theme"].get<std::string>();
            book.personal_note = item["personal_note"].get<std::string>();
            book.summary = item.contains("summary") ? item["summary"].get<std::string>() : "";
            book.is_favorite = item.contains("is_favorite") ? item["is_favorite"].get<bool>() : false;
            books.push_back(book); // Add the populated book to the vector
        }

        file.close(); // Close the file
    }
    else {
        std::cerr << "Unable to open config file." << std::endl; // Print error message if file cannot be opened
    }

    return books; // Return the vector of books
}

// we're not using it in the application but it can be usefull
void OpenLibraryAPI::displayBook(const Book& book) {
    std::cout << "ISBN: " << book.isbn << std::endl;
    std::cout << "Title: " << book.title << std::endl;
    std::cout << "Author: " << book.author << std::endl;
    std::cout << "Publish Date: " << book.publish_date << std::endl;
    std::cout << "Number of Pages: " << book.number_of_pages << std::endl;
    std::cout << "Description: " << book.description << std::endl;
    std::cout << "Theme: " << book.subjects << std::endl;
    std::cout << "Summary: " << book.summary << std::endl;
    std::cout << "Personal Note: " << book.personal_note << std::endl;
    std::cout << "Favorite: " << (book.is_favorite ? "Yes" : "No") << std::endl;
    std::cout << "---------------------" << std::endl;
}

void OpenLibraryAPI::addPersonalNote(std::vector<Book>& books, const std::string& title, const std::string& note) {
    for (auto& book : books) {
        if (book.title == title) {
            book.personal_note = note;
            return;
        }
    }
    std::cerr << "Book not found." << std::endl;
}

void OpenLibraryAPI::markAsFavorite(std::vector<Book>& books, const std::string& title) {
    for (auto& book : books) {
        if (book.title == title) {
            book.is_favorite = !book.is_favorite;
            return;
        }
    }
    std::cerr << "Book not found." << std::endl;
}

// we're not using it in the application but it can be usefull
void OpenLibraryAPI::filterBooksByTitle(const std::vector<Book>& books, const std::string& title) {
    for (const auto& book : books) {
        if (book.title.find(title) != std::string::npos) {
            displayBook(book);
        }
    }
}

// we're not using it in the application but it can be usefull
void OpenLibraryAPI::filterBooksByAuthor(const std::vector<Book>& books, const std::string& author) {
    for (const auto& book : books) {
        if (book.author.find(author) != std::string::npos) {
            displayBook(book);
        }
    }
}

// we're not using it in the application but it can be usefull
void OpenLibraryAPI::searchBooksByTitle(const std::vector<Book>& books) {
    std::string title;
    while (true) {
        std::cout << "Enter title to search (or type 'exit' to go back): ";
        std::getline(std::cin, title);
        if (title == "exit") {
            break;
        }
        std::cout << "Search results for: " << title << std::endl;
        filterBooksByTitle(books, title);
    }
}

// we're not using it in the application but it can be usefull
void OpenLibraryAPI::searchBooksByAuthor(const std::vector<Book>& books) {
    std::string author;
    while (true) {
        std::cout << "Enter author to search (or type 'exit' to go back): ";
        std::getline(std::cin, author);
        if (author == "exit") {
            break;
        }
        std::cout << "Search results for: " << author << std::endl;
        filterBooksByAuthor(books, author);
    }
}

bool OpenLibraryAPI::isBookInCollection(const std::vector<Book>& books, const std::string& isbn) {
    for (const auto& book : books) {
        if (book.isbn == isbn) {
            return true;
        }
    }
    return false;
}
