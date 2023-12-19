# ! pip install chromadb

from langchain.vectorstores import Chroma

persist_directory = 'chroma'

!rm -rf ./chroma  # remove old database files if any

vectordb = Chroma.from_documents(
    documents=divs,
    embedding=embedding,
    persist_directory=persist_directory
)

question = "what's the tenant's address"

docs = vectordb.similarity_search(question,k=3)

# Let's save this so we can use it later!
vectordb.persist()

