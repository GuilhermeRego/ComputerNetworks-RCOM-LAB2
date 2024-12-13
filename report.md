## Experiência 1: Configurar IP Network

### Passos:

1 - Reiniciar a rede com:

```systemctl restart networking em todos os pcs```

2 - ifconfig usando a seguinte template:

```
ifconfig eth1 up
ifconfig eth1 172.16.Y0.1/24 (para o TuxY3)
ifconfig eth1 172.16.Y0.254/24 (para o TuxY4)
```

Onde Y é o número do pc onde estamos a fazer a experiência (ver terminal o número).

## Experiência 2: Criar Bridges

### Passos:

1 - ifconfig para o 2:

```
ifconfig eth1 up
ifconfig eth1 172.16.Y1.1/24 (para o Tux 2)
```

2 - Setup dos cabos para as bridges:

- E1 do TUX2 ligado ao ether2
- E1 do TUX3 ligado ao ether3
- E1 do TUX4 ligado ao ether4

3 - Setup das bridges:

- Ligar cabo amarelo ao S0 do TUX4

- Entrar no GTKTerm, configurar a baudrate para 1152000.

- Credênciais:
user: admin,
password: [enter]

- Criar as bridges:

```
/interface bridge add name=bridgeY0
/interface bridge add name=bridgeY1
```

- Eliminar as portas que o Tux está ligado por defeito:

```
/interface bridge port remove [find interface=ether2]
/interface bridge port remove [find interface=ether3]
/interface bridge port remove [find interface=ether4]
```

- Criar as duas bridges:

```
/interface bridge port add bridge=bridgeY0 interface=ether3
/interface bridge port add bridge=bridgeY0 interface=ether4
/interface bridge port add bridge=bridgeY1 interface=ether2
```

## Experiência 3: Transformar TUX4 num Router

### Passos:

1 - Setup dos cabos para o router

Ligar E2 do TUX4 ao ether5

2 - Configurar IP do TUX4 para a bridge21

```
ifconfig eth2 up
ifconfig eth2 172.16.Y1.253/24
```

```
/interface bridge port remove [find interface=ether5]
/interface bridge port add bridge=bridgeY1 interface=ether5
```

3 - Ativar IP Forwarding e desativar ICMP echo-ignore-broadcast no TUX4

```
echo 1 > /proc/sys/net/ipv4/ip_forward
echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcast
```

4 - Adicionar routes para que o TUX3 e o TUX2 consigam alcançar-se

```
route add -net 172.16.Y0.0/24 gw 172.16.Y1.253 (no TUX2)
route add -net 172.16.Y1.0/24 gw 172.16.Y0.254 (no TUX3)
```

5 - Verificar se está tudo ok pingando tudo

No TUX3:
```
ping 172.16.Y0.254 (Testar pingar o TUX4 na bridgeY0)
ping 172.16.Y1.253 (Testar pingar o TUX4 na bridgeY1)
ping 172.16.Y1.1 (Testar pingar o TUX2 através do TUX4)
```

6 - Limpar as tabelas ARP em todos os pcs

```
arp -d 172.16.51.253 (TuxY2)
arp -d 172.16.50.254 (TuxY3)
arp -d 172.16.50.1 (TuxY4)
arp -d 172.16.51.1 (TuxY4)
```

7 - Começar uma captura com duas instancias do Wireshark no TUX4 (eth1 e eth2) e fazer ping de TUX3 para TUX2:

```
ping 172.16.Y1.1 (TuxY3)
```

## Experiência 4: Configurar router

### Passos:

1 - Ligar ETH1 do Router a P.12 e ETH2 do Router a algum port do Switch

2 - Adicionar Rc à bridgeY1

```
/interface bridge port remove [find interface=ether6]
/interface bridge port add bridge=bridgeY1 interface=ether6
```

3 - Configurar MikroTIK Router

```
/ip address add address=172.16.1.Y9/24 interface=ether1
/ip address add address=172.16.Y1.254/24 interface=ether2
/ip address print
```

4 - Adicionar rotas default para todos os TUXs

```
route add default gw 172.16.T1.254 (no Tux2)
route add default gw 172.16.Y0.254 (no Tux3)
route add default gw 172.16.Y1.254 (no Tux4)

Na consola do MikroTik:
```
/ip route add dst-address=172.16.Y0.0/24 gateway=172.16.Y1.253
/ip route add dst-address=0.0.0.0/0 gateway=172.16.1.254
```	

A partir do TUX3 pingar o TUX2, TUX4 e o Router, e verificar os resultados no Wireshark
```
ping 172.16.Y0.255
ping 172.16.Y1.1
ping 172.16.Y1.254
```

5 - Remover a rota do TUX2 ao 172.16.Y0.0/24 pelo TUX4 e desativa Accept Redirects
    
    ```
    sysctl net.ipv4.conf.eth1.accept_redirects=0
    sysctl net.ipv4.conf.all.accept_redirects=0
    route del -net 172.16.Y0.0 gw 172.16.Y1.253 netmask 255.255.255.0
    route add -net 172.16.Y0.0/24 gw 172.16.Y1.254
    ```

    Pingar o TUX3 a partir do TUX2 e verificar os resultados no Wireshark

    ```
    ping 172.16.Y0.1
    ```

    Usamos `traceroute -n 172.16.Y0.1` e observamos que a ligação é establecida a usar o Rc como Router. 

    Removemos a nova rota e voltamos a usar o TUX4 como Router e reativar o Accept_Redirects

    ```
    route del -net 172.16.Y0.0 gw 172.16.Y1.254 netmask 255.255.255.0
    route add -net 172.16.Y0.0/24 gw 172.16.Y1.253
    ```

    Usamos `traceroute -n 172.16.Y0.1` novamente para verificar o percurso.

6 - Pingar o FTP Server e verificar o resultado no Wireshark
    
    ```
    ping 172.16.1.10
    ``` 

    Na consola do MikroTik desativar o NAT:
    
    ```
    /ip firewall nat disable 0
    ```

    Pingar novamente e verificar que não é establecida uma ligação

    ```
    ping 172.16.1.10
    ```

## Experiência 5: DNS

### Passos: 

1 - Editar o ficheiro /etc/resolv.conf em todos os TUXs, limpar o ficheiro e adicionar a linha
```
nameserver 10.227.netlab.fe.up.pt
```

    ```
    nameserver