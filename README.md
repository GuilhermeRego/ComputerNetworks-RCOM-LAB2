# FTP Download Application and Network Experiments

A project developed for the **Redes de Computadores** (Computer Networks) course at **FEUP**, showcasing an FTP client in C and practical networking experiments.

## Overview
### 1. FTP Download Application
- Implements **RFC959** and **RFC1738** standards.
- Features:
  - URL parsing, TCP connection, authentication, passive mode.
  - Robust file download with error handling.
- Usage:
  ```bash
  gcc -o download download.c
  ./download ftp://[user:password@]host/path
  
### 2. Network Experiments
- IP Configuration: Addressing and pings.
- Bridging and Routing: Network segmentation and connectivity.
- NAT and DNS: Internet access via NAT and domain resolution.
- TCP Analysis: Congestion control and multi-connection effects.
### Results
- Successful file downloads and error handling.
- Insights into ARP, NAT, DNS, and TCP behavior.

### Authors
- Gabriel da Quinta Braga (up202207784)
- Guilherme Silveira Rego (up202207041)
