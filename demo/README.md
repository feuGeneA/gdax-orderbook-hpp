Sample output:

```
[2018-06-01 20:27:29] [connect] Successful connection
[2018-06-01 20:27:29] [connect] WebSocket Connection 104.16.107.31:443 v-2 "WebSocket++/0.7.0" / 101
current best bid: Ξ0.00116 @ $579.07/Ξ ; current best offer: Ξ67.6797 @ $579.08/Ξ
waiting 5 seconds for the market to shift
current best bid: Ξ0.18384 @ $578.83/Ξ ; current best offer: Ξ74.5585 @ $578.84/Ξ
running stress test for 90 seconds, with 5 threads constantly iterating over the whole order book, and notifying of any iterations that take greater than 75 milliseconds...
long-running iteration! 77.2797 ms!
long-running iteration! 98.5918 ms!
```

Sample output of run using build with " -DPROFILE_MAX_QUEUE=5" appended to g++ command line in Makefile:

```
[2018-06-01 20:35:34] [connect] Successful connection
[2018-06-01 20:35:34] [connect] WebSocket Connection 104.16.107.31:443 v-2 "WebSocket++/0.7.0" / 101
current best bid: Ξ25.5403 @ $578.6/Ξ ; current best offer: Ξ25.8921 @ $578.61/Ξ
waiting 5 seconds for the market to shift
current best bid: Ξ25.5403 @ $578.6/Ξ ; current best offer: Ξ56.5691 @ $578.61/Ξ
running stress test for 90 seconds, with 5 threads constantly iterating over the whole order book, and notifying of any iterations that take greater than 75 milliseconds...
6 updates are waiting to be processed on the internal queue
6 updates are waiting to be processed on the internal queue
7 updates are waiting to be processed on the internal queue
6 updates are waiting to be processed on the internal queue
6 updates are waiting to be processed on the internal queue
long-running iteration! 114.287 ms!
long-running iteration! 82.6479 ms!
long-running iteration! 81.8908 ms!
long-running iteration! 92.2271 ms!
long-running iteration! 76.9375 ms!
long-running iteration! 96.0782 ms!
long-running iteration! 91.0367 ms!
long-running iteration! 80.5312 ms!
long-running iteration! 83.865 ms!
long-running iteration! 75.2219 ms!
long-running iteration! 83.6684 ms!
long-running iteration! 77.1879 ms!
6 updates are waiting to be processed on the internal queue
long-running iteration! 77.846 ms!
long-running iteration! 94.7492 ms!
long-running iteration! 86.43 ms!
long-running iteration! 121.205 ms!
6 updates are waiting to be processed on the internal queue
7 updates are waiting to be processed on the internal queue
8 updates are waiting to be processed on the internal queue
9 updates are waiting to be processed on the internal queue
long-running iteration! 141.153 ms!
long-running iteration! 76.9864 ms!
6 updates are waiting to be processed on the internal queue
6 updates are waiting to be processed on the internal queue
7 updates are waiting to be processed on the internal queue
8 updates are waiting to be processed on the internal queue
9 updates are waiting to be processed on the internal queue
8 updates are waiting to be processed on the internal queue
8 updates are waiting to be processed on the internal queue
long-running iteration! 77.2942 ms!
long-running iteration! 91.3865 ms!
long-running iteration! 76.6396 ms!
6 updates are waiting to be processed on the internal queue
7 updates are waiting to be processed on the internal queue
8 updates are waiting to be processed on the internal queue
9 updates are waiting to be processed on the internal queue
10 updates are waiting to be processed on the internal queue
11 updates are waiting to be processed on the internal queue
12 updates are waiting to be processed on the internal queue
13 updates are waiting to be processed on the internal queue
14 updates are waiting to be processed on the internal queue
15 updates are waiting to be processed on the internal queue
16 updates are waiting to be processed on the internal queue
17 updates are waiting to be processed on the internal queue
18 updates are waiting to be processed on the internal queue
19 updates are waiting to be processed on the internal queue
20 updates are waiting to be processed on the internal queue
21 updates are waiting to be processed on the internal queue
22 updates are waiting to be processed on the internal queue
23 updates are waiting to be processed on the internal queue
24 updates are waiting to be processed on the internal queue
24 updates are waiting to be processed on the internal queue
24 updates are waiting to be processed on the internal queue
24 updates are waiting to be processed on the internal queue
24 updates are waiting to be processed on the internal queue
23 updates are waiting to be processed on the internal queue
23 updates are waiting to be processed on the internal queue
24 updates are waiting to be processed on the internal queue
23 updates are waiting to be processed on the internal queue
21 updates are waiting to be processed on the internal queue
20 updates are waiting to be processed on the internal queue
20 updates are waiting to be processed on the internal queue
18 updates are waiting to be processed on the internal queue
18 updates are waiting to be processed on the internal queue
17 updates are waiting to be processed on the internal queue
17 updates are waiting to be processed on the internal queue
12 updates are waiting to be processed on the internal queue
11 updates are waiting to be processed on the internal queue
12 updates are waiting to be processed on the internal queue
13 updates are waiting to be processed on the internal queue
6 updates are waiting to be processed on the internal queue
6 updates are waiting to be processed on the internal queue
6 updates are waiting to be processed on the internal queue
7 updates are waiting to be processed on the internal queue
8 updates are waiting to be processed on the internal queue
8 updates are waiting to be processed on the internal queue
9 updates are waiting to be processed on the internal queue
10 updates are waiting to be processed on the internal queue
11 updates are waiting to be processed on the internal queue
12 updates are waiting to be processed on the internal queue
```
