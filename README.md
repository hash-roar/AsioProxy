# AsioProxy

![image-20220821115420115](D:\working\github\AsioProxy\static\cpp.png)



AsioProxy is a tcp proxy  built on asio(no boost);It is implemented with modern cpp(cpp17).with clear code structure and high performance;



## feature

------



- high performance with modern cpp.(utilization of move semantic,Linux kernal zero copy...

  )

- fast and detailed log almost about every packet

- highly configurable 

- linux only 

- docker support



## install

------



### docker (recommend)



### build from  source code

#### requirements

- gcc >= 7 or clang >= 4

#### build

```bash
git clone https://github.com/hash-roar/AsioProxy.git
mkidr build and cd build
cmake .. && make 
```



