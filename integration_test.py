import socket
import struct
import json
import random
from collections import namedtuple

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

def handle_load(args):
    print("Handle LOAD")

    float_list = [float(item) for item in args]

    num_bytes = len(float_list) * 4
    print("num_bytes: " + str(num_bytes))

    return struct.pack('<Q', num_bytes) + "\n".encode('latin1') + struct.pack(f'<{len(float_list)}f', *float_list)


def handle_search(args):
    print("Handle SEARCH")

    float_list = [float(item) for item in args["xq"]]
    # print("float_list:")
    # print(float_list)
    num_bytes = len(float_list) * 4

    print("args.n: " + str(args["n"]))
    print("args.k: " + str(args["k"]))

    return struct.pack('<QQQ', args["n"], args["k"], num_bytes) + "\n".encode('latin1') + struct.pack(f'<{len(float_list)}f', *float_list)



def read_string(sock, length):
    """ Helper function to read a given length of bytes and decode it to string. """
    chunks = []
    bytes_recd = 0
    while bytes_recd < length:
        chunk = sock.recv(min(length - bytes_recd, 2048))
        if chunk == b'':
            raise RuntimeError("socket connection broken")
        chunks.append(chunk)
        bytes_recd += len(chunk)
    return b''.join(chunks).decode('utf-8')

def read_floats(sock, num_floats):
    """ Helper function to read a specific number of 4-byte floats. """
    num_bytes = num_floats * 4  # Calculate total bytes to read for the floats
    floats_data = []
    bytes_recd = 0
    while bytes_recd < num_bytes:
        chunk = sock.recv(min(num_bytes - bytes_recd, 2048))
        if chunk == b'':
            raise RuntimeError("socket connection broken")
        floats_data.append(chunk)
        bytes_recd += len(chunk)
    all_data = b''.join(floats_data)
    return list(struct.unpack(f'{num_floats}f', all_data))

Document = namedtuple('Document', ['id', 'embedding', 'document', 'metadata'])

def deserialize_document(sock):
    """ Deserialize structured query results from an already connected TCP socket and return as a namedtuple. """
    try:
        # Read first length (8-byte unsigned integer) for ID string
        data = sock.recv(8)
        id_length = struct.unpack('Q', data)[0]
        id_str = read_string(sock, id_length)

        # Read the number of floats (8-byte unsigned integer)
        data = sock.recv(8)
        num_floats = struct.unpack('Q', data)[0]
        embedding = read_floats(sock, num_floats)

        # Read the length of the second string
        data = sock.recv(8)
        document_length = struct.unpack('Q', data)[0]
        document = read_string(sock, document_length)

        # Read the length of the final string
        data = sock.recv(8)
        metadata_length = struct.unpack('Q', data)[0]
        metadata = read_string(sock, metadata_length)

        # Return the received data as a namedtuple
        return Document(id=id_str, embedding=embedding, document=document, metadata=metadata)

    except socket.error as e:
        # Handle potential socket errors
        print(f"Socket error: {e}")


def deserialize_query_results(sock, num_queries):
    results = []
    print("num_queries: " + str(num_queries))
    for i in range(num_queries):
        data = sock.recv(4)
        num_results = struct.unpack('i', data)[0]
        print("num_results: " + str(num_results))
        results.append([])
        for _ in range(num_results):
            document = deserialize_document(sock)
            results[i].append(document)

    return results


# Mapping of commands to handling functions
command_handlers = {
    'INITIALIZE': handle_initialize,
    'TRAIN': handle_train,
    'LOAD': handle_load,
    'SEARCH': handle_search,
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
                for i in range(315000):
                    num = random.uniform(-100, 100)
                    if i % 50000 == 0:
                        print("vector " + str(i / 2) + "[0]: " + str(num))
                    args.append(num)

                for _ in range(2):
                    args.append(1)

            print("COMMAND: ")
            print(json.dumps(cmd, indent=4))
            expected = cmd['expected_response']

            handler = command_handlers.get(command)
            if handler:
                packed_data = handler(args)
                message = command + "\r\n" + packed_data.decode('latin1') + "\r\n"
                print("Sending " + command + " command")
                sock.sendall(message.encode('latin1'))
                if (command == "SEARCH"):
                    print("Processing SEARCH response")
                    response = deserialize_query_results(sock, args["n"])
                    for i in range(len(response)):
                        result_set = response[i]
                        for j in range(len(result_set)):
                            doc = result_set[j]
                            if (doc.document != expected[i][j]['document']):
                                print("Document did not match")
                                raise "Document did not match. Expected: (" + expected[i][j]['document'] + "), but got: (" + doc.document + ")"
                            if doc.embedding:
                                for k in range(len(doc.embedding)):
                                    if doc.embedding[k] != expected[i][j]['embedding'][k]:
                                        print("Embedding did not match")
                                        raise "Found embedding that did not match the expected response"
                else:
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
