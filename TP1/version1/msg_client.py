import asyncio
import sys
from cryptography.hazmat.backends import default_backend

from sharedFuncs import *

conn_port = 8443
max_msg_size = 9999
bit_length = 512

def generate_p_and_g(bit_length):
    backend = default_backend()
    # Gerar um número primo grande para p
    p = dh.generate_parameters(generator=2, key_size=bit_length, backend=backend)

    return p.parameter_numbers().p, p.parameter_numbers().g

p_value, g_value = generate_p_and_g(bit_length)
    
HELP = '''Correct usage:
$ -user <FNAME> -- argumento opcional (que deverá surgir sempre em primeiro lugar) que especifica o ficheiro com dados do utilizador. Por omissão, será assumido que esse ficheiro é userdata.p12.

$ send <UID> <SUBJECT> -- Envia uma mensagem (lida do stdin com tamanho máximo de 1000Bytes) com assunto <SUBJECT> destinada ao utilizador com identificador <UID>.

$ askqueue -- Solicita ao servidor a lista de mensagens não lidas da queue do utilizador, sob o formato: <NUM>:<SENDER>:<TIME>:<SUBJECT>

$ getmsg <NUM> -- Solicita ao servidor o envio da mensagem da sua queue com número <NUM>, imprimindo-a no stdout em caso de sucesso e marcando-a como lida.

$ help -- Imprime instruções de uso do programa.
'''

# Validação da assinatura durante a troca de msg (cliente-cliente)
def valida_assinatura2(certs,assinatura,mensagem_para_assinar):
    try:
        cert = x509.load_pem_x509_certificate(certs)
        public_key = cert.public_key()
        public_key.verify(
        assinatura,
        mensagem_para_assinar,
        padding.PSS(
            mgf=padding.MGF1(hashes.SHA256()),
            salt_length=padding.PSS.MAX_LENGTH
        ),
        hashes.SHA256()
        )
        print("Assinatura válida!")
        return True
    except Exception as e:
        print("Assinatura inválida!")
        return False

# Instruções de uso do programa, impressas quando pedidas ou face a má utilização.


