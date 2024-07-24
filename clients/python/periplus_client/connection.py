import asyncio

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
