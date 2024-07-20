import asyncio
import struct
from collections import namedtuple

class Connection:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.reader = None
        self.writer = None
        self.loop = asyncio.get_event_loop()
        self.connected = False

    async def connect(self):
        self.reader, self.writer = await asyncio.open_connection(self.host, self.port)
        self.connected = True

    async def send(self, message):
        if self.writer is None:
            raise ConnectionError("Client is not connected.")
        self.writer.write(message.encode('latin1'))
        await self.writer.drain()

    async def receive(self, buffer_size=1024):
        if self.reader is None:
            raise ConnectionError("Client is not connected.")
        data = await self.reader.read(buffer_size)
        return data

    async def close(self):
        self.connected = False
        if self.writer is not None:
            self.writer.close()
            await self.writer.wait_closed()
            self.reader = None
            self.writer = None


Document = namedtuple('Document', ['id', 'embedding', 'document', 'metadata'])

class CacheClient:
    def __init__(self, host, port):
        self.conn = Connection(host, port)

    def __format_command__(command, static_args, dynamic_args):
        return command + "\r\n" + static_args.decode('latin1') + "\n" + dynamic_args.decode('latin1') + "\r\n"



    async def initialize(self, d, db_url, options={}):
        if not self.conn.connected:
            await self.conn.connect()

        command = "INITIALIZE"

        max_mem = 1024
        if 'max_mem' in options:
            max_mem = options['max_mem']

        nTotal = 250000
        if 'nTotal' in options:
            nTotal = options['nTotal']

        fmt = '<QQQQ'
        static_args = struct.pack(fmt, d, max_mem, nTotal, len(db_url))
        dynamic_args = db_url.encode('latin1')
        message = CacheClient.__format_command__(command, static_args, dynamic_args)

        await self.conn.send(message)

        # Await confirmation the command was successful
        res = await self.conn.receive()
        if res.decode() != "Initialized cache":
            raise "ERROR: FAILED TO INITIALIZE CACHE"
        
        await self.conn.close()
        return True
    

    async def train(self, training_data):
        # TODO: Check that training_data is an array of arrays
        if not self.conn.connected:
            await self.conn.connect()

        command = "TRAIN"
        num_bytes = len(training_data) * len(training_data[0]) * 4
        float_list = [item for sublist in training_data for item in sublist]
        
        fmt = "<Q"
        static_args = struct.pack(fmt, num_bytes)
        dynamic_args = struct.pack(f'<{len(float_list)}f', *float_list)
        assert num_bytes == len(dynamic_args)
        message = CacheClient.__format_command__(command, static_args, dynamic_args)

        await self.conn.send(message)

        res = await self.conn.receive()
        if res.decode() != "Trained cache":
            raise "ERROR: FAILED TO TRAIN CACHE"
        
        await self.conn.close()
        return True
    
    
    async def add(self, ids, embeddings):
        if not self.conn.connected:
            await self.conn.connect()

        assert len(ids) == len(embeddings)
        command = "ADD"

        print("computing dynamic length")
        # Compute the dynamic length
        num_bytes = 0
        # account for ids
        for id in ids:
            num_bytes += len(str(id))

        # account for id lengths
        num_bytes += len(ids) * 8

        # account for delimiter between ids and embeddings
        num_bytes += 1

        # account for embeddings
        num_bytes += len(embeddings) * len(embeddings[0]) * 4
        print("num_bytes: " + str(num_bytes))

        
        # Send the static args
        print("Sending static data")
        fmt = "<QQ"
        static_args = struct.pack(fmt, len(ids), num_bytes)
        static_message = command + "\r\n" + static_args.decode('latin1') + "\n"
        await self.conn.send(static_message)

        # Serialize the integer-string pairs
        # dynamic_data = b""  # Initialize as bytes object

        print("Sending id data")
        # Send the ids
        for id in ids:
            id = str(id)
            # Pack the unsigned integer
            length = len(id)
            id_data = ""
            id_data += struct.pack('<Q', length).decode('latin1')
            id_data += id

            await self.conn.send(id_data)
            # dynamic_data += struct.pack('<Q', length)
            # # Pack the string with the corresponding length
            # # dynamic_data += struct.pack(f'{length}s', id.encode('utf-8'))

            # dynamic_data += id.encode('latin1')

        print("Sending id / embedding delimiter")
        # Send the id / embedding delimiter
        delimiter = b'\n'
        await self.conn.send(delimiter.decode('latin1'))


        # Add the newline character as bytes
        # dynamic_data += b'\n'
        # Send the embeddings
        print("Sending embeddings")
        for i, embedding in enumerate(embeddings):
            if i % 100 == 0:
                print("Sending " + str(i) + "th embedding") 
            embedding_data = struct.pack(f'<{len(embedding)}f', *embedding)
            await self.conn.send(embedding_data.decode('latin1'))
        
        await self.conn.send("\r\n")

        # float_list = [item for sublist in embeddings for item in sublist]

        # dynamic_data += struct.pack(f'<{len(float_list)}f', *float_list)

        # # # Serialize the floating-point numbers
        # # for embedding in embeddings:
        # #     for num in embedding:
        # #         dynamic_data += struct.pack('f', num)

        # print("len(dynamic_data): " + str(len(dynamic_data)))
        # # static_args = struct.pack(fmt, len(ids), num_bytes)
        # message = CacheClient.__format_command__(command, static_args, dynamic_data)


        # assert len(dynamic_data) == num_bytes
        # await self.conn.send(message)

        res = await self.conn.receive()
        if res.decode() != "Added vectors":
            raise "ERROR: FAILED TO ADD VECTORS TO CACHE"
        
        await self.conn.close()
        return True
        
    

    async def load(self, vector, options={}):
        if not self.conn.connected:
            await self.conn.connect()

        command = "LOAD"
        num_bytes = len(vector) * 4

        nload = 1
        if 'nLoad' in options:
            nload = options['nLoad']
        
        fmt = "<QQ"
        static_args = struct.pack(fmt, nload, num_bytes)
        dynamic_args = struct.pack(f'<{len(vector)}f', *vector)
        assert num_bytes == len(dynamic_args)
        message = CacheClient.__format_command__(command, static_args, dynamic_args)

        await self.conn.send(message)

        res = await self.conn.receive()

        if res.decode() != "Loaded cell":
            raise "ERROR: COULD NOT LOAD CELL"
        
        await self.conn.close()
        return True


    async def read_string(self, length):
        """ Helper function to read a given length of bytes and decode it to string. """
        chunks = []
        bytes_recd = 0
        while bytes_recd < length:
            chunk = await self.conn.receive(min(length - bytes_recd, 2048))
            if chunk == b'':
                raise RuntimeError("socket connection broken")
            chunks.append(chunk)
            bytes_recd += len(chunk)
        return b''.join(chunks).decode('utf-8')

    async def read_floats(self, num_floats):
        """ Helper function to read a specific number of 4-byte floats. """
        num_bytes = num_floats * 4  # Calculate total bytes to read for the floats
        floats_data = []
        bytes_recd = 0
        while bytes_recd < num_bytes:
            chunk = await self.conn.receive(min(num_bytes - bytes_recd, 2048))
            if chunk == b'':
                raise RuntimeError("socket connection broken")
            floats_data.append(chunk)
            bytes_recd += len(chunk)
        all_data = b''.join(floats_data)
        return list(struct.unpack(f'{num_floats}f', all_data))


    async def deserialize_document(self):
        """ Deserialize structured query results from an already connected TCP socket and return as a namedtuple. """
        # try:
        # Read first length (8-byte unsigned integer) for ID string
        data = await self.conn.receive(8)
        id_length = struct.unpack('Q', data)[0]
        id_str = await self.read_string(id_length)

        # Read the number of floats (8-byte unsigned integer)
        data = await self.conn.receive(8)
        num_floats = struct.unpack('Q', data)[0]
        embedding = await self.read_floats(num_floats)

        # Read the length of the second string
        data = await self.conn.receive(8)
        document_length = struct.unpack('Q', data)[0]
        document = await self.read_string(document_length)

        # Read the length of the final string
        data = await self.conn.receive(8)
        metadata_length = struct.unpack('Q', data)[0]
        metadata = await self.read_string(metadata_length)

        # Return the received data as a namedtuple
        return Document(id=id_str, embedding=embedding, document=document, metadata=metadata)
        
        # TODO: error handling
        # except socket.error as e:
        #     # Handle potential socket errors
        #     print(f"Socket error: {e}")


    async def deserialize_query_results(self, num_queries):
        results = []
        for i in range(num_queries):
            data = await self.conn.receive(4)
            num_results = struct.unpack('i', data)[0]
            results.append([])
            for _ in range(num_results):
                document = await self.deserialize_document()
                results[i].append(document)

        return results
    

    async def search(self, k, query_vectors, options={}):
        # TODO: Check that query_vectors is a list of lists of size d
        if not self.conn.connected:
            await self.conn.connect()

        command = "SEARCH"
        n = len(query_vectors)

        # Parse options
        nprobe = 1
        if 'nprobe' in options:
            nprobe = options['nprobe']

        require_all = True
        if 'require_all' in options:
            require_all = options['require_all']

        float_list = [item for sublist in query_vectors for item in sublist]
        num_bytes = len(float_list) * 4

        fmt = "<QQQ?Q"
        static_args = struct.pack(fmt, n, k, nprobe, require_all, num_bytes)
        dynamic_args = struct.pack(f'<{len(float_list)}f', *float_list)
        message = CacheClient.__format_command__(command, static_args, dynamic_args)

        await self.conn.send(message)

        res = await self.deserialize_query_results(len(query_vectors))

        await self.conn.close()
        return res
    

    async def evict(self, vector, options={}):
        if not self.conn.connected:
            await self.conn.connect()

        command = "EVICT"
        num_bytes = len(vector) * 4

        nevict = 1
        if 'nEvict' in options:
            nevict = options['nEvict']
        
        fmt = "<QQ"
        static_args = struct.pack(fmt, nevict, num_bytes)
        dynamic_args = struct.pack(f'<{len(vector)}f', *vector)
        message = CacheClient.__format_command__(command, static_args, dynamic_args)

        await self.conn.send(message)

        res = await self.conn.receive()

        if res.decode() != "Evicted cell":
            raise "ERROR: COULD NOT EVICT CELL"
        
        await self.conn.close()
        return True