class Client:
    """ Classe que implementa a funcionalidade de um CLIENTE. """
    def __init__(self, sckt=None, file_user=None, args =[]):
        """ Construtor da classe. """
        global p_value, g_value
        self.sckt = sckt
        self.msg_cnt = 0
        self.file_user = file_user
        self.args = args        
        
        #Guarda as informações do cliente do ficheiro file_user
        self.private_key_rsa, self.user_cert, self.ca_cert = self.get_userdata(self.file_user)
        
        certificado = x509.load_pem_x509_certificate(self.user_cert.public_bytes(serialization.Encoding.PEM))
        #Extrai o id e o cn do certificado do cliente
        self.clientID, self.clientCN = extract_certificate_info(certificado)
        
         # Parametros dh para gerar das chaves
        parameters_numbers = dh.DHParameterNumbers(p_value, g_value)
        parameters = parameters_numbers.parameters()
        
        # Gerar chave privada
        self.private_key = parameters.generate_private_key()
        
        #Gerar a chave publica(e serealizada) do cliente
        public_key = self.private_key.public_key()
        self.serialized_public_key = public_key.public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        )
        
    def get_userdata(self, file_user):
        if not os.path.exists(file_user):
            sys.stderr.write(f'O arquivo {file_user} não foi encontrado.\n')
            sys.exit(1)
        with open(file_user, "rb") as f:
            p12 = f.read()
        password = None  # a keystore não está protegida
        private_key, user_cert, [ca_cert] = serialization.pkcs12.load_key_and_certificates(p12, password)
        return (private_key, user_cert, ca_cert)

    def handle_send_message(self):
        # Envia pedido de conexao
        if self.args[0] == 'login':             
            #Adiciona a resposta a um dicionario para poder passar a BSON
            data = {"resposta":  mkpair(b'login', self.clientID.encode())}  
        #Envia mensagem
        elif self.args[0] == 'send':
            msg_assunto = b""
            msg_conteudo = b""
            if self.args[2] and self.args[3]:
                msg_assunto += self.args[2].encode()
                msg_conteudo += self.args[3].encode()

            # Cliente assina a mensagem a enviar
            mensagem_codificada = mkpair(msg_assunto,msg_conteudo)

            sigC = sign(mensagem_codificada, self.private_key_rsa)
            
            mensagem = mkpair(mensagem_codificada, sigC)
            
            #Adiciona a resposta a um dicionario para poder passar a BSON
            data = {"resposta": mkpair(b'send', mkpair(mkpair(self.args[1].encode(), self.clientID.encode()), mensagem))}
            return encode(to_bson(data), self.derive_key) if len(mensagem_codificada) > 0 and len(mensagem_codificada) < 1000  else sys.stderr.write("MSG RELAY SERVICE: message size error\n")
        
        # Envia pedido de consulta das mensagens por ler
        elif self.args[0] == 'askqueue':  
            #Adiciona a resposta a um dicionario para poder passar a BSON
            data = {"resposta": mkpair(b'askqueue', self.clientID.encode())}              
        
        # Envia pedido de consulta das mensagens lidas
        elif self.args[0] == 'getmsg':
            num_Msg = self.args[1]   
            #Adiciona a resposta a um dicionario para poder passar a BSON
            data = {"resposta": mkpair(b'getmsg', mkpair(self.clientID.encode(), num_Msg.encode()))}    
            
        else:
            sys.stderr.write(f"MSG RELAY SERVICE: command error!\n{HELP}") 
            return None 
            
        return encode(to_bson(data), self.derive_key)
    
    
    def handle_received_message(self, tipo, dados ):
        if tipo.decode() == 'login':
            print(dados.decode())
            
        # Confirmação do send 
        elif tipo.decode() == 'send':
            print(dados.decode()) 
            
        # Informação pedida pelo askqueue         
        elif tipo.decode() == 'askqueue':
            count = 0 
            # Quando nao tem msgs por ler
            if dados == b'erro':
                sys.stderr.write("MSG RELAY SERVICE: message list error!\n")  
            else:
                dadosDECODE = decode(dados, self.derive_key)
                dadosFROMBSON = from_bson(dadosDECODE)
                for i, mensagens in enumerate(dadosFROMBSON["pedido"]):
                    # Campos da resposta
                    certSender = mensagens["certificado"]
                    mensagem = mensagens["mensagem"]
                    if mensagem["lida"] == 0 :
                        count += 1
                        # Campos guardados na mensagem
                        sigSender = mensagem["assinatura"]
                        sent_msg = mensagem["message"]
                        # Campos guardados na message dentro da mensagem
                        clientID = sent_msg["clientID"]
                        timestamp = sent_msg["timestamp"]
                        message = sent_msg["message"]
                        subject = sent_msg["subject"]
                        
                        #Valida mensagem recebida
                        if not valida_assinatura2(certSender, sigSender, mkpair(subject,message)):
                            sys.stderr.write("MSG RELAY SERVICE: verification error!\n")
                        else:
                            print(i + 1, " : ", clientID.decode(), " : ", timestamp.decode(), " : ", subject.decode())
                if count == 0:
                    sys.stderr.write("MSG RELAY SERVICE: no messages!\n")
                    
        # Informação pedida pelo getmsg   
        elif self.args[0] == 'getmsg':
            if dados == b'erro':
                sys.stderr.write("MSG RELAY SERVICE: unknown message!\n")   
            else:
                print("Leitura concluida:", dados.decode())
    
    def process(self, msg=b""):
        global p_value, g_value
        """ Processa uma mensagem (`bytestring`) enviada pelo SERVIDOR.
            Retorna a mensagem a transmitir como resposta (`None` para
            finalizar ligação) """
        self.msg_cnt += 1

        #ESTABELECER CONEXÂO COM O SERVIDOR
        if self.msg_cnt == 1:
            p_bytes = p_value.to_bytes((p_value.bit_length() + 7) // 8, byteorder='big') 
            g_bytes = g_value.to_bytes((g_value.bit_length() + 7) // 8, byteorder='big')
            return mkpair(mkpair(p_bytes, g_bytes),self.serialized_public_key)
        
        elif self.msg_cnt == 2:
            received_data, received_certS = unpair(msg)
            self.received_public_Key_Server, received_sigS = unpair(received_data)

            server_cert_reloaded = x509.load_pem_x509_certificate(received_certS)

            #Extrai o id e o cn do certificado do servidor
            self.serverID, self.serverCN = extract_certificate_info(server_cert_reloaded)
            
            # Validação do certificado enviado pelo servidor
            if not valida_cert(server_cert_reloaded, self.serverCN):
                return None
            # Validação a assinatura enviado pelo servidor
            if not valida_assinatura(received_certS, received_sigS, self.received_public_Key_Server, self.serialized_public_key):
                return None
            # Receber a chave publica do servidor
            peer_public_key = derive_shared_key(self.received_public_Key_Server, self.private_key)
            # Derivar a chave
            self.derive_key = derive_encryption_key(peer_public_key)
            
            # Assinar a troca de chaves (chave pública do cliente + chave pública recebida do servidor)        
            par = mkpair(self.serialized_public_key, self.received_public_Key_Server)
            sigC = sign(par, self.private_key_rsa)
            
            # Adiciona a resposta a um dicionario para poder passar a BSON
            data = {"resposta":   mkpair(sigC, self.user_cert.public_bytes(serialization.Encoding.PEM))}
            return encode(to_bson(data), self.derive_key)   
             
        #CONEXÂO ESTABELECIDA   
        # ENVIAR MENSAGENS AO SERVIDOR
        elif self.msg_cnt == 3:
            return self.handle_send_message()
        # RECEBER MENSAGENS DO SERVIDOR
        else:
            resposta = decode(msg, self.derive_key)
            respostaFROMBSON = from_bson(resposta)
            tipo, dados = unpair(respostaFROMBSON['resposta'])
            return self.handle_received_message(tipo, dados)
        
async def tcp_echo_client(args=None,file_user=None):
    reader, writer = await asyncio.open_connection('127.0.0.1', conn_port)
    addr = writer.get_extra_info('peername')
    client = Client(addr, file_user,args)
    msg = client.process()
    while msg:
        writer.write(msg)
        msg = await reader.read(max_msg_size)
        if msg:
            msg = client.process(msg)
        else:
            break
    writer.write(b'\n')
    print('Socket closed!')
    writer.close()

def run_client(args=None, file_user = None):
    loop = asyncio.get_event_loop()
    loop.run_until_complete(tcp_echo_client(args,file_user))
        
if __name__ == "__main__":
    if len(sys.argv) > 1:
        if (sys.argv[1] =='-user'):
            if(sys.argv[3] == 'help'):
                print(HELP)
            else: 
                run_client(sys.argv[3:],sys.argv[2])
        else:
            if(sys.argv[1] == 'help'):
                print(HELP)
            else:
                run_client(sys.argv[1:], file_user = 'userdata.p12' )
    else:
        print(HELP)
