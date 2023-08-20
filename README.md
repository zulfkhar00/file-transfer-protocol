# File Transfer Protocol (FTP) 📁

`file-transfer-protocol` is a simplified version of the FTP application protocol in the C programming language, consisting of two separate programs: an FTP client and an FTP server.

## Table of Contents

- [Features](#features)
- [Dependencies](#dependencies)
- [Installation](#installation)
- [Usage](#usage)
- [Implemented Commands](#implemented-commands)
- [Example](#example)
- [Contributing](#contributing)
- [License](#license)

## Features ✨

- FTP client and server written in C
- Server maintains FTP sessions and provides file access
- Client includes a simple user interface with a command prompt for issuing requests to the server
- Server supports concurrent connections by handling multiple simultaneous requests from different or the same client
- Server uses the `select()` and `fork()` functions in C for handling resource-intensive requests and simple control connections
- Implements a subset of FTP commands from `RFC 959` and restricts the transfer mode to stream mode

## Dependencies 📚

- A C compiler (such as GCC) and a Unix-based system (Linux, macOS, etc.) with standard C libraries

## Installation ⚙️

1. Clone this repository:

   ```
   git clone https://github.com/zulfkhar00/file-transfer-protocol.git
   ```

2. Compile both client and server programs:

   ```
   cd file-transfer-protocol
   make all
   ```

3. This will create two executables: `./client` and `./server`.

## Usage 🚀

1. First go to `/server` and run the FTP server program:

   ```
   ./server
   ```

   The server will open a TCP socket, listen for incoming connections, and process incoming FTP commands.

2. In a separate terminal window, run the FTP client program:

   ```
   ./client
   ```

   The client program will establish a control TCP connection with the FTP server and respond with a prompt ftp> waiting for the user's FTP commands.

3. Call the desired FTP command.

## FTP Commands 📝

The following commands are implemented:

1. `PORT h1,h2,h3,h4,p1,p2`: Specify the client IP address and port number for the data channel.
2. `USER username`: Identify the user trying to log in to the FTP server.
3. `PASS password`: Authenticate using the user password.
4. `STOR filename`: Upload a local file from the current client directory to the current server directory.
5. `RETR filename`: Download a file named filename from the current server directory to the current client directory.
6. `LIST`: List all the files under the current server directory.
7. `!LIST`: List all the files under the current client directory.
8. `CWD foldername`: Change the current server directory.
9. `!CWD foldername`: Change the current client directory.
10. `PWD`: Displays the current server directory.
11. `!PWD`: Displays the current client directory.
12. `QUIT`: Quits the FTP session and closes the control TCP connection.

## Session example 👀

```

220 Service ready for new user.
ftp> USER bob
331 Username OK, need password.
ftp> PASS donuts
230 User logged in, proceed.
ftp> CWD test
200 directory changed to /Users/bob/test
ftp> RETR vanilla_donut.txt
200 PORT command successful.
150 File status okay; about to open. data connection.
226 Transfer completed.
ftp> LIST
200 PORT command successful.
150 File status okay; about to open. data connection.
vanilla_donut.txt
choco_donut.txt
226 Transfer completed.
ftp> QUIT
221 Service closing control connection.
```

## Contributing 🤝

1. Fork the Repository

2. Clone your fork

3. Create your feature branch (`git checkout -b feature/AmazingFeature`)

4. Commit your changes (`git commit -m 'Add some AmazingFeature'`)

5. Push to the branch (`git push origin feature/AmazingFeature`)

6. Open a Pull Request

## License 📄

This project is licensed under the MIT License. Please see the [LICENSE](LICENSE) file for more details.
