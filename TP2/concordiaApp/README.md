# Guia de Uso do programa

## Intalação

É necessário que o sistema consiga utilizar Acess Control Lists, Systemd e Syslog

### Concordia Daemon

1ºPasso -> Criar o utilizador "concordia" (sudo useradd -m -s /bin/bash -g concordia), é necessário um método de autenticação. O escolhido foi 
simplemesnte definir uma palava passe. (sudo passwd concordia)

2ºPasso -> Para simplificar a demonstração, achamos que é cómodo definir uma diretoria que todos os utilizadores consigam aceder e correr os executáveis.
Alternativamente, pode ser colocado o código numa diretoria onde apenas o utilizador consegue gerar os executáveis.

3ºPasso -> Abrir secção com o utilzador concordia e, na pasta concordiaDaemon, executar o script ./src/bootstrap.sh

OBS: Os scripits precisam de ser ligeiramente adaptados para os paths dos executáveis e services ficarem corretos.

### Utilizadores Concordia

1ºPasso -> Utilizador tem que ser sudoer, foi a forma que encontramos para automatizar a criação dos outro micro utilizadores.

2ºPasso-> Construir executáveis ( mais ágil se tiver numa pasta onde todos os utilizadores conseguem aceder).

3ºPasso -> Executar ./concordia-boot concordia-ativar. ( Apenas depois de adaptar os scripts)

4ºPasso -> Utilizar os restantes executáveis (com excessão dos daemon-xxx) para comunicar com o serviço.


OBS: Os scripits precisam de ser ligeiramente adaptados para os paths dos executáveis e services ficarem corretos.
OBS: Reparamos que, da primeira vez um utilizador tenta se juntar à rede, existe um choque de permissões que é resolvido com reboot do sistema.