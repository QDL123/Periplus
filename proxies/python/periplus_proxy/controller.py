from fastapi import FastAPI, Request, HTTPException
from pydantic import ValidationError
from typing import List

from .models import QueryResult, Query

class ProxyController:

    def __init__(self, fetch_ids, endpoint: str):
        self.handler_function = fetch_ids
        self.app = FastAPI()
        self.app.post(endpoint)(self.handle_load_data)
    
    
    async def handle_load_data(self, request: Request):
        payload = await request.body()

        try:
            event = Query.parse_raw(payload)
        except ValidationError as e:
            raise HTTPException(status_code=422, detail=e.errors())

        response = await self.handler_function(event)
        if isinstance(response, QueryResult):
            return response
        else:
            raise HTTPException(status_code=500, detail="Invalid response from handler function")


    def run(self, host: str = '0.0.0.0', port: int = 8000):
        import uvicorn
        uvicorn.run(self.app, host=host, port=port)