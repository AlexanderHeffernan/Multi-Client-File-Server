# Multi-Client File Server

This project is part of a networking assignment aimed at implementing a multi-client file server using C programming language. The server is designed to handle concurrent client connections efficiently by utilizing the `fork()` system call. 

## Features

- **Concurrent Connections**: The server can handle multiple client connections simultaneously.
- **GET and PUT Operations**: Clients can request files from the server using the GET command and upload files using the PUT command.
- **Error Handling**: The server provides appropriate error messages for invalid commands or file operations.

## Technologies Used

- **Language**: C
- **Libraries**: Socket programming, Standard C libraries
- **System Calls**: `fork()`, `socket()`, `bind()`, `listen()`, `accept()`
  
## Usage

1. Clone the repository.
2. Compile the `server.c` file using a C compiler.
3. Run the compiled executable providing the port number as a command-line argument (e.g., `./server <port_number>`).
4. Open another terminal window.
5. Run `nc localhost <port_number>` in the new terminal window (replace `<port_number>` with the port number your server is using) to establish a connection with your server program.
6. Once `nc` is connected to the server program, type commands such as GET, PUT, and BYE in the `nc` terminal window to interact with the server and transfer files.
7. 
## Acknowledgements

These implementations were developed as part of a project for NWEN241 at Victoria University of Wellington.
