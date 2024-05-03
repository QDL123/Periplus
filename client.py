import socket

def main():
    # Server's IP address and port number
    # Assume the server is on the same machine, replace with the actual server IP if different
    server_host = '127.0.0.1'  
    server_port = 13

    # Create a TCP/IP socket
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        # Connect to the server
        sock.connect((server_host, server_port))
        
        while True:
            # Get user input
            message = input("Enter your message ('exit' to quit): ")
            if message.lower() == 'exit':
                break

            # Send data
            sock.sendall(message.encode())

            # Receive response
            response = sock.recv(1024)  # Buffer size is 1024 bytes
            print("Received from server:", response.decode())

        print("Closing connection.")
        
if __name__ == '__main__':
    main()
