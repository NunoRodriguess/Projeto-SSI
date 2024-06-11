# Código baseado em https://docs.python.org/3.6/library/asyncio-stream.html#tcp-echo-client-using-streams
import asyncio
from sharedFuncs import *

conn_cnt = 0
conn_port = 8443
max_msg_size = 9999

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
            print("Cliente inexistente (get_cert_cliente)")
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
    
    def handle_login_message(self):              
        # Adiciona a resposta a um dicionario para poder passar a BSON
        data = {"resposta": mkpair(b'login', b'Login efetuado com sucesso')}
        return encode(to_bson(data), self.derive_key)   
    
    def handle_send_message(self, dados):
        # Timestamp da mensagem
        timestamp = datetime.datetime.now().isoformat().encode()
        
        # Descompactar dados
        received_data, msg = unpair(dados)
        idRecetor, idSender = unpair(received_data)
        text, sigSender = unpair(msg)
        subject, content = unpair(text)
        # Adicionar à mensagem o timestamp e o ID do cliente que envia a mensagem
        data = {
            "clientID": idSender,
            "timestamp": timestamp,
            "subject": subject,
            "message": content
        }
        
        # Atualizar dicionário de mensagens a serem lidas pelo receptor com a assinatura do remetente
        self.add_msgSig_DIC(idRecetor.decode(), sigSender, data)
        
        #Adiciona a resposta a um dicionario para poder passar a BSON
        data = {"resposta": mkpair(b'send', b'Mensagem enviada com sucesso')}
        return encode(to_bson(data), self.derive_key)

    def handle_ask_queue(self, dados):
        idRecetor = dados
        msg_por_ler = self.get_msgSig_DIC(idRecetor.decode())
        # Verificar se há mensagens para ler e se há mensagens não nulas
        if msg_por_ler is not None:
            # Inicializa uma lista vazia para armazenar as entradas de dados
            dados = []
            for mensagem in msg_por_ler:
                certSender = self.get_cert_Client(mensagem['message']['clientID'].decode())
                # Adiciona uma entrada ao dicionário dados para cada mensagem
                entrada = {"certificado": certSender, "mensagem": mensagem}
                dados.append(entrada)
             
            #Adiciona a resposta a um dicionario para poder passar a BSON
            dados = {"pedido": dados}
            #Adiciona a resposta a um dicionario para poder passar a BSON
            data = {"resposta": mkpair(b'askqueue', encode(to_bson(dados), self.derive_key))}
            return encode(to_bson(data), self.derive_key)
    
        #Sem mensagens para ler
        #Adiciona a resposta a um dicionario para poder passar a BSON
        data = {"resposta": mkpair(b'askqueue', b'erro')}
        return encode(to_bson(data), self.derive_key)

    def handle_get_message(self, dados):
        idRecetor, numMsg = unpair(dados)
        indice = int(numMsg.decode()) - 1
        msg_por_ler = self.get_msgSig_DIC(idRecetor.decode())
        
        if msg_por_ler and indice >= 0 and indice < len(msg_por_ler) :
            if  msg_por_ler[indice] is not None:
                # Altera a entrada no dicionário
                self.dicionario_infoleitura[idRecetor.decode()][indice]['lida'] = 1
                # Vai buscar a entrada alterada
                mensagem_lida = self.get_msgSig_DIC(idRecetor.decode())[indice]
                
                #Adiciona a resposta a um dicionario para poder passar a BSON
                data = {"resposta": mkpair(b'getmsg', mensagem_lida["message"]["message"])}
                return encode(to_bson(data), self.derive_key)
        else:
            # Adiciona a resposta a um dicionario para poder passar a BSON
            data = {"resposta": mkpair(b'getmsg', b'erro')}
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
            data = {"resposta":   b'Conexao estabelecida'}
            return encode(to_bson(data), self.derive_key)   

        #CONEXÂO ESTABELECIDA  
         
        #RECEBER MENSAGENS DOS CLIENTES
        resposta = decode(msg, self.derive_key)
        respostaFROMBSON = from_bson(resposta)
        tipo, dados = unpair(respostaFROMBSON['resposta'])
        if tipo.decode() == 'login':
            return self.handle_login_message()
        #Envia confirmação ao cliente do envia da msg
        elif tipo.decode() == 'send':
            return self.handle_send_message(dados)
        
        #Envia a informação das msgs por ler do cliente
        elif tipo.decode() == 'askqueue':
            return self.handle_ask_queue(dados)
        
        #Envia a informação das msgs por lidas do cliente 
        elif tipo.decode() == 'getmsg':
            return self.handle_get_message(dados)
            
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