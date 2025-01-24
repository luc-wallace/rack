# rack

A fast, single-threaded HTTP server implemented in C.

Currently very primitive.

Planned features:
- Response pipeline
- Routing with paths/methods
- JSON?

# Usage

Build the project by creating a build directory in the root folder and running:

```
cmake ..
```

Then, compile the project using:

```
cmake --build .
```

# Benchmarks

ApacheBench (5000 requests):

```
Server Software:        rack/1.0
Server Hostname:        localhost
Server Port:            80

Document Path:          /
Document Length:        47 bytes

Concurrency Level:      500
Time taken for tests:   0.178 seconds
Complete requests:      5000
Failed requests:        0
Total transferred:      915000 bytes
HTML transferred:       235000 bytes
Requests per second:    28161.40 [#/sec] (mean)
Time per request:       17.755 [ms] (mean)
Time per request:       0.036 [ms] (mean, across all concurrent requests)
Transfer rate:          5032.75 [Kbytes/sec] received
```
