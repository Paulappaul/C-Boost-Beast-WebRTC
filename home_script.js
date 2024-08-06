const loginButton = document.getElementById('loginButton');

loginButton.addEventListener('click', login);

function login() {
    const username = document.getElementById("username").value;
    const password = document.getElementById("password").value;

    console.log("Login attempt with username:", username, "and password:", password);

    fetch('/login', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ username, password })
    })
    .then(response => response.json())
    .then(data => {
        if (data.success && data.session_id) {
            localStorage.setItem('session_id', data.session_id);
            window.location.href = '/call.html';
        } else {
            document.getElementById("message").innerText = data.message || 'Login failed';
        }
    })
    .catch(error => {
        console.error('Error:', error);
        document.getElementById("message").innerText = 'Login failed';
    });
}

