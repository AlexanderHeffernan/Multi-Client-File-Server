#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define BUFFER_LEN 100

void error (const char * msg)
{
    printf("Error : %s\n", msg);
    exit(1);
}

/**
 * Parses the port number from the command-line arguments and validates it.
 * The port number must be provided as the second command-line argument.
 * @param argv The array of command-line arguments.
 * @return The parsed port number.
 */
int get_port_num(char *argv[])
{
    int port_num;
    if (sscanf(argv[1], "%d", &port_num) != 1)
        error("Invalid port number");

    // Ensure the port number isn't below the minimum
    if (port_num < 1024)
        return -1;

    printf("Port number: %d\n", port_num);

    return port_num;
}

/**
 * Creates a TCP socket.
 *
 * @return The file descriptor of the created socket.
 */
int create_socket()
{
    // Create a TCP socket
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) 
        error("Unable to create a socket");

    printf("Socket created\n");

    return fd;
}

/**
 * Creates a socket address structure for the given port number.
 *
 * @param port_num The port number to use.
 * @return A sockaddr_in structure representing the server address.
 */
struct sockaddr_in create_address(int port_num)
{
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port_num);
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    printf("Address created\n");

    return serveraddr;
}

/**
 * Binds a socket to the given address.
 * 
 * @param fd The file descriptor of the socket.
 * @param serveraddr The sockaddr_in structure representing the server address.
 */
