# WMS API Specification

## Overview

해당 문서는 WMS와 Client 사이의 통신 규격에 대한 명세서이다.  

## Connection

- Protocol: TCP  
- Port: 미정  
- TLS: 미정  
- Idle Timeout: 75s  

## Protocol  

- Numerical type: Big-endian  
- String: UTF-8   
- Char: 1byte
- Max Body Length: 2MB  

## Common Rules

- 각 API 요청에 해당하는 service code (request id) 값이 존재한다.    
- Ping과 Pong은 Content Data를 비워두어야 한다.  
- API에 대한 응답은 요청과 동일한 service code를 이용한다.  
  단, Ping, Pong은 Content가 없고 service code로만 구분이 가능하기에 해당 Rule에서 제외한다.  
  Ex. Login 요청에 대한 응답은 Login Service Code로 답한다.   
- Server는 45초마다 Client의 생사유무를 알기위해 Ping을 보낸다.  
- Client는 Ping을 받을 때마다 Pong을 쏘아 자신이 아직 활동 중이라는 사실을 서버에 알린다.  
- Server는 75초 동안 Client의 Pong을 받지 못한 경우 해당 세션을 종료한다.  

## API Table

| Service Code  | Name          | Direction         | Description                   |
| ------------- | ------------- | ----------------- | ----------------------------- |
| 0             | Ping          | Server <-> Client | Check connection              |
| 1             | Pong          | Client <-> Server | Answer ping                   |
| 2             | Login         | Client -> Server  | Login request                 |

## Socket Frame

모든 응답, 요청은 밑의 형태를 갖춘다.

- Full Frame
    ```
    | Header | Body |
    ```

- Header
    ```
    | Service Code | Body Length |
    ```

- Service Code  
    Type: unsigned int  
    Length: 4byte  

- Body Length   
    Type: unsigned int  
    Length: 4byte  

- Body  
    ```
    | Content 1 Length | Content 1 Data | Content 2 Length | Content 2 Data | Content 3 Length | Content 3 Data | ... |
    ```

- Content Length  
    Type: unsigned int  
    Length: 4byte  

- Content Data    
    Type: string, integer, char 등 다양할 수 있음  
    Length: Content Length에 명시된 길이  

- Example  
    Client에서 Server로 Id와 PW를 전송하는 경우의 예시  
    Client와 WMS Server 사이에 전달할 모든 자료형은 사전 논의가 되어있어야 한다.  
    해당 예시에서 ID와 PW 자료형은 `string`이라고 협의가 되어있다는 가정이다.  
    ```
    | Login (4byte) | 24 (4byte) | 9 (4byte) | "tongstar" (9byte) | 7 (4byte) | "970309" (7byte) |
    ```
    - Login이라는 코드를 소켓에 넣어 로그인과 관련한 소통이라는 것을 알림  
    - Body의 크기가 4byte + 9byte + 4byte + 7byte = 24byte라는 것을 알림
    - ID가 9byte 크기의 어떤 데이터라는 것을 알림
    - ID가 `tongstar`라는 것을 알림
    - PW가 7byte 크기의 어떤 데이터라는 것을 알림
    - PW가 `970309`라는 것을 알림

## Message Definition

범주 별로 각 Server Code에 대한 설명을 다룬다.  

### Common

공통적인 Server Code에 대해 다룬다.  

#### Ping

##### Description

서버는 마지막 수신 시각 기준 45초 무수신이면 Ping 전송  
Ping 전송 후 30초 내에 Pong 또는 다른 유효한 패킷을 못 받으면 세션 종료  

##### Frame

```
| 0 | 0 |   |
```
특이사항으로는 Ping 소켓의 body는 아무것도 없다.  
Ping을 받았다면 그대로 밑의 Pong을 응답하면 될 뿐이다.  

#### Pong

##### Description

Ping에 대한 응답  

##### Frame

```
| 1 | 0 |   |
```
Ping과 마찬가지로 Pong의 body는 없다.  

#### Login

##### Description 

클라이언트에서 서버로 보내는 로그인 요청

##### Request

###### Request Body

```
| Len (uint) | ID (string) | Len (uint) | PW (string) |
```

###### Request Frame

```
| 2 | 24 | 9 | tongstar | 7 | 970309 |
```

##### Response

###### Response Body

```
| Len (uint) | Login Result (uint) |
```

###### Login Result

```c++
enum class LoginResult : std::uint32_t
{
    PwInconsistency = 0,
    Success = 1,
    NonexistentId = 2,
    NonexistentPw = 3,
    DbError = 4
};
```

###### Response Frame

```
| 2 | 8 | 4 | 1 |
```
