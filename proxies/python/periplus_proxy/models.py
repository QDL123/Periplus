from pydantic import BaseModel
from typing import List

class StoredObject(BaseModel):
    embedding: List[float]
    document: str
    id: str
    metadata: str

class QueryResult(BaseModel):
    results: List[StoredObject]

class IdsModel(BaseModel):
    ids: List[str]