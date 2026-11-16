 ## 1st Secure Partition implemented as a freestanding C code

- introduced a small Secure Partition as an AArch64 SP written in C, to run at S-EL1 under Hafnium as SPMC
- The initial test, or very first feature is a Direct Messaging Request-Response Test invoked from Linux

```text
The SP used a basic application-level test protocol for request and response over the FF-A 32-bit 
Direct Messaging. It was a totally self-defined protocol where the SP exposes a XOR operation as the 
service for its invocators, and it also used some simple stringed protocol request/response magic, nonce, opcode,
etc. 
 These were the application payload registers and what they held:
 * w3 = request magic
 * w4 = opcode
 * w5 = nonce
 * w6 = input value
 * w7 = Reserved
```

### The Linux FF-A Client Driver discovered the SP, sent a request, and validated the response, that's all.
```text
The test protocol used request/response magic values, an opcode to signify the XOR service, a nonce, 
an XOR mask with an input value, alongwith flags, status code, and response trailer, which was just a test
of Linux sending a FF-A direct message request to SP via Hafnium as the SPMC, the SP validating the request, 
doing the operation, responding using FF-A direct message response, and Linux validating it. 
```


