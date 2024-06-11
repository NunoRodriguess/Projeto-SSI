# Código baseado em https://docs.python.org/3.6/library/asyncio-stream.html#tcp-echo-client-using-streams
import asyncio
import json
from sharedFuncs import *

conn_cnt = 0
conn_port = 8443
max_msg_size = 9999
file_name = 'log.json'

def write_to_json_file(file_name, entrada):
    registros = []

    # Verificar se o arquivo já existe e não está vazio
    if os.path.exists(file_name) and os.stat(file_name).st_size > 0:
        with open(file_name, 'r', encoding='utf-8') as file:
            registros = json.load(file).get("registos", [])

    # Adiciona a nova entrada à lista de registros
    registros.append(entrada)

    # Escreve a lista de registros no arquivo JSON
    with open(file_name, 'w', encoding='utf-8') as file:
        json.dump({"registos": registros}, file, indent=2)

class ServerWorker(object):
    dicionario_certClientes = {}   #id:(Certificado)
    dicionario_infoleitura  = {}   #id:[{Assinatura, mensagem a ler}]

    """ Classe que implementa a funcionalidade do SERVIDOR. """
    def __init__(self, cnt, addr=None):
        """ Construtor da classe. """
        self.id = cnt
        self.addr = addr
        self.msg_cnt = 0
        
    #id, certificado
    def add_cert_Client(self, id, certificado):
        self.dicionario_certClientes[id] = certificado
    
    def get_cert_Client(self, id):
        # Verifica se o ID existe
        if id in self.dicionario_certClientes:
            certificado = self.dicionario_certClientes[id]
            return certificado
        else:
            return None  
        
    #id numero idSender sig e msg
    def add_msgSig_DIC(self, idRecetor, assinatura, mensagem):
        # Verifica se o cliente já existe no dicionário
        if idRecetor in self.dicionario_infoleitura:
            # Adicione a nova mensagem ao dicionário com o número de mensagem incrementado
            self.dicionario_infoleitura[idRecetor].append({"assinatura": assinatura, "message": mensagem, "lida": 0})
        else:
            # Se o cliente não existir, crie uma nova entrada com a mensagem
            self.dicionario_infoleitura[idRecetor] = [{"assinatura": assinatura, "message": mensagem, "lida": 0}]
        
    def get_msgSig_DIC(self, id):
        # Verifica se o ID existe
        if id in self.dicionario_infoleitura:
            # Retorna as mensagens do cliente correspondente
            return self.dicionario_infoleitura[id]
        else:
            return None

    def get_userdata(self, server_id = None):
        with open(server_id, "rb") as f:
            p12 = f.read()
        password = None  # a keystore não está protegida
        return serialization.pkcs12.load_key_and_certificates(p12, password)
    
    def handle_login_message(self, tipo, idSender):    
        #Escreve a entrada no ficheiro
        timestamp = datetime.datetime.now().isoformat()
        entrada = {
            "timestamp": timestamp,
            "mensagem": {
                "tipo": tipo.decode(),
                "clientID": idSender,
                "dados": "Envio da confirmacao do login do cliente"
            }
        }
        write_to_json_file(file_name, entrada)      
               
        #RECIBO
        message_bytes = str(entrada).encode()
        sigS = sign(message_bytes, self.private_key_rsa)
        rep = mkpair(message_bytes, sigS)
        
        # Adiciona a resposta a um dicionario para poder passar a BSON
        data = {"resposta": mkpair(mkpair(b'login', b'Login efetuado com sucesso'), rep)}
        return encode(to_bson(data), self.derive_key)   
    
    def handle_send_message(self, tipo, dados):
        # Timestamp da mensagem
        timestamp = datetime.datetime.now().isoformat().encode()
        
        # Descompactar dados
        received_data, msg = unpair(dados)
        idRecetor, idSender = unpair(received_data)
        ciphertext, sigSender = unpair(msg)
        # Adicionar à mensagem o timestamp e o ID do cliente que envia a mensagem
        data = {
            "clientID": idSender,
            "timestamp": timestamp,
            "message": ciphertext 
        }
        
        #Escreve a entrada no ficheiro
        entrada = {
            "timestamp": timestamp.decode(),
            "mensagem": {
                "tipo": tipo.decode(),
                "clientID": idSender.decode(),
                "dados": "Envio da confirmacao do envio da mensagem ao cliente"
            }
        }
        write_to_json_file(file_name, entrada)    
        
        #RECIBO
        message_bytes = str(entrada).encode()
        sigS = sign(message_bytes, self.private_key_rsa)
        rep = mkpair(message_bytes, sigS)
        
        # Atualizar dicionário de mensagens a serem lidas pelo receptor com a assinatura do remetente
        self.add_msgSig_DIC(idRecetor.decode(), sigSender, data)

        #Adiciona a resposta a um dicionario para poder passar a BSON
        data = {"resposta": mkpair(mkpair(b'send', b'Mensagem enviada com sucesso'), rep)}
        return encode(to_bson(data), self.derive_key)

    def handle_ask_queue(self,tipo, dados):
        idRecetor = dados
        msg_por_ler = self.get_msgSig_DIC(idRecetor.decode())
        porler = False
        # Escreve a entrada no ficheiro
        timestamp = datetime.datetime.now().isoformat() 
        
        # Verificar se há mensagens para ler e se há mensagens não nulas
        if msg_por_ler is not None:
            # Inicializa uma lista vazia para armazenar as entradas de dados
            dados = []
            for mensagem in msg_por_ler:
                if mensagem["lida"] == 0:
                    porler = True
                certSender = self.get_cert_Client(mensagem['message']['clientID'].decode())
                # Adiciona uma entrada ao dicionário dados para cada mensagem
                entrada = {"certificado": certSender, "mensagem": mensagem}
                dados.append(entrada)
            
            if porler :
                m = "Envio das mensagens na queue do cliente"
            else:
                m = "Cliente sem mensagens para ler"
                    
            entrada = {
                "timestamp": timestamp,
                "mensagem": {
                    "tipo": tipo.decode(),
                    "clientID": idRecetor.decode(),
                    "dados": m
                }
            }
            write_to_json_file(file_name, entrada)  
                    
            #RECIBO
            message_bytes = str(entrada).encode()
            sigS = sign(message_bytes, self.private_key_rsa)
            rep = mkpair(message_bytes, sigS)
        
            #Adiciona a resposta a um dicionario para poder passar a BSON
            dados = {"pedido": dados}
            #Adiciona a resposta a um dicionario para poder passar a BSON
            data = {"resposta": mkpair(mkpair(b'askqueue', encode(to_bson(dados), self.derive_key)), rep)}
            return encode(to_bson(data), self.derive_key)
        
        #Sem mensagens para o cliente
        else:
            entrada = {
                "timestamp": timestamp,
                "mensagem": {
                    "tipo": tipo.decode(),
                    "clientID": idRecetor.decode(),
                    "dados": "Cliente sem mensagens"
                }
            }
            write_to_json_file(file_name, entrada)  
                            
            #RECIBO
            message_bytes = str(entrada).encode()
            sigS = sign(message_bytes, self.private_key_rsa)
            rep = mkpair(message_bytes, sigS)
        
            #Adiciona a resposta a um dicionario para poder passar a BSON
            data = {"resposta": mkpair(mkpair(b'askqueue', b'erro'), rep)}
            return encode(to_bson(data), self.derive_key)

    def handle_get_message(self, tipo, dados):
        idRecetor, numMsg = unpair(dados)
        indice = int(numMsg.decode()) - 1
        msg_por_ler = self.get_msgSig_DIC(idRecetor.decode())

        timestamp = datetime.datetime.now().isoformat()
        idRecetor, _ = unpair(dados)
                
        if msg_por_ler and indice >= 0 and indice < len(msg_por_ler):
            if  msg_por_ler[indice] is not None:
                
                entrada = {
                    "timestamp": timestamp,
                    "mensagem": {
                        "tipo": tipo.decode(),
                        "clientID": idRecetor.decode(),
                        "dados": "Leitura da mensagem na queue do cliente"
                    }
                }
                # Escreve a entrada no ficheiro
                write_to_json_file(file_name, entrada)         
                       
                #RECIBO
                message_bytes = str(entrada).encode()
                sigS = sign(message_bytes, self.private_key_rsa)
                rep = mkpair(message_bytes, sigS)
                
                # Altera a entrada no dicionário
                self.dicionario_infoleitura[idRecetor.decode()][indice]['lida'] = 1
                # Vai buscar a entrada alterada
                mensagem_lida = self.get_msgSig_DIC(idRecetor.decode())[indice]
                
                #Adiciona a resposta a um dicionario para poder passar a BSON
                certSender = self.get_cert_Client(mensagem_lida['message']['clientID'].decode())
                # Adiciona uma entrada ao dicionário dados para cada mensagem
                entrada = {"certificado": certSender, "mensagem": mensagem_lida}
                #Adiciona a resposta a um dicionario para poder passar a BSON
                data = {"resposta": mkpair(mkpair(b'getmsg', encode(to_bson(entrada), self.derive_key)), rep)}
                return encode(to_bson(data), self.derive_key)
        else:
            entrada = {
                "timestamp": timestamp,
                "mensagem": {
                    "tipo": tipo.decode(),
                    "clientID": idRecetor.decode(),
                    "dados": "Mensagem invalida"
                }
            }
            # Escreve a entrada no ficheiro
            write_to_json_file(file_name, entrada)
                      
            #RECIBO
            message_bytes = str(entrada).encode()
            sigS = sign(message_bytes, self.private_key_rsa)
            rep = mkpair(message_bytes, sigS)
                
            # Adiciona a resposta a um dicionario para poder passar a BSON
            data = {"resposta": mkpair(mkpair(b'getmsg', b'erro'), rep)}
            return encode(to_bson(data), self.derive_key)  

    def process(self, msg):
        """ Processa uma mensagem (`bytestring`) enviada pelo CLIENTE.
            Retorna a mensagem a transmitir como resposta (`None` para
            finalizar ligação) """
        self.msg_cnt += 1
        
        #ESTABELECER CONEXÂO COM O CLIENTE
        if self.msg_cnt == 1:    
            receivedGP_bytes, self.received_public_Key_Client = unpair(msg)
            p_bytes, g_bytes = unpair(receivedGP_bytes)
           
            p_value = int.from_bytes(p_bytes, byteorder='big')
            g_value = int.from_bytes(g_bytes, byteorder='big')
            
            #Armazenar informações do ficheiro MSG_SERVER.p12        
            self.private_key_rsa, self.server_cert, self.ca_cert = self.get_userdata('MSG_SERVER.p12')
            
            # Parametros dh para gerar das chaves
            parameters_numbers = dh.DHParameterNumbers(p_value, g_value)
            parameters = parameters_numbers.parameters()
            # Gerar chave privada
            self.private_key = parameters.generate_private_key()
            # Gerar chave publica
            public_key = self.private_key.public_key()
            # Gerar chave publica serealizada
            self.serialized_public_key = public_key.public_bytes(
                encoding=serialization.Encoding.PEM,
                format=serialization.PublicFormat.SubjectPublicKeyInfo
            )
            
            # Receber a chave publica do cliente
            peer_public_key = derive_shared_key(self.received_public_Key_Client, self.private_key)
            # Derivar a chave
            self.derive_key = derive_encryption_key(peer_public_key)

            par = mkpair(self.serialized_public_key, self.received_public_Key_Client)
            sigS = sign(par, self.private_key_rsa)
        
            return mkpair(mkpair(self.serialized_public_key,sigS), self.server_cert.public_bytes(serialization.Encoding.PEM))
        
        if self.msg_cnt == 2:
            resposta = decode(msg, self.derive_key)
            respostaFROMBSON = from_bson(resposta)
            received_sigC, received_certC = unpair(respostaFROMBSON['resposta'])
            
            client_cert_reloaded = x509.load_pem_x509_certificate(received_certC)
            #Extrai o id e o cn do certificado do cliente
            self.clientID, clientCN = extract_certificate_info(client_cert_reloaded)    

            # Validar certificado do cliente
            if not valida_cert(client_cert_reloaded, clientCN):
                return None
            # Validar assinatura do cliente
            if not valida_assinatura(received_certC, received_sigC, self.received_public_Key_Client, self.serialized_public_key):
                return None
            
            self.add_cert_Client(self.clientID,received_certC)
            
            # Adiciona a resposta a um dicionario para poder passar a BSON
            data = {"resposta": b'Conexao estabelecida'}
            return encode(to_bson(data), self.derive_key)   
        
        #CONEXÂO ESTABELECIDA   
        
        #RECEBER MENSAGENS DOS CLIENTES
        resposta = decode(msg, self.derive_key)
        respostaFROMBSON = from_bson(resposta)
        tipo, dados = unpair(respostaFROMBSON['resposta'])
            
        #Envia confirmação ao cliente do login
        if tipo.decode() == 'login':
            #Escreve a entrada no ficheiro
            timestamp = datetime.datetime.now().isoformat()
            entrada = {
                "timestamp": timestamp,
                "mensagem": {
                    "tipo": tipo.decode(),
                    "clientID": self.clientID,
                    "dados": "Rececao do pedido de login do cliente"
                }
            }
            write_to_json_file(file_name, entrada)  
            return self.handle_login_message(tipo, self.clientID)
        
        elif tipo.decode() == "sendF1":
            idRecetor, idSender = unpair(dados)
            certificadoRecetor = self.get_cert_Client(idRecetor.decode())
            if certificadoRecetor is not None:
                data = {"resposta": mkpair(mkpair(b'send', encode(certificadoRecetor, self.derive_key)), b"")}
            else:
                data = {"resposta": mkpair(mkpair(b'send', b'erro'), b"")}
            return encode(to_bson(data), self.derive_key)  

        #Envia confirmação ao cliente do envia da msg
        elif tipo.decode() == 'send':
            #Escreve a entrada no ficheiro
            timestamp = datetime.datetime.now().isoformat()
            received_data, msg = unpair(dados)
            idRecetor, idSender = unpair(received_data)
            entrada = {
                "timestamp": timestamp,
                "mensagem": {
                    "tipo": tipo.decode(),
                    "clientID": idSender.decode(),
                    "dados": "Rececao da mensagem enviada do cliente"
                }
            }
            write_to_json_file(file_name, entrada)
            return self.handle_send_message(tipo, dados)
        
        #Envia a informação das msgs por ler do cliente
        elif tipo.decode() == 'askqueue':
            #Escreve a entrada no ficheiro
            timestamp = datetime.datetime.now().isoformat()
            idRecetor = dados
            entrada = {
                "timestamp": timestamp,
                "mensagem": {
                    "tipo": tipo.decode(),
                    "clientID": idRecetor.decode(),
                    "dados": "Rececao do pedido de leitura das mensagens na queue"
                }
            }
            write_to_json_file(file_name, entrada)
            return self.handle_ask_queue(tipo, dados)
        
        #Envia a informação das msgs por lidas do cliente 
        elif tipo.decode() == 'getmsg':
            #Escreve a entrada no ficheiro
            timestamp = datetime.datetime.now().isoformat()
            idRecetor, _ = unpair(dados)
            entrada = {
                "timestamp": timestamp,
                "mensagem": {
                    "tipo": tipo.decode(),
                    "clientID": idRecetor.decode(),
                    "dados": "Rececao do pedido de leitura de uma mensagem"
                }
            }
            write_to_json_file(file_name, entrada)
            return self.handle_get_message(tipo, dados)
            
#
#
# Funcionalidade Cliente/Servidor
#
# obs: não deverá ser necessário alterar o que se segue
#


async def handle_echo(reader, writer):
    global conn_cnt
    conn_cnt +=1
    addr = writer.get_extra_info('peername')
    srvwrk = ServerWorker(conn_cnt, addr)
    data = await reader.read(max_msg_size)
    while True:
        if not data: continue
        if data[:1]==b'\n': break
        data = srvwrk.process(data)
        if not data: break
        writer.write(data)
        await writer.drain()
        data = await reader.read(max_msg_size)
    print("[%d]" % srvwrk.id)
    writer.close()


def run_server():
    loop = asyncio.new_event_loop()
    coro = asyncio.start_server(handle_echo, '127.0.0.1', conn_port)
    server = loop.run_until_complete(coro)
    # Serve requests until Ctrl+C is pressed
    print('Serving on {}'.format(server.sockets[0].getsockname()))
    print('  (type ^C to finish)\n')
    try:
        loop.run_forever()
    except KeyboardInterrupt:
        pass
    # Close the server
    server.close()
    loop.run_until_complete(server.wait_closed())
    loop.close()
    print('\nFINISHED!')

run_server()