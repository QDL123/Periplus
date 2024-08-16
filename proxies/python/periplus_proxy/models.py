from pydantic import BaseModel
from typing import List

class Record(BaseModel):
    embedding: List[float]
    document: str
    id: str
    metadata: str

class Query(BaseModel):
    ids: List[str]

class QueryResult(BaseModel):
    results: List[Record]