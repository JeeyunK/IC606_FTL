# IC606_FTL

### 실행방법
~~~
make clean; make all -j
./simul <trace_file> <replacement policy (FIFO/LRU/cNRU)> <cached mapping table size ratio (ex: 100)>
~~~

trace file은 필요하면 경로 알려주거나 보내드림

### 시뮬레이션 세팅

1. 디바이스 크기 설정

in *settings.h*
~~~
LOGSIZE // logical device size
OP // op영역 비율
DEVSIZE // physical device size
~~~

2. 새로운 replacement policy 구현

in *map.cpp*
~~~
int check_mtable(uint32_t lba, SSD* ssd, STATS* stats) {
  ...
  if (entry_index == -1) {
    ...
    else if (RPOLICY == 1) {
      //TODO 여기에 구현하면됨
    }
    ...
  }
  ...
}
~~~
RPOLICY 변수는 main.cpp에 전역변수로 정의되어있음

3. 여러 latency 추가

map.cpp 파일이랑 gc.cpp 파일에 flash에 접근하는 부분은 //TODO 로 표시해둠
