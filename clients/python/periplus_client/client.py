import struct
from collections import namedtuple
from .connection import Connection
from .error import PeriplusConnectionError, PeriplusServerError

Document = namedtuple('Document', ['id', 'embedding', 'document', 'metadata'])

class Periplus:
    def __init__(self, host, port):
        self.conn = Connection(host, port)

    def _format_command(command, static_args, dynamic_args):
        return command + "\r\n" + static_args.decode('latin1') + "\n" + dynamic_args.decode('latin1') + "\r\n"
    
    async def _connect(self):
         if not self.conn.connected:
            try:
                await self.conn.connect()
            except ConnectionError as e:
                raise PeriplusConnectionError(message="Could not connect to Periplus", address=self.conn.host, port=self.conn.port) from e


    async def initialize(self, d, db_url, options={}):
        """
        Initialize sets up the Periplus instance and prepares it for other commands.
        It must be called before any other commands and subsquent initialize calls will
        wipe the Periplus instance and reset it.

        Parameters:
        d (int): This dictates the dimensionality of the vector collection (e.g. 128 if
        you vectors have 128 dimensions).

        db_url (str): This tells Periplus the url it should use to load data from the database.
        This url should point to an endpoint which has implemented the Periplus database proxy
        specification. For details, see the periplus-proxy documentation.
        
        options (dict, optional): A dictionary containing additional optional configuration
        settings. Heres a description of each of those options:
            - nTotal (int): This is an estimate of the total number of vectors in the collection.
            It's not meant to be precise, but rather to provide the order of magnitude of the
            collection in order to better optimize the number of IVF cells which cannot be changed later
            without wiping the whole instance. Not providing this option will cause Periplus to guess
            about the collection size which can lead to suboptimal performance.
            - use_flat (bool): This boolean determines whether Periplus should use product quantization
            (PQ). When set to false, which it is by default if not specified, Periplus will use product
            quantization when the vectors are sufficiently large (>= 64 dimensions) and evenly divisible 
            into subvectors of 8. If those conditions are not met, or if use_flat is set to true, a flat
            IVF index will be used instead.

        Returns:
        bool: Returns true if the Periplus instance was initialized successfully.

        Raises:
        Error: If Periplus is not successfully initialized for any reason, an error will be raised.
        """
        await self._connect()

        command = "INITIALIZE"

        max_mem = 1024
        if 'max_mem' in options:
            max_mem = options['max_mem']

        nTotal = 250000
        if 'nTotal' in options:
            nTotal = options['nTotal']

        use_flat = False
        if 'use_flat' in options:
            use_flat = options['use_flat']

        fmt = '<QQQ?Q'
        static_args = struct.pack(fmt, d, max_mem, nTotal, use_flat, len(db_url))
        dynamic_args = db_url.encode('latin1')
        message = Periplus._format_command(command, static_args, dynamic_args)

        await self.conn.send(message)

        # Await confirmation the command was successful
        res = await self.conn.receive()
        if res.decode() != "Initialized cache":
            # TODO: Insert message from Periplus server into error
            raise PeriplusServerError(message="ERROR: FAILED TO INITIALIZE CACHE")
        
        await self.conn.close()
        return True
    

    async def train(self, training_data):
        """
        Train sets up Periplus's IVF index. A representative sample of the vector collection
        is provided to Periplus which is then used to detemine the optimal position of the 
        IVF centroids. Initialize must be called beforehand, and train must be called before 
        any other commands. Once train has been called, it cannot be called again after calling
        ADD without re-initializing.

        Parameters:
        training_data (List[List[float]]): This is a representative sample of the vector collection
        for Periplus to train on. It is recommended to be 10% of the total collection, but a smaller
        percentage is fine for large datasets where that's not possible. Each inner list must be of 
        length d (as specificed in the prior initialize command).

        Returns:
        bool: Returns true if the Periplus instance was trained successfully.

        Raises:
        Error: If training fails for any reason, an error will be raised.
        """

        await self._connect()

        command = "TRAIN"
        num_bytes = len(training_data) * len(training_data[0]) * 4
        float_list = [item for sublist in training_data for item in sublist]
        
        fmt = "<Q"
        static_args = struct.pack(fmt, num_bytes)
        dynamic_args = struct.pack(f'<{len(float_list)}f', *float_list)
        assert num_bytes == len(dynamic_args)
        message = Periplus._format_command(command, static_args, dynamic_args)

        await self.conn.send(message)

        res = await self.conn.receive()
        if res.decode() != "Trained cache":
            # TODO: Insert message from Periplus server into error
            raise PeriplusServerError(message="ERROR: FAILED TO TRAIN CACHE")
        
        await self.conn.close()
        return True
    
    
    async def add(self, ids, embeddings):
        """
        Add makes Periplus aware of vectors which have been added to the collection. This command must
        be called with any vectors which Periplus should be able to load. 

        Parameters:
        ids (List[str]): This is a list of unique identifiers which correspond to the vectors
        provided in the second argument. These ids will be used by Periplus to load data
        from the vector database

        embeddings (List[List[float]]): This is a list of vector embeddings defined by the ids
        in the previous argument. Each inner list represents a vector and must be of length d as
        specified in the initialization step.

        Returns:
        bool: Returns true if the data was added to the Periplus instance successfully.

        Raises:
        Error: If adding the data fails for any reason, an error will be raised.
        """
        await self._connect()

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


        print("Sending id / embedding delimiter")
        # Send the id / embedding delimiter
        delimiter = b'\n'
        await self.conn.send(delimiter.decode('latin1'))

        # Send the embeddings
        print("Sending embeddings")
        for i, embedding in enumerate(embeddings):
            if i % 100 == 0:
                print("Sending " + str(i) + "th embedding") 
            embedding_data = struct.pack(f'<{len(embedding)}f', *embedding)
            await self.conn.send(embedding_data.decode('latin1'))
        
        await self.conn.send("\r\n")

        res = await self.conn.receive()
        if res.decode() != "Added vectors":
            # TODO: Insert message from Periplus server into error
            raise PeriplusServerError(message="ERROR: FAILED TO ADD VECTORS TO CACHE")
        
        await self.conn.close()
        return True
        
    

    async def load(self, xq, options={}):
        """
        Load instructs Periplus to load 1 or more IVF cells of data from the vector
        database. It can only add vectors which it has been made aware of using the
        Add command.

        Parameters:
        xq (List[float]): This tells Periplus which IVF cell(s) to load. The cell(s) defined by the
        nearest centroid(s) to xq will be loaded. The number of cells can be specified
        in the options but is 1 by default.

        options (dict, optional): A dictionary containing additional optional settings.
        Heres a description of each of those options:
            - nLoad (int): This specifies how many IVF cells to load. The cells with the nLoad
            nearest centroids will be loaded from the database. The default is 1 if not specified.

        Returns:
        bool: Returns true if the data was added to the Periplus instance successfully.

        Raises:
        Error: If adding the data fails for any reason, an error will be raised.
        """
        await self._connect()

        command = "LOAD"
        num_bytes = len(xq) * 4

        nload = 1
        if 'nLoad' in options:
            nload = options['nLoad']
        
        fmt = "<QQ"
        static_args = struct.pack(fmt, nload, num_bytes)
        dynamic_args = struct.pack(f'<{len(xq)}f', *xq)
        assert num_bytes == len(dynamic_args)
        message = Periplus._format_command(command, static_args, dynamic_args)

        await self.conn.send(message)

        res = await self.conn.receive()

        if res.decode() != "Loaded cell":
            # TODO: Insert message from Periplus server into error
            raise PeriplusServerError(message="ERROR: COULD NOT LOAD CELL")
        
        await self.conn.close()
        return True


    async def _read_string(self, length):
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

    async def _read_floats(self, num_floats):
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


    async def _deserialize_document(self):
        """ Deserialize structured query results from an already connected TCP socket and return as a namedtuple. """
        # TODO: implement error handling
        # Read first length (8-byte unsigned integer) for ID string
        data = await self.conn.receive(8)
        id_length = struct.unpack('Q', data)[0]
        id_str = await self._read_string(id_length)

        # Read the number of floats (8-byte unsigned integer)
        data = await self.conn.receive(8)
        num_floats = struct.unpack('Q', data)[0]
        embedding = await self._read_floats(num_floats)

        # Read the length of the second string
        data = await self.conn.receive(8)
        document_length = struct.unpack('Q', data)[0]
        document = await self._read_string(document_length)

        # Read the length of the final string
        data = await self.conn.receive(8)
        metadata_length = struct.unpack('Q', data)[0]
        metadata = await self._read_string(metadata_length)

        # Return the received data as a namedtuple
        return Document(id=id_str, embedding=embedding, document=document, metadata=metadata)


    async def _deserialize_query_results(self, num_queries):
        results = []
        for i in range(num_queries):
            data = await self.conn.receive(4)
            num_results = struct.unpack('i', data)[0]
            results.append([])
            for _ in range(num_results):
                document = await self._deserialize_document()
                results[i].append(document)

        return results
    

    async def search(self, k, xq, options={}):
        """
        Search lets you run queries against Periplus. It takes a set of query vectors and returns the k nearest neighbors
        of each if the relevant sections of the index are in-residence.

        Parameters:
        k (int): This tells Periplus how many nearest neighbors to return for each query vector.

        xq (List[List[float]]): This is a list of query vectors. The inner lists (which each represents a query vector) must
        all be of size d as specified in the initialization step.

        options (dict, optional): A dictionary containing additional optional search settings.
        Heres a description of each of those options:
            - nprobe (int): This specifies how many IVF cells to search for nearest neighbors.
            - require_all (bool): This boolean determines whether the IVF cells defined by the nprobe nearest centroids
            all need to be in-residence for the query to be a cache hit. If it's true then any relevant cells not being in-residence
            will result in nothing being returned. If it's false, then so long as the IVF cell defined by the nearest centroid is
            in-residence then the query will be a cache hit and the subset of the IVF cells defined by the nprobe nearest
            centroids to the query vector will be searched. This means the total number of cells searched will be >= 1 and <= k. By
            default, require_all is true.

        Returns:
        List[List[Document]]: The outer list corresponds to the list of query vectors and each inner list contains the k nearest
        neighbors in the form of Document tuples. Some inner lists may be of size 0 if the corresponding query vector resulted in 
        a cache miss. If the length is > 0 but < k, then k was greater than the number of documents contained in the search
        space. Each document Tuple contains 4 properties: id, embedding, document, and metadata. These will correspond to what was
        given to Periplus when loading data from the vector database / database proxy.
        """
        await self._connect()

        command = "SEARCH"
        n = len(xq)

        # Parse options
        nprobe = 1
        if 'nprobe' in options:
            nprobe = options['nprobe']

        require_all = True
        if 'require_all' in options:
            require_all = options['require_all']

        float_list = [item for sublist in xq for item in sublist]
        num_bytes = len(float_list) * 4

        fmt = "<QQQ?Q"
        static_args = struct.pack(fmt, n, k, nprobe, require_all, num_bytes)
        dynamic_args = struct.pack(f'<{len(float_list)}f', *float_list)
        message = Periplus._format_command(command, static_args, dynamic_args)

        await self.conn.send(message)


        # TODO: throw Periplus Server Error if search is unsuccessful
        res = await self._deserialize_query_results(len(xq))
        await self.conn.close()
        return res
    

    async def evict(self, vector, options={}):
        """
        Evict instructs Periplus to evict 1 or more IVF cells of data.

        Parameters:
        xq (List[float]): This tells Periplus which IVF cell(s) to evict. The cell(s) defined by the
        nearest centroid(s) to xq will be loaded. The number of cells can be specified in the options but
        by default it's set to 1.

        options (dict, optional): A dictionary containing additional optional settings.
        Heres a description of each of those options:
            - nEvict (int): This specifies how many IVF cells to evict. The cells defined by the nEvict
            nearest centroids will be evicted from the database. The default is 1 if not specified.

        Returns:
        bool: Returns true if the data was evicted from the Periplus instance successfully.

        Raises:
        Error: If evicting the data fails for any reason, an error will be raised.
        """
        await self._connect()

        command = "EVICT"
        num_bytes = len(vector) * 4

        nevict = 1
        if 'nEvict' in options:
            nevict = options['nEvict']
        
        fmt = "<QQ"
        static_args = struct.pack(fmt, nevict, num_bytes)
        dynamic_args = struct.pack(f'<{len(vector)}f', *vector)
        message = Periplus._format_command(command, static_args, dynamic_args)

        await self.conn.send(message)

        res = await self.conn.receive()

        if res.decode() != "Evicted cell":
            raise PeriplusServerError("ERROR: COULD NOT EVICT CELL")
        
        await self.conn.close()
        return True