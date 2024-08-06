#pragma once
#include "directory.h"
#include "websocket_logic.h" 

//When a Client requests data, we need to redirect them to the source of that data. A target is the destination they are requesting that pocesses the data. 

void handle_request(http::request<http::string_body> const& req, http::response<http::string_body>& res)
{
    
    std::string target = req.target().to_string();
    
	


    if (target == "/login" && req.method() == http::verb::post) // POST:: Data posted to the Server
    {
        try
        {
            auto body = json::parse(req.body());
            std::cout << "Received login request body: " << req.body() << std::endl;

            std::string username = body.at("username").get<std::string>();
            std::string password = body.at("password").get<std::string>();

            json response;
            
         
	     if (users.find(username) != users.end() && users[username] == password)
	     {
                boost::uuids::uuid uuid = boost::uuids::random_generator()();
                std::string session_id = boost::uuids::to_string(uuid);

                {
                    std::lock_guard<std::mutex> lock(sessions_mutex);
                    sessions[session_id] = username;
                }

                response["success"] = true;
                response["message"] = "LOGIN SUCCESSFUL!";
                response["session_id"] = session_id;

                std::cout << "Session created with ID: " << session_id << std::endl;
             }
             else
             {
                response["success"] = false;
                response["message"] = "Invalid credentials";
	   	std::cerr << "Invalid Logic Credendetials provided!" << std::endl; 
	     } 

             res.result(http::status::ok);
             res.set(http::field::server, "Boost.Beast");
             res.set(http::field::content_type, "application/json");
             res.body() = response.dump();
             res.prepare_payload();
        } 
	catch (const std::exception& e) 
	{
            std::cerr << "Error parsing login request: " << e.what() << std::endl;
            res.result(http::status::bad_request);
            res.set(http::field::content_type, "application/json");
            res.body() = "{\"error\": \"Invalid request\"}";
            res.prepare_payload();
        }
        return;
    }
    
    //forward our contacts map when the page is Call.html page is loaded
    if (target == "/contacts" && req.method() == http::verb::get)
    {
        json response;
        response["contacts"] = contacts;
	
	
        res.result(http::status::ok);
        res.set(http::field::server, "Boost.Beast");
        res.set(http::field::content_type, "application/json");
        res.body() = response.dump();
        res.prepare_payload();
        return;
    }
    
    //When the Callee accepts a call, they ping the server with the following function searching first if the caller is real and online.  
    if (target == "/accept" && req.method() == http::verb::post)
    {
 	try
        {
        	auto body = json::parse(req.body());
        	std::string id = body.at("caller_id").get<std::string>();

       	        json response;
       	 	//if the Caller is found, we notify the client and it will request a Websocket upgrade.
		if (users.find(id) != users.end() && isOnline)
	        {
            		std::cout << "Users Found: " << body << " and they are Online" << std::endl;
            		res.result(http::status::ok);
            		res.set(http::field::server, "Boost.Beast");
            		res.set(http::field::content_type, "application/json");
            		response["result"] = "true";
                	res.body() = response.dump();
         	} 
		else
	        {
            		std::cerr << "Username not found and/or is not online " << std::endl;
            		res.result(http::status::ok);
            		res.set(http::field::server, "Boost.Beast");
            		res.set(http::field::content_type, "application/json");
            		response["result"] = "false";
            		res.body() = response.dump();
        	}

        		res.prepare_payload();
    } 
    catch (const std::exception& e)
    {
        std::cerr << "Error parsing request: " << e.what() << std::endl;
        res.result(http::status::bad_request);
        res.set(http::field::content_type, "application/json");
        res.body() = "{\"error\": \"Invalid request\"}";
        res.prepare_payload();
    }
    	return;
    }
            // "/" if an address is not provided, we go to the home.html in our directory
            if (target == "/")
                target = "/home.html";

             std::string full_path = "." + target;
             std::cout << "Requested file: " << full_path << std::endl;

 // Determine content type based on file extension
    std::string content_type = "text/html";
    if (target.ends_with(".css")) {
        content_type = "text/css";
    } else if (target.ends_with(".js")) {
        content_type = "application/javascript";
    }

    try {
        std::string content = load_file(full_path);
        res.result(http::status::ok);
        res.set(http::field::server, "Boost.Beast");
        res.set(http::field::content_type, content_type);
        res.body() = std::move(content);
        res.prepare_payload();
    } catch (const std::exception& e) {
        res.result(http::status::not_found);
        res.set(http::field::content_type, "text/plain");
        res.body() = "File not found";
        res.prepare_payload();
    }


}



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
            
	    isOnline = true;    
	    std::thread(&websocket_session, std::move(ws), std::move(session_id)).detach();
            return;
        }

        // Otherwise, process the HTTP request
        http::response<http::string_body> res;
        handle_request(req, res);

        http::write(stream, res);

        // Remove the session if it was created
        if (req.target() == "/login" && req.method() == http::verb::post) {
            try
            {
                auto body = json::parse(req.body());
                if (body.contains("session_id") && body["session_id"].is_string())
                {
                    std::string session_id = body["session_id"].get<std::string>();
                    std::lock_guard<std::mutex> lock(sessions_mutex);
                    sessions.erase(session_id);
                }
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error removing session: " << e.what() << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "HTTP session error: " << e.what() << std::endl;
    }
}
