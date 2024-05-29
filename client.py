import socket
import struct

def handle_initialize():
    d = int(input("Enter the number of dimensions: "))
    max_mem = int(input("Enter the maximum amount of memory in bytes: "))
    nTotal = int(input("Enter the total size of the data: "))
    db_url = input("Enter the url of the database endpoint for loading data: ")

    url_size = len(db_url)

    fmt = '<QQQQ'
    # Pack as two unsigned longs
    # Add + b'\0' to indicate end of string
    return struct.pack(fmt, d, max_mem, nTotal, url_size) + "\n".encode('latin1') + db_url.encode('latin1')


def handle_train():
    # Take user input for an array of floats
    floats_input = input("Enter a list of floats separated by spaces: ")
    float_list = [float(item) for item in floats_input.split()]

    # Calculate the number of bytes for the float array
    num_bytes = len(float_list) * 4  # Each float (using 'f' format) is 4 bytes

    # Pack data as the number of bytes followed by the array of floats
    return struct.pack('I', num_bytes) + struct.pack(f'{len(float_list)}f', *float_list)



# Mapping of commands to handling functions
command_handlers = {
    'INITIALIZE': handle_initialize,
    'TRAIN': handle_train,
}

def main():
    server_host = '127.0.0.1'
    server_port = 13

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((server_host, server_port))
        
        while True:
            command = input("Enter command ('exit' to quit): ")
            if command.lower() == 'exit':
                break

            handler = command_handlers.get(command)
            if handler:
                packed_data = handler()
                message = command + "\r\n" + packed_data.decode('latin1') + "\r\n"
                print("Message:")
                print(repr(message))
                sock.sendall(message.encode('latin1'))
                response = sock.recv(1024)
                print("Received from server:", response.decode())
            else:
                print("Unknown command")

        print("Closing connection.")

if __name__ == '__main__':
    main()
