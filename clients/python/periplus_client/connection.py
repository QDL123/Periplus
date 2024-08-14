import asyncio
import errno

class Connection:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.reader = None
        self.writer = None
        self.loop = asyncio.get_event_loop()
        self.connected = False

    async def connect(self):
        try:
            self.reader, self.writer = await asyncio.open_connection(self.host, self.port)
            self.connected = True
        except OSError as e:
            if self.writer is not None:
                self.writer.close()
                await self.writer.wait_closed()

            if e.errno == errno.ECONNREFUSED:
                print("Connection refused.")
                raise ConnectionError("Connection refused") from e
            elif e.errno == errno.ETIMEDOUT:
                print("Connection timed out.")
                raise ConnectionError("Connection timed out") from e
            elif e.errno == errno.ENETUNREACH:
                print("Network is unreachable.")
                raise ConnectionError("Network is unreachable") from e
            else:
                print(f"Some other OSError occurred: {e}")
                raise ConnectionError("An OSError occurred")

        except Exception as e:
            print(f"An unexpected error occurred: {e}")
            if self.writer is not None:
                self.writer.close()
                await self.writer.wait_closed()

            raise ConnectionError("Failed to connect to server") from e


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
