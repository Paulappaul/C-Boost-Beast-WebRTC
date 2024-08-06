#pragma once
#include "directory.h" 

struct Candidate
{
	std::string can; 
	std::string sdpMid; 
	int sdpMLineIndex; 
	
	friend std::ostream& operator<< (std::ostream &outs, const Candidate& c)
	{
		outs << "Candidate: " <<
		c.can << " sdpMid: " <<
		c.sdpMid << " sdpMlineindex: " <<
		c.sdpMLineIndex;  	
		return outs; 
	}
}; 

struct offer_container
{
	std::vector<Candidate> candidates; 
	std::string offer; 
	websocket::stream<beast::ssl_stream<tcp::socket>>*  ws_ptr; 	
};

offer_container oc; 
offer_container coc; 

void websocket_session(websocket::stream<beast::ssl_stream<tcp::socket>> ws, std::string session_id)
{
    try
    {
        for (;;)
        {
            beast::flat_buffer buffer;

            // Read a message
            ws.read(buffer);
            auto message = beast::buffers_to_string(buffer.data());
            auto json_message = json::parse(message);

            if(json_message.contains("type") && json_message["type"].is_string())
            {
                std::string tag = json_message["type"].get<std::string>();
               	// Caller Offer Candidate
		if(tag == "caller_offer")
                {
        		std::string sdp_offer = json_message["sdp"].get<std::string>();
        		std::cout << "Received offer: " << sdp_offer << std::endl;
        		oc.offer = sdp_offer;       
			std::cout << "leaving offer function" << std::endl; 
                	oc.ws_ptr = &ws; 
		}
                else if(tag == "caller_candidate")
                {
			Candidate Cancon; 	
                        
			Cancon.can  = json_message["candidate"].get<std::string>();
                        Cancon.sdpMid = json_message["sdpMid"].get<std::string>(); 
			Cancon.sdpMLineIndex = json_message["sdpMLineIndex"].get<int>(); 
			
			std::cout << "received ICE candidate: \n"  << Cancon << std::endl;
			
			oc.candidates.push_back(Cancon); 
                }
		else if(tag == "accept_call")
		{
			if(!oc.offer.empty()  && !oc.candidates.empty())
			{
		
				json response;
				response["type"] = "callers_offer"; 
				response["sdp"] = oc.offer;
				ws.write(net::buffer(response.dump())); 
				
				for(const auto& candidate : oc.candidates) 
				{
					json candidate_message; 
					candidate_message["type"] = "candidate"; 
					candidate_message["candidate"] = candidate.can; 
					candidate_message["sdpMid"] = candidate.sdpMid; 
					candidate_message["sdpMLineIndex"] = candidate.sdpMLineIndex;
					ws.write(net::buffer(candidate_message.dump())); 
				
				}
			}
		
		}
		else if(tag == "callee_answer")
		{
			std::string sdp_offer = json_message["sdp"].get<std::string>();                                           std::cout << "Received Callee offer: " << sdp_offer << std::endl;					    coc.offer = sdp_offer; 
		}
		else if(tag == "callee_candidate")
		{
                        Candidate Cancon;

                        Cancon.can  = json_message["candidate"].get<std::string>();
                        Cancon.sdpMid = json_message["sdpMid"].get<std::string>();
                        Cancon.sdpMLineIndex = json_message["sdpMLineIndex"].get<int>();

                        coc.candidates.push_back(Cancon);	
		}

	else if(tag == "callee_push")
	{
    		json response;

    		// Send the callee's offer to the caller
    		response["type"] = "callee_offer";
    		response["sdp"] = coc.offer;

    		// Send the callee's ICE candidates to the caller
    		oc.ws_ptr->write(net::buffer(response.dump()));

    		for(const auto& candidate : coc.candidates)
    		{
        		json iceResponse;
        		iceResponse["type"] = "callee_iceSend";
        		iceResponse["candidate"] = candidate.can;
        		iceResponse["sdpMid"] = candidate.sdpMid;
        		iceResponse["sdpMLineIndex"] = candidate.sdpMLineIndex;

        		// Send each ICE candidate
        		oc.ws_ptr->write(net::buffer(iceResponse.dump()));
    		}
	}
	else
             std::cout << "Unknown Tag: " << tag << std::endl;

        }

    else
    {
                std::cout << "Invalid message format" << std::endl;
           }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "WebSocket session error: " << e.what() << std::endl;
    }
    // Remove the session when done
    {
        std::lock_guard<std::mutex> lock(sessions_mutex);
        sessions.erase(session_id);
    }
}


