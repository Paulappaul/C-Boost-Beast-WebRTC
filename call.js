
document.addEventListener('DOMContentLoaded', function() {
    const callButton = document.getElementById('callButton');
    let socket = null;
    let pc = null;
    const callField = document.getElementById('callField'); 

    fetch('/contacts')
        .then(response => response.json())
        .then(data => {
            const contactList = document.getElementById('contactListItems');
            contactList.innerHTML = '';
            data.contacts.forEach(contact => {
                const li = document.createElement('li');
                li.textContent = contact;
                contactList.appendChild(li);
            });
        });

acceptButton.addEventListener('click', () => {

    const session_id = localStorage.getItem('session_id');
    if (!session_id) {
        console.error('No session ID found');
        return;
    }

    // Check if the user is in our contacts by posting the contents of our callField to the server
    fetch('/accept', {
	method: 'POST', 
	headers: {
		'Content-Type': 'applicaiton/json'
	        
	},
        body: JSON.stringify({ caller_id: callField.value.trim() })
    })
    .then(response => response.json())
    .then(data => {
        // If the server returns true, we upgrade to a WebSocket connection.
        if(data.result === "true") {
	    console.log("Establishing WebSocket connection!"); 
            socket = new WebSocket(`wss://localhost:8080/ws?session_id=${session_id}`);
            socket.onopen = function(event) {
                const message = {
                    type: 'accept_call'
                };
                socket.send(JSON.stringify(message));
            };

    socket.onmessage = function(event)
    {
    	const message = JSON.parse(event.data);
    	//console.log("Received message:", message);

   	 if (message.type === 'callers_offer')
	 {
		console.log("Receieved SDP offer: ", message.sdp); 
		const sdp = message.sdp; 
		 
        
        	pc.setRemoteDescription(new RTCSessionDescription({ type: 'offer', sdp: sdp })) 

         .then(() => {
                console.log("Remote description set successfully");
                return pc.createAnswer();
            })
         .then(answer => {
                console.log("Answer created successfully");
                return pc.setLocalDescription(answer);
            })
          .then(() => {
                console.log("Local description set successfully");
                const response = 
		{
                    type: 'callee_answer',
                    sdp: pc.localDescription.sdp
                };

                socket.send(JSON.stringify(response));
            })
            .catch((error) => {
                console.error("Error handling offer: ", error);
            });
    } 
    else if (message.type === 'candidate') {
    console.log("Received ICE Candidate: ", message.candidate);
    console.log("Received sdpMid: ", message.sdpMid);
    console.log("Received sdpMidLineIndex: ", message.sdpMLineIndex);
    
    const candidate = new RTCIceCandidate({
        candidate: message.candidate,
        sdpMid: message.sdpMid,
        sdpMLineIndex: message.sdpMLineIndex
    });
    pc.addIceCandidate(candidate)
        .catch((error) => console.error("Error adding ICE candidate: ", error));
}

};
let iceCandidatesSent = false; // Flag to track if candidates have been sent

pc.onicecandidate = function(event) {
    if (event.candidate) {
        const candidate = event.candidate;
        const message = {
            type: 'callee_candidate',
            candidate: candidate.candidate,
            sdpMid: candidate.sdpMid,
            sdpMLineIndex: candidate.sdpMLineIndex
        };
        console.log('Sending CALLEE ICE candidate: ', message);
        socket.send(JSON.stringify(message));
    } else {
        // No more candidates
        if (!iceCandidatesSent) {
            console.log('Ice Candidates Delivered');
            const message = {
                type: 'callee_push'
            };
            socket.send(JSON.stringify(message));
            iceCandidatesSent = true; // Set the flag to avoid sending the message multiple times
        }
    }
};
 
    

            socket.onclose = function(event) {
                console.log('WebSocket is closed now.');
            };

            socket.onerror = function(error) {
                console.error('WebSocket error:', error);
            };
        } else {
            console.error('Accepting call failed.');
        }
    })
    .catch((error) => console.error('Error fetching /accept:', error));
});

		

	

    callButton.addEventListener('click', () => {
        const session_id = localStorage.getItem('session_id');
        if (!session_id) {
            console.error('No session ID found');
            return;
        }
	
	if(callField.value.trim() === '')
	{
	   //the user has not called anyone!
	   console.error('No Caller ID provided!'); 
           return; 
	}	


        // Establish WebSocket connection
        socket = new WebSocket(`wss://localhost:8080/ws?session_id=${session_id}`);

        socket.onopen = function(event) {
            console.log('WebSocket is open now.');
            pc.createOffer()
                .then((offer) => pc.setLocalDescription(offer))
                .then(() => {
			const sdpMessage = pc.localDescription.sdp; 
			const message =  { 
				type: 'caller_offer', 
				sdp: pc.localDescription.sdp, 
			}; 
			console.log(message.sdp);  	
			socket.send(JSON.stringify(message));                 
		})
                .catch((error) => console.error("Error Creating Offer: ", error));
        };

        pc.onicecandidate = function(event) {
            if (event.candidate) {
                const candidate = event.candidate; 
		const message = {
                    type: 'caller_candidate',
		    candidate: candidate.candidate,      // The ICE candidate string
                    sdpMid: candidate.sdpMid,            // The SDP mid
            	    sdpMLineIndex: candidate.sdpMLineIndex  // The SDP m-line index
                };
		console.log('sending candidate: ', message); 
                socket.send(JSON.stringify(message));
            }
        };

socket.onmessage = function(event) {
    const message = JSON.parse(event.data);

    if (message.type === "callee_offer") {
        console.log("Received SDP answer from callee: ", message.sdp);
        const sdp = message.sdp;

        pc.setRemoteDescription(new RTCSessionDescription({ type: 'answer', sdp: sdp }))
            .then(() => {
                console.log("Remote description set successfully");
            })
            .catch((error) => {
                console.error("Error setting remote description: ", error);
            });
    } else if (message.type === "callee_iceSend") {
        console.log("Received ICE Candidate from callee: ", message.candidate);
        const candidate = new RTCIceCandidate({
            candidate: message.candidate,
            sdpMid: message.sdpMid,
            sdpMLineIndex: message.sdpMLineIndex
        });

        pc.addIceCandidate(candidate)
            .then(() => {
                console.log("ICE candidate added successfully");
            })
            .catch((error) => {
                console.error("Error adding ICE candidate: ", error);
            });
    } else {
        console.log('Unknown message received', message);
    }
};

        socket.onclose = function(event) {
            console.log('WebSocket is closed now.');
        };

        socket.onerror = function(error) {
            console.error('WebSocket error:', error);
        };
    });

    const hangupButton = document.getElementById('hangupButton');
    hangupButton.addEventListener('click', () => {
        if (socket) {
            socket.close();
        }
    });

    const logoutButton = document.getElementById('logoutButton');
    logoutButton.addEventListener('click', () => {
        localStorage.removeItem('session_id');
        window.location.href = '/home.html';
    });

	function initWebRTC() {
        pc = new RTCPeerConnection({
            iceServers: [{
                urls: 'stun:stun.l.google.com:19302'
            }]
        });

        pc.ontrack = function(event) {
            const remoteVideo = document.getElementById('receiverVideo');
            if (remoteVideo.srcObject !== event.streams[0]) {
                remoteVideo.srcObject = event.streams[0];
            }
        };

        navigator.mediaDevices.getUserMedia({ video: true, audio: true })
            .then(stream => {
                const localVideo = document.getElementById('callerVideo');
                localVideo.srcObject = stream;
                stream.getTracks().forEach(track => pc.addTrack(track, stream));
            })
            .catch(error => console.error('Error accessing media devices.', error));
    }

    initWebRTC();


});


//FROM THE CALL FUNCTION
/*
	socket.onmessage = function(event) {
            console.log('WebSocket message received:', event.data);
            const message = JSON.parse(event.data);

            if (message.type === 'offer') {
                pc.setRemoteDescription(new RTCSessionDescription(message.sdp))
                    .then(() => pc.createAnswer())
                    .then(answer => pc.setLocalDescription(answer))
                    .then(() => {
                        const response = {
                            type: 'answer',
                            sdp: pc.localDescription
                        };
                        socket.send(JSON.stringify(response));
                    })
                    .catch((error) => console.error("Error handling offer: ", error));
            } else if (message.type === 'candidate') {
                pc.addIceCandidate(new RTCIceCandidate(message.candidate))
                    .catch((error) => console.error("Error adding ICE candidate: ", error));
            }
        };
*/

