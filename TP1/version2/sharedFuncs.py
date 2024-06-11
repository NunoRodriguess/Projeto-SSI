import os
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives.asymmetric import dh
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography import x509
import datetime
from bson import BSON


# Trocar os dados entre BSON
def from_bson(data):
    return BSON.decode(data)

def to_bson(msg):
    return BSON.encode(msg)

# Extrair informação do certificado (ID e CN)
def extract_certificate_info(cert):
    id = cert.subject.get_attributes_for_oid(x509.NameOID.PSEUDONYM)[0].value
    cn = cert.subject.get_attributes_for_oid(x509.NameOID.COMMON_NAME)[0].value
    return id,cn

# Assinar uma msg com uma chave privada
def sign(message,private_key_rsa):
    signature = private_key_rsa.sign(
    message,
    padding.PSS(
        mgf=padding.MGF1(hashes.SHA256()),
        salt_length=padding.PSS.MAX_LENGTH
    ),
    hashes.SHA256()
    )
        
    return signature

# Criar um par
def mkpair(x, y):
    """produz uma byte-string contendo o tuplo '(x,y)' ('x' e 'y' são byte-strings)"""
    len_x = len(x)
    len_x_bytes = len_x.to_bytes(2, "little")
    return len_x_bytes + x + y

# Desfazer um par
def unpair(xy):
    """extrai componentes de um par codificado com 'mkpair'"""
    len_x = int.from_bytes(xy[:2], "little")
    x = xy[2 : len_x + 2]
    y = xy[len_x + 2 :]
    return x, y

# Criar o certificado a partir dos bytes
def cert_load(fname):
    """lê certificado de ficheiro"""
    with open(fname, "rb") as fcert:
        cert = x509.load_pem_x509_certificate(fcert.read())
    return cert

#    _____________VALIDAÇÂO DO CERTIFICADO________________
def cert_validtime(cert, now=None):
    """valida que 'now' se encontra no período
    de validade do certificado."""
    if now is None:
        now = datetime.datetime.now(tz=datetime.timezone.utc)
    if now < cert.not_valid_before_utc or now > cert.not_valid_after_utc:
        raise x509.verification.VerificationError(
            "Certificate is not valid at this time"
        )


def cert_validsubject(cert, attrs=[]):
    """verifica atributos do campo 'subject'. 'attrs'
    é uma lista de pares '(attr,value)' que condiciona
    os valores de 'attr' a 'value'."""
    for attr in attrs:
        if cert.subject.get_attributes_for_oid(attr[0])[0].value != attr[1]:
            raise x509.verification.VerificationError(
                "Certificate subject does not match expected value"
            )


def cert_validexts(cert, policy=[]):
    """valida extensões do certificado.
    'policy' é uma lista de pares '(ext,pred)' onde 'ext' é o OID de uma extensão e 'pred'
    o predicado responsável por verificar o conteúdo dessa extensão."""
    for check in policy:
        ext = cert.extensions.get_extension_for_oid(check[0]).value
        if not check[1](ext):
            raise x509.verification.VerificationError(
                "Certificate extensions does not match expected value"
            )
                
def valida_cert(cert,sub1):
    try:
        # obs: pressupõe que a cadeia de certifica só contém 2 níveis
        cert.verify_directly_issued_by(cert_load("MSG_CA.crt"))
        # verificar período de validade...
        cert_validtime(cert)
        # verificar identidade... (e.g.)
        cert_validsubject(cert, [(x509.NameOID.COMMON_NAME, sub1)])
        # verificar aplicabilidade... (e.g.)
        # cert_validexts(
        #     cert,
        #     [
        #         (
        #             x509.ExtensionOID.EXTENDED_KEY_USAGE,
        #             lambda e: x509.oid.ExtendedKeyUsageOID.CLIENT_AUTH in e,
        #         )
        #     ],
        # )
        print("Certificado válido")
        return True
    except:
        print("Certificado inválido")
        return False

#   _______________________________________


# Cria a chave partilhada entre as duas entidades
def derive_shared_key(peer_public_key, private_key):
    peer_public_key = serialization.load_pem_public_key(peer_public_key)
    shared_key = private_key.exchange(peer_public_key)
    return shared_key

# Deriva a chave partilhada usando o HKDF para AES-GCM
def derive_encryption_key(shared_key):
    hkdf = HKDF(
        algorithm=hashes.SHA256(),
        length=32,  # Tamanho da chave
        salt=None,
        info=b'diffie-hellman-encryption-key',
    )
    encryption_key = hkdf.derive(shared_key)
    return encryption_key

# Codifica a mensagem
def encode(message, encryption_key):
    nonce = os.urandom(12)

    aesgcm = AESGCM(encryption_key)
    ct = aesgcm.encrypt(nonce, message, None)

    return nonce + ct

# Descodifica a mensagem
def decode(message, encryption_key):
    nonce = message[:12]
    cifra = message[12:]

    aesgcm = AESGCM(encryption_key)
    mensagem = aesgcm.decrypt(nonce, cifra, None)

    return mensagem

# Valida a assinatura (cliente-servidor/servidor-cliente)
def valida_assinatura(certs,assinatura,chave_peer,serialized_public_key):
    try:
        par = mkpair(chave_peer,serialized_public_key)
        cert = x509.load_pem_x509_certificate(certs)
        public_key = cert.public_key()
        public_key.verify(
        assinatura,
        par,
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