void bind_address_to_socket(int fd, struct sockaddr_in serveraddr)
{
    int ret = bind(fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (ret == -1)
        error("Unable to bind socket to address\n");

    printf("Bind successful\n");
}

/**
 * Listens for incoming connections on a socket.
 *
 * @param fd The file descriptor of the socket.
 */
void listen_for_incoming_connection(int fd)
{
    if (listen(fd, SOMAXCONN) < 0)
        error("Unable to listen for incoming connection\n");

    printf("Listen successful\n");
}

/**
 * Accepts an incoming client connection on a socket.
 *
 * @param fd The file descriptor of a socket.
 * @return The file descriptor of the accepted client connection.
 */
int accept_client_connection(int fd)
{
    struct sockaddr_in client_addr;
    int addrlen = sizeof(client_addr);

    // Accept an incoming client connection
    int client_fd = accept(fd, (struct sockaddr *)&client_addr, (socklen_t*)&addrlen);

    // Check if accepting the client connection was successful
    if (client_fd < 0)
        error("Unable to accept client connection\n");

    return client_fd;
}

/**
 * Sends a message to a client.
 *
 * @param client_fd The file descriptor of the client socket.
 */
void send_message_to_client(int client_fd, char msg[])
{
    ssize_t bytes_sent = send(client_fd, msg, strlen(msg), 0);

    // Check if the message was successfuly sent
    if (bytes_sent < 0)
        error("Unable to send message to client\n");
}

/**
 * Receives a message from a client into the provided buffer.
 *
 * @param client_fd The file descriptor of the client socket.
 * @param buffer A pointer to the buffer where the received message will be stored.
 * @return The number of bytes received, or -1 if an error occurs.
 */
char receive_client_message(int client_fd, char* buffer)
{
    // Receive the message from the client
    ssize_t bytes_received = recv(client_fd, buffer, BUFFER_LEN, 0);

    // Check if the message was successfuly received
    if (bytes_received < 0)
        error("Unable to receive client message\n");

    return bytes_received;
}

/**
 * Function to check if a message matches a target message (case-insensitive).
 * 
 * @param input Incoming message to check against target message.
 * @param target Target message to be checked against.
 */
int do_strings_match(char *input, char *target)
{
    char input_first_word[BUFFER_LEN];

    // Find the first word in input
    for (int i = 0; input[i] != ' ' && input[i] != '\n'; i++)
        input_first_word[i] = tolower(input[i]);

    // Compare the first words
    return strcmp(input_first_word, target);
}

/**
 * Opens a file specified in the buffer with the given open mode.
 *
 * @param buffer The buffer containing the filename.
 * @param open_mode The mode to open the file (e.g. 'r' for read, 'w' for write).
 * @param client_fd The file descriptor of the client's socket connection.
 * @return A pointer to the opened file, or NULL if the file cannot be opened.
 */
FILE* open_file(char buffer[], char open_mode, int client_fd, char other_error_msg[], char not_found_msg[])
{
    // Find the filename in the buffer
    char *filename = strchr(buffer, ' ');

    // If the file is not found, send error message to client
    if (filename == NULL) {
        send_message_to_client(client_fd, other_error_msg);
        return NULL; // Go back to accepting new connections
    }
    // Move filename pointer past the space
    filename++;

    // Remove newline character from filename
    filename[strcspn(filename, "\n")] = '\0';

    // Attempt to open file
    FILE *file = fopen(filename, &open_mode);
    if (file == NULL)
        send_message_to_client(client_fd, not_found_msg);

    return file; 
}

/**
 * Retrieves a file requested by the client and sends it to the client.
 *
 * @param buffer The buffer containing the clien'ts request
 * @param client_fd The file descriptor of the client's socket connection.
 */
void get_file(char buffer[], int client_fd)
{
    FILE *file = open_file(buffer, 'r', client_fd, "SERVER 500 Get Error\n", "SERVER 404 Not Found\n");
    if (file == NULL)
        return;
    
    send_message_to_client(client_fd, "SERVER 200 OK\n\n");

    // Send file contents to client
    char file_buffer[BUFFER_LEN];
    while (fgets(file_buffer, BUFFER_LEN, file) != NULL)
        send_message_to_client(client_fd, file_buffer);

    send_message_to_client(client_fd, "\n\n\n");

    fclose(file);
}

/**
 * Saves data received from the client into a file specified by the client.
 *
 * @param buffer The buffer containing the client's request.
 * @param client_fd The file descriptor of the client's socket connection.
 */
void put_file(char buffer[], int client_fd)
{
    // Open the file for writing
    FILE *file = open_file(buffer, 'w', client_fd, "SERVER 501 Put Error\n", "SERVER 501 Put Error\n");
    if (file == NULL) // If the file cannot be opened, return
        return;

    // Count of consecutive empty lines received
    int empty_lines = 0;

    // Receive and write data to file until two consecutive empty lines
    while (1) {
        int bytes_received = receive_client_message(client_fd, buffer);

        // Check for empty lines
        if (bytes_received <= 2 && strncmp(buffer, "\n", 1) == 0)
            empty_lines++;
        else
            empty_lines = 0;
        
        // Break loop if two consecutive empty lines are received
        if (empty_lines > 1)
            break;

        // Write received data to file
        fwrite(buffer, 1, bytes_received, file);
    }

    fclose(file);
    send_message_to_client(client_fd, "SERVER 201 Created\n");
}

/**
 * Entry point of the server program.
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments:
 *                  argv[0]: program name
 *                  argv[1]: port number
 * @return Returns 0 upon successful execution, -1 otherwise.
 */ 
int main(int argc, char *argv[])
{
    // Ensure there are 2 arguments
    if (argc != 2)
        return -1;

    // Get the port number from the command-line arguments
    int port_num = get_port_num(argv);
    if (port_num == -1)
        return -1;

    int fd = create_socket();

    // Create a bind a server address to the socket
    struct sockaddr_in serveraddr = create_address(port_num);
    bind_address_to_socket(fd, serveraddr);

    listen_for_incoming_connection(fd);

    // Main loop to handle client connections
    while (1) {
        int client_fd = accept_client_connection(fd);

        // Fork a child process
        pid_t pid = fork();

        if (pid == -1) { // Fork failed
            send_message_to_client(client_fd, "HELLO\n");
            close(client_fd);
        }
        else if (pid == 0) { // Child process
            close(fd); // Close listening socket in child process

            send_message_to_client(client_fd, "HELLO\n");

            // Retrieve the client's request
            char buffer[BUFFER_LEN];
            receive_client_message(client_fd, buffer);

            if (do_strings_match(buffer, "get") == 0)
                get_file(buffer, client_fd);
            else if (do_strings_match(buffer, "put") == 0)
                put_file(buffer, client_fd);
            else if (do_strings_match(buffer, "bye") != 0)
                send_message_to_client(client_fd, "SERVER 502 Command Error\n");

            close(client_fd);
            exit(1);
        }
        else { // Parent process
            close(client_fd); // Close client socket in parent process
        }
    }

    close(fd);

    return 0;
}