#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>

#include <nlohmann/json.hpp>
#include <algorithm>

using json = nlohmann::json;

struct Book {
    std::string isbn;
    std::string title;
    std::string author;
    std::string publish_date;
    int number_of_pages;
    std::string description;
    std::string subjects;
    std::string personal_note;
    std::string summary; 
    bool is_favorite = false; 
};
namespace OpenLibraryAPI
{

    void fetchBookDataThread(std::string& isbn, std::string& result, std::mutex& mtx, std::atomic<bool>& isSSLCalled, std::atomic<bool>& isTaskFinished);

    void fetchBookSummaryThread(std::string& isbn, std::string& result, std::mutex& mtx, std::atomic<bool>& isSSLCalled, std::atomic<bool>& isTaskFinished);

    Book parseBookData(const std::string& isbn, const std::string& jsonData);

    void addBookSummary(Book& book, const std::string& jsonData);

    void saveConfig(const std::vector<Book>& books);

    std::vector<Book> loadConfig();

    void displayBook(const Book& book);

    void addPersonalNote(std::vector<Book>& books, const std::string& title, const std::string& note);

    void markAsFavorite(std::vector<Book>& books, const std::string& title);

    void filterBooksByTitle(const std::vector<Book>& books, const std::string& title);

    void filterBooksByAuthor(const std::vector<Book>& books, const std::string& author);

    void searchBooksByTitle(const std::vector<Book>& books);

    void searchBooksByAuthor(const std::vector<Book>& books);

    bool isBookInCollection(const std::vector<Book>& books, const std::string& isbn);


};

//std::vector<Book> books = loadConfig();
