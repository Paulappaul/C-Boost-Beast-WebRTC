#ifndef DIRECTORY_H
#define DIRECTORY_H
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

// JSON Parsing
#include <nlohmann/json.hpp>

// Unique IDs
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

// C++
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <sstream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;
using json = nlohmann::json;
namespace websocket = beast::websocket;


bool isOnline = false; 

std::unordered_map<std::string, std::string> users =
{
    {"Paul", "password1"},
    {"Adrian", "password2"},
    {"Serene", "password3"}
};

std::vector<std::string> contacts = {"Ari", "Adrian", "Serene"};
std::unordered_map<std::string, std::string> sessions;

std::mutex sessions_mutex;

std::string load_file(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
#endif // DIRECTORY_H
