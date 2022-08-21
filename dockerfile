# Get the base Ubuntu image from Docker Hub
FROM ubuntu:latest  as build
 
RUN apt-get -y install sed 
# Update apps on the base image
# Install the Clang compiler and cmake
RUN sed -i s@/archive.ubuntu.com/@/mirrors.aliyun.com/@g /etc/apt/sources.list && \
sed -i s@/security.ubuntu.com/@/mirrors.aliyun.com/@g /etc/apt/sources.list && \
apt-get clean  &&\
apt-get update && \
apt-get -y install clang  cmake


copy . /src

workdir /src/build
run cmake .. && make 

FROM ubuntu:latest

copy --from=build /src/build/server /app/
copy --from=build /src/proxy.conf /app/
workdir /app/
cmd ["proxy.conf"]
entrypoint ["./server"]


