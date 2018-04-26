FROM ubuntu:16.04
RUN mkdir /code
WORKDIR /code
RUN apt-get update
RUN apt-get install -y libssl-dev
RUN apt-get install -y libxerces-c-dev
RUN apt-get install -y libpqxx-dev
RUN apt-get install -y libboost-dev
RUN apt-get install -y libboost-system-dev
RUN apt-get install -y libboost-thread-dev
RUN apt-get install -y protobuf-compiler
ADD . /code/
