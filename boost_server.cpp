#include "directory.h"
#include "websocket_logic.h" 
#include "http_logic.h"


/**********************************************************************************************************
Internet Protocol (IP): This is the core protocol of the internet. It is responsible for addressing and routing data packets between devices on different networks. Two versions of IP are widely used: IPv4 and IPv6.

Each IP address is unique. HTTP requests are made to an IP address - think a mailbox. Made of digits (ipv4) or hexadecimal (ipv6). Call and Responses contain source and destination IP address, again, like mail.
**********************************************************************************************************/

/*********************************************************************************************************
HTTP is a protocol, stands for Hyper Text Transfer Protocol. Set of Rules of how two computers talk to each other. Client Machine ( Local ) vs Server ( Remote ) . Client machine makes a request to server, which in turn sends a response. Errors return in a status code, Success satisfies the Request. Statues codes are three digits.

HTTP Methods: HTTP supports various methods for performing different actions on web resources. The most commonly used methods include:

	GET: Retrieve data from the server.
	POST: Send data to the server to be processed.
	PUT: Update or create a resource.
	DELETE: Remove a resource.
	HEAD: Retrieve only the headers of a response.
	OPTIONS: Query the server for supported methods and capabilities.

HTTP Status Codes: HTTP uses status codes to indicate the outcome of an HTTP request. Common status codes include:

	200 OK: Successful request.
	404 Not Found: The requested resource is not available.
	500 Internal Server Error: Server-side error.
	301 Moved Permanently: Resource has been moved to a new URL.

Headers: HTTP headers provide additional information about the request and response. They include details such as content type, content length, cookies, and caching instructions.


*********************************************************************************************************/

/**********************************************************************************************************
Transmission Control Protocol (TCP): TCP provides reliable, connection-oriented communication. It ensures data is transmitted without errors, in the correct order, and with flow control mechanisms to prevent congestion. Defines how applications can create communication channels, how messages delivered on these channels should be broken down in packets to be transmitted
***********************************************************************************************************/

/**********************************************************************************************************

SSL: The Secure Sockets Layer is a protocol designed to provide secure data transmission over a network, its been succeed by TLS, Transport Layer Security but  you'll commonly see them specifed together. SSL/TLS use encryption to protect data, ensuring data remains confidential so that it cannot be esaily intercepted or deciphered by malcious or unauthorized  parties. It also provides protocols for data integrity, ensuring data reamins untampered in a transmission. This is provided in part  by server authentication; when you connect toa website, the server that hosts the site presents you a digitial certificate. This certificate confirms the server's identity, ensuring that you are connecting to a legitate website and not an imposter. These security protocols also provide a handshake mechanism: when a server and client agree to establish a connection abiding by the protocol they agree on encrpytion parameters, exchange keys and verify each other's identity.

When these protocols are used to secure web based communciation, it is refered to as HTTPS. You'll recongize this because its the predicate in most web addresses because SSL/TLS is the cornerstone of internet secuirty. Websites that use HTTPS encrypt data exchanged between the user's browser and the web server which in turn provides a secure and private browsing experience.

***********************************************************************************************************/


int main(int argc, char* argv[]) {
    try {
        // 0.0.0.0 is a special IP that means "any address" - Server will accept any connection on this machine.
        auto const address = net::ip::make_address("0.0.0.0");
        unsigned short port = static_cast<unsigned short>(std::atoi("8080"));

        // IO handles reading and writing to sockets async
        net::io_context ioc{1}; // "1" delegates the number of connections we can handle concurrently.

        // Create and configure the SSL context
        ssl::context ctx{ssl::context::tlsv12_server};
        ctx.set_options(ssl::context::default_workarounds | ssl::context::no_sslv2 | ssl::context::single_dh_use);
        //SSL
	ctx.use_certificate_chain_file("server.crt");
        ctx.use_private_key_file("server.key", ssl::context::pem);

        tcp::acceptor acceptor{ioc, {address, port}};

        // Forever Loop
        while (true)
        {
            tcp::socket socket{ioc};
            acceptor.accept(socket);
            std::thread{&http_session, std::move(socket), std::ref(ctx)}.detach();
        }

    } catch (std::exception &e) {
        std::cerr << "Top Level Error Found: " << e.what() << std::endl;
    }
}

