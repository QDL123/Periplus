import socket
import struct
import json
import random

def handle_initialize(args):
    print("Handle INITIALIZE")
    d = args[0]
    max_mem = args[1]
    nTotal = args[2]
    db_url = args[3]

    url_size = len(db_url)

    fmt = '<QQQQ'
    # Pack as two unsigned longs
    # Add + b'\0' to indicate end of string
    return struct.pack(fmt, d, max_mem, nTotal, url_size) + "\n".encode('latin1') + db_url.encode('latin1')


def handle_train(args):
    print("Handle TRAIN")
    # Take user input for an array of floats
    float_list = [float(item) for item in args]
    # print("float_list:")
    # print(float_list)
    # Calculate the number of bytes for the float array
    num_bytes = len(float_list) * 4  # Each float (using 'f' format) is 4 bytes
    print("num_bytes: " + str(num_bytes))

    # Pack data as the number of bytes followed by the array of floats
    return struct.pack('<Q', num_bytes) + "\n".encode('latin1') + struct.pack(f'<{len(float_list)}f', *float_list)



# Mapping of commands to handling functions
command_handlers = {
    'INITIALIZE': handle_initialize,
    'TRAIN': handle_train,
}

def main():
    server_host = '127.0.0.1'
    server_port = 13

    # Load commands from a JSON file
    with open('commands.json', 'r') as file:
        commands = json.load(file)

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((server_host, server_port))
        
        for cmd in commands:
            command = cmd['command']
            args = cmd['args']
            if (command == 'TRAIN'):
                args = []
                random.seed(42)
                for _ in range(315000):
                    args.append(random.uniform(-1000, 1000))

                for _ in range(2):
                    args.append(1)

            
            expected = cmd['expected_response']

            handler = command_handlers.get(command)
            if handler:
                packed_data = handler(args)
                message = command + "\r\n" + packed_data.decode('latin1') + "\r\n"
                print("Sending " + command + " command")
                sock.sendall(message.encode('latin1'))
                response = sock.recv(1024)
                response_string = response.decode()
                print("Received from server:", response_string)
                if (response_string != expected):
                    print("Got unexpected response: " + response_string)
                    raise "Expected response: ()" + expected + ") but got: (" + response_string + ") instead."
            else:
                print("Unknown command")
                raise "Encountered Unknown Command"

        print("Closing connection.")

    print("All commands passed!")


if __name__ == '__main__':
    main()
