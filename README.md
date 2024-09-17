# network_labs

## Projeto Servidor e Cliente em C
### Descrição do Projeto (PT-BR)

Este projeto consiste na implementação de um **servidor** e um **cliente** em linguagem C, com suporte a **múltiplos usuários** através do uso de **threads**. O servidor é capaz de atender a **várias conexões simultâneas**, permitindo que vários clientes se conectem e realizem operações ao mesmo tempo.

O cliente pode enviar dois tipos de comandos para o servidor:

> *MyGet <caminho_do_arquivo>*: O servidor envia o conteúdo de um arquivo de texto especificado pelo cliente.

> *MyLastAccess*: O servidor retorna o timestamp do último acesso realizado por aquele cliente específico.

### Como compilar e executar

- Compilar todo o código
```
make all
```

- Limpar os arquivos objetos e executáveis:
```
make clean
```

- Executar o servidor
```
make run_server
```

- Executar o cliente
```
make run_client
```

- Fecha a conexão na interface do cliente:
```
exit
```

### Notas Adicionais

- Certifique-se de que os arquivos Makefile, server.cpp, client.cpp e as funções auxiliares estejam corretamente configurados.

- O servidor e o cliente se comunicam através do protocolo TCP na porta **8000**.

- Para testes, certifique-se de que o arquivo que deseja acessar com o comando MyGet esteja presente no diretório correto.

- Para fins educacionais, o código apresenta diversos comentários explicativos.

---


## Server and Client Project in C
### Project Description (EN)

This project involves the implementation of a **server** and a **client** in the C programming language, with support for **multiple users** through the use of **threads**. The server can handle multiple simultaneous connections, allowing several clients to connect and perform operations concurrently.

The client can send two types of commands to the server:

> *MyGet <file_path>*: The server sends the content of a text file specified by the client.

> *MyLastAccess*: The server returns the timestamp of the last access made by that specific client.

### How to Compile and Run

- Compile all the code:
```
make all
```

- Clean object files and executables:
```
make clean
```

- Run the server:
```
make run_server
```

- Run the client:
```
make run_client
```

- Close the connection on client's interface:
```
exit
```

### Notas Adicionais

- Ensure that the Makefile, server.c, client.c, and auxiliary functions are properly set up.

- The server and client communicate via TCP protocol on port **8000**.

- For testing purposes, make sure that the file you wish to access with the MyGet command is present in the correct directory.

- For educational purposes, there are several explaining comments through the code.
