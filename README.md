# 📘 분산시스템 과제 (HW1 ~ HW5)

4-1 분산시스템 과목의 과제 레포지토리입니다. 

## 📑 목차
- [HW1: UDP 기반 Key-Value Store](#hw1-udp-기반-key-value-store)  
- [HW2: 파티셔닝된 UDP 기반 Key-Value Store](#hw2-파티셔닝된-udp-기반-key-value-store)  
- [HW3: 캐시(Cache) + 서버 구조](#hw3-캐시cache--서버-구조)  
- [HW4: 4-서버 분산 UDP Key-Value Store](#hw4-4-서버-분산-udp-key-value-store)  
- [HW5: Primary–Follower 복제 UDP Key-Value Store](#hw5-primaryfollower-복제-udp-key-value-store)  

---

## HW1: UDP 기반 Key-Value Store

### 📌 개요
- UDP 소켓을 이용해 클라이언트와 서버가 `Key-Value Store` 구조체를 주고받음  
- 클라이언트: `READ_REQ` 전송  
- 서버: 문자열을 **역순 변환**하여 `READ_REP` 응답  

### 🛠 실행 방법
```bash
make
./server <port>
./client <port>

---

## HW2: 파티셔닝된 UDP 기반 Key-Value Store

### 📌 개요
- 클라이언트가 요청 시 **Key 해시값 기반 파티셔닝** 수행  
- 여러 서버(예: 5001, 5002)에 요청을 분산 전송  

### ⚙️ 동작 방식
- 요청 타입 랜덤 결정 (`READ_REQ` 또는 `WRITE_REQ`)  
- `READ_REQ`: Key만 전송 (17바이트)  
- `WRITE_REQ`: Key+Value 전송  
- Key 해시(`hash64(key) % NUM_SRV`) 결과로 파티션 번호 결정  
- 포트 번호 = `5001 + partition`  

### 🛠 실행 방법
```bash
make

# 서버 실행
./server 5001
./server 5002

# 클라이언트 실행
./client

---

## HW3: 캐시(Cache) + 서버 구조

### 📌 개요
- 요청이 먼저 **캐시 노드(5001)** 로 전달됨  
- 캐시 히트(`CACHE_HIT`): 캐시에서 바로 응답 반환  
- 캐시 미스(`CACHE_MISS`): 서버 노드(5002)로 요청 전달 후 응답 반환  

### ⚙️ 구성
- `cache.c`: 캐시 노드 (히트/미스를 랜덤으로 결정)  
- `server.c`: 서버 노드 (실제 Key-Value Store 보관 및 응답)  
- `client.c`: 캐시에 먼저 요청, 캐시 미스일 경우 서버에 재요청  

### 🛠 실행 방법
```bash
make

# 터미널 1: 캐시 실행
./cache 5001

# 터미널 2: 서버 실행
./server 5002

# 터미널 3: 클라이언트 실행
./client

---

## HW4: 4-서버 분산 UDP Key-Value Store

### 📌 개요
- **4개의 서버(5001~5004)** 를 두고 클라이언트가 요청을 분산  
- 부하 분산 알고리즘: 무작위로 2개 서버를 선택 → **연결 수가 더 적은 서버**로 요청 전송  

### ⚙️ 동작 방식
- 클라이언트: 총 5번 요청, 2-choice 방식으로 서버 선택  
- 서버: `READ_REQ` 수신 시 `READ_REP` 응답 반환  

### 🛠 실행 방법
```bash
make

# 서버 실행 (각 포트에서 각각 실행)
./server 5001
./server 5002
./server 5003
./server 5004

# 클라이언트 실행
./client

---

## HW5: Primary–Follower 복제 UDP Key-Value Store

### 📌 개요
- **Primary 서버 (5001)** 와 **Follower 서버 (5002, 5003)** 구조  
- Primary는 쓰기 요청 수신 시:  
  1. 자체 저장소 갱신  
  2. 두 Follower에 `WRITE_REQ` 전송  
  3. 모든 Follower의 ACK(`WRITE_REP`) 수신 시 클라이언트에 최종 `WRITE_REP` 반환  

### ⚙️ 동작 방식
- **클라이언트**: `READ_REQ`, `WRITE_REQ` 요청을 랜덤 전송  
- **Primary 서버 (5001)**: 쓰기 요청을 Follower로 복제 후, 모든 ACK 수신 시 클라이언트에 응답  
- **Follower 서버 (5002, 5003)**: Primary로부터 쓰기 요청 수신 → 저장 후 ACK 응답  

### 🛠 실행 방법
```bash
make

# 터미널 1: Primary 실행
./server 5001

# 터미널 2: Follower 1 실행
./server 5002

# 터미널 3: Follower 2 실행
./server 5003

# 터미널 4: 클라이언트 실행
./client


