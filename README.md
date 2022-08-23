# AsioProxy

![image-20220821115420115](static/cpp.png)



🎈***AsioProxy is a tcp proxy  built on asio(no boost);It is implemented with modern cpp(cpp17).with clear code structure and high performance;***



## 🎷feature

------



- high performance with modern cpp.(utilization of move semantic,Linux kernal zero copy...

  )

- fast and detailed log almost about every packet

- highly configurable 

- linux only 

- docker support

- gracefully start and close  / close inactive connection  / load balancer with configurable priority



## 🎻install

------



### docker (recommended)

```bash
docker build -t proxy_server .
docker container run  -itd  --rm --name=proxy_server proxy_server:latest  
```



if docker compose available,you can do things  below

```bash
docker compose up -d
```



### build from  source code

#### requirements

- gcc >= 7 or clang >= 4

#### build

```bash
git clone https://github.com/hash-roar/AsioProxy.git
mkidr build and cd build
cmake .. && make 
```



## ✨todo

- object pool. we can implement it as another level of indirection with a class connection  manager which  construct get a connection and destruct release a connection so we can manage the life time of abstract connection with smart pointer yet;
- high water mark. to be cache line friendly ,we can use adjust buffer size for heavily burdened connection    
- to use less heap allocation we have implement buffer by compile time std::array
