

// Any time a user connects we open a new thread:
void http_session(tcp::socket socket, ssl::context& ctx) 
{
    try 
	{
	//We provide a Context and a Socket that we will use for connection, we use Std::Move to transfer exclusive ownership 2 stream. Wraps around tcp::socket to provide secure communcation. The socket stream is secure with SSL/TLS

        beast::ssl_stream<tcp::socket> stream{std::move(socket), ctx};
        stream.handshake(ssl::stream_base::server);

        beast::flat_buffer buffer; // stores data received from the network
        http::request<http::string_body> req; //the http request body is stored a string
        http::read(stream, buffer, req); //// Reads an HTTP request from the stream (which is encrypted-> tcp::socket), buffer stores all incoming data, req will hold the parsed HTTP request after reading

        // Check for WebSocket upgrade request
        if (beast::websocket::is_upgrade(req))
	{
            std::cout << "Attempting Websocket upgrade!" << std::endl; 
	    boost::beast::websocket::stream<beast::ssl_stream<tcp::socket>> ws{std::move(stream)};
            ws.accept(req);
            std::string session_id = "";  // Assign or generate a session_id as needed
            std::thread(&websocket_session, std::move(ws), std::move(session_id)).detach();
            return;
        }

        // Otherwise, process the HTTP request
        http::response<http::string_body> res;
        handle_request(req, res);

        http::write(stream, res);

        // Remove the session if it was created
        if (req.target() == "/login" && req.method() == http::verb::post) {
            try {
                auto body = json::parse(req.body());
                if (body.contains("session_id") && body["session_id"].is_string()) {
                    std::string session_id = body["session_id"].get<std::string>();
                    std::lock_guard<std::mutex> lock(sessions_mutex);
                    sessions.erase(session_id);
                }
            } catch (const std::exception& e) {
                std::cerr << "Error removing session: " << e.what() << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "HTTP session error: " << e.what() << std::endl;
    }
}
