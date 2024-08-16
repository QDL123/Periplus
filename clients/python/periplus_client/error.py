class PeriplusError(Exception):
    """Base exception for all Periplus-related errors."""

    def __init__(self, message=None, *args):
        super().__init__(message, *args)
        self.message = message

    def __str__(self):
        return self.message or "A Periplus error occurred."
        

class PeriplusConnectionError(PeriplusError):
    """Raised for connection-related errors when interacting with Periplus."""

    def __init__(self, message=None, address=None, port=None, *args):
        super().__init__(message, *args)
        self.address = address
        self.port = port

    def __str__(self):
        base_message = super().__str__()
        if self.address and self.port:
            return f"{base_message} (Address: {self.address}, Port: {self.port})"
        return base_message
        

class PeriplusServerError(PeriplusError):
    """Raised when an operation fails to complete within Periplus."""

    def __init__(self, message=None, operation=None, error_code=None, *args):
        super().__init__(message, *args)
        self.operation = operation

    def __str__(self):
        base_message = super().__str__()
        if self.operation or self.error_code:
            return f"{base_message} (Operation: {self.operation})"
        return base_message