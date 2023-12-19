FROM python:3.11
##FROM ubuntu:latest
RUN apt-get update && apt-get install -y \
    gcc \
    make 
##    python3 \
##    python3-pip \
##    && rm -rf /var/lib/apt/lists/*

WORKDIR app
COPY . .

RUN make -C src

##### PYTHON #########
RUN pip install boto3

##### JUPIT #########
# Install Jupyter and ipykernel
RUN pip install jupyter ipykernel

# Install Python kernel for Jupyter
RUN python -m ipykernel install --user --name=python3.11

# convert from jupyter screen to python script
RUN pip install jupyter nbconvert

# Expose the Jupyter Lab port
EXPOSE 5000


# Start Jupyter Lab
CMD ["jupyter", "lab", "--ip=0.0.0.0", "--port=5000", "--allow-root"]

##### JUPIT #########


