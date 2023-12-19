# Load Python
FROM python:3.11
RUN python --version
RUN which python


# Set working directory
WORKDIR /app

# copy the files into the container
COPY . .

# Set environment variables if needed
ENV OPENAI_API_KEY='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'

# Install Jupyter and ipykernel
RUN pip install jupyter ipykernel

# Install Python kernel for Jupyter
RUN python -m ipykernel install --user --name=python3.11

## openai
RUN pip install --upgrade openai

# Install the python-dotenv package
RUN pip install python-dotenv

# Harrison's langchain
RUN pip install langchain

# convert from jupyter screen to python script
RUN pip install jupyter nbconvert

RUN pip install pandas

RUN pip install docarray
RUN pip install tiktoken
RUN pip install unstructured

RUN pip install -r requirements.txt  ## added pysqlite3 for chroma

RUN pip install chromadb  ## for combining memory with DB

RUN pip install pypdf PyPDF2



# Expose the Jupyter Lab port
EXPOSE 5000


# Start Jupyter Lab
CMD ["jupyter", "lab", "--ip=0.0.0.0", "--port=5000", "--allow-root"]
