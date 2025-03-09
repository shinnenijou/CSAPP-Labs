# 15-213 CSAPP:3e Labs
Summaries of Labs autograding

## Data Lab
```
Score   Rating  Errors  Function
 1      1       0       bitXor
 1      1       0       tmin
 1      1       0       isTmax
 2      2       0       allOddBits
 2      2       0       negate
 3      3       0       isAsciiDigit
 3      3       0       conditional
 3      3       0       isLessOrEqual
 4      4       0       logicalNeg
 4      4       0       howManyBits
 4      4       0       floatScale2
 4      4       0       floatFloat2Int
 4      4       0       floatPower2
Total points: 36/36
```

## Attack Lab
```
$./test-trans -N 32 -M 32

Function 0 (1 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 0 (Transpose submission): hits:2737, misses:276, evictions:244

Summary for official submission (func 0): correctness=1 misses=276

TEST_TRANS_RESULTS=1:276
```
```
$./test-trans -N 64 -M 64

Function 0 (1 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 0 (Transpose submission): hits:10209, misses:1316, evictions:1284

Summary for official submission (func 0): correctness=1 misses=1316

TEST_TRANS_RESULTS=1:1316
```

## Malloc Lab
```
$./mdriver -t traces -v -l
Using default tracefiles in traces/
Measuring performance with gettimeofday().

Results for libc malloc:
trace  valid  util     ops      secs  Kops
 0       yes    0%    5694  0.001109  5133
 1       yes    0%    5848  0.000719  8132
 2       yes    0%    6648  0.002322  2863
 3       yes    0%    5380  0.002088  2576
 4       yes    0%   14400  0.000999 14409
 5       yes    0%    4800  0.001224  3921
 6       yes    0%    4800  0.000867  5534
 7       yes    0%   12000  0.000886 13547
 8       yes    0%   24000  0.001389 17276
 9       yes    0%   14401  0.002741  5254
10       yes    0%   14401  0.000471 30569
Total           0%  112372  0.014817  7584

Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   99%    5694  0.000390 14611
 1       yes   98%    5848  0.000342 17124
 2       yes   99%    6648  0.000428 15533
 3       yes   99%    5380  0.000401 13433
 4       yes   86%   14400  0.000512 28120
 5       yes   87%    4800  0.005149   932
 6       yes   88%    4800  0.005504   872
 7       yes   94%   12000  0.003259  3683
 8       yes   89%   24000  0.005775  4156
 9       yes  100%   14401  0.000712 20240
10       yes   86%   14401  0.000617 23333
Total          93%  112372  0.023087  4867

Perf index = 56 (util) + 40 (thru) = 96/100
```

## Proxy Lab
```
$./driver.sh 
*** Basic ***
Starting tiny on 17293
Starting proxy on 10035
1: home.html
   Fetching ./tiny/home.html into ./.proxy using the proxy
   Fetching ./tiny/home.html into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
2: csapp.c
   Fetching ./tiny/csapp.c into ./.proxy using the proxy
   Fetching ./tiny/csapp.c into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
3: tiny.c
   Fetching ./tiny/tiny.c into ./.proxy using the proxy
   Fetching ./tiny/tiny.c into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
4: godzilla.jpg
   Fetching ./tiny/godzilla.jpg into ./.proxy using the proxy
   Fetching ./tiny/godzilla.jpg into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
5: tiny
   Fetching ./tiny/tiny into ./.proxy using the proxy
   Fetching ./tiny/tiny into ./.noproxy directly from Tiny
   Comparing the two files
   Success: Files are identical.
Killing tiny and proxy
basicScore: 40/40

*** Concurrency ***
Starting tiny on port 8783
Starting proxy on port 9361
Starting the blocking NOP server on port 17702
Trying to fetch a file from the blocking nop-server
Fetching ./tiny/home.html into ./.noproxy directly from Tiny
Fetching ./tiny/home.html into ./.proxy using the proxy
Checking whether the proxy fetch succeeded
Success: Was able to fetch tiny/home.html from the proxy.
Killing tiny, proxy, and nop-server
concurrencyScore: 15/15

*** Cache ***
Starting tiny on port 20107
Starting proxy on port 18540
Fetching ./tiny/tiny.c into ./.proxy using the proxy
Fetching ./tiny/home.html into ./.proxy using the proxy
Fetching ./tiny/csapp.c into ./.proxy using the proxy
Killing tiny
Fetching a cached copy of ./tiny/home.html into ./.noproxy
Success: Was able to fetch tiny/home.html from the cache.
Killing proxy
cacheScore: 15/15

totalScore: 70/70